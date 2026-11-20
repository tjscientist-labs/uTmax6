"""A positive grid is allowed only when the positive-grid board is fitted.

History: uTmax 3.01 allowed a positive grid voltage on any uTracer6. Without
Ronald Dekker's positive-grid extension board the firmware has no positive grid
supply to send it to. The uTracer reports the board in every reply: the firmware
checks RC0 at power-up and sets the highest bit of the status byte (Dekker,
uTracer6 log, section 14).

uTmax6 remembers that bit from the last reply. Start refuses a positive grid
without it (an entry error, nothing sent), and the command check refuses a
positive grid word without it (src/command_check.h). With the board fitted, a
positive-grid sweep runs and the grid words have bit 15 set.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_positive_grid -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 50, "VaEnd": 100, "VaSteps": 1,
         "VsStart": 50, "VsEnd": 50, "VsSteps": 0, "VgStart": 2, "VgEnd": 0, "VgSteps": 2,
         "Delay": 0}


def grid_words(commands):
    return [int(c["rx"][10:14], 16) for c in commands if c["cmd"] in ("measure", "apply_hold")]


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class WithoutTheBoard(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "r:100k"], timeout=60)

    def test_refused_at_start(self):
        dialogs = [n["text"] for n in self.report["notes"] if n["kind"] == "dialog"]
        self.assertTrue(any("positive-grid extension board" in d for d in dialogs), dialogs)

    def test_no_positive_grid_word_sent(self):
        self.assertEqual([w for w in grid_words(self.commands) if w & 0x8000], [])
        self.assertEqual([c for c in self.commands if "warning" in c], [])


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class WithTheBoard(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "r:100k", "--grid-board"])

    def test_sweep_runs(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(len(self.report["points"]), 6)

    def test_positive_grid_sent(self):
        positive = [w for w in grid_words(self.commands) if w & 0x8000]
        self.assertTrue(positive, "no positive grid word sent")
        # +2 V and +1 V: Dekker's 32767 * |Vg| / 100, with bit 15 set
        self.assertIn(0x8000 | round(32767 * 2 / 100), positive)


if __name__ == "__main__":
    unittest.main()
