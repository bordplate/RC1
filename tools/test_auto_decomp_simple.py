import tempfile
import unittest
from pathlib import Path

import auto_decomp_simple as auto


class SimpleAutoDecompTests(unittest.TestCase):
    def target(self, name, rows, suffix=".cpp"):
        instructions = [
            auto.Instruction(index * 4, 0x200000 + index * 4, mnemonic, operands)
            for index, (mnemonic, operands) in enumerate(rows)
        ]
        return auto.Target(
            Path("/tmp/source" + suffix), 1, "game/test", name,
            Path("/tmp/reference.s"), instructions, len(instructions) * 4)

    def test_decodes_old_cpp_pointer_signature(self):
        signature = auto.decode_signature("setValue__FP8TestTypei", ".cpp", [])
        self.assertTrue(signature.known)
        self.assertEqual(signature.source_name, "setValue")
        self.assertEqual([value.spelling for value in signature.params], ["TestType*", "int"])

    def test_repeated_parameter_index_is_zero_based(self):
        signature = auto.decode_signature("copy__FPUciT0", ".cpp", [])
        self.assertTrue(signature.known)
        self.assertEqual([value.spelling for value in signature.params],
                         ["unsigned char*", "int", "unsigned char*"])

    def test_constant_return_recipe(self):
        target = self.target("enabled__Fv", [
            ("jr", ("$31",)),
            ("addiu", ("$2", "$0", "0x1")),
        ])
        candidates = auto.load_recipe(target)
        self.assertEqual(len(candidates), 1)
        self.assertEqual(candidates[0].kind, "constant-return")
        self.assertIn("int enabled(void)", candidates[0].replacement)
        self.assertIn("return 1", candidates[0].replacement)

    def test_field_getter_recipe(self):
        target = self.target("getCount__FP8TestType", [
            ("lw", ("$2", "0xC($4)")),
            ("jr", ("$31",)),
            ("nop", ()),
        ])
        candidate = auto.load_recipe(target)[0]
        self.assertEqual(candidate.kind, "field-getter")
        self.assertTrue(candidate.known_signature)
        self.assertIn("struct TestType;", candidate.replacement)
        self.assertIn("(char*)arg0 + 12", candidate.replacement)

    def test_field_setter_recipe(self):
        target = self.target("setCount__FP8TestTypei", [
            ("sw", ("$5", "0xC($4)")),
            ("jr", ("$31",)),
            ("nop", ()),
        ])
        candidate = auto.load_recipe(target)[0]
        self.assertEqual(candidate.kind, "field-setter")
        self.assertIn("void setCount(TestType* arg0, int arg1)", candidate.replacement)

    def test_wrapper_requires_return_choice(self):
        target = self.target("outer__FP5Outer", [
            ("addiu", ("$29", "$29", "-0x10")),
            ("sq", ("$31", "0x0($29)")),
            ("jal", ("inner__FP5Inner",)),
            ("addiu", ("$4", "$4", "0x20")),
            ("lq", ("$31", "0x0($29)")),
            ("jr", ("$31",)),
            ("addiu", ("$29", "$29", "0x10")),
        ])
        candidates = auto.load_recipe(target)
        self.assertEqual([candidate.return_choice for candidate in candidates], ["int", "void"])
        self.assertTrue(all(candidate.semantic_ambiguity for candidate in candidates))

    def test_global_object_wrapper_recipe(self):
        target = self.target("proceed__Fv", [
            ("lui", ("$2", "%hi(D_0016120C)")),
            ("lw", ("$2", "%lo(D_0016120C)($2)")),
            ("lui", ("$4", "(0xD9100 >> 16)")),
            ("addiu", ("$29", "$29", "-0x10")),
            ("ori", ("$4", "$4", "(0xD9100 & 0xFFFF)")),
            ("sq", ("$31", "0x0($29)")),
            ("jal", ("audioSend",)),
            ("addu", ("$4", "$2", "$4")),
            ("lq", ("$31", "0x0($29)")),
            ("jr", ("$31",)),
            ("addiu", ("$29", "$29", "0x10")),
        ])
        candidates = auto.load_recipe(target)
        self.assertEqual([candidate.return_choice for candidate in candidates], ["int", "void"])
        self.assertIn("D_0016120C", candidates[0].replacement)
        self.assertIn("0xd9100", candidates[0].replacement)
        self.assertEqual(candidates[0].linker_definitions, {"D_0016120C": 0x16120C})

    def test_integer_scale_recipe(self):
        target = self.target("scale", [
            ("addiu", ("$5", "$0", "0x5F4")),
            ("addiu", ("$3", "$0", "0x2E5")),
            ("mult", ("$2", "$4", "$5")),
            ("beql", ("$3", "$0", ".Lx")),
            ("break", ("0", "7")),
            ("div", ("$0", "$2", "$3")),
            ("mflo", ("$2",)),
            ("jr", ("$31",)),
            ("nop", ()),
        ], suffix=".c")
        candidate = auto.load_recipe(target)[0]
        self.assertEqual(candidate.kind, "integer-scale-wrapper")
        self.assertIn("arg0 * 1524 / 741", candidate.replacement)

    def test_exact_rollback_does_not_replace_other_text(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.cpp"
            replacement = "int value(void) {\n    return 1;\n}"
            placeholder = 'INCLUDE_ASM("code/_generated/nonmatchings/game/test", value);'
            path.write_text(replacement + "\nunchanged\n", encoding="utf-8")
            target = auto.Target(path, 1, "game/test", "value", Path("x"), [], 0)
            auto.rollback_replacement(target, placeholder, replacement)
            self.assertEqual(path.read_text(encoding="utf-8"), placeholder + "\nunchanged\n")


if __name__ == "__main__":
    unittest.main()
