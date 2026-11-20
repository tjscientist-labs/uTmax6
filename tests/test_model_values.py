"""Hardware values that must match the uTracer model (src/model_values.h).

History: uTmax converts volts to the uTracer's numbers with the divider resistor
(RaVal), the sense resistors and the grid full scale (VgMax) from cal.txt. In
uTmax 3.01, ticking "uTracer6" set VgMax to 100 and left the divider at the
uTracer3's 6800 ohm (a uTracer6 has 4700). Run against the software uTracer on
2026-11-20 with those values, a request for 300 V charged the reservoir to
460.8 V, uTmax showed 299.3 V, and it showed 1.15 mA for a real 4.41 mA. With
VgMax left at 50 the grid gets twice the bias asked for. Its "Reset
calibration" set 5350 ohm / 5.1 ohm / 50 V, which fits neither model.

uTmax6: changing the model moves each value still at the other model's default
to this model's; Reset uses the model's values; Start refuses a uTracer6 whose
divider or grid full scale is more than 10 % from Dekker's (nothing is sent).

The C++ part needs g++. The end-to-end part needs a built uTmax:
    UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_model_values -v
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 100, "VaEnd": 300, "VaSteps": 2,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": 0, "VgEnd": 0, "VgSteps": 0,
         "Delay": 0}

# A uTracer6 with the uTracer3's divider and grid full scale: the state 3.01 left
# after ticking "uTracer6" on a new install (VgMax 100 would have been set; 50
# here so that both checks are exercised).
WRONG_CAL = ("COM={port}\nVa=1\nVs=1\nIa=1\nIs=1\nVsu=1\nVg0=0\nVgm=1\nVn=1\n"
             "RaVal=6800\nVaMax=1000\nIaRsense=18\nIsRsense=18\nVgMax=50\n"
             "Firmware=1\nuTracer6=1\nheater=0\nStyle={style}\n")


@unittest.skipUnless(shutil.which("g++"), "needs g++")
class Header(unittest.TestCase):

    def test_cpp_checks_pass(self):
        tmp = tempfile.mkdtemp(prefix="modelvalues-")
        try:
            exe = os.path.join(tmp, "test_model_values")
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Werror", "-o", exe,
                            os.path.join(ROOT, "tests", "cpp", "test_model_values.cpp")],
                           check=True)
            r = subprocess.run([exe], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_reset_no_longer_uses_5350(self):
        with open(os.path.join(ROOT, "src", "cal_dialog.cpp"), encoding="latin-1") as f:
            src = f.read()
        self.assertNotIn("RaVal=5350", src.replace(" ", ""))


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class WrongValuesRefused(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "r:100k"],
                                             cal=WRONG_CAL, timeout=60)

    def test_refused_at_start(self):
        dialogs = [n["text"] for n in self.report["notes"] if n["kind"] == "dialog"]
        self.assertTrue(any("RaVal" in d and "VgMax" in d for d in dialogs), dialogs)

    def test_no_high_voltage_sent(self):
        self.assertEqual([c for c in self.commands if c["cmd"] in ("measure", "apply_hold")], [])
        self.assertEqual(self.report["points"], [])


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class RightValuesRun(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "r:100k"])

    def test_requested_voltage_reaches_the_tube(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        highest = max(c["decoded"]["Va_reservoir_V"] for c in self.commands if c["cmd"] == "measure")
        # 300 V on the tube = reservoir 300 V + the 19.5 V cathode offset (+ the
        # sense-resistor drop), not the 460 V of the wrong divider.
        self.assertAlmostEqual(highest, 319.5, delta=2.5)


if __name__ == "__main__":
    unittest.main()
