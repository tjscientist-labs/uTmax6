"""End-to-end: start the real uTmax program against the software uTracer and check,
from the simulator's log, what uTmax sends on its own.

This is the first "characterisation" test: it records what uTmax 3.01 does today,
so that later changes cannot alter it unnoticed.

Needs a built uTmax. Run from the repository root with:

    UTMAX_BIN=build/release/uTmax python3 -m unittest tests.test_utmax_smoke -v

uTmax runs with Qt's "offscreen" platform (no window) and a temporary home folder,
so your own ~/uTmax_files is never touched. The test takes about 30 seconds,
because uTmax's idle heartbeat comes every 20 seconds.
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UTMAX = os.environ.get("UTMAX_BIN")


@unittest.skipUnless(UTMAX and os.path.exists(UTMAX), "set UTMAX_BIN to a built uTmax")
class UTmaxTalksToSimulator(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.mkdtemp(prefix="utmax-smoke-")
        home = os.path.join(cls.tmp, "home")
        files = os.path.join(home, "uTmax_files")
        os.makedirs(files)
        shutil.copy(os.path.join(ROOT, "src", "data.csv"), files)
        shutil.copy(os.path.join(ROOT, "src", "Style.qss"), files)
        cls.port = os.path.join(cls.tmp, "utracer")
        cls.log = os.path.join(cls.tmp, "sim.jsonl")
        # A cal.txt for a uTracer6 on the simulator's port. uTmax reads only the
        # keys it knows; the rest take its built-in defaults.
        with open(os.path.join(files, "cal.txt"), "w") as f:
            f.write(f"COM={cls.port}\nuTracer6=1\nFirmware=1\nVaMax=300\nRaVal=4700\n"
                    "IaRsense=4.7\nIsRsense=4.7\nVgMax=100\n")
        ready = os.path.join(cls.tmp, "ready")
        cls.sim = subprocess.Popen(
            [sys.executable, os.path.join(ROOT, "tools", "utracer_sim", "utracer_sim.py"),
             "--link", cls.port, "--log", cls.log, "--ready-file", ready, "--load", "r:10k"],
            stdout=subprocess.DEVNULL)
        for _ in range(100):
            if os.path.exists(ready):
                break
            time.sleep(0.05)
        env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen")
        cls.utmax = subprocess.Popen([os.path.abspath(UTMAX)], cwd=files, env=env,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(26)          # past one 20-second heartbeat
        cls.utmax.terminate()
        cls.utmax.wait(timeout=10)
        cls.sim.terminate()
        cls.sim.wait(timeout=10)
        with open(cls.log) as f:
            cls.commands = [json.loads(line) for line in f]

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def test_reads_the_adc_when_the_port_opens(self):
        # OpenComPort() sets sendADC, so the first command is a 50 (read all ADCs).
        self.assertGreater(len(self.commands), 0, "uTmax sent nothing")
        self.assertEqual(self.commands[0]["cmd"], "ping_read_adc")

    def test_idle_heartbeat_is_end_measurement(self):
        # uTmax 3.01 sends 30 (end measurement / discharge) every 20 s while idle,
        # and labels it "Send Ping". Recorded here as current behaviour.
        self.assertIn("end_discharge", [c["cmd"] for c in self.commands])

    def test_sends_no_unknown_commands(self):
        self.assertEqual([c for c in self.commands if c["cmd"] == "unknown"], [])


if __name__ == "__main__":
    unittest.main()
