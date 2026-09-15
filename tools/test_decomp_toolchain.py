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
    def probe(self, source, reference, symbol, flags="", differences=0, assembler="snas"):
        with tempfile.TemporaryDirectory(prefix="rc1-probe-") as directory:
            result = subprocess.run(
                [sys.executable, "tools/decomp_probe.py",
                 f"decomp_state/probes/{source}",
                 f"decomp_state/probes/reference/{reference}", symbol,
                 f"--flags={flags}", "--assembler", assembler, "--out", directory],
                cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(result.returncode, int(differences != 0),
                             result.stdout + result.stderr)
            data = json.loads((Path(directory) / "candidate.json").read_text())
            self.assertEqual(len(data["differences"]), differences)
            self.assertEqual(data["match"], differences == 0)

    def test_gp_relocation(self):
        # This historical extern-only control relies on GNU GP relaxation.
        self.probe("gp_control.cpp", "gp_control.s", "gp_control", assembler="gnu")

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
        # The production sound TU retains GNU assembly. This historical
        # control isolates the compiler's RPC return-type effect.
        self.probe("snd_batch.c", "snd_batch.s", "snd_SendCurrentBatch", assembler="gnu")
        self.probe("snd_batch.c", "snd_batch.s", "snd_SendCurrentBatch",
                   "-DRPC_RETURN=void", differences=10, assembler="gnu")

    def test_sn_symbolic_addresses(self):
        from research_symbolic_addresses import CASES
        for name, reference, symbol, definitions in CASES[:3]:
            with self.subTest(name=name), tempfile.TemporaryDirectory(prefix="rc1-snas-") as directory:
                command = [sys.executable, "tools/decomp_probe.py",
                           f"decomp_state/probes/symbolic_addresses/{name}.cpp",
                           f"code/_generated/matchings/game/{reference}", symbol,
                           "--assembler", "snas", "--out", directory]
                for definition in definitions:
                    command.extend(["--define", definition])
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                data = json.loads((Path(directory) / "candidate.json").read_text())
                self.assertTrue(data["match"])
                self.assertEqual(data["differences"], [])


if __name__ == "__main__":
    unittest.main()
