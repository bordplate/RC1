#!/usr/bin/env python3
"""Generate, prove, and optionally apply trivial matching decompilations.

Scanning is read-only and is the default.  ``--verify`` compiles standalone
candidates through decomp_probe.py.  ``--apply`` additionally replaces the
INCLUDE_ASM, builds the game, and keeps the edit only when the boot image is
byte-identical to the original.

This intentionally recognizes a small instruction grammar.  Unsupported or
semantically ambiguous functions are left alone rather than guessed.
"""

import argparse
import dataclasses
import datetime
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
INCLUDE_RE = re.compile(
    r'(?P<indent>^[ \t]*)INCLUDE_ASM\("code/_generated/nonmatchings/'
    r'(?P<folder>[^"\n]+)",\s*(?P<name>[^\)\s]+)\);[ \t]*$', re.M
)
ROW_RE = re.compile(
    r'/\*\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]{8})\s*\*/'
    r'\s*([.A-Za-z0-9]+)\s*(.*?)\s*(?:#.*)?$'
)
SIZE_RE = re.compile(r'^nonmatching\s+\S+,\s*(0x[0-9A-Fa-f]+)', re.M)
MANGLED_RE = re.compile(r'^(?P<base>[A-Za-z_]\w*)__F(?P<params>.+)$')


@dataclasses.dataclass(frozen=True)
class Instruction:
    offset: int
    address: int
    mnemonic: str
    operands: tuple[str, ...]


@dataclasses.dataclass(frozen=True)
class CType:
    spelling: str
    named_structs: tuple[str, ...] = ()

    def pointer(self):
        return CType(self.spelling + "*", self.named_structs)


@dataclasses.dataclass
class Signature:
    source_name: str
    params: list[CType]
    known: bool
    cpp_linkage: bool
    c_source: bool = False


@dataclasses.dataclass
class Target:
    source: Path
    line: int
    folder: str
    name: str
    asm: Path
    instructions: list[Instruction]
    size: int

    @property
    def id(self):
        return f"{self.source.relative_to(ROOT)}:{self.line}:{self.name}"


@dataclasses.dataclass
class Candidate:
    target: Target
    kind: str
    return_choice: str
    replacement: str
    probe_source: str
    known_signature: bool
    semantic_ambiguity: str | None = None
    linker_definitions: dict[str, int] = dataclasses.field(default_factory=dict)


PRIMITIVES = {
    "b": "bool",
    "c": "char",
    "d": "double",
    "f": "float",
    "i": "int",
    "l": "long",
    "s": "short",
    "v": "void",
    "x": "long long",
}


def split_operands(text):
    return tuple(part.strip() for part in text.split(",") if part.strip())


def parse_reference(path):
    text = path.read_text(encoding="utf-8")
    body = text.split("endlabel", 1)[0]
    instructions = []
    for line in body.splitlines():
        match = ROW_RE.search(line)
        if match:
            instructions.append(Instruction(
                int(match.group(1), 16), int(match.group(2), 16),
                match.group(4), split_operands(match.group(5))))
    size_match = SIZE_RE.search(text)
    if not instructions or not size_match:
        raise ValueError(f"cannot parse generated assembly: {path}")
    size = int(size_match.group(1), 16)
    if len(instructions) * 4 != size:
        raise ValueError(f"instruction count disagrees with size in {path}")
    return instructions, size


def discover_targets():
    result = []
    for source in sorted((ROOT / "code").rglob("*")):
        if source.suffix not in {".c", ".cpp"}:
            continue
        text = source.read_text(encoding="utf-8")
        for match in INCLUDE_RE.finditer(text):
            asm = ROOT / "code/_generated/nonmatchings" / match.group("folder") / (match.group("name") + ".s")
            if not asm.exists():
                continue
            instructions, size = parse_reference(asm)
            result.append(Target(
                source, text.count("\n", 0, match.start()) + 1,
                match.group("folder"), match.group("name"), asm,
                instructions, size))
    return result


def decode_type(encoded, position, previous):
    if position >= len(encoded):
        raise ValueError("missing type")
    token = encoded[position]
    if token == "P":
        inner, position = decode_type(encoded, position + 1, previous)
        return inner.pointer(), position
    if token == "R":
        inner, position = decode_type(encoded, position + 1, previous)
        return CType(inner.spelling + "&", inner.named_structs), position
    if token == "U":
        inner, position = decode_type(encoded, position + 1, previous)
        if inner.named_structs or inner.spelling.endswith(("*", "&")):
            raise ValueError("unsupported unsigned type")
        return CType("unsigned " + inner.spelling), position
    if token == "T":
        end = position + 1
        while end < len(encoded) and encoded[end].isdigit():
            end += 1
        if end == position + 1:
            raise ValueError("missing repeated-type index")
        index = int(encoded[position + 1:end])
        if index >= len(previous):
            raise ValueError("invalid repeated-type index")
        return previous[index], end
    if token.isdigit():
        end = position
        while end < len(encoded) and encoded[end].isdigit():
            end += 1
        length = int(encoded[position:end])
        name = encoded[end:end + length]
        if len(name) != length or not re.match(r'^[A-Za-z_]\w*$', name):
            raise ValueError("invalid named type")
        return CType(name, (name,)), end + length
    if token in PRIMITIVES:
        return CType(PRIMITIVES[token]), position + 1
    raise ValueError(f"unsupported mangled type {token!r}")


def decode_signature(name, source_suffix, inferred_params):
    if source_suffix == ".c":
        return Signature(name, inferred_params, False, False, True)
    match = MANGLED_RE.match(name)
    if not match:
        return Signature(name, inferred_params, False, False)
    encoded = match.group("params")
    if encoded == "v":
        return Signature(match.group("base"), [], True, True)
    params = []
    position = 0
    try:
        while position < len(encoded):
            value, position = decode_type(encoded, position, params)
            params.append(value)
    except ValueError:
        return Signature(match.group("base"), inferred_params, False, True)
    return Signature(match.group("base"), params, True, True)


def parse_integer(text):
    value = text.strip()
    masked = re.fullmatch(r'\(([-+]?0x[0-9A-Fa-f]+|[-+]?\d+)\s*&\s*0xFFFF\)', value)
    if masked:
        number = int(masked.group(1), 0) & 0xFFFF
        return number - 0x10000 if number & 0x8000 else number
    if re.fullmatch(r'[-+]?(?:0x[0-9A-Fa-f]+|\d+)', value):
        return int(value, 0)
    raise ValueError("not an integer")


def parse_full_constant(high, low):
    high_match = re.fullmatch(r'\((0x[0-9A-Fa-f]+)\s*>>\s*16\)', high.strip())
    low_match = re.fullmatch(r'\((0x[0-9A-Fa-f]+)\s*&\s*0xFFFF\)', low.strip())
    if not high_match or not low_match or int(high_match.group(1), 16) != int(low_match.group(1), 16):
        raise ValueError("not two halves of one constant")
    return int(high_match.group(1), 16)


def relocation_symbol(operand, operator):
    match = re.fullmatch(rf'%{operator}\(([A-Za-z_]\w*)\)', operand.strip())
    if not match:
        raise ValueError("not a symbolic relocation")
    return match.group(1)


def memory_relocation_symbol(operand, operator, base_register):
    match = re.fullmatch(
        rf'%{operator}\(([A-Za-z_]\w*)\)\(\${base_register}\)', operand.strip())
    if not match:
        raise ValueError("not a symbolic memory relocation")
    return match.group(1)


def data_label_address(name):
    match = re.fullmatch(r'D_([0-9A-Fa-f]{8})', name)
    if not match:
        raise ValueError("symbol does not encode an absolute data address")
    return int(match.group(1), 16)


def parse_memory(operand):
    match = re.fullmatch(r'(.+)\(\$(\d+)\)', operand)
    if not match:
        raise ValueError("not a memory operand")
    return parse_integer(match.group(1)), int(match.group(2))


def is_insn(insn, mnemonic, *operands):
    return insn.mnemonic == mnemonic and insn.operands == operands


def is_nop(insn):
    return is_insn(insn, "nop")


def function_text(signature, return_type, body):
    declarations = []
    for param in signature.params:
        for name in param.named_structs:
            declaration = f"struct {name};"
            if declaration not in declarations:
                declarations.append(declaration)
    linkage = "" if signature.cpp_linkage or signature.c_source else 'extern "C" '
    params = ", ".join(f"{value.spelling} arg{index}" for index, value in enumerate(signature.params))
    if not params:
        params = "void"
    function = f"{linkage}{return_type} {signature.source_name}({params}) {{\n{body}\n}}"
    return "\n".join(declarations + ([""] if declarations else []) + [function])


def candidate_for_body(target, kind, return_type, inferred_params, body,
                       return_choice="inferred", ambiguity=None, prelude="",
                       linker_definitions=None):
    signature = decode_signature(target.name, target.source.suffix, inferred_params)
    if signature.known and len(signature.params) != len(inferred_params):
        return None
    replacement = function_text(signature, return_type, body)
    probe = '#include "common.h"\n\n'
    if prelude:
        probe += prelude.rstrip() + "\n\n"
    probe += replacement + "\n"
    return Candidate(target, kind, return_choice, replacement, probe,
                     signature.known, ambiguity, linker_definitions or {})


def load_recipe(target):
    insns = target.instructions
    load_types = {
        "lb": "signed char", "lbu": "unsigned char",
        "lh": "short", "lhu": "unsigned short", "lw": "int",
        "ld": "long",
    }
    store_types = {"sb": "unsigned char", "sh": "unsigned short", "sw": "int", "sd": "long"}

    # Empty leaf.
    if len(insns) == 2 and is_insn(insns[0], "jr", "$31") and is_nop(insns[1]):
        return [candidate_for_body(target, "empty", "void", [], "")]

    # Constant-return leaves, including a scheduled return in the delay slot.
    constant = None
    if len(insns) == 2 and is_insn(insns[0], "jr", "$31"):
        value = insns[1]
        if value.mnemonic in {"addiu", "ori"} and value.operands[:2] == ("$2", "$0"):
            constant = parse_integer(value.operands[2])
        elif is_insn(value, "daddu", "$2", "$0", "$0"):
            constant = 0
    if (len(insns) == 3 and is_insn(insns[1], "jr", "$31") and is_nop(insns[2])
            and insns[0].mnemonic in {"addiu", "ori"}
            and insns[0].operands[:2] == ("$2", "$0")):
        constant = parse_integer(insns[0].operands[2])
    if constant is not None:
        return [candidate_for_body(target, "constant-return", "int", [], f"    return {constant};")]

    # Return the first argument unchanged.
    if (len(insns) == 2 and is_insn(insns[0], "jr", "$31")
            and is_insn(insns[1], "daddu", "$2", "$4", "$0")):
        return [candidate_for_body(target, "identity", "int", [CType("int")], "    return arg0;")]

    # One direct field load, optionally converted to a zero/nonzero predicate.
    if len(insns) in {3, 4} and insns[0].mnemonic in load_types:
        load = insns[0]
        try:
            offset, base = parse_memory(load.operands[1])
        except (IndexError, ValueError):
            offset, base = 0, -1
        if load.operands[0] == "$2" and base == 4:
            field_type = load_types[load.mnemonic]
            expression = f"*({field_type}*)((char*)arg0 + {offset})"
            if len(insns) == 3 and is_insn(insns[1], "jr", "$31") and is_nop(insns[2]):
                body = f"    return {expression};"
                return [candidate_for_body(target, "field-getter", "int", [CType("void").pointer()], body)]
            if len(insns) == 3 and is_insn(insns[1], "jr", "$31"):
                if is_insn(insns[2], "sltiu", "$2", "$2", "0x1"):
                    body = f"    return {expression} == 0;"
                    return [candidate_for_body(target, "field-zero-predicate", "int", [CType("void").pointer()], body)]
                if is_insn(insns[2], "sltu", "$2", "$0", "$2"):
                    body = f"    return {expression} != 0;"
                    return [candidate_for_body(target, "field-nonzero-predicate", "int", [CType("void").pointer()], body)]

    # One direct field store.
    if len(insns) == 3 and insns[0].mnemonic in store_types:
        store = insns[0]
        try:
            offset, base = parse_memory(store.operands[1])
        except (IndexError, ValueError):
            offset, base = 0, -1
        if store.operands[0] == "$5" and base == 4 and is_insn(insns[1], "jr", "$31") and is_nop(insns[2]):
            field_type = store_types[store.mnemonic]
            body = f"    *({field_type}*)((char*)arg0 + {offset}) = arg1;"
            return [candidate_for_body(target, "field-setter", "void",
                                       [CType("void").pointer(), CType("int")], body)]

    # Return the old word while replacing it with the second argument.
    if len(insns) == 4 and insns[0].mnemonic in load_types and insns[1].mnemonic in store_types:
        load, store = insns[:2]
        try:
            load_offset, load_base = parse_memory(load.operands[1])
            store_offset, store_base = parse_memory(store.operands[1])
        except (IndexError, ValueError):
            load_offset = store_offset = 0
            load_base = store_base = -1
        if (load.operands[0] == "$2" and store.operands[0] == "$5"
                and load_base == store_base == 4 and load_offset == store_offset
                and is_insn(insns[2], "jr", "$31") and is_nop(insns[3])):
            field_type = load_types[load.mnemonic]
            body = (f"    {field_type} old = *({field_type}*)((char*)arg0 + {load_offset});\n"
                    f"    *({store_types[store.mnemonic]}*)((char*)arg0 + {store_offset}) = arg1;\n"
                    "    return old;")
            return [candidate_for_body(target, "field-exchange", "int",
                                       [CType("void").pointer(), CType("int")], body)]

    recipes = arithmetic_ratio_recipe(target)
    if recipes:
        return recipes
    recipes = global_object_wrapper_recipe(target)
    if recipes:
        return recipes
    recipes = printf_callback_recipe(target)
    if recipes:
        return recipes
    return wrapper_recipe(target)


def arithmetic_ratio_recipe(target):
    """Recognize return arg0 * numerator / denominator."""
    insns = target.instructions
    if len(insns) != 9:
        return []
    try:
        numerator = parse_integer(insns[0].operands[2])
        denominator = parse_integer(insns[1].operands[2])
    except (IndexError, ValueError):
        return []
    if not (is_insn(insns[0], "addiu", "$5", "$0", insns[0].operands[2])
            and is_insn(insns[1], "addiu", "$3", "$0", insns[1].operands[2])
            and is_insn(insns[2], "mult", "$2", "$4", "$5")
            and insns[3].mnemonic == "beql" and insns[3].operands[0:2] == ("$3", "$0")
            and insns[4].mnemonic == "break"
            and is_insn(insns[5], "div", "$0", "$2", "$3")
            and is_insn(insns[6], "mflo", "$2")
            and is_insn(insns[7], "jr", "$31") and is_nop(insns[8])):
        return []
    body = f"    return arg0 * {numerator} / {denominator};"
    candidate = candidate_for_body(
        target, "integer-scale-wrapper", "int", [CType("int")], body)
    return [candidate] if candidate else []


def global_object_wrapper_recipe(target):
    """Recognize a call on *(global base) + constant offset."""
    insns = target.instructions
    if len(insns) not in {11, 12}:
        return []
    tail_index = 8
    try:
        global_name = relocation_symbol(insns[0].operands[1], "hi")
        low_name = memory_relocation_symbol(insns[1].operands[1], "lo", 2)
        global_address = data_label_address(global_name)
        object_offset = parse_full_constant(insns[2].operands[1], insns[4].operands[2])
        frame_down = parse_integer(insns[3].operands[2])
        frame_up = parse_integer(insns[-1].operands[2])
    except (IndexError, ValueError):
        return []
    if global_name != low_name:
        return []
    if not (is_insn(insns[0], "lui", "$2", insns[0].operands[1])
            and is_insn(insns[1], "lw", "$2", insns[1].operands[1])
            and insns[1].operands[1].endswith("($2)")
            and is_insn(insns[2], "lui", "$4", insns[2].operands[1])
            and is_insn(insns[3], "addiu", "$29", "$29", insns[3].operands[2])
            and frame_down < 0
            and is_insn(insns[4], "ori", "$4", "$4", insns[4].operands[2])
            and insns[5].mnemonic == "sq" and insns[5].operands[0] == "$31"
            and insns[6].mnemonic == "jal"
            and is_insn(insns[7], "addu", "$4", "$2", "$4")
            and insns[tail_index].mnemonic == "lq" and insns[tail_index].operands[0] == "$31"
            and frame_up == -frame_down):
        return []
    if len(insns) == 11:
        if not (is_insn(insns[9], "jr", "$31")
                and is_insn(insns[10], "addiu", "$29", "$29", insns[10].operands[2])):
            return []
        explicit_return = None
    else:
        if not (is_insn(insns[9], "addiu", "$2", "$0", insns[9].operands[2])
                and is_insn(insns[10], "jr", "$31")
                and is_insn(insns[11], "addiu", "$29", "$29", insns[11].operands[2])):
            return []
        try:
            explicit_return = parse_integer(insns[9].operands[2])
        except ValueError:
            return []
    callee_name = insns[6].operands[0]
    global_decl = f'extern void* {global_name} __attribute__((section(".data")));'
    signature = decode_signature(target.name, target.source.suffix, [])
    if signature.known and signature.params:
        return []
    results = []
    choices = ("int", "void") if explicit_return is None else ("int",)
    for return_type in choices:
        callee_return = return_type if explicit_return is None else "void"
        callee = decode_signature(callee_name, target.source.suffix, [CType("void").pointer()])
        if callee.known and len(callee.params) != 1:
            continue
        callee_linkage = "" if callee.cpp_linkage or callee.c_source else 'extern "C" '
        callee_param = callee.params[0]
        forwards = [f"struct {name};" for name in callee_param.named_structs]
        callee_decl = f"{callee_linkage}{callee_return} {callee.source_name}({callee_param.spelling});"
        call_arg = f"({callee_param.spelling})((char*){global_name} + {hex(object_offset)})"
        if explicit_return is None:
            statement = f"{callee.source_name}({call_arg})"
            body = f"    return {statement};" if return_type == "int" else f"    {statement};"
            ambiguity = "wrapper return type is not encoded in its symbol"
        else:
            body = f"    {callee.source_name}({call_arg});\n    return {explicit_return};"
            ambiguity = None
        prelude = "\n".join(dict.fromkeys(forwards + [global_decl, callee_decl]))
        candidate = candidate_for_body(
            target, "global-object-wrapper", return_type, [], body,
            return_choice=return_type, ambiguity=ambiguity,
            prelude=prelude, linker_definitions={global_name: global_address})
        if candidate:
            candidate.replacement = prelude + "\n\n" + candidate.replacement
            results.append(candidate)
    return results


def printf_callback_recipe(target):
    """Recognize printf(global_format, callback_data->word) then return 1."""
    insns = target.instructions
    if len(insns) != 10:
        return []
    try:
        global_name = relocation_symbol(insns[1].operands[1], "hi")
        low_name = relocation_symbol(insns[3].operands[2], "lo")
        global_address = data_label_address(global_name)
        field_offset, field_base = parse_memory(insns[5].operands[1])
        return_value = parse_integer(insns[7].operands[2])
        frame_down = parse_integer(insns[0].operands[2])
        frame_up = parse_integer(insns[9].operands[2])
    except (IndexError, ValueError):
        return []
    if global_name != low_name:
        return []
    if not (is_insn(insns[0], "addiu", "$29", "$29", insns[0].operands[2])
            and frame_down < 0
            and is_insn(insns[1], "lui", "$4", insns[1].operands[1])
            and insns[2].mnemonic == "sq" and insns[2].operands[0] == "$31"
            and is_insn(insns[3], "addiu", "$4", "$4", insns[3].operands[2])
            and is_insn(insns[4], "jal", "STUB_printf")
            and is_insn(insns[5], "lw", "$5", insns[5].operands[1]) and field_base == 5
            and insns[6].mnemonic == "lq" and insns[6].operands[0] == "$31"
            and is_insn(insns[7], "addiu", "$2", "$0", insns[7].operands[2])
            and is_insn(insns[8], "jr", "$31")
            and is_insn(insns[9], "addiu", "$29", "$29", insns[9].operands[2])
            and frame_up == -frame_down):
        return []
    inferred = [CType("void").pointer(), CType("void").pointer(), CType("void").pointer()]
    body = (f"    STUB_printf({global_name}, *(int*)((char*)arg1 + {field_offset}));\n"
            f"    return {return_value};")
    prelude = (f'extern char {global_name}[] __attribute__((section(".data")));\n'
               'extern "C" int STUB_printf(char*, ...);')
    candidate = candidate_for_body(
        target, "printf-callback", "int", inferred, body, prelude=prelude,
        linker_definitions={global_name: global_address})
    if candidate:
        candidate.replacement = prelude + "\n\n" + candidate.replacement
        return [candidate]
    return []


def wrapper_recipe(target):
    insns = target.instructions
    if len(insns) != 7:
        return []
    try:
        frame_down = parse_integer(insns[0].operands[2])
        frame_up = parse_integer(insns[6].operands[2])
    except (IndexError, ValueError):
        return []
    if not (is_insn(insns[0], "addiu", "$29", "$29", insns[0].operands[2])
            and frame_down < 0 and frame_up == -frame_down
            and insns[1].mnemonic == "sq" and insns[1].operands[0] == "$31"
            and insns[2].mnemonic == "jal"
            and insns[4].mnemonic == "lq" and insns[4].operands[0] == "$31"
            and is_insn(insns[5], "jr", "$31")
            and is_insn(insns[6], "addiu", "$29", "$29", insns[6].operands[2])):
        return []
    callee_symbol = insns[2].operands[0]
    callee = decode_signature(callee_symbol, target.source.suffix, [])
    wrapper = decode_signature(target.name, target.source.suffix, [])
    if not callee.known or not wrapper.known or len(callee.params) != len(wrapper.params):
        return []
    arguments = [f"arg{index}" for index in range(len(wrapper.params))]
    delay = insns[3]
    if is_nop(delay):
        pass
    elif (len(arguments) and delay.mnemonic == "addiu"
          and delay.operands[:2] == ("$4", "$4")):
        try:
            adjustment = parse_integer(delay.operands[2])
        except ValueError:
            return []
        arguments[0] = f"({callee.params[0].spelling})((char*)arg0 + {adjustment})"
    else:
        return []
    callee_linkage = "" if callee.cpp_linkage else 'extern "C" '
    callee_params = ", ".join(value.spelling for value in callee.params) or "void"
    declarations = []
    for value in callee.params:
        declarations.extend(f"struct {name};" for name in value.named_structs)
    call = f"{callee.source_name}({', '.join(arguments)})"
    results = []
    for return_type, body in (("int", f"    return {call};"), ("void", f"    {call};")):
        declaration = f"{callee_linkage}{return_type} {callee.source_name}({callee_params});"
        prelude = "\n".join(dict.fromkeys(declarations + [declaration]))
        candidate = candidate_for_body(
            target, "forwarding-wrapper", return_type, wrapper.params, body,
            return_choice=return_type,
            ambiguity="wrapper return type is not encoded in its symbol",
            prelude=prelude)
        if candidate:
            # Integrated source also needs the callee declaration. Repeated
            # compatible declarations are valid and a conflict safely fails.
            candidate.replacement = prelude + "\n\n" + candidate.replacement
            results.append(candidate)
    return results


def compile_flags(source):
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    stem = source.relative_to(ROOT / "code").with_suffix("")
    object_token = f"$(OBJ_DIR)/{stem}.o"
    for line in makefile.splitlines():
        if ": PRIVATE_COMPILE_FLAGS =" not in line:
            continue
        targets, flags = line.split(": PRIVATE_COMPILE_FLAGS =", 1)
        if object_token in targets.split():
            return flags.strip()
    return ""


def verify_candidate(candidate):
    with tempfile.TemporaryDirectory(prefix="rc1-auto-decomp-") as directory:
        work = Path(directory)
        source = work / ("candidate" + candidate.target.source.suffix)
        source.write_text(candidate.probe_source, encoding="utf-8")
        command = [
            sys.executable, str(ROOT / "tools/decomp_probe.py"),
            str(source), str(candidate.target.asm), candidate.target.name,
            "--out", str(work / "result"),
        ]
        flags = compile_flags(candidate.target.source)
        if flags:
            command.extend(["--flags", flags])
        for name, address in candidate.linker_definitions.items():
            command.extend(["--define", f"{name}={hex(address)}"])
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        result_file = work / "result/candidate.json"
        data = json.loads(result_file.read_text(encoding="utf-8")) if result_file.exists() else None
        return result.returncode == 0 and bool(data and data.get("match")), data, result.stdout + result.stderr


def run_parity(jobs):
    build = subprocess.run(["make", "--no-print-directory", f"-j{jobs}"], cwd=ROOT)
    if build.returncode:
        return False
    return subprocess.run(
        ["cmp", "-s", "build/boot_elf.elf", "assets/boot_elf.elf"], cwd=ROOT
    ).returncode == 0


def source_is_dirty(path):
    relative = str(path.relative_to(ROOT))
    tracked = subprocess.run(
        ["git", "ls-files", "--error-unmatch", "--", relative], cwd=ROOT,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    if not tracked:
        return True
    return subprocess.run(
        ["git", "diff", "--quiet", "--", relative], cwd=ROOT).returncode != 0


def replace_placeholder(target, replacement):
    old_text = target.source.read_text(encoding="utf-8")
    matches = [match for match in INCLUDE_RE.finditer(old_text)
               if match.group("folder") == target.folder and match.group("name") == target.name]
    if len(matches) != 1:
        raise RuntimeError("placeholder changed since scan")
    match = matches[0]
    indent = match.group("indent")
    indented = "\n".join(indent + line if line else "" for line in replacement.splitlines())
    new_text = old_text[:match.start()] + indented + old_text[match.end():]
    target.source.write_text(new_text, encoding="utf-8")
    return match.group(0), indented


def rollback_replacement(target, placeholder, replacement):
    text = target.source.read_text(encoding="utf-8")
    if text.count(replacement) != 1:
        raise RuntimeError(f"cannot safely roll back concurrent edit in {target.source}")
    target.source.write_text(text.replace(replacement, placeholder, 1), encoding="utf-8")


def record_match(candidate):
    path = ROOT / "decomp_state/matched.json"
    state = json.loads(path.read_text(encoding="utf-8"))
    first = candidate.target.instructions[0]
    state[candidate.target.id] = {
        "name": candidate.target.name,
        "source": str(candidate.target.source.relative_to(ROOT)),
        "asm": str(candidate.target.asm.relative_to(ROOT)),
        "vram": f"0x{first.address:08X}",
        "file_offset": f"0x{first.offset:X}",
        "size": candidate.target.size,
        "matched_date": datetime.date.today().isoformat(),
        "note": (f"Generated as {candidate.kind} by tools/auto_decomp_simple.py; "
                 "standalone relocated bytes and the full boot image matched mechanically."),
    }
    path.write_text(json.dumps(state, indent=2) + "\n", encoding="utf-8")


def remaining_count():
    result = subprocess.run(
        [sys.executable, "tools/decomp_status.py", "--count"], cwd=ROOT,
        capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else "unknown"


def notify(candidate, outcome, detail):
    count = remaining_count()
    subtitle = f"{outcome} {candidate.target.name}"
    if detail:
        subtitle += f": {detail}"
    body = f"Simple-function automation {outcome} {candidate.kind}. {count} nonmatching left"
    result = subprocess.run(
        [sys.executable, "brrr.py", "-t", "RC1 decomp", "-s", subtitle, body],
        cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        print("warning: mobile notification failed", file=sys.stderr)


def choose_candidates(candidates, wrapper_return):
    if not candidates:
        return []
    if not candidates[0].semantic_ambiguity:
        return candidates
    if wrapper_return:
        return [candidate for candidate in candidates if candidate.return_choice == wrapper_return]
    return candidates


def select_targets(targets, selectors):
    if not selectors:
        return targets
    selected = []
    for selector in selectors:
        matches = [target for target in targets if target.name == selector or target.id == selector
                   or f"{target.source.relative_to(ROOT)}:{target.name}" == selector]
        if not matches:
            raise SystemExit(f"target not found: {selector}")
        if len(matches) > 1:
            raise SystemExit(f"target is ambiguous; use source:name: {selector}")
        selected.extend(matches)
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("targets", nargs="*", help="symbol, target ID, or source:symbol")
    parser.add_argument("--verify", action="store_true", help="compile and byte-compare candidates")
    parser.add_argument("--apply", action="store_true", help="apply only verified candidates and require full parity")
    parser.add_argument("--limit", type=int, help="process at most this many supported targets")
    parser.add_argument("--kind", action="append", help="restrict to a recognized recipe kind")
    parser.add_argument("--wrapper-return", choices=("int", "void"),
                        help="resolve the return-type ambiguity for forwarding wrappers")
    parser.add_argument("--allow-unknown-signature", action="store_true",
                        help="permit applying unmangled functions whose unused parameters are unknowable")
    parser.add_argument("--allow-dirty", action="store_true",
                        help="permit applying to a source file with existing uncommitted edits")
    parser.add_argument("--jobs", type=int, default=2, help="parallel make jobs for parity checks")
    parser.add_argument("--json", action="store_true", help="emit scan results as JSON")
    parser.add_argument("--no-notify", action="store_true", help="skip brrr.py notifications after apply attempts")
    args = parser.parse_args()
    if args.apply:
        args.verify = True

    targets = select_targets(discover_targets(), args.targets)
    rows = []
    work = []
    for target in targets:
        candidates = [candidate for candidate in choose_candidates(load_recipe(target), args.wrapper_return)
                      if candidate is not None]
        if args.kind and (not candidates or candidates[0].kind not in args.kind):
            continue
        row = {
            "id": target.id, "name": target.name,
            "instructions": len(target.instructions), "size": target.size,
            "kind": candidates[0].kind if candidates else None,
            "known_signature": bool(candidates and candidates[0].known_signature),
            "ambiguity": candidates[0].semantic_ambiguity if candidates else None,
        }
        rows.append(row)
        if candidates:
            work.append((target, candidates))
    if args.limit is not None:
        work = work[:args.limit]

    if not args.verify:
        if args.json:
            print(json.dumps(rows, indent=2))
        else:
            for row in rows:
                if row["kind"]:
                    certainty = "known-signature" if row["known_signature"] else "unknown-signature"
                    ambiguity = f", {row['ambiguity']}" if row["ambiguity"] else ""
                    print(f"{row['name']}: {row['kind']} ({row['instructions']} insns, {certainty}{ambiguity})")
            print(f"recognized {len(work)} of {len(targets)} selected nonmatching functions")
        return 0

    if args.apply and not run_parity(args.jobs):
        print("refusing to edit: baseline build does not match the original boot image", file=sys.stderr)
        return 2
    initial_dirty = {target.source: source_is_dirty(target.source) for target, _ in work} if args.apply else {}

    failures = 0
    for target, candidates in work:
        verified = []
        for candidate in candidates:
            matched, data, output = verify_candidate(candidate)
            if matched:
                verified.append(candidate)
                print(f"verified {target.name}: {candidate.kind} ({candidate.return_choice})")
            else:
                failures += 1
                complete = data and data.get("status") != "incomplete"
                differences = len(data.get("differences", [])) if complete else "compile/link failure"
                detail = f"{differences} differences" if isinstance(differences, int) else differences
                print(f"rejected {target.name}: {candidate.kind} ({detail})")
                if not complete:
                    print(output, file=sys.stderr)
        if not args.apply:
            continue
        if not verified:
            if not args.no_notify:
                notify(candidates[0], "bailed", "candidate mismatch")
            continue
        if len(verified) != 1:
            failures += 1
            print(f"not applying {target.name}: multiple byte-identical semantic variants; use --wrapper-return", file=sys.stderr)
            if not args.no_notify:
                notify(verified[0], "bailed", "ambiguous return type")
            continue
        candidate = verified[0]
        if not candidate.known_signature and not args.allow_unknown_signature:
            failures += 1
            print(f"not applying {target.name}: signature is not encoded; use --allow-unknown-signature after review", file=sys.stderr)
            if not args.no_notify:
                notify(candidate, "bailed", "unknown signature")
            continue
        if initial_dirty[target.source] and not args.allow_dirty:
            failures += 1
            print(f"not applying {target.name}: {target.source.relative_to(ROOT)} is dirty; use --allow-dirty after review", file=sys.stderr)
            if not args.no_notify:
                notify(candidate, "bailed", "source file dirty")
            continue
        placeholder, replacement = replace_placeholder(target, candidate.replacement)
        if run_parity(args.jobs):
            record_match(candidate)
            print(f"applied {target.name}: full boot image matches")
            if not args.no_notify:
                notify(candidate, "matched", "")
        else:
            failures += 1
            rollback_replacement(target, placeholder, replacement)
            run_parity(args.jobs)
            print(f"rolled back {target.name}: integrated full-image parity failed", file=sys.stderr)
            if not args.no_notify:
                notify(candidate, "bailed", "integrated parity failed")
    return int(failures != 0)


if __name__ == "__main__":
    raise SystemExit(main())
