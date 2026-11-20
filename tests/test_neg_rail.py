"""The negative-supply reading in the result string.

History: the uTracer6 reports its -85 V rail, measured through R27 200 kOhm /
R29 4k7 (Dekker, uTracer6 log, fig. 7.2 and section 9). uTmax 3.01 decoded it
with the -15 V rail's divider (47k / 4k7), so a healthy uTracer6 showed about
-17.7 V in the debug window and the calibration wizard. On the uTracer3 it
wrote the divider as (47 + 2)/2, integer division, 24 instead of 24.5, about 2 %
off. uTmax6: src/neg_rail.h.

Two checks: the conversion on its own (tests/cpp/test_neg_rail.cpp, needs g++),
and whole sweeps through the test hook, where the real uTmax reads the rail the
simulator reports (needs UTMAX_BIN).
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6,
         "VaStart": 30, "VaEnd": 60, "VaSteps": 1,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0,
         "VgStart": 0, "VgEnd": 0, "VgSteps": 0, "Delay": 0}


@unittest.skipUnless(shutil.which("g++"), "needs g++")
class NegRailConversion(unittest.TestCase):

    def test_cpp_checks_pass(self):
        tmp = tempfile.mkdtemp(prefix="negrail-")
        try:
            exe = os.path.join(tmp, "test_neg_rail")
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Werror", "-o", exe,
                            os.path.join(ROOT, "tests", "cpp", "test_neg_rail.cpp")],
                           check=True)
            r = subprocess.run([exe], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class NegRailUTracer6(unittest.TestCase):
    """The simulator's uTracer6 reports -85 V; uTmax must show -85 V."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "r:100k"])

    def test_reads_minus_85_volts(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertAlmostEqual(self.report["Vn"], -85.0, delta=0.5)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class NegRailUTracer3(unittest.TestCase):
    """The simulator's uTracer3 boost converter is at -40 V (a placeholder)."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "r:100k"],
                                  utracer6=False, va_max=400)

    def test_reads_minus_40_volts(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertAlmostEqual(self.report["Vn"], -40.0, delta=0.3)


if __name__ == "__main__":
    unittest.main()
