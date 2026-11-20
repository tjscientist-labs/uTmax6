"""uTmax plots a sweep whether or not its style sheet can be found.

History: uTmax 3.01 built its list of plot-line pens only while reading the
style sheet named in cal.txt (Style=). If that file could not be opened (moved,
renamed, a typo, a theme folder not copied to a new machine), the list stayed
empty, and the first plotted point divided by its length
(PlotTabWidget::plotUpdate, "gpnum % penList->length()"): uTmax died with a
floating-point exception (SIGFPE) on the first point of every sweep. The plot
code also read a pen past the end of the list when a curve number jumped by more
than one, or when "colour by group" picked a pen by screen-voltage step.

uTmax6 builds the default pens when the main window is created (src/plot_pens.h)
and fetches every pen through penAt(), which grows the list as needed.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_plot_pens -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

# A complete uTracer6 cal.txt whose style sheet does not exist.
NO_STYLE = ("COM={port}\nVa=1\nVs=1\nIa=1\nIs=1\nVsu=1\nVg0=0\nVgm=1\nVn=1\n"
            "RaVal=4700\nVaMax=1000\nIaRsense=4.7\nIsRsense=4.7\nVgMax=100\n"
            "Firmware=1\nuTracer6=1\nheater=0\nStyle=/no/such/folder/theme.qss\n")

# Three grid curves, so more than one pen is used.
SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 3,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": 0, "VgEnd": -2, "VgSteps": 2,
         "Delay": 0}


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class SweepWithoutStyleSheet(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"],
                                             cal=NO_STYLE)

    def test_does_not_crash(self):
        self.assertEqual(self.report["exit_status"], 0, self.report["notes"])

    def test_completes(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])

    def test_all_points_measured(self):
        self.assertEqual(len(self.report["points"]), 12)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class SweepWithStyleSheet(unittest.TestCase):
    """The same sweep with the style sheet found: unchanged behaviour."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"])

    def test_completes(self):
        self.assertEqual(self.report["exit_status"], 0, self.report["notes"])
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(len(self.report["points"]), 12)


if __name__ == "__main__":
    unittest.main()
