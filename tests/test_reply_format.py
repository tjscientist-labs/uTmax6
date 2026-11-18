"""The uTracer result string with and without Dekker's positive-grid board.

With the extension board fitted to a uTracer6, the status byte has bit 7 set
(90H / 91H) and four hex characters of grid current are appended, so replies
are 42 characters instead of 38 (Dekker, uTracer6 log, section 14).

History: uTmax 3.01 waited for exactly 38 characters and compared the status
with "11". With the board it never accepted a reply (it timed out and
reconnected), and would not have recognised 91H as a current limit. The
end-to-end check is in test_utmax_smoke.py (UTmaxWithPositiveGridBoard).

Compiles tests/cpp/test_reply_format.cpp against src/reply_format.h, runs it,
and checks that the simulator's replies have the lengths the header expects.
Needs g++ for the C++ part (skipped without it).
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
from loads import Resistor  # noqa: E402
from utracer_sim import UTracer  # noqa: E402


@unittest.skipUnless(shutil.which("g++"), "needs g++")
class ReplyFormatHeader(unittest.TestCase):

    def test_cpp_checks_pass(self):
        tmp = tempfile.mkdtemp(prefix="replyformat-")
        try:
            exe = os.path.join(tmp, "test_reply_format")
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Werror", "-o", exe,
                            os.path.join(ROOT, "tests", "cpp", "test_reply_format.cpp")],
                           check=True)
            r = subprocess.run([exe], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


class SimulatorAgreesWithHeader(unittest.TestCase):
    """The simulator's replies are what reply_format.h says they are."""

    def reply(self, grid_board):
        sim = UTracer(Profile.find("utracer6"), Resistor(10000), {}, None,
                      grid_board=grid_board)
        return sim.format_reply("90" if grid_board else "10", 0, 0, 0, 0, 0, 0)

    def test_without_board_38_characters_status_10(self):
        r = self.reply(False)
        self.assertEqual(len(r), 38)
        self.assertEqual(int(r[:2], 16) & 0x80, 0)

    def test_with_board_42_characters_status_bit7(self):
        r = self.reply(True)
        self.assertEqual(len(r), 42)
        self.assertEqual(int(r[:2], 16) & 0x80, 0x80)


if __name__ == "__main__":
    unittest.main()
