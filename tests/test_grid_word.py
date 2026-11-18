"""The uTracer6 grid word, fixed in uTmax6 (was: positive grid sent as +100 V).

Compiles tests/cpp/test_grid_word.cpp against src/grid_word.h, runs its checks,
then takes the measure commands it builds and decodes them with the simulator's
uTracer6 model, so the encoding is checked against the device, not against itself.

History: uTmax 3.01 sent 1000E30000FFFFFEB8 for +1 V (grid FFFF = +100 V,
heater word lost). tests/repro/positive_grid_word.cpp reproduces that.
Needs g++ (skipped without it).
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "utracer_sim"))
from hardware import Profile  # noqa: E402

UTMAX_301_PLUS_1V = "1000E30000FFFFFEB8"


@unittest.skipUnless(shutil.which("g++"), "needs g++")
class GridWord(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.mkdtemp(prefix="gridword-")
        cls.exe = os.path.join(cls.tmp, "test_grid_word")
        subprocess.run(["g++", "-std=c++17", "-Wall", "-Werror", "-o", cls.exe,
                        os.path.join(ROOT, "tests", "cpp", "test_grid_word.cpp")], check=True)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def test_cpp_checks_pass(self):
        r = subprocess.run([self.exe], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout)

    def test_device_reads_the_voltage_that_was_asked_for(self):
        p = Profile.find("utracer6")
        out = subprocess.run([self.exe, "--commands"], capture_output=True, text=True,
                             check=True).stdout.split("\n")
        for line in filter(None, out):
            volts, cmd = line.split()
            asked = max(-100.0, min(100.0, float(volts)))
            self.assertEqual(len(cmd), 18, cmd)
            self.assertEqual(cmd[14:18], "006B", f"heater word lost in {cmd}")
            got = p.grid_code_to_volts(int(cmd[10:14], 16))
            self.assertAlmostEqual(got, asked, delta=0.01, msg=f"{volts} V sent as {cmd}")

    def test_the_old_301_command_would_have_meant_plus_100_volts(self):
        p = Profile.find("utracer6")
        self.assertAlmostEqual(p.grid_code_to_volts(int(UTMAX_301_PLUS_1V[10:14], 16)), 100.0, places=2)


if __name__ == "__main__":
    unittest.main()
