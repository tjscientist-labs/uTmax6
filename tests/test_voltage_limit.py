"""The backstop on anode and screen voltage before each measurement.

History: uTmax 3.01 checked a fixed 425 V (the uTracer3's ceiling) in the hold
path only, so on a uTracer6 every sweep with a delay stopped above 425 V with
"Internal Error: Va or Vs excessive", and sweeps without a delay had no backstop.
uTmax6 checks every measurement against VaMax, capped at 425 V (uTracer3) or
1000 V (uTracer6): src/voltage_limit.h.

Compiles tests/cpp/test_voltage_limit.cpp against the header and runs it.
Needs g++ (skipped without it). A full sweep is not yet driven end to end:
that needs a test hook in uTmax (docs/PLAN.md).
"""

import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


@unittest.skipUnless(shutil.which("g++"), "needs g++")
class VoltageLimit(unittest.TestCase):

    def test_cpp_checks_pass(self):
        tmp = tempfile.mkdtemp(prefix="voltlimit-")
        try:
            exe = os.path.join(tmp, "test_voltage_limit")
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Werror", "-o", exe,
                            os.path.join(ROOT, "tests", "cpp", "test_voltage_limit.cpp")],
                           check=True)
            r = subprocess.run([exe], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_no_fixed_425_left_in_the_program(self):
        with open(os.path.join(ROOT, "src", "mainwindow.cpp"), encoding="latin-1") as f:
            src = f.read()
        self.assertNotIn(">425", src.replace(" ", ""))


if __name__ == "__main__":
    unittest.main()
