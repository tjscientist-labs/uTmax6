"""A reply uTmax can't read stops the sweep and discharges.

History: uTmax 3.01 read every field of the uTracer's result string with
toInt(), so a character that wasn't hex silently became 0 and the point was
stored and plotted as a real measurement. It also used the two gain codes at
the end of the reply to index an 8-entry table without checking them.

uTmax6 checks the result string before anything reads it (checkReply(),
src/reply_format.h): length, hex, a known status byte with Dekker's firmware,
gain codes 0-7. A reply that fails stops the sweep with End Measurement (30)
and a message, like a wrong echo.

The simulator's --garble-every 10 puts "ZZ" into the anode-current field of
the 10th reply, a measurement part-way through the sweep (21 replies in all).

Needs a built uTmax:  UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_bad_reply -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utmax_run import have_utmax, run_sweep  # noqa: E402
from test_bad_echo import SWEEP  # noqa: E402

POINTS = 18


@unittest.skipUnless(have_utmax(), "set UTMAX_BIN to a built uTmax")
class GarbledReplyDuringASweep(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.report, cls.commands = run_sweep(
            SWEEP, sim_args=["--load", "triode:12AX7", "--garble-every", "10"])
        cls.bad = next(i for i, c in enumerate(cls.commands) if c.get("fault") == "garbled")

    def test_the_fault_hit_a_measurement(self):
        self.assertEqual(self.commands[self.bad]["cmd"], "measure")

    def test_discharge_follows_at_once(self):
        after = self.commands[self.bad + 1]
        self.assertEqual(after["cmd"], "end_discharge")
        self.assertLess(after["t"] - self.commands[self.bad]["t"], 2.0)

    def test_garbled_point_not_stored(self):
        # The points before the garbled one are kept; it and the rest are not.
        good = sum(1 for c in self.commands[:self.bad] if c["cmd"] == "measure")
        self.assertEqual(len(self.report["points"]), good)
        self.assertLess(len(self.report["points"]), POINTS)

    def test_says_so(self):
        texts = [n["text"] for n in self.report["notes"]]
        self.assertTrue(any("not usable" in t and "not hex" in t for t in texts), texts)

    def test_ends_idle(self):
        self.assertEqual(self.report["result"], "done", self.report["notes"])


if __name__ == "__main__":
    unittest.main()
