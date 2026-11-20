"""uTmax leaves cal.txt alone when nothing in it has changed.

History: uTmax 3.01 rewrote cal.txt whenever the serial port opened (every
start, and every reconnect), on every exit, and after plot and pen updates,
changed or not. On a Raspberry Pi that is a needless write to the SD card each
time, and a power cut during one could leave the calibration half written.
Dean Jenkins' fork stopped the start-up rewrite.

uTmax6 builds the text and writes it only if it differs from the file, through
a temporary file renamed into place (src/cal_file.h).

The test starts uTmax with exactly the cal.txt it would write itself, runs a
sweep and quits: the file must be the same file (inode) with the same
modification time, to the nanosecond. With a cal.txt that differs, it must
still be rewritten.

Making that pass showed a second 3.01 bug: Index= (the tube last used) was
applied before the tube list was loaded, so it was lost; every start came up on
the first tube and wrote Index=0. uTmax6 applies it at the end of start-up.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_cal_file -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 2,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": -1, "VgEnd": -1, "VgSteps": 0,
         "Delay": 0}

# What uTmax writes for this set-up, 12AX7/ECC83 selected (Index=2), firmware
# version read from the simulator (100).
AS_UTMAX_WRITES = ("COM={port}\nVa=1\nVs=1\nIa=1\nIs=1\nVsu=1\nVg0=0\nVgm=1\nVn=1\n"
                   "RaVal=4700\nVaMax=1000\nIaRsense=4.7\nIsRsense=4.7\nVgMax=100\n"
                   "Firmware=100\nSavePath={home}\nStyle={style}\nSort=0\nIndex=2\n"
                   "uTracer6=1\nheater=0\n")


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class UnchangedCalFileNotRewritten(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"],
                                  cal=AS_UTMAX_WRITES)

    def test_sweep_ran(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])

    def test_content_unchanged(self):
        before = AS_UTMAX_WRITES.split("\n")
        after = self.report["cal_after"].split("\n")
        # Paths differ per run; compare the keys and the values that are not paths.
        strip = lambda ls: [l for l in ls if not l.startswith(("COM=", "SavePath=", "Style="))]
        self.assertEqual(strip(after), strip(before))

    def test_file_not_rewritten(self):
        before, after = self.report["cal_stat"]
        self.assertEqual(after, before, "cal.txt was rewritten (inode, mtime changed)")


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class LastTubeRemembered(unittest.TestCase):
    """3.01 applied Index= before the tube list was loaded, so it was lost and
    every start came up on the first tube. The sweep here picks no tube."""

    @classmethod
    def setUpClass(cls):
        sweep = {k: v for k, v in SWEEP.items() if k != "tube"}
        cls.report, _ = run_sweep(sweep, sim_args=["--load", "triode:12AX7"],
                                  cal=AS_UTMAX_WRITES)

    def test_index_kept(self):
        self.assertIn("Index=2\n", self.report["cal_after"])
        before, after = self.report["cal_stat"]
        self.assertEqual(after, before)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class ChangedCalFileStillSaved(unittest.TestCase):
    """A cal.txt in another layout (the harness's own) is still rewritten."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"])

    def test_rewritten(self):
        before, after = self.report["cal_stat"]
        self.assertNotEqual(after, before)
        self.assertIn("Index=2\n", self.report["cal_after"])
        self.assertIn("Firmware=100\n", self.report["cal_after"])


if __name__ == "__main__":
    unittest.main()
