"""uTmax sets up ~/uTmax_files on a new machine and reads its style sheet there.

History: uTmax 3.01 never created ~/uTmax_files. On a new machine it opened a
file dialog asking for the tube library (data.csv), and since the folder did not
exist, cal.txt was never written: every setting was lost at exit, silently. It
also read its application style sheet (Style.qss) from whatever folder it was
started in, and wrote its debug logs to the current folder or the home folder.
Dean Jenkins' fork checked for the folder.

uTmax6 creates the folder at start-up, copies in the tube library built into the
program if there is none, and looks for Style.qss in the folder first
(src/app_paths.h).

On 3.01's code the first test hangs in the file dialog until the run times out.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_app_paths -v
"""

import os
import subprocess
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep, ROOT  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 2,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": -1, "VgEnd": -1, "VgSteps": 0,
         "Delay": 0}


def run_or_hang(**kw):
    """run_sweep, but a uTmax that never finishes (a file dialog) is a failure."""
    try:
        return run_sweep(**kw)
    except (subprocess.TimeoutExpired, AssertionError) as e:
        raise AssertionError(f"uTmax did not finish: {e}") from None


def real_serial_adapters():
    """USB serial adapters on this machine. A new install opens the first one, so
    the new-machine tests are skipped when there is one: they must never talk to
    a real uTracer (or anything else)."""
    import glob
    return glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
@unittest.skipIf(real_serial_adapters(), "a USB serial adapter is plugged in")
class NewMachine(unittest.TestCase):
    """A home folder with no uTmax_files at all. The new cal.txt names a serial
    port that isn't there, so uTmax can't connect; the hook never presses Start
    (Start=0) and ends with "no port"."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_or_hang(sweep={"Start": 0}, fresh=True, timeout=60)

    def test_starts(self):
        self.assertIn(self.report["result"], ("no port", "ready"), self.report["notes"])

    def test_folder_created_with_files(self):
        self.assertIsNotNone(self.report["files_after"], "~/uTmax_files not created")
        self.assertIn("cal.txt", self.report["files_after"])
        self.assertIn("data.csv", self.report["files_after"])

    def test_tube_library_installed(self):
        # uTmax rewrites the header line when it saves on exit; compare the tubes.
        with open(os.path.join(ROOT, "src", "data.csv")) as f:
            tubes = sorted(l.split(",")[0] for l in f.read().splitlines()[1:] if l)
        after = sorted(l.split(",")[0] for l in self.report["data_csv_after"].splitlines()[1:] if l)
        self.assertEqual(after, tubes)

    def test_calibration_saved(self):
        self.assertIn("Vgm=", self.report["cal_after"])


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class StyleSheetFromTheFolder(unittest.TestCase):
    """Started from another folder, uTmax still uses ~/uTmax_files/Style.qss."""

    STYLE = "/* uTmax6 test style sheet */\nQWidget { }\n"

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"],
                                  start_in="elsewhere", app_style=cls.STYLE)

    def test_sweep_ran(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])

    def test_style_sheet_used(self):
        self.assertEqual(self.report["appStyleSheet"], self.STYLE)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class ExistingTubeLibraryKept(unittest.TestCase):
    """A tube library already in the folder is never replaced."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"])

    def test_sweep_ran(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertIn("12AX7/ECC83,", self.report["data_csv_after"])


if __name__ == "__main__":
    unittest.main()
