"""Tests for the software uTracer (tools/utracer_sim).

Run from the repository root:   python3 -m unittest discover -s tests -v

The protocol tests start the simulator as a separate process on a pseudo-terminal
and talk to it exactly as a host program would: send a command, read back the
echo and the reply. They need Linux or macOS and are skipped on Windows, where
the conversion tests and the --port checks still run.
"""

import json
import os
import select
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

HAVE_PTY = os.name != "nt"
if HAVE_PTY:
    import termios
    import tty

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIM_DIR = os.path.join(ROOT, "tools", "utracer_sim")
sys.path.insert(0, SIM_DIR)

from hardware import Profile  # noqa: E402
import loads  # noqa: E402


class Conversions(unittest.TestCase):
    """The hardware arithmetic, checked against figures from Dekker's pages."""

    def test_utracer3_hv_full_scale_is_358_V(self):
        # Dekker §15: V_HV = 0.35 n, "(358 V)" at n = 1023.
        p = Profile.find("utracer3")
        self.assertAlmostEqual(p.hv_code_to_volts(1023), 358.0, delta=1.0)

    def test_utracer3_current_matches_dekker(self):
        # Dekker §15: n = 3683 * Ia * Gain * N_average (18 ohm sense).
        p = Profile.find("utracer3")
        n, _ = p.current_code(0.010, p.r_sense_a, 2, 4)
        self.assertAlmostEqual(n, 3683 * 0.010 * 2 * 4, delta=2)

    def test_utracer3_negative_rail_matches_dekker(self):
        # Dekker §15: V_boost = 0.12 n - 117.5, so n = 8.33 V + 979.
        p = Profile.find("utracer3")
        self.assertAlmostEqual(p.vn_code(), 8.33 * p.neg_rail_v + 979, delta=2)

    def test_dekker_compliance_example_8F_is_200_mA_at_18_ohm(self):
        from utracer_sim import UTracer
        sim = UTracer(Profile.find("utracer3"), loads.Open(), {}, None)
        sim.compliance = 0x8F
        self.assertAlmostEqual(sim.current_limit_amps(18.0) * 1000, 199.7, delta=0.5)

    def test_hv_code_round_trip(self):
        p = Profile.find("utracer6")
        for code in (0, 1, 100, 512, 1023):
            self.assertEqual(p.hv_volts_to_code(p.hv_code_to_volts(code)), code)

    def test_utracer6_grid_word_is_sign_and_magnitude(self):
        # Dekker tubetester6.html §9: n = 32767 |Vg| / 100, OR 8000H if Vg > 0.
        p = Profile.find("utracer6")
        self.assertAlmostEqual(p.grid_code_to_volts(0x7FFF), -100.0, places=3)
        self.assertAlmostEqual(p.grid_code_to_volts(0x028F), -2.0, places=2)   # n = 655
        self.assertAlmostEqual(p.grid_code_to_volts(0x8148), +1.0, places=2)   # 8000H | 328
        self.assertAlmostEqual(p.grid_code_to_volts(0x0000), 0.0)

    def test_utracer6_dekker_scale_factors(self):
        # Dekker tubetester6.html §8: V_HV = 1.0448 n, V_sup = 0.0538 n, the -85 V
        # rail (the one in the result string) reads n = 600.
        p = Profile.find("utracer6")
        self.assertAlmostEqual(p.hv_code_to_volts(1), 1.0448, places=3)
        self.assertAlmostEqual(p.hv_code_to_volts(1023) - p.hv_code_to_volts(1022), 1.0448, places=3)
        self.assertAlmostEqual(p.vsu_code(), 18.6 * 19.5, delta=1.5)
        self.assertEqual(p.vn_code(), 600)

    def test_heater_pwm_is_rms(self):
        p = Profile.find("utracer6")
        # uTmax: f = 1023 * (Vf/Vsu)^2. 6.3 V from 19.5 V -> 107.
        word = round(1023 * (6.3 / 19.5) ** 2)
        self.assertAlmostEqual(p.heater_code_to_volts(word), 6.3, delta=0.05)


class Loads(unittest.TestCase):
    def test_resistor(self):
        r = loads.from_spec("r:10k")
        self.assertAlmostEqual(r.currents(100, 0, 0)[0], 0.01)

    def test_koren_12ax7_typical_point(self):
        # 12AX7 at 250 V, -2 V draws about 1 mA (RCA data: 1.2 mA at 250 V / -2 V).
        t = loads.from_spec("triode:12AX7")
        self.assertAlmostEqual(t.currents(250, 0, -2)[0] * 1000, 1.0, delta=0.3)

    def test_spec_parsing(self):
        self.assertEqual(loads.parse_ohms("4.7k"), 4700)
        self.assertEqual(loads.parse_ohms("1M"), 1e6)


def command(code, va=0, vs=0, vg=0, vf=0):
    """An 18-character uTracer command: 2-digit code, then four 4-digit hex words."""
    out = f"{code}{va:04X}{vs:04X}{vg:04X}{vf:04X}"
    assert len(out) == 18
    return out


class SimulatorProcess:
    """Start utracer_sim.py on a pseudo-terminal and talk to it."""

    def __init__(self, *args):
        if not HAVE_PTY:
            raise unittest.SkipTest("needs pseudo-terminals (Linux or macOS)")
        self.tmp = tempfile.mkdtemp(prefix="utsim-")
        self.link = os.path.join(self.tmp, "port")
        self.logfile = os.path.join(self.tmp, "log.jsonl")
        ready = os.path.join(self.tmp, "ready")
        self.proc = subprocess.Popen(
            [sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"), "--link", self.link,
             "--log", self.logfile, "--ready-file", ready, "--reply-delay", "5", *args],
            stdout=subprocess.DEVNULL)
        for _ in range(100):
            if os.path.exists(ready):
                break
            time.sleep(0.05)
        self.fd = os.open(self.link, os.O_RDWR | os.O_NOCTTY)
        tty.setraw(self.fd)
        termios.tcflush(self.fd, termios.TCIOFLUSH)

    def send(self, cmd, expect_reply, reply_len=38):
        os.write(self.fd, cmd.encode())
        return self.read(len(cmd) + (reply_len if expect_reply else 0))

    def read(self, want, seconds=3):
        """Up to `want` characters, waiting at most `seconds` (never blocks)."""
        got = b""
        deadline = time.time() + seconds
        while len(got) < want and time.time() < deadline:
            r, _, _ = select.select([self.fd], [], [], 0.1)
            if r:
                got += os.read(self.fd, 256)
        return got.decode()

    def log(self):
        with open(self.logfile, encoding="utf-8") as f:
            return [json.loads(line) for line in f]

    def close(self):
        os.close(self.fd)
        self.proc.terminate()
        self.proc.wait(timeout=5)
        shutil.rmtree(self.tmp, ignore_errors=True)


class Protocol(unittest.TestCase):

    def setUp(self):
        self.sim = SimulatorProcess("--load", "r:10k")

    def tearDown(self):
        self.sim.close()

    def test_every_character_is_echoed_and_settings_get_no_reply(self):
        cmd = "00000000008F010808"
        self.assertEqual(self.sim.send(cmd, expect_reply=False), cmd)
        self.assertEqual(self.sim.read(1, seconds=0.3), "")     # and nothing after the echo

    def test_ping_returns_38_characters_after_the_echo(self):
        out = self.sim.send("500000000000000000", expect_reply=True)
        self.assertEqual(len(out), 18 + 38)
        self.assertTrue(out.startswith("500000000000000000"))
        self.assertEqual(out[18:20], "10")               # status OK

    def test_measure_into_10k_gives_the_right_current(self):
        p = Profile.find("utracer6")
        self.sim.send("00000000008F010808", expect_reply=False)   # auto gain, 1 sample
        code = p.hv_volts_to_code(200 + p.supply_v)      # about 200 V on the anode
        out = self.sim.send(command("10", va=code), expect_reply=True)
        reply = out[18:]
        ia_code, gain = int(reply[2:6], 16), [1, 2, 5, 10, 20, 50, 100, 200][int(reply[34:36], 16)]
        ia_ma = ia_code / gain * (1000 * 5.0 / 1023) / p.r_sense_a
        # Independent of the code under test: Dekker's uTracer6 figures (log §8:
        # 1.0448 V per count, cathode at the 19.5 V supply; §10: 4.7 ohm sense).
        expected = (1.0448 * code - 19.5) / (10000 + 4.7) * 1000
        self.assertAlmostEqual(ia_ma, expected, delta=expected * 0.02)
        entry = [e for e in self.sim.log() if e["cmd"] == "measure"][-1]
        self.assertEqual(entry["measured"]["status"], "10")

    def test_end_measurement_discharges(self):
        p = Profile.find("utracer6")
        code = p.hv_volts_to_code(300)
        self.sim.send(command("20", va=code), expect_reply=False)
        out = self.sim.send("500000000000000000", expect_reply=True)
        self.assertEqual(int(out[18 + 18:18 + 22], 16), code)  # charged first
        self.sim.send("300000000000000000", expect_reply=False)
        out = self.sim.send("500000000000000000", expect_reply=True)
        self.assertEqual(int(out[18 + 18:18 + 22], 16), 0)   # Va field back to zero


class GridBoard(unittest.TestCase):
    """Dekker's positive-grid extension board: status bit 7 set, 42-character reply."""

    def test_reply_is_42_characters_with_status_90(self):
        sim = SimulatorProcess("--grid-board")
        try:
            out = sim.send("500000000000000000", expect_reply=True, reply_len=42)
            self.assertEqual(len(out), 18 + 42)
            self.assertEqual(out[18:20], "90")
        finally:
            sim.close()

    def test_positive_grid_without_board_is_ignored_and_logged(self):
        sim = SimulatorProcess()
        try:
            sim.send(command("10", va=100, vg=0x8148), expect_reply=True)
            entry = [e for e in sim.log() if e["cmd"] == "measure"][-1]
            self.assertIn("no extension board", entry.get("warning", ""))
        finally:
            sim.close()


class UTracerNXT(unittest.TestCase):
    """The NXT profile, checked against Dekker's NXT log (uTracerNXT.html §8-9).

    Same command and reply strings as the uTracer3/6; different conversions:
    cathode at ground, 12-bit grid DAC, 14.3 ohm sense, 1 Mohm minimum load."""

    def setUp(self):
        self.p = Profile.find("utracer_nxt")

    def test_hv_full_scale_is_517_V(self):
        # Dekker §9: R64 = 9.76k with R63 = 1M limits the reservoir to 517 V at n = 1023.
        self.assertAlmostEqual(self.p.hv_code_to_volts(1023), 517.0, delta=0.5)

    def test_grid_is_12_bit_and_always_negative(self):
        # Fig. 8.8: Vg = -(240k/10k) * 5/4095 * n, so n = 4095 is -120 V.
        self.assertAlmostEqual(self.p.grid_code_to_volts(4095), -120.0, places=3)
        self.assertAlmostEqual(self.p.grid_code_to_volts(round(4095 * 2 / 120)), -2.0, delta=0.02)
        self.assertFalse(self.p.grid_board_possible)

    def test_negative_rail_minus_125_V(self):
        # Fig. 8.10: Vneg = 5 (R1+R2)/R1 (n/1023 - 1) + 5 with 4.7k / 240k.
        n = self.p.vn_code()
        vneg = 5 * (4.7 + 240) / 4.7 * (n / 1023 - 1) + 5
        self.assertAlmostEqual(vneg, -125.0, delta=0.3)

    def test_cathode_is_at_ground(self):
        self.assertEqual(self.p.cathode_volts(), 0.0)
        self.assertEqual(Profile.find("utracer6").cathode_volts(), 19.5)

    def test_measure_into_10k(self):
        # 200 V reservoir into 10k: the tube sees 200 V (no supply offset) and the
        # sense resistor also carries 200 V / 1M through the minimum load.
        sim = SimulatorProcess("--profile", "utracer_nxt", "--load", "r:10k")
        try:
            sim.send("00000000008F010808", expect_reply=False)
            code = self.p.hv_volts_to_code(200)
            out = sim.send(command("10", va=code), expect_reply=True)
            reply = out[18:]
            self.assertEqual(len(reply), 38)
            self.assertEqual(reply[:2], "10")
            gain = [1, 2, 5, 10, 20, 50, 100, 200][int(reply[34:36], 16)]
            sensed_ma = int(reply[2:6], 16) / gain * (1000 * 5.0 / 1023) / self.p.r_sense_a
            v = self.p.hv_code_to_volts(code)
            tube_ma = v / (10000 + self.p.r_sense_a) * 1000
            self.assertAlmostEqual(sensed_ma, tube_ma + v / 1e6 * 1000, delta=tube_ma * 0.02)
            # Dekker's correction (fig. 8.11) brings it back to the tube current.
            self.assertAlmostEqual(sensed_ma - v / 1e6 * 1000, tube_ma, delta=tube_ma * 0.02)
        finally:
            sim.close()

    def test_open_output_still_reads_the_minimum_load(self):
        # Nothing connected: the sense resistor carries only V / 1 Mohm (fig. 8.11),
        # 0.4 mA at 400 V. The uTracer3/6 profiles have no such load and read zero.
        sim = SimulatorProcess("--profile", "utracer_nxt", "--load", "open")
        try:
            sim.send("00000000008F010808", expect_reply=False)
            code = self.p.hv_volts_to_code(400)
            reply = sim.send(command("10", va=code), expect_reply=True)[18:]
            gain = [1, 2, 5, 10, 20, 50, 100, 200][int(reply[34:36], 16)]
            sensed_ma = int(reply[2:6], 16) / gain * (1000 * 5.0 / 1023) / self.p.r_sense_a
            self.assertAlmostEqual(sensed_ma, self.p.hv_code_to_volts(code) / 1e6 * 1000, delta=0.01)
        finally:
            sim.close()

    def test_grid_board_refused(self):
        r = subprocess.run([sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"),
                            "--profile", "utracer_nxt", "--grid-board"],
                           capture_output=True, text=True, timeout=10)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("no positive-grid extension board", r.stderr)


class InputChecks(unittest.TestCase):
    """Bad input is refused cleanly (Ihor Smal's review, 2026-11-21)."""

    def test_command_with_minus_sign_is_not_hex(self):
        # int("-001", 16) is -1; the old check let it through as 1068 V-ish codes.
        from utracer_sim import UTracer
        sim = UTracer(Profile.find("utracer6"), loads.Open(), {}, None)
        self.assertEqual(sim.handle("10-001000000000000"), "")
        self.assertEqual(sim.va_res, 0.0)
        self.assertEqual(sim.handle("00000000-08F010808"), "")
        self.assertEqual(sim.handle("1_0010000000000000"[:18]), "")

    def test_resistance_formats(self):
        for text, ohms in [("470", 470), ("4.7k", 4700), ("4k7", 4700), ("1M", 1e6),
                           ("1meg", 1e6), ("2R2", 2.2), ("10 ohm", 10)]:
            self.assertAlmostEqual(loads.parse_ohms(text), ohms, msg=text)
        for text in ("1m", "abc", "4k7k"):
            with self.assertRaises(ValueError, msg=text):
                loads.parse_ohms(text)

    def test_load_specs(self):
        self.assertEqual(loads.from_spec("r:10k, s:47k").rs, 47000)
        self.assertEqual(loads.from_spec("R:10k,S:47k").rs, 47000)
        for bad in ("r:0", "r:-1k", "r:10k,x:1", "r:10k,s:1,s:2", "diode:0.7",
                    "triode:6L6", "resistor:1k", "open:1"):
            with self.assertRaises(ValueError, msg=bad):
                loads.from_spec(bad)

    def test_bad_options_are_one_line_errors(self):
        for args in (["--load", "r:0"], ["--profile", "nope"], ["--reply-delay", "-5"],
                     ["--garble-every", "0"], ["--supply-volts", "0"]):
            r = subprocess.run([sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"), *args],
                               capture_output=True, text=True, timeout=10)
            self.assertEqual(r.returncode, 2, args)
            self.assertNotIn("Traceback", r.stderr, args)
            self.assertIn("error:", r.stderr, args)


class LowResistanceAndOverload(unittest.TestCase):
    """The operating point is exact for any load, and a reading stops at 1023."""

    def sim(self, spec, profile="utracer6"):
        from utracer_sim import UTracer
        sim = UTracer(Profile.find(profile), loads.from_spec(spec), {}, None)
        sim.compliance = 0x8F
        return sim

    def test_current_is_exact_down_to_half_an_ohm(self):
        for r in (0.5, 1, 1.6, 2.2, 5, 10):
            for prof in ("utracer6", "utracer3"):
                sim = self.sim(f"r:{r}", prof)
                p = sim.p
                sim.va_res = 30.0 + p.cathode_volts()      # 30 V of head-room
                want = (30.0 - p.v_drop_switch + p.v_drop_diode) / (r + p.r_sense_a)
                _, meas = sim.measure()
                self.assertAlmostEqual(meas["Ia_mA"] / 1000, want, delta=want * 1e-6,
                                       msg=f"{r} ohm, {prof}")

    def test_trip_at_the_set_limit_with_a_1_ohm_load(self):
        # 8F with 4.7 ohm: 764.6 mA. The old solver tripped a 1 ohm load at 612 mA.
        sim = self.sim("r:1")
        limit = sim.current_limit_amps(sim.p.r_sense_a)
        for amps, status in ((0.98 * limit, "10"), (1.02 * limit, "11")):
            sim.va_res = amps * (1 + sim.p.r_sense_a) + sim.p.supply_v
            self.assertEqual(sim.measure()[1]["status"], status, amps)

    def test_overloaded_reading_saturates_at_1023(self):
        p = Profile.find("utracer6")
        code, per_sample = p.current_code(1.0, p.r_sense_a, 200, 1)
        self.assertEqual(code, 1023)
        code, _ = p.current_code(1.0, p.r_sense_a, 200, 8)
        self.assertEqual(code, 8 * 1023)

    def test_overload_in_a_reply(self):
        sim = self.sim("r:1k")
        sim.handle("0000000000AF010707")              # gain 200 forced, limit off-scale
        reply = sim.handle(f"10{sim.p.hv_volts_to_code(200):04X}000000000000")
        self.assertEqual(int(reply[2:6], 16), 1023)    # was 34467


class Framing(unittest.TestCase):
    """A pause inside a command starts a new one (Ihor Smal's review, 2026-11-21)."""

    def setUp(self):
        self.sim = SimulatorProcess("--frame-gap", "100")

    def tearDown(self):
        self.sim.close()

    def test_stray_character_does_not_shift_later_commands(self):
        os.write(self.sim.fd, b"3")                     # a stray character
        time.sleep(0.3)
        termios.tcflush(self.sim.fd, termios.TCIFLUSH)   # drop its echo
        out = self.sim.send("500000000000000000", expect_reply=True)
        self.assertEqual(out[:18], "500000000000000000")
        self.assertEqual(out[18:20], "10")
        log = self.sim.log()
        self.assertEqual(log[0]["cmd"], "incomplete")
        self.assertEqual(log[0]["rx"], "3")
        self.assertEqual(log[1]["cmd"], "ping_read_adc")

    def test_short_firmware_command_is_logged_by_name(self):
        os.write(self.sim.fd, b"5556")                  # uTmax with Firmware >= 200
        time.sleep(0.3)
        termios.tcflush(self.sim.fd, termios.TCIFLUSH)
        self.sim.send("300000000000000000", expect_reply=False)
        log = self.sim.log()
        self.assertEqual(log[0]["cmd"], "incomplete")
        self.assertIn("debug messages on", log[0]["note"])
        self.assertEqual(log[1]["cmd"], "end_discharge")   # not read as "00" settings


class LinkHandling(unittest.TestCase):
    """The --link path (Ihor Smal's review, 2026-11-21)."""

    def test_link_removed_on_sigterm(self):
        sim = SimulatorProcess()
        link, tmp = sim.link, sim.tmp
        os.close(sim.fd)
        sim.proc.terminate()
        sim.proc.wait(timeout=5)
        self.assertFalse(os.path.lexists(link))
        shutil.rmtree(tmp, ignore_errors=True)

    def test_a_real_file_is_not_deleted(self):
        tmp = tempfile.mkdtemp(prefix="utsim-")
        try:
            path = os.path.join(tmp, "port")
            with open(path, "w") as f:
                f.write("keep me")
            r = subprocess.run([sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"),
                                "--link", path], capture_output=True, text=True, timeout=10)
            self.assertNotEqual(r.returncode, 0)
            with open(path) as f:
                self.assertEqual(f.read(), "keep me")
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_second_copys_link_left_alone(self):
        first = SimulatorProcess()
        try:
            second = subprocess.Popen(
                [sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"), "--link", first.link,
                 "--ready-file", first.link + ".ready2"], stdout=subprocess.DEVNULL)
            for _ in range(100):
                if os.path.exists(first.link + ".ready2"):
                    break
                time.sleep(0.05)
            theirs = os.readlink(first.link)
            os.close(first.fd)
            first.proc.terminate()
            first.proc.wait(timeout=5)
            self.assertEqual(os.readlink(first.link), theirs)   # still the second copy's
            second.terminate()
            second.wait(timeout=5)
            self.assertFalse(os.path.lexists(first.link))
        finally:
            shutil.rmtree(first.tmp, ignore_errors=True)


class SmallFixes(unittest.TestCase):
    """The smaller points of Ihor Smal's review (2026-11-21)."""

    def sim(self, profile="utracer6"):
        from utracer_sim import UTracer
        return UTracer(Profile.find(profile), loads.from_spec("r:10k"), {}, None)

    def test_heater_word_is_masked_to_10_bits(self):
        p = Profile.find("utracer6")
        self.assertEqual(p.heater_code_to_volts(0x2000), 0.0)    # was full heater
        self.assertAlmostEqual(p.heater_code_to_volts(0x03FF), p.supply_v)

    def test_external_heater_word_is_the_same_in_40_and_10(self):
        for code in ("40", "10", "20"):
            sim = self.sim()
            sim.handle(command("40", vf=500))
            self.assertGreater(sim.vf, 0)
            sim.handle(command(code, vf=0xE000 | 500))
            self.assertEqual(sim.vf, 0.0, code)

    def test_gain_bytes_above_8_logged_as_auto(self):
        import io
        from utracer_sim import UTracer
        buf = io.StringIO()
        sim = UTracer(Profile.find("utracer6"), loads.Open(), {}, buf)
        sim.handle("00000000008F01090A")
        d = json.loads(buf.getvalue())["decoded"]
        self.assertEqual((d["gain_screen"], d["gain_anode"]), ("auto", "auto"))

    def test_above_the_rating_is_warned(self):
        import io
        from utracer_sim import UTracer
        buf = io.StringIO()
        sim = UTracer(Profile.find("utracer3"), loads.Open(), {}, buf)
        sim.handle(command("20", va=1023))                       # 358 V reservoir: fine
        sim.handle(command("20", va=1023, vs=1023))
        for line in buf.getvalue().splitlines():
            self.assertNotIn("warning", json.loads(line))
        buf = io.StringIO()
        sim = UTracer(Profile.find("utracer6"), loads.Open(), {}, buf)
        sim.handle(command("20", va=1023))                       # 1069 V - 19.5 V
        self.assertIn("1000 V rating", json.loads(buf.getvalue())["warning"])


class PlotCurves(unittest.TestCase):

    def run_plot(self, *args):
        tmp = tempfile.mkdtemp(prefix="utsim-plot-")
        try:
            out = os.path.join(tmp, "c.svg")
            r = subprocess.run([sys.executable, os.path.join(SIM_DIR, "plot_curves.py"),
                                "--out", out, *args], capture_output=True, text=True, timeout=20)
            svg = ""
            if os.path.exists(out):
                with open(out, encoding="utf-8") as f:
                    svg = f.read()
            return r, svg
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_tiny_currents_do_not_hang(self):
        r, svg = self.run_plot("--grids=-20")                    # 12AX7 cut off
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("</svg>", svg)

    def test_negative_grids_without_equals(self):
        r, svg = self.run_plot("--grids", "-1,-2")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("Vg -2 V", svg)

    def test_va_max_zero_is_an_error(self):
        r, _ = self.run_plot("--va-max", "0")
        self.assertEqual(r.returncode, 2)
        self.assertNotIn("Traceback", r.stderr)


class SerialPortSimulator(SimulatorProcess):
    """The simulator on a named port (--port), as on Windows with com0com or with
    a real serial cable. Here a pseudo-terminal stands in for the port: the
    simulator opens its far end by name through pyserial, the test talks to the
    near end."""

    def __init__(self, *args):
        if not HAVE_PTY:
            raise unittest.SkipTest("needs pseudo-terminals (Linux or macOS)")
        self.tmp = tempfile.mkdtemp(prefix="utsim-")
        self.logfile = os.path.join(self.tmp, "log.jsonl")
        ready = os.path.join(self.tmp, "ready")
        self.fd, far = os.openpty()
        tty.setraw(self.fd)
        tty.setraw(far)
        self.port_name = os.ttyname(far)
        self.proc = subprocess.Popen(
            [sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"), "--port", self.port_name,
             "--log", self.logfile, "--ready-file", ready, "--reply-delay", "5", *args],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        os.close(far)
        for _ in range(100):
            if os.path.exists(ready) or self.proc.poll() is not None:
                break
            time.sleep(0.05)
        self.link = None

    def close(self):
        os.close(self.fd)
        self.proc.terminate()
        self.proc.communicate(timeout=5)
        shutil.rmtree(self.tmp, ignore_errors=True)


def have_pyserial():
    try:
        import serial  # noqa: F401
        return True
    except ImportError:
        return False


@unittest.skipUnless(have_pyserial(), "needs pyserial")
class PortOption(unittest.TestCase):
    """--port: the simulator on a named serial port (Windows, or a real cable)."""

    def test_echo_and_reply_through_a_named_port(self):
        sim = SerialPortSimulator("--load", "r:10k")
        try:
            self.assertIsNone(sim.proc.poll(), "simulator exited")
            cmd = "00000000008F010808"
            self.assertEqual(sim.send(cmd, expect_reply=False), cmd)
            out = sim.send("500000000000000000", expect_reply=True)
            self.assertEqual(out[:18], "500000000000000000")
            self.assertEqual(len(out), 18 + 38)
            self.assertEqual(out[18:20], "10")
            self.assertEqual([e["cmd"] for e in sim.log()], ["settings", "ping_read_adc"])
        finally:
            sim.close()

    def test_banner_names_the_port(self):
        sim = SerialPortSimulator()
        try:
            time.sleep(0.2)
        finally:
            os.close(sim.fd)
            sim.proc.terminate()
            out = sim.proc.communicate(timeout=5)[0]
            shutil.rmtree(sim.tmp, ignore_errors=True)
        self.assertIn(f"serial port: {sim.port_name} at 9600 baud, 8N1", out)

    def test_missing_port_is_a_one_line_error(self):
        name = "COM250" if os.name == "nt" else "/dev/no-such-utracer-port"
        r = subprocess.run([sys.executable, os.path.join(SIM_DIR, "utracer_sim.py"),
                            "--port", name], capture_output=True, text=True, timeout=10)
        self.assertNotEqual(r.returncode, 0)
        self.assertNotIn("Traceback", r.stderr)
        self.assertIn(name, r.stderr)


@unittest.skipUnless(os.name == "nt", "Windows only")
class OnWindows(unittest.TestCase):

    def test_without_port_says_what_to_do(self):
        r = subprocess.run([sys.executable, os.path.join(SIM_DIR, "utracer_sim.py")],
                           capture_output=True, text=True, timeout=10)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("com0com", r.stderr)


class Faults(unittest.TestCase):

    def test_trip_sets_status_11(self):
        sim = SimulatorProcess("--trip")
        try:
            out = sim.send(command("10", va=100), expect_reply=True)
            self.assertEqual(out[18:20], "11")
        finally:
            sim.close()

    def test_supply_volts_changes_the_supply_reading(self):
        p = Profile.find("utracer6")
        p.supply_v = 15.0
        want = f"{p.vsu_code():04X}"
        sim = SimulatorProcess("--supply-volts", "15")
        try:
            out = sim.send("500000000000000000", expect_reply=True)
            self.assertEqual(out[18 + 26:18 + 30], want)      # Vsupply, the 7th field
        finally:
            sim.close()

    def test_bad_echo_on_n(self):
        sim = SimulatorProcess("--bad-echo-on", "2")
        try:
            self.assertEqual(sim.send("300000000000000000", False), "300000000000000000")
            self.assertEqual(sim.send("300000000000000000", False), "30000000000000000X")
            self.assertEqual(sim.send("300000000000000000", False), "300000000000000000")
            self.assertEqual(sim.log()[1].get("fault"), "bad echo")
        finally:
            sim.close()

    def test_garble_counts_replies_and_hits_the_anode_current(self):
        sim = SimulatorProcess("--garble-every", "2")
        try:
            sim.send("300000000000000000", False)          # no reply: not counted
            self.assertEqual(sim.send("500000000000000000", True)[18:20], "10")
            out = sim.send("500000000000000000", True)
            self.assertEqual(out[20:22], "ZZ")
            self.assertEqual(sim.log()[-1]["fault"], "garbled")
        finally:
            sim.close()

    def test_no_reply_after_n(self):
        sim = SimulatorProcess("--no-reply-after", "1")
        try:
            self.assertEqual(len(sim.send("500000000000000000", True)), 56)
            self.assertEqual(sim.send("500000000000000000", True), "")   # no echo either
            self.assertEqual(sim.log()[-1]["fault"], "connection lost")
        finally:
            sim.close()


if __name__ == "__main__":
    unittest.main()
