"""uTmax starts even when its tube library has no usable tube.

History: uTmax 3.01 picked the first tube of the library (data.csv) at
start-up without checking there was one. A library that was empty, had only its
header line, or whose rows were all damaged made it abort (std::out_of_range)
before the window opened, every time, until the file was fixed by hand.

uTmax6 keeps such a file under another name (data.csv.unusable-<date-time>),
installs the tube library built into the program, and says so in a message box.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_tube_library -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep, ROOT  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 2,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": -1, "VgEnd": -1, "VgSteps": 0,
         "Delay": 0}

with open(os.path.join(ROOT, "src", "data.csv")) as _f:
    HEADER = _f.readline()


class Unusable:
    data_csv = None

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"],
                                  data_csv=cls.data_csv, timeout=60)

    def test_starts_and_sweeps(self):
        self.assertEqual(self.report["exit_status"], 0)
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(len(self.report["points"]), 3)

    def test_says_so(self):
        dialogs = [n["text"] for n in self.report["notes"] if n["kind"] == "dialog"]
        self.assertTrue(any("no usable tubes" in d for d in dialogs), dialogs)

    def test_old_file_kept(self):
        kept = [f for f in self.report["files_after"] if f.startswith("data.csv.unusable-")]
        self.assertEqual(len(kept), 1, self.report["files_after"])

    def test_built_in_library_installed(self):
        self.assertIn("12AX7/ECC83,", self.report["data_csv_after"])


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class HeaderOnly(Unusable, unittest.TestCase):
    data_csv = HEADER


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class EmptyFile(Unusable, unittest.TestCase):
    data_csv = ""


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class DamagedRows(Unusable, unittest.TestCase):
    data_csv = HEADER + "12AX7,1,6,2\nnot a tube at all\n"


if __name__ == "__main__":
    unittest.main()
