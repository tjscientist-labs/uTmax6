"""uTmax discharges the reservoirs as soon as a sweep ends or is stopped.

History: uTmax 3.01 ended every sweep (the last point, Stop, a current-limit
abort, the voltage backstop) through End(), which sends one last measurement at
4 V on anode and screen and goes idle. End Measurement (command 30), which
discharges the anode and screen reservoir capacitors and sets the grid to 0 V
(Dekker, uTracer3 log, "Communication protocol"), went out only with the idle
heartbeat, every 20 seconds. So after a sweep the reservoirs could stay charged
for up to 20 s, on a uTracer6 from as much as 1000 V. Dean Jenkins' fork sends
it on Stop; uTmax6 sends it at the end of End(), whatever the reason.

The test reads the simulator's command log: after the last measurement there
must be an End Measurement within a couple of seconds (the heartbeat would take
20 s, and the test hook would already have finished).

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_discharge -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402

SWEEP = {"tube": "12AX7/ECC83", "Vh": 12.6, "VaStart": 30, "VaEnd": 300, "VaSteps": 5,
         "VsStart": 30, "VsEnd": 30, "VsSteps": 0, "VgStart": 0, "VgEnd": -2, "VgSteps": 2,
         "Delay": 0}
POINTS = 18                    # 6 anode steps x 3 grid curves
SOON_S = 2.0                   # the heartbeat would take 20 s


def discharge_after_last_measure(commands):
    """Seconds from the last measure to the first End Measurement after it, or None."""
    measures = [i for i, c in enumerate(commands) if c["cmd"] == "measure"]
    if not measures:
        return None
    last = measures[-1]
    for c in commands[last + 1:]:
        if c["cmd"] == "end_discharge":
            return c["t"] - commands[last]["t"]
    return None


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class DischargeAtEndOfSweep(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(SWEEP, sim_args=["--load", "triode:12AX7"])

    def test_completes(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertEqual(len(self.report["points"]), POINTS)

    def test_discharge_sent_straight_after(self):
        dt = discharge_after_last_measure(self.commands)
        self.assertIsNotNone(dt, "no End Measurement (30) after the last measurement")
        self.assertLess(dt, SOON_S)

    def test_last_measure_is_the_safe_one(self):
        # End()'s own measurement at 4 V comes first, unchanged from 3.01.
        last = [c for c in self.commands if c["cmd"] == "measure"][-1]
        self.assertLess(last["decoded"]["Va_reservoir_V"], 50, last)


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class DischargeOnStop(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(dict(SWEEP, StopAfter=4),
                                             sim_args=["--load", "triode:12AX7"])

    def test_stopped_early(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])
        self.assertIn({"kind": "hook", "text": "stop"}, self.report["notes"])
        self.assertLess(len(self.report["points"]), POINTS)

    def test_discharge_sent_straight_after(self):
        dt = discharge_after_last_measure(self.commands)
        self.assertIsNotNone(dt, "no End Measurement (30) after Stop")
        self.assertLess(dt, SOON_S)


if __name__ == "__main__":
    unittest.main()
