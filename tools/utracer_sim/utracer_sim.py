#!/usr/bin/env python3
"""A software uTracer: pretends to be a uTracer on a serial port, so a host
program such as uTmax can be run and tested without the hardware.

How it works
------------
It opens a pseudo-terminal (a pair of connected virtual serial ports) and links
the host end to a fixed path, /tmp/utracer-sim by default. Point uTmax at that
path (the COM= line in ~/uTmax_files/cal.txt, or the Debug dialog). Then:

* every character received is echoed straight back, as the uTracer does;
* when a full command (18 hex characters) has arrived it is decoded and acted
  on, following Ronald Dekker's protocol (dos4ever.com, uTracer3 log §15):

    00  settings: compliance, averaging, screen gain, anode gain   (no reply)
    10  measure at the given Va, Vs, Vg, Vf                         (38-char reply)
    20  apply the given voltages and hold                           (no reply)
    30  end measurement: discharge the reservoirs, grid to 0        (no reply)
    40  heater on/off at the given voltage                          (no reply)
    50  "ping": read all ADC channels                               (38-char reply)

  With --grid-board (Dekker's positive-grid extension, uTracer6 only) the
  status byte has bit 7 set and replies are 42 characters.

* the currents come from a model of the device under test (see loads.py),
  and every conversion between volts and codes comes from a hardware profile
  (see hardware.py and profiles/), not from uTmax.

Every command and reply is written as one JSON line to the log, which is how
the tests see exactly what the host sent.

Faults can be injected to test the host's error handling: stop replying,
garble a reply, reply late, or force a current-limit trip.

Run ``utracer_sim.py --help`` for the options. Linux and macOS (it uses
pseudo-terminals); on Windows, use a virtual COM-port pair (com0com) with the
``--port`` option once that is added.
"""

import argparse
import json
import os
import select
import sys
import time
import tty

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hardware import ADC_FULL, ADC_VREF, PGA_GAINS, Profile  # noqa: E402
import loads  # noqa: E402

CMD_LEN = 18         # 9 bytes as hex
NAMES = {"00": "settings", "10": "measure", "20": "apply_hold", "30": "end_discharge",
         "40": "heater", "50": "ping_read_adc"}
AUTO_AVG_FOR_GAIN = [1, 1, 1, 1, 2, 2, 4, 8]   # what uTmax 3.01 assumes for auto-averaging


class UTracer:
    """The simulated instrument's state and its reaction to each command."""

    def __init__(self, profile, load, faults, log, grid_board=False):
        self.p = profile
        # Dekker's positive-grid extension board (uTracer6 only): allows a positive
        # grid, sets bit 7 of the status byte and adds a grid-current word, making
        # the reply 42 characters (tubetester6.html §14).
        self.grid_board = grid_board
        self.load = load
        self.faults = faults
        self.log = log
        self.n_commands = 0
        # Settings (command 00). Power-on defaults: 200 mA-class limit, 1 sample, auto gain.
        self.compliance = 0x8F
        self.averaging = 1
        self.gain_s = 8
        self.gain_a = 8
        # Present reservoir voltages (volts to ground) and heater/grid.
        self.va_res = 0.0
        self.vs_res = 0.0
        self.vg = 0.0
        self.vf = 0.0

    # -- the compliance byte is written straight into the PIC's CVRCON register --
    def current_limit_amps(self, r_sense):
        """Trip current for a sense resistor, from the comparator reference.

        CVRCON bit 5 (VRR) selects the range; bits 3..0 pick the ladder tap
        (PIC16F882/3/4/6/7 data sheet, comparator voltage reference):
          VRR=1: Vref = tap/24 * VDD        VRR=0: Vref = VDD/4 + tap/32 * VDD
        Dekker's example: 8F (VRR=0, tap 15) with 18 ohm sense = 200 mA.
        """
        tap = self.compliance & 0x0F
        if self.compliance & 0x20:
            vref = tap / 24.0 * ADC_VREF
        else:
            vref = ADC_VREF / 4.0 + tap / 32.0 * ADC_VREF
        return vref / r_sense

    def handle(self, cmd):
        """Act on one complete 18-character command. Returns the reply (or "")."""
        self.n_commands += 1
        code = cmd[:2]
        entry = {"t": round(time.time(), 3), "n": self.n_commands, "rx": cmd,
                 "cmd": NAMES.get(code, "unknown")}
        try:
            words = [int(cmd[i:i + 4], 16) for i in (2, 6, 10, 14)]
        except ValueError:
            entry["error"] = "not hex"
            self.write_log(entry)
            return ""

        reply = ""
        if code == "00":
            b = bytes.fromhex(cmd[10:18])
            self.compliance, avg, self.gain_s, self.gain_a = b[0], b[1], b[2], b[3]
            self.averaging = avg
            entry["decoded"] = {"compliance_hex": f"{b[0]:02X}",
                                "limit_anode_mA": round(1000 * self.current_limit_amps(self.p.r_sense_a), 1),
                                "averaging": "auto" if avg == 0x40 else avg,
                                "gain_screen": "auto" if b[2] == 8 else PGA_GAINS[b[2] & 7],
                                "gain_anode": "auto" if b[3] == 8 else PGA_GAINS[b[3] & 7]}
        elif code in ("10", "20"):
            self.set_voltages(words)
            entry["decoded"] = self.describe()
            if self.grid_note:
                entry["warning"] = self.grid_note
            if code == "10":
                reply, meas = self.measure()
                entry["measured"] = meas
        elif code == "30":
            self.va_res = self.vs_res = 0.0
            self.vg = 0.0
            entry["decoded"] = {"action": "reservoirs discharged, grid to 0 V"}
        elif code == "40":
            self.vf = self.p.heater_code_to_volts(words[3]) or 0.0
            entry["decoded"] = {"heater_Vrms": round(self.vf, 2)}
        elif code == "50":
            reply = self.format_reply("90" if self.grid_board else "10", 0, 0, 0, 0, 0, 0)
            entry["decoded"] = self.describe()

        reply = self.apply_faults(reply, entry)
        entry["tx"] = reply
        self.write_log(entry)
        return reply

    def set_voltages(self, words):
        va, vs, vg, vf = words
        self.va_res = self.p.hv_code_to_volts(va & 0x3FF)
        self.vs_res = self.p.hv_code_to_volts(vs & 0x3FF)
        self.vg = self.p.grid_code_to_volts(vg)
        self.grid_note = None
        if self.vg > 0 and not self.grid_board:
            # Without the extension board a positive grid word drives nothing.
            self.grid_note = f"positive grid {self.vg:+.2f} V requested but no extension board: grid at 0 V"
            self.vg = 0.0
        hv = self.p.heater_code_to_volts(vf)
        if hv is not None:
            self.vf = hv

    def describe(self):
        return {"Va_reservoir_V": round(self.va_res, 2), "Vs_reservoir_V": round(self.vs_res, 2),
                "Vg_V": round(self.vg, 3), "Vf_Vrms": round(self.vf, 2)}

    def measure(self):
        """One pulse: solve the load's currents, then encode the 38-char reply."""
        p = self.p
        # The cathode sits at the supply voltage, so the tube sees the reservoir
        # voltage minus the supply, minus the drops in the HV switch and sense resistor.
        head_a = self.va_res - p.supply_v - p.v_drop_switch + p.v_drop_diode
        head_s = self.vs_res - p.supply_v - p.v_drop_switch + p.v_drop_diode
        ia = is_ = 0.0
        for _ in range(60):                      # fixed-point solve for the sense drop
            va_t = max(head_a - ia * p.r_sense_a, 0.0)
            vs_t = max(head_s - is_ * p.r_sense_s, 0.0)
            na, ns = self.load.currents(va_t, vs_t, self.vg)
            if abs(na - ia) < 1e-9 and abs(ns - is_) < 1e-9:
                break
            ia, is_ = 0.5 * (ia + na), 0.5 * (is_ + ns)

        status = "90" if self.grid_board else "10"
        tripped = (ia > self.current_limit_amps(p.r_sense_a)
                   or is_ > self.current_limit_amps(p.r_sense_s)
                   or self.faults.get("trip"))
        if tripped:
            status = "91" if self.grid_board else "11"

        ga = self.gain_a if self.gain_a < 8 else p.auto_gain(ia, p.r_sense_a)
        gs = self.gain_s if self.gain_s < 8 else p.auto_gain(is_, p.r_sense_s)
        if self.averaging == 0x40:
            n_avg = AUTO_AVG_FOR_GAIN[max(ga, gs)]
        else:
            n_avg = max(1, self.averaging)
        ia_code, _ = p.current_code(ia, p.r_sense_a, PGA_GAINS[ga], n_avg)
        is_code, _ = p.current_code(is_, p.r_sense_s, PGA_GAINS[gs], n_avg)
        ia_raw, _ = p.current_code(ia, p.r_sense_a, 1, n_avg)
        is_raw, _ = p.current_code(is_, p.r_sense_s, 1, n_avg)
        if tripped:                              # the pulse is cut short; currents meaningless
            ia_code = is_code = ia_raw = is_raw = 0
        reply = self.format_reply(status, ia_code, ia_raw, is_code, is_raw, ga, gs)
        meas = {"Ia_mA": round(ia * 1000, 4), "Is_mA": round(is_ * 1000, 4),
                "Va_tube_V": round(max(head_a - ia * p.r_sense_a, 0), 2),
                "gain_a": PGA_GAINS[ga], "gain_s": PGA_GAINS[gs], "averaging": n_avg,
                "status": status}
        return reply, meas

    def format_reply(self, status, ia, ia_raw, is_, is_raw, ga, gs):
        """38 characters, the order uTmax's saveADCInfo() reads them in:
        status, Ia, Ia_raw, Is, Is_raw, Va, Vs, Vsupply, Vneg, gain_a, gain_s."""
        def w(x):
            return f"{max(0, min(0xFFFF, int(x))):04X}"
        out = (status + w(ia) + w(ia_raw) + w(is_) + w(is_raw)
               + w(self.p.hv_volts_to_code(self.va_res))
               + w(self.p.hv_volts_to_code(self.vs_res))
               + w(self.p.vsu_code()) + w(self.p.vn_code())
               + f"{ga:02X}{gs:02X}")
        if self.grid_board:
            out += w(0)          # grid-current ADC word (AN12); the loads draw no grid current
        return out

    def apply_faults(self, reply, entry):
        f = self.faults
        if not reply:
            return reply
        if f.get("no_reply_after") is not None and self.n_commands > f["no_reply_after"]:
            entry["fault"] = "no reply"
            return ""
        if f.get("garble_every") and self.n_commands % f["garble_every"] == 0:
            entry["fault"] = "garbled"
            return reply[:10] + "ZZ" + reply[12:]
        return reply

    def write_log(self, entry):
        if self.log:
            self.log.write(json.dumps(entry) + "\n")
            self.log.flush()


def serve(args):
    profile = Profile.find(args.profile)
    if args.supply_volts is not None:
        profile.supply_v = args.supply_volts      # a weak or sagging power brick
    load = loads.from_spec(args.load)
    faults = {"no_reply_after": args.no_reply_after, "garble_every": args.garble_every,
              "trip": args.trip}
    log = open(args.log, "a", encoding="utf-8") if args.log else None
    sim = UTracer(profile, load, faults, log, grid_board=args.grid_board)

    master, slave = os.openpty()
    tty.setraw(slave)
    slave_name = os.ttyname(slave)
    if os.path.islink(args.link) or os.path.exists(args.link):
        os.unlink(args.link)
    os.symlink(slave_name, args.link)
    print(f"uTracer simulator: profile={profile.name}, load={load.name}"
          + (", positive-grid board fitted" if args.grid_board else ""), flush=True)
    print(f"serial port: {args.link} -> {slave_name}", flush=True)
    if args.ready_file:
        with open(args.ready_file, "w") as f:
            f.write(args.link + "\n")

    buf = ""
    try:
        while True:
            r, _, _ = select.select([master], [], [], 0.5)
            if not r:
                continue
            try:
                data = os.read(master, 256)
            except OSError:
                time.sleep(0.1)      # host closed the port; keep waiting for it to reopen
                continue
            for ch in data.decode("ascii", errors="replace"):
                os.write(master, ch.encode("ascii", errors="replace"))   # echo, as the firmware does
                if ch in "\r\n ":
                    continue
                buf += ch
                if len(buf) == CMD_LEN:
                    reply = sim.handle(buf.upper())
                    buf = ""
                    if reply:
                        time.sleep(args.reply_delay / 1000.0)
                        os.write(master, reply.encode("ascii"))
    except KeyboardInterrupt:
        pass
    finally:
        if os.path.islink(args.link):
            os.unlink(args.link)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Software uTracer on a virtual serial port.")
    ap.add_argument("--profile", default="utracer6", help="hardware profile in profiles/ (default utracer6)")
    ap.add_argument("--load", default="r:10k",
                    help="device under test: open | r:10k | r:10k,s:47k | diode:0.7,1k | triode:12AX7")
    ap.add_argument("--link", default="/tmp/utracer-sim", help="path to link the serial port to")
    ap.add_argument("--log", help="append a JSON line per command to this file")
    ap.add_argument("--reply-delay", type=float, default=30.0,
                    help="milliseconds before a measurement reply (default 30)")
    ap.add_argument("--no-reply-after", type=int, help="fault: stop replying after N commands")
    ap.add_argument("--garble-every", type=int, help="fault: corrupt every Nth reply")
    ap.add_argument("--trip", action="store_true", help="fault: report a current-limit trip on every measurement")
    ap.add_argument("--supply-volts", type=float,
                    help="the supply (power brick) voltage, instead of the profile's (fault: a weak supply)")
    ap.add_argument("--grid-board", action="store_true",
                    help="fit Dekker's positive-grid extension board (42-character replies)")
    ap.add_argument("--ready-file", help="write the port path here once listening (for test scripts)")
    serve(ap.parse_args(argv))


if __name__ == "__main__":
    main()
