"""Known bugs in uTmax 3.01, recorded as tests before they are fixed.

Each test states what uTmax 3.01 does and what the uTracer makes of it. When a bug
is fixed, its test is changed in the same commit to assert the correct behaviour.

Bug 1: positive grid voltage on a uTracer6.
    uTmax 3.01 GetVg() (mainwindow.cpp:2293-2297) returns a signed int as uint, so
    a positive grid gives a negative number; "%04X" then prints 8 hex digits and
    snprintf(buf, 19, ...) (mainwindow.cpp:1415) cuts the command at 18 characters.
    tests/repro/positive_grid_word.cpp reproduces the arithmetic exactly; its output
    for +1.0 V is the command below. Dekker's firmware reads the grid word FFFF as
    bit 15 set plus full magnitude: +100 V on the grid instead of +1 V. The heater
    word is also lost (FEB8 instead of 006B).
"""

import os
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "utracer_sim"))
from hardware import Profile  # noqa: E402

# What uTmax 3.01 sends for Va code 00E3, Vs 0, Vg = +1.0 V, heater code 006B
# (output of tests/repro/positive_grid_word.cpp).
UTMAX_301_PLUS_1V = "1000E30000FFFFFEB8"
INTENDED_PLUS_1V = "1000E300008148006B"


class PositiveGridOnUTracer6(unittest.TestCase):

    def test_intended_command_means_plus_1_volt(self):
        p = Profile.find("utracer6")
        self.assertAlmostEqual(p.grid_code_to_volts(int(INTENDED_PLUS_1V[10:14], 16)), 1.0, places=2)

    def test_utmax_301_command_means_plus_100_volts(self):
        # BUG, recorded as current behaviour: the uTracer6 would apply +100 V.
        p = Profile.find("utracer6")
        grid_word = int(UTMAX_301_PLUS_1V[10:14], 16)
        self.assertAlmostEqual(p.grid_code_to_volts(grid_word), 100.0, places=2)

    def test_utmax_301_command_loses_the_heater_word(self):
        self.assertNotEqual(UTMAX_301_PLUS_1V[14:18], INTENDED_PLUS_1V[14:18])


if __name__ == "__main__":
    unittest.main()
