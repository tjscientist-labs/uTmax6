"""Tests for the software uTracer (tools/utracer_sim).

Run from the repository root:   python3 -m unittest discover -s tests -v

The protocol tests start the simulator as a separate process on a pseudo-terminal
and talk to it exactly as a host program would: send a command, read back the
echo and the reply.
"""

import json
import os
import subprocess
import sys
import tempfile
import termios
import time
import tty
import unittest

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

    def send(self, cmd, expect_reply):
        os.write(self.fd, cmd.encode())
        want = len(cmd) + (38 if expect_reply else 0)
        got = b""
        deadline = time.time() + 3
        while len(got) < want and time.time() < deadline:
            import select
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


class Protocol(unittest.TestCase):

    def setUp(self):
        self.sim = SimulatorProcess("--load", "r:10k")

    def tearDown(self):
        self.sim.close()

    def test_every_character_is_echoed_and_settings_get_no_reply(self):
        cmd = "00000000008F010808"
        self.assertEqual(self.sim.send(cmd, expect_reply=False), cmd)

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
        expected = (p.hv_code_to_volts(code) - p.supply_v) / (10000 + p.r_sense_a) * 1000
        self.assertAlmostEqual(ia_ma, expected, delta=expected * 0.02)
        entry = [e for e in self.sim.log() if e["cmd"] == "measure"][-1]
        self.assertEqual(entry["measured"]["status"], "10")

    def test_end_measurement_discharges(self):
        p = Profile.find("utracer6")
        code = p.hv_volts_to_code(300)
        self.sim.send(command("20", va=code), expect_reply=False)
        self.sim.send("300000000000000000", expect_reply=False)
        out = self.sim.send("500000000000000000", expect_reply=True)
        self.assertEqual(int(out[18 + 18:18 + 22], 16), 0)   # Va field back to zero


class GridBoard(unittest.TestCase):
    """Dekker's positive-grid extension board: status bit 7 set, 42-character reply."""

    def test_reply_is_42_characters_with_status_90(self):
        sim = SimulatorProcess("--grid-board")
        try:
            os.write(sim.fd, b"500000000000000000")
            time.sleep(0.3)
            out = os.read(sim.fd, 256).decode()
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

    def test_no_reply_after_n(self):
        sim = SimulatorProcess("--no-reply-after", "1")
        try:
            self.assertEqual(len(sim.send("500000000000000000", True)), 56)
            self.assertEqual(len(sim.send("500000000000000000", True)), 18)   # echo only
        finally:
            sim.close()


if __name__ == "__main__":
    unittest.main()
