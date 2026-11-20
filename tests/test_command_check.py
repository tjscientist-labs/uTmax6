"""Every command is checked before it goes to the uTracer.

History: Ronald Dekker's protocol has no checksum, and uTmax 3.01 sent whatever
it had built. That is how a positive grid on a uTracer6 went out as +100 V
(src/grid_word.h). It also lets a heater setting above the supply voltage out
as a heater word beyond the 10-bit range: uTmax allows up to 18 V, and on a weak
15 V supply that is 1023 * (18/15)^2 = 1473 (5C1H).

uTmax6 checks every Dekker command just before it is sent (src/command_check.h):
form, code, settings bytes, anode and screen against the voltage limit, grid
and heater ranges, unused words. A command that fails is not sent; End
Measurement (30) is sent instead and the reason is shown.

Two parts: the rules on their own (tests/cpp/test_command_check.cpp, needs g++),
and the real program asked for an 18 V heater with the simulator's supply at
15 V (--supply-volts).

Needs a built uTmax for the second part:
    UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_command_check -v
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep, ROOT  # noqa: E402


@unittest.skipUnless(shutil.which("g++"), "needs g++")
class Rules(unittest.TestCase):

    def test_cpp_checks_pass(self):
        tmp = tempfile.mkdtemp(prefix="cmdcheck-")
        try:
            exe = os.path.join(tmp, "test_command_check")
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Werror", "-o", exe,
                            os.path.join(ROOT, "tests", "cpp", "test_command_check.cpp")],
                           check=True)
            r = subprocess.run([exe], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


def heater_word(c):
    return int(c["rx"][14:18], 16)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class HeaterAboveAWeakSupply(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        sweep = {"tube": "12AX7/ECC83", "Vh": 18, "VaStart": 30, "VaEnd": 300, "VaSteps": 2,
                 "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": -1, "VgEnd": -1,
                 "VgSteps": 0, "Delay": 0}
        cls.report, cls.commands = run_sweep(sweep, sim_args=["--load", "triode:12AX7",
                                                              "--supply-volts", "15"])

    def test_no_heater_word_out_of_range_sent(self):
        heater = [c for c in self.commands if c["cmd"] in ("heater", "measure", "apply_hold")]
        bad = [c["rx"] for c in heater if heater_word(c) > 0x3FF]
        self.assertEqual(bad, [], "heater words beyond 3FF reached the uTracer")

    def test_refused_and_said_why(self):
        texts = [n["text"] for n in self.report["notes"]]
        self.assertTrue(any("Command refused, heater value out of range" in t for t in texts), texts)

    def test_discharged_and_no_measurement(self):
        names = [c["cmd"] for c in self.commands]
        self.assertNotIn("measure", names)
        self.assertEqual(names[-1], "end_discharge")

    def test_uTmax_ends_idle(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(self.report["exit_status"], 0)


if __name__ == "__main__":
    unittest.main()
