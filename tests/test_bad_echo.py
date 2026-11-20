"""A wrong echo stops the sweep and discharges the reservoirs.

History: the uTracer echoes every character it receives, and that echo is the
protocol's only check. uTmax compares it, but only after the uTracer has already
acted on the command. When the echo didn't match, uTmax 3.01 showed
"Unexpected response from Tracer; power cycle and restart" and went idle, with
the reservoirs left as they were until the 20-second heartbeat.

uTmax6 sends End Measurement (30) straight away, which discharges the
reservoirs and sets the grid to 0 V, and says so. If the 30 itself comes back
wrong it goes idle as before, so it can't loop.

The simulator's --bad-echo-on N echoes the last character of command N wrongly
while still acting on it, as a uTracer would if the line corrupted the echo.

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_bad_echo -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 5,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": 0, "VgEnd": -2, "VgSteps": 2,
         "Delay": 0}
BAD = 50             # a measurement part-way through: after start-up 50, then 00, 50 and
                     # the 41-step heater ramp, the 18 points are commands 45-62


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class BadEchoDuringASweep(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(
            SWEEP, sim_args=["--load", "triode:12AX7", "--bad-echo-on", str(BAD)])
        cls.bad = next(i for i, c in enumerate(cls.commands) if c.get("fault") == "bad echo")

    def test_the_fault_hit_a_measurement(self):
        self.assertEqual(self.commands[self.bad]["cmd"], "measure")

    def test_discharge_follows_at_once(self):
        after = self.commands[self.bad + 1]
        self.assertEqual(after["cmd"], "end_discharge")
        self.assertLess(after["t"] - self.commands[self.bad]["t"], 2.0)

    def test_sweep_stopped(self):
        later = [c["cmd"] for c in self.commands[self.bad + 1:]]
        self.assertNotIn("measure", later)

    def test_says_so(self):
        texts = [n["text"] for n in self.report["notes"]]
        self.assertTrue(any("echo did not match" in t and "discharged" in t for t in texts), texts)

    def test_ends_idle(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])


if __name__ == "__main__":
    unittest.main()
