import contextlib
import io
import unittest
from unittest.mock import patch

import decomp_status as status


class StatusTests(unittest.TestCase):
    def invoke(self, targets, blocked, parity=True, arguments=("--complete",)):
        output = io.StringIO()
        with patch.object(status, "targets", return_value=targets), \
             patch.object(status, "blocked_entries", return_value=blocked), \
             patch.object(status, "run_parity", return_value=parity) as oracle, \
             patch("sys.argv", ["decomp_status.py", *arguments]), \
             contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            result = status.main()
        return result, output.getvalue(), oracle.called

    def test_shifted_lines_remain_blocked(self):
        result, output, _ = self.invoke(
            [{"id": "code/game/test.cpp:100:foo"}],
            {"code/game/test.cpp:2:foo": {"note": "register mismatch"}},
            arguments=())
        self.assertEqual(result, 0)
        self.assertIn("active=0 blocked=1", output)

    def test_blocked_is_not_complete(self):
        result, _, called = self.invoke(
            [{"id": "test.cpp:10:foo"}],
            {"test.cpp:10:foo": {"reason": "dead tail"}})
        self.assertEqual(result, 1)
        self.assertFalse(called)

    def test_active_is_not_complete(self):
        self.assertEqual(self.invoke([{"id": "test.cpp:1:foo"}], {})[0], 1)

    def test_stale_blocker_must_have_note(self):
        self.assertEqual(self.invoke([], {"test.cpp:1:foo": {"note": "  "}})[0], 1)

    def test_empty_requires_parity(self):
        self.assertEqual(self.invoke([], {}, parity=False)[0], 1)
        self.assertEqual(self.invoke([], {}, parity=True)[0], 0)


if __name__ == "__main__":
    unittest.main()
