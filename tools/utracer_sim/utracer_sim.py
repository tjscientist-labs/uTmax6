#!/usr/bin/env python3
"""A software uTracer: pretends to be a uTracer on a serial port, so a host
program such as uTmax can be run and tested without the hardware.

How it works
------------
It opens a pseudo-terminal (a pair of connected virtual serial ports) and links
the host end to a fixed path, /tmp/utracer-sim by default. Point uTmax at that
path (the COM= line in ~/uTmax_files/cal.txt; the Debug dialog lists only real
serial ports). Then:

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
pseudo-terminals). Windows support, through a virtual COM-port pair such as
com0com, is planned.
"""

import argparse
import json
import os
import re
import select
import signal
import sys
import time
import tty

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hardware import ADC_FULL, ADC_VREF, PGA_GAINS, Profile  # noqa: E402
import loads  # noqa: E402

CMD_LEN = 18         # 9 bytes as hex
HEX_COMMAND = re.compile(r"[0-9A-F]{18}")
# Short commands uTmax sends to Nick Barton's replacement uTracer3 firmware
# (Firmware >= 200 in cal.txt; mainwindow.cpp). Logged, not answered.
SHORT_COMMANDS = {"54": "Nick Barton firmware: read version",
                  "59": "Nick Barton firmware: bootloader / reset",
                  "5556": "Nick Barton firmware: debug messages on",
                  "5758": "Nick Barton firmware: debug messages off"}
NAMES = {"00": "settings", "10": "measure", "20": "apply_hold", "30": "end_discharge",
         "40": "heater", "50": "ping_read_adc"}
AUTO_AVG_FOR_GAIN = [1, 1, 1, 1, 2, 2, 4, 8]   # what uTmax 3.01 assumes for auto-averaging


def solve_current(head, r_sense, current_at):
    """The current i with i = current_at(head - i * r_sense), for a load whose
    current never falls as its voltage rises. Bisection between 0 and the
    short-circuit current head / r_sense."""
    if head <= 0:
        return 0.0
    lo, hi = 0.0, head / r_sense
    for _ in range(80):                      # 2^-80 of the bracket: far below 1 nA
        mid = 0.5 * (lo + hi)
        if current_at(max(head - mid * r_sense, 0.0)) > mid:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


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
        self.n_replies = 0
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
        if getattr(self, "pending_fault", None):
            entry["fault"] = self.pending_fault
            self.pending_fault = None
        # int(x, 16) alone would also take '-', '+', '_' and spaces.
        if not HEX_COMMAND.fullmatch(cmd):
            entry["error"] = "not hex"
            self.write_log(entry)
            return ""
        words = [int(cmd[i:i + 4], 16) for i in (2, 6, 10, 14)]

        reply = ""
        if code == "00":
            b = bytes.fromhex(cmd[10:18])
            self.compliance, avg, self.gain_s, self.gain_a = b[0], b[1], b[2], b[3]
            self.averaging = avg
            entry["decoded"] = {"compliance_hex": f"{b[0]:02X}",
                                "limit_anode_mA": round(1000 * self.current_limit_amps(self.p.r_sense_a), 1),
                                "averaging": "auto" if avg == 0x40 else avg,
                                # measure() uses a fixed gain for 0-7 and auto-range
                                # for anything above; the log says the same.
                                "gain_screen": "auto" if b[2] >= 8 else PGA_GAINS[b[2]],
                                "gain_anode": "auto" if b[3] >= 8 else PGA_GAINS[b[3]]}
        elif code in ("10", "20"):
            self.set_voltages(words)
            entry["decoded"] = self.describe()
            if self.notes:
                entry["warning"] = "; ".join(self.notes)
            if code == "10":
                reply, meas = self.measure()
                entry["measured"] = meas
        elif code == "30":
            self.va_res = self.vs_res = 0.0
            self.vg = 0.0
            entry["decoded"] = {"action": "reservoirs discharged, grid to 0 V"}
        elif code == "40":
            self.set_heater(words[3])
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
        self.notes = []
        # The model's rating (profile max_hv_volts) is measured at the tube, as
        # uTmax's VaMax is; the reservoir also carries the cathode offset.
        for name, v in (("anode", self.va_res), ("screen", self.vs_res)):
            if v - self.p.cathode_volts() > self.p.max_hv + 1:
                self.notes.append(f"{name} {v - self.p.cathode_volts():.0f} V is above the "
                                  f"{self.p.name}'s {self.p.max_hv:g} V rating")
        if self.vg > 0 and not self.grid_board:
            # Without the extension board a positive grid word drives nothing.
            self.notes.append(f"positive grid {self.vg:+.2f} V requested but no extension board: grid at 0 V")
            self.vg = 0.0
        self.set_heater(vf)

    def set_heater(self, word):
        """The same in 10, 20 and 40: uTmax's external-heater word (E000 set)
        leaves the internal heater off."""
        hv = self.p.heater_code_to_volts(word)
        self.vf = 0.0 if hv is None else hv

    def describe(self):
        return {"Va_reservoir_V": round(self.va_res, 2), "Vs_reservoir_V": round(self.vs_res, 2),
                "Vg_V": round(self.vg, 3) + 0.0, "Vf_Vrms": round(self.vf, 2)}   # + 0.0: no "-0.0"

    def measure(self):
        """One pulse: solve the load's currents, then encode the 38-char reply."""
        p = self.p
        # On the uTracer3/6 the cathode sits at the supply voltage (on the NXT at
        # ground), so the tube sees the reservoir voltage minus that, minus the drops
        # in the HV switch and sense resistor.
        head_a = self.va_res - p.cathode_volts() - p.v_drop_switch + p.v_drop_diode
        head_s = self.vs_res - p.cathode_volts() - p.v_drop_switch + p.v_drop_diode
        # The current through the sense resistor lowers the voltage the load sees.
        # Each load's current rises with its own electrode's voltage, so the
        # operating point is found by bisection, which always converges. (A
        # fixed-point iteration swung instead of settling below about 1.6 ohm on
        # the uTracer6: Ihor Smal's review, 2026-11-21.)
        vs_head = max(head_s, 0.0)
        ia = solve_current(head_a, p.r_sense_a,
                           lambda v: self.load.currents(v, vs_head, self.vg)[0])
        va_t = max(head_a - ia * p.r_sense_a, 0.0)
        is_ = solve_current(head_s, p.r_sense_s,
                            lambda v: self.load.currents(va_t, v, self.vg)[1])

        ia_tube, is_tube = ia, is_
        if p.min_load_ohm:
            # The fixed minimum load sits across the output, so its current passes the
            # sense resistor as well; Dekker's program subtracts V/1M (fig. 8.11).
            ia += max(head_a - ia * p.r_sense_a, 0.0) / p.min_load_ohm
            is_ += max(head_s - is_ * p.r_sense_s, 0.0) / p.min_load_ohm

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
        meas = {"Ia_mA": round(ia_tube * 1000, 4), "Is_mA": round(is_tube * 1000, 4),
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
        """--garble-every N: every Nth reply (replies counted, not commands) has
        "ZZ" in the anode-current field, which every measurement fills."""
        f = self.faults
        if not reply:
            return reply
        self.n_replies += 1
        if f.get("garble_every") and self.n_replies % f["garble_every"] == 0:
            entry["fault"] = "garbled"
            return reply[:2] + "ZZ" + reply[4:]
        return reply

    def log_incomplete(self, text):
        """Characters that never made a full 18-character command."""
        self.write_log({"t": round(time.time(), 3), "rx": text, "cmd": "incomplete",
                        "note": SHORT_COMMANDS.get(text.upper(),
                                                   "fewer than 18 characters, then a pause")})

    def write_log(self, entry):
        if self.log:
            self.log.write(json.dumps(entry) + "\n")
            self.log.flush()


def stop_on_signal(signum, frame):
    raise KeyboardInterrupt


def serve(args, load):
    profile = Profile.find(args.profile)
    if args.supply_volts is not None:
        profile.supply_v = args.supply_volts      # a weak or sagging power brick
    if args.grid_board and not profile.grid_board_possible:
        raise SystemExit(f"--grid-board: the {profile.name} has no positive-grid extension board")
    faults = {"garble_every": args.garble_every, "trip": args.trip}
    log = open(args.log, "a", encoding="utf-8") if args.log else None
    sim = UTracer(profile, load, faults, log, grid_board=args.grid_board)

    master, slave = os.openpty()
    tty.setraw(slave)
    slave_name = os.ttyname(slave)
    # Replace an old link (a simulator that was killed), but never a real file.
    if os.path.exists(args.link) and not os.path.islink(args.link):
        raise SystemExit(f"--link {args.link}: a file is already there and is not a link; "
                         "choose another path")
    if os.path.islink(args.link):
        os.unlink(args.link)
    os.symlink(slave_name, args.link)
    # Stopped with SIGTERM (kill, a test's terminate()), clean up as for Ctrl-C.
    signal.signal(signal.SIGTERM, stop_on_signal)
    print(f"uTracer simulator: profile={profile.name}, load={load.name}"
          + (", positive-grid board fitted" if args.grid_board else ""), flush=True)
    print(f"serial port: {args.link} -> {slave_name}", flush=True)
    if args.ready_file:
        with open(args.ready_file, "w") as f:
            f.write(args.link + "\n")

    buf = ""
    last_char = 0.0
    gap = args.frame_gap / 1000.0
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
            now = time.monotonic()
            if buf and gap and now - last_char > gap:
                # A pause in the middle of a command: whatever came before was a
                # stray character or a short command (Nick Barton's firmware uses
                # 2- and 4-character ones). Start a new command, so one bad
                # character can't shift every command after it.
                sim.log_incomplete(buf)
                buf = ""
            last_char = now
            for ch in data.decode("ascii", errors="replace"):
                if args.no_reply_after is not None and sim.n_commands >= args.no_reply_after:
                    # Fault: the connection is gone. Nothing comes back, not even
                    # the echo, and nothing is acted on; each lost command is logged.
                    buf += ch
                    if len(buf) == CMD_LEN:
                        sim.n_commands += 1
                        sim.write_log({"t": round(time.time(), 3), "n": sim.n_commands,
                                       "rx": buf, "cmd": NAMES.get(buf[:2], "unknown"),
                                       "fault": "connection lost"})
                        buf = ""
                    continue
                if ch in "\r\n ":
                    os.write(master, ch.encode("ascii", errors="replace"))
                    continue
                buf += ch
                out = ch
                bad_echo = (len(buf) == CMD_LEN and args.bad_echo_on is not None
                            and sim.n_commands + 1 == args.bad_echo_on)
                if bad_echo:                 # fault: the last echoed character comes back wrong
                    out = "X" if ch != "X" else "Y"
                os.write(master, out.encode("ascii", errors="replace"))   # echo, as the firmware does
                if len(buf) == CMD_LEN:
                    sim.pending_fault = "bad echo" if bad_echo else None
                    reply = sim.handle(buf.upper())
                    buf = ""
                    if reply:
                        time.sleep(args.reply_delay / 1000.0)
                        os.write(master, reply.encode("ascii"))
    except KeyboardInterrupt:
        pass
    finally:
        # Remove the link only if it is still ours: a second copy may have
        # replaced it with its own.
        if os.path.islink(args.link) and os.readlink(args.link) == slave_name:
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
    ap.add_argument("--bad-echo-on", type=int, metavar="N",
                    help="fault: echo the last character of command N wrongly (the command still acts)")
    ap.add_argument("--supply-volts", type=float,
                    help="the supply (power brick) voltage, instead of the profile's (fault: a weak supply)")
    ap.add_argument("--grid-board", action="store_true",
                    help="fit Dekker's positive-grid extension board (42-character replies)")
    ap.add_argument("--frame-gap", type=float, default=100.0, metavar="MS",
                    help="a pause longer than this (ms) inside a command starts a new one; "
                         "0 = never (default 100)")
    ap.add_argument("--ready-file", help="write the port path here once listening (for test scripts)")
    args = ap.parse_args(argv)
    # Check the options here, so a mistake is a one-line message, not a crash
    # in the middle of a session.
    profiles = sorted(f[:-5] for f in os.listdir(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "profiles")) if f.endswith(".json"))
    if args.profile not in profiles:
        ap.error(f"--profile {args.profile}: no such profile; known: {', '.join(profiles)}")
    try:
        load = loads.from_spec(args.load)
    except ValueError as e:
        ap.error(f"--load: {e}")
    if not args.frame_gap >= 0:
        ap.error("--frame-gap must be 0 or more milliseconds")
    if not args.reply_delay >= 0:
        ap.error("--reply-delay must be 0 or more milliseconds")
    for name in ("no_reply_after", "garble_every", "bad_echo_on"):
        value = getattr(args, name)
        if value is not None and value < 1:
            ap.error(f"--{name.replace('_', '-')} must be 1 or more")
    if args.supply_volts is not None and not args.supply_volts > 0:
        ap.error("--supply-volts must be above 0")
    serve(args, load)


if __name__ == "__main__":
    main()
