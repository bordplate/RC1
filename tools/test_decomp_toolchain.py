"""Integration controls using the installed EE compiler and original boot bytes.

Run after make split, using the project venv. Compiler failures are NOT accepted
as negative controls: each expected mismatch must have a completed byte diff.
"""
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


class ToolchainTests(unittest.TestCase):
    def probe(self, source, reference, symbol, flags="", differences=0):
        with tempfile.TemporaryDirectory(prefix="rc1-probe-") as directory:
            result = subprocess.run(
                [sys.executable, "tools/decomp_probe.py",
                 f"decomp_state/probes/{source}",
                 f"decomp_state/probes/reference/{reference}", symbol,
                 f"--flags={flags}", "--out", directory],
                cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(result.returncode, int(differences != 0),
                             result.stdout + result.stderr)
            data = json.loads((Path(directory) / "candidate.json").read_text())
            self.assertEqual(len(data["differences"]), differences)
            self.assertEqual(data["match"], differences == 0)

    def test_gp_relocation(self):
        self.probe("gp_control.cpp", "gp_control.s", "gp_control")

    def test_menu_scheduling(self):
        self.probe("menu_edge.cpp", "menu_edge.s", "menu_pointIsClockwise")
        self.probe("menu_callback.cpp", "menu_callback.s", "menu_restoreSelection",
                   "-fno-schedule-insns2")
        self.probe("menu_callback.cpp", "menu_callback.s", "menu_restoreSelection",
                   "-fno-schedule-insns", differences=7)

    def test_tag(self):
        self.probe("vibuf_tag.cpp", "vibuf_tag.s", "scTag2")

    def test_menu_threshold(self):
        self.probe("menu_threshold.cpp", "menu_threshold.s", "menu_threshold")

    def test_rpc_return_type(self):
        self.probe("snd_batch.c", "snd_batch.s", "snd_SendCurrentBatch")
        self.probe("snd_batch.c", "snd_batch.s", "snd_SendCurrentBatch",
                   "-DRPC_RETURN=void", differences=10)


if __name__ == "__main__":
    unittest.main()
