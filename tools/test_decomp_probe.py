import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import decomp_probe as probe


class ProbeTests(unittest.TestCase):
    def test_invalid_reference_invalidates_old_success_without_compiling(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "input.cpp"
            reference = root / "input.s"
            source.write_text("void test() {}\n")
            reference.write_text("/* 000000 00000000 01020304 */ nop\n")
            result = root / "candidate.json"
            result.write_text('{"match": true}\n')
            with patch("sys.argv", ["decomp_probe.py", str(source), str(reference),
                                    "test", "--out", str(root)]), \
                 patch.object(probe, "run") as compiler, \
                 contextlib.redirect_stderr(io.StringIO()), \
                 self.assertRaises(SystemExit) as error:
                probe.main()
            self.assertEqual(error.exception.code, 2)
            compiler.assert_not_called()
            self.assertFalse(json.loads(result.read_text())["match"])


if __name__ == "__main__":
    unittest.main()
