import unittest

from tu_assembler_diff import function_extents


class FunctionExtentTests(unittest.TestCase):
    def test_interior_and_entry_aliases_are_covered(self):
        self.assertEqual(function_extents([
            ("entry_alias", 0, 0),
            ("parent", 0, 60),
            ("interior", 8, 0),
            ("parent.NON_MATCHING", 0, 60),
            ("next", 64, 8),
        ]), [("parent", 0, 60), ("next", 64, 8)])

    def test_unsized_entries_outside_extents_are_not_hidden(self):
        self.assertEqual(function_extents([
            ("parent", 0, 60),
            ("at_end", 60, 0),
            ("orphan", 80, 0),
        ]), [("parent", 0, 60), ("at_end", 60, 0), ("orphan", 80, 0)])


if __name__ == "__main__":
    unittest.main()
