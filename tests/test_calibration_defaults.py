"""Calibration values always start from defined defaults.

History: uTmax 3.01 never gave its calibration values (calData) a default.
MainWindow lives on the stack, so a key missing from cal.txt took whatever was
in memory (seen in a test run: an anode factor of -89254.6, and uTmax sent 0 V
instead of 30 V), and 3.01 then wrote that value back to cal.txt at the next
start. The cal.txt it created on a new install wrote obsolete keys (Vg05, Vg1,
Vg4, Vg40) instead of the grid calibration Vg0 and Vgm, so every new install
ran with an uninitialised grid calibration until the wizard was used. An
unreadable value ("Va=abc") became 0.

uTmax6 gives every value a default in calData_t, writes Vg0/Vgm in a new
cal.txt, and keeps the default for a value it cannot read.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_calibration_defaults -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

# A uTracer6 cal.txt with NO calibration factors (Va, Vs, Ia, Is, Vsu, Vg0, Vgm, Vn).
PARTIAL = ("COM={port}\nRaVal=4700\nVaMax=1000\nIaRsense=4.7\nIsRsense=4.7\nVgMax=100\n"
           "Firmware=1\nuTracer6=1\nStyle={style}\n")

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 3,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": -2, "VgEnd": -2, "VgSteps": 0,
         "Delay": 0}


def sweep_measures(commands):
    return [c for c in commands if c["cmd"] == "measure"][:4]


class CorrectSweep:
    """Checks shared by the cases where calibration factors must default to 1 / 0."""

    def test_completes(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])

    def test_anode_voltages_as_asked(self):
        for c, want in zip(sweep_measures(self.commands), (30, 120, 210, 300)):
            self.assertAlmostEqual(c["measured"]["Va_tube_V"], want, delta=0.02 * want + 1, msg=c)

    def test_grid_voltage_as_asked(self):
        for c in sweep_measures(self.commands):
            self.assertAlmostEqual(c["decoded"]["Vg_V"], -2.0, delta=0.05, msg=c)

    def test_currents_reported_correctly(self):
        self.assertEqual(len(self.report["points"]), 4)
        for p in self.report["points"]:
            self.assertAlmostEqual(p["Ia_mA"], p["Va"] / 100.0, delta=0.02 * p["Va"] / 100 + 0.01, msg=p)

    def test_cal_txt_written_back_with_defaults(self):
        cal = self.report["cal_after"].splitlines()
        for key in ("Va=1", "Vs=1", "Ia=1", "Is=1", "Vsu=1", "Vg0=0", "Vgm=1", "Vn=1"):
            self.assertIn(key, cal)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class MissingCalibrationKeys(CorrectSweep, unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "r:100k"], cal=PARTIAL)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class UnreadableCalibrationValues(CorrectSweep, unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        bad = PARTIAL + "Va=abc\nVs=\nIa=1,0\nIs=x\nVsu=?\nVg0=zero\nVgm=one\nVn=-\n"
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "r:100k"], cal=bad)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class NewInstallCalFile(unittest.TestCase):
    """No cal.txt at all: the one uTmax creates must carry the grid calibration."""

    @classmethod
    def setUpClass(cls):
        # No port in the new file (COM=COM1), so the hook gives up after its timeout.
        cls.report, _ = run_sweep(SWEEP, cal=False, timeout=25)

    def test_new_file_has_grid_calibration(self):
        cal = self.report["cal_after"].splitlines()
        self.assertIn("Vg0=0", cal)
        self.assertIn("Vgm=1", cal)

    def test_new_file_has_no_obsolete_grid_keys(self):
        for key in ("Vg05", "Vg1=", "Vg4=", "Vg40"):
            self.assertNotIn(key, self.report["cal_after"])


if __name__ == "__main__":
    unittest.main()
