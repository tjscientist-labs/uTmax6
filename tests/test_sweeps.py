"""Whole sweeps, end to end: the real uTmax drives the software uTracer.

Uses the test hook ("uTmax --test-sweep", src/test_hook.cpp) through
tests/utmax_run.py. Each test checks two sides: what uTmax reports (its own
data store, status messages and dialogs) and what the simulator received and
answered (its command log). A resistor load gives an exact expected current,
so a wrong conversion anywhere shows up as a wrong number.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_sweeps -v
Each sweep takes 5-15 seconds.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep

TRIODE = {"tube": "12AX7/ECC83", "Vh": 12.6,
          "VsStart": 30, "VsEnd": 30, "VsSteps": 0,
          "VgStart": 0, "VgEnd": 0, "VgSteps": 0}


def measures(commands, kind="measure"):
    return [c for c in commands if c["cmd"] == kind]


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class SweepUTracer6(unittest.TestCase):
    """A plain sweep on a uTracer6 into 100 kOhm: 30, 120, 210, 300 V."""

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(
            dict(TRIODE, VaStart=30, VaEnd=300, VaSteps=3, Delay=0),
            sim_args=["--load", "r:100k"])

    def test_completes(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(self.report["exit_status"], 0)

    def test_four_points(self):
        self.assertEqual(len(self.report["points"]), 4)

    def test_simulator_got_the_voltages_asked_for(self):
        # Four sweep points, then End() parks the anode at a few volts.
        sweep = measures(self.commands)[:4]
        for c, want in zip(sweep, (30, 120, 210, 300)):
            got = c["measured"]["Va_tube_V"]
            self.assertAlmostEqual(got, want, delta=0.02 * want + 1, msg=c)

    def test_current_is_va_over_r(self):
        for p in self.report["points"]:
            want = p["Va"] / 100.0                      # mA, 100 kOhm
            self.assertAlmostEqual(p["Ia_mA"], want, delta=0.02 * want + 0.01, msg=p)

    def test_heater_on_during_sweep_and_off_after(self):
        sweep = measures(self.commands)
        self.assertAlmostEqual(sweep[0]["decoded"]["Vf_Vrms"], 12.6, delta=0.1)
        self.assertEqual(sweep[-1]["decoded"]["Vf_Vrms"], 0.0)

    def test_no_dialogs(self):
        self.assertEqual([n for n in self.report["notes"] if n["kind"] == "dialog"], [])


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class DelayedSweepAbove425V(unittest.TestCase):
    """A sweep with a hold delay to 600 V on a uTracer6.

    uTmax 3.01 stopped this at the first point above 425 V ("Internal Error:
    Va or Vs excessive"); fixed in uTmax6 (src/voltage_limit.h)."""

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(
            dict(TRIODE, VaStart=30, VaEnd=600, VaSteps=2, Delay=1),
            sim_args=["--load", "r:100k"])

    def test_completes_all_three_points(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(len(self.report["points"]), 3)
        self.assertAlmostEqual(max(p["Va"] for p in self.report["points"]), 600, delta=12)

    def test_used_the_hold_command(self):
        self.assertGreaterEqual(len(measures(self.commands, "apply_hold")), 3)

    def test_no_internal_error(self):
        texts = " ".join(n["text"] for n in self.report["notes"])
        self.assertNotIn("excessive", texts)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class SweepWithPositiveGridBoard(unittest.TestCase):
    """The plain sweep again, with Dekker's extension board (42-character replies)."""

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(
            dict(TRIODE, VaStart=30, VaEnd=300, VaSteps=3, Delay=0),
            sim_args=["--load", "r:100k", "--grid-board"])

    def test_completes_with_correct_currents(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(len(self.report["points"]), 4)
        for p in self.report["points"]:
            self.assertAlmostEqual(p["Ia_mA"], p["Va"] / 100.0, delta=0.02 * p["Va"] / 100 + 0.01)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class SweepAboveVaMaxIsRefused(unittest.TestCase):
    """Asking for more than VaMax is refused before anything is sent."""

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(
            dict(TRIODE, VaStart=30, VaEnd=1200, VaSteps=3, Delay=0),
            sim_args=["--load", "r:100k"], va_max=1000)

    def test_entry_error_and_no_sweep(self):
        self.assertEqual(self.report["result"], "never started")
        dialogs = [n["text"] for n in self.report["notes"] if n["kind"] == "dialog"]
        self.assertTrue(any("Va End" in d for d in dialogs), dialogs)
        self.assertEqual(measures(self.commands), [])
        self.assertEqual(measures(self.commands, "apply_hold"), [])


if __name__ == "__main__":
    unittest.main()
