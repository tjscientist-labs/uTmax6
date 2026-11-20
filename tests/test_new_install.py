"""A new install starts with settings that work on Linux, and themes that work
from any folder.

History: the cal.txt uTmax 3.01 created on a new machine named the serial port
COM1 (a Windows name; on Linux nothing opened), and the style sheet as
<program folder>/style.qss, a file that doesn't exist. Its themes find their images with paths such as
url(./Themes/img/dark/check.png), relative to the folder uTmax was started from.

uTmax6 names the first USB serial port it sees (else /dev/ttyUSB0 on Linux),
installs the themes built into the program in ~/uTmax_files/Themes, starts on
"Utracer Dark", and makes a theme's ./ paths absolute (src/new_install.h).
VaMax stays 300 V on a new install, on purpose.

The new-machine runs are skipped when a USB serial adapter is plugged in, so a
test can never talk to a real uTracer, and they never press Start.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_new_install -v
"""

import os
import re
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep, ROOT  # noqa: E402
from test_app_paths import real_serial_adapters, run_or_hang  # noqa: E402


def cal_values(text):
    return dict(l.split("=", 1) for l in text.splitlines() if "=" in l)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
@unittest.skipIf(real_serial_adapters(), "a USB serial adapter is plugged in")
class NewInstallDefaults(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_or_hang(sweep={"Start": 0}, fresh=True, timeout=60)
        cls.cal = cal_values(cls.report["cal_after"])
        cls.home = os.path.join(cls.report["tmp"], "home")

    def test_serial_port_is_not_com1(self):
        if sys.platform.startswith("linux"):
            # Qt saves a /dev/ port by its name alone ("ttyUSB0") and opens it either way.
            self.assertRegex(self.cal["COM"], r"^(/dev/)?tty(USB|ACM)\d+$")
        self.assertNotEqual(self.cal["COM"], "COM1")

    def test_theme_is_installed_and_named(self):
        self.assertEqual(self.cal["Style"],
                         os.path.join(self.home, "uTmax_files", "Themes", "Utracer Dark.qss"))
        self.assertIn(os.path.join("Themes", "Utracer Dark.qss"), self.report["themes_after"])

    def test_every_theme_and_image_installed(self):
        src = os.path.join(ROOT, "src", "Themes")
        want = [f for f in os.listdir(src) if f.endswith(".qss")]
        for f in want:
            self.assertIn(os.path.join("Themes", f), self.report["themes_after"])
        self.assertIn(os.path.join("Themes", "img", "dark", "check.png"), self.report["themes_after"])

    def test_voltage_limit_stays_safe(self):
        self.assertEqual(self.cal["VaMax"], "300")

    def test_theme_images_found_from_any_folder(self):
        # Started from the bare home folder: ./Themes/... would not resolve here.
        sheet = self.report["windowStyleSheet"]
        urls = re.findall(r"url\(([^)]*)\)", sheet)
        self.assertTrue(urls, "theme not applied")
        for u in urls:
            self.assertTrue(os.path.isabs(u), u)
            self.assertTrue(u.startswith(os.path.join(self.home, "uTmax_files", "Themes")), u)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class ThemeOutsideAThemesFolder(unittest.TestCase):
    """A theme that isn't in a Themes folder is applied unchanged (as in 3.01)."""

    @classmethod
    def setUpClass(cls):
        cls.report, _ = run_sweep({"Start": 0}, sim_args=["--load", "triode:12AX7"])

    def test_unchanged(self):
        self.assertEqual(self.report["result"], "ready", self.report["notes"])
        with open(os.path.join(ROOT, "src", "Themes", "Utracer Dark.qss"), encoding="utf-8",
                  newline="") as f:
            self.assertEqual(self.report["windowStyleSheet"], f.read())


if __name__ == "__main__":
    unittest.main()
