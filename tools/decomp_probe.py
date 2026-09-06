#!/usr/bin/env python3
"""Compile one standalone candidate and compare relocated bytes to Splat/boot.

Requires the project's venv (pyelftools) and a completed `make split`.
Does not edit game sources, flags, or durable matching state.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent


def run(command, log):
    log.write(json.dumps(command) + "\n")
    log.flush()
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    log.write(result.stdout)
    log.flush()
    print(result.stdout, end="")
    result.check_returncode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("reference", type=Path, help="generated original .s")
    parser.add_argument("symbol", help="candidate's linker symbol (check nm for C++)")
    parser.add_argument("--flags", default="", help="extra flags, e.g. --flags=-fno-schedule-insns")
    parser.add_argument("--out", type=Path, required=True, help="experiment output directory")
    parser.add_argument("--cross", default=str(ROOT / "tools/mips64r5900el-ps2-elf/usr/bin/mips64r5900el-ps2-elf"))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    base = args.out.resolve() / "candidate"
    # A failed rerun must not leave an earlier successful result looking current.
    base.with_suffix(".json").write_text('{"match": false, "status": "incomplete"}\n')
    reference = args.reference.read_text()
    (args.out / "reference.s").write_text(reference)
    (args.out / ("source" + args.source.suffix)).write_text(args.source.read_text())
    rows = re.findall(r"/\*\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]{8})\s*\*/", reference.split("endlabel", 1)[0])
    if not rows:
        parser.error("reference contains no Splat instruction rows")
    offset, address = (int(value, 16) for value in rows[0][:2])
    expected = bytes.fromhex("".join(row[2] for row in rows))
    for index, row in enumerate(rows):
        if int(row[0], 16) != offset + index * 4 or int(row[1], 16) != address + index * 4:
            parser.error("reference instruction rows are not contiguous")
    original = (ROOT / "assets/boot_elf.elf").read_bytes()
    if original[offset:offset + len(expected)] != expected:
        parser.error("reference bytes disagree with original boot image")
    with base.with_suffix(".log").open("w") as log:
        run(["make", "--no-print-directory", "probe", f"PROBE_SOURCE={args.source.resolve()}",
             f"PROBE_OUT={base}", f"PROBE_FLAGS={args.flags}"], log)
    symbols = {}
    for path in [ROOT / "config/symbols.txt", ROOT / "build/undefined_syms_auto.txt", ROOT / "build/undefined_funcs_auto.txt"]:
        for name, value in re.findall(r"^\s*(\w+)\s*=\s*(0x[0-9a-fA-F]+)\s*;", path.read_text(), re.M):
            if name in symbols and symbols[name] != value.lower():
                if int(symbols[name], 16) != int(value, 16):
                    parser.error(f"conflicting address for {name}")
            symbols[name] = value.lower()
    with base.with_suffix(".o").open("rb") as stream:
        elf = ELFFile(stream)
        table = elf.get_section_by_name(".symtab")
        found = table.get_symbol_by_name(args.symbol)
        if not found or found[0]["st_shndx"] == "SHN_UNDEF":
            parser.error("candidate symbol not defined; inspect nm/mangling")
        symbol = found[0]
        if elf.get_section(symbol["st_shndx"]).name != ".text":
            parser.error("candidate must be in .text")
        start, size = symbol["st_value"], symbol["st_size"]
        undefined = [s.name for s in table.iter_symbols() if s.name and s["st_shndx"] == "SHN_UNDEF"]
    # The default linker script assigns _gp after --defsym. Use our own script
    # so GP-relative relocations really use the game's runtime GP.
    base.with_suffix(".ld").write_text(
        f"SECTIONS {{ .text {hex(address - start)} : {{ *(.text) }} }}\n"
        "_gp = 0x166c00;\n")
    command = [args.cross + "-ld", "-EL", "-m", "elf32lr5900",
               "-T", str(base.with_suffix(".ld")), "-e", args.symbol]
    for name in undefined:
        if name not in symbols:
            parser.error(f"unknown external {name}; add a verified address to config/symbols.txt, not a guessed relocation mask")
        command.extend(["--defsym", f"{name}={symbols[name]}"])
    command.extend([str(base.with_suffix(".o")), "-o", str(base.with_suffix(".elf"))])
    with base.with_suffix(".log").open("a") as log:
        run(command, log)
    with base.with_suffix(".elf").open("rb") as stream:
        linked = ELFFile(stream)
        gp = linked.get_section_by_name(".symtab").get_symbol_by_name("_gp")
        if not gp or gp[0]["st_value"] != 0x166C00:
            parser.error("probe linker did not preserve the runtime GP")
        section = linked.get_section_by_name(".text")
        actual = section.data()[start:start + size]
    differences = []
    for index in range(0, max(len(actual), len(expected)), 4):
        old, new = expected[index:index + 4], actual[index:index + 4]
        if old != new:
            differences.append({"address": hex(address + index), "original": old.hex(), "candidate": new.hex()})
    result = {"source": str(args.source), "reference": str(args.reference), "symbol": args.symbol,
              "flags": args.flags, "original_size": len(expected), "candidate_size": size,
              "source_sha256": hashlib.sha256(args.source.read_bytes()).hexdigest(),
              "original_sha256": hashlib.sha256(original).hexdigest(),
              "match": not differences, "differences": differences}
    base.with_suffix(".json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    print("Function bytes only; full build + cmp still required. No fallback was removed.")
    return 0 if not differences else 1


if __name__ == "__main__":
    raise SystemExit(main())
