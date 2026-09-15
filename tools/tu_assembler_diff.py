#!/usr/bin/env python3
"""Per-function diff of one translation unit's object against the boot ELF.

Compiles nothing: give it a compiled object (e.g. a TU recompiled with a
different assembler via `make probe ASSEMBLER=...`) and a LINKED candidate
boot ELF (swap the object into build/, run the link step, copy the result
out). Each function's bytes in the candidate ELF are compared with the
original at the same VMA, so relocations are resolved on both sides.

Usage (from the repository root, with the venv and a completed split):
  python3 tools/tu_assembler_diff.py build/code/game/menu.o build/boot_elf.elf
  python3 tools/tu_assembler_diff.py /tmp/x.o /tmp/candidate.elf --all

Exit status: 0 when every function matches, 1 otherwise.

NOTE: BFD 2.9-ee readelf prints st_size in DECIMAL (verified:
func_00208508 size 616 + 0x1b90 == next symbol 0x1df8). Do not "fix" this
to hex parsing.
"""
import argparse
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "tools/mipsel-linux-gnu/bin"
LINKER_SCRIPT = ROOT / "build/SCUS_971.99.ld"
ORIGINAL = ROOT / "assets/boot_elf.elf"


def load_elf_bytes(path):
    data = Path(path).read_bytes()
    off = int.from_bytes(data[0x1C:0x20], "little")
    phentsize = struct.unpack_from("<H", data, 0x2A)[0]
    phnum = struct.unpack_from("<H", data, 0x2C)[0]
    segs = []
    for i in range(phnum):
        base = off + i * phentsize
        if struct.unpack_from("<I", data, base)[0] != 1:
            continue
        p_offset = struct.unpack_from("<I", data, base + 4)[0]
        p_vaddr = struct.unpack_from("<I", data, base + 8)[0]
        p_filesz = struct.unpack_from("<I", data, base + 16)[0]
        segs.append((p_vaddr, p_offset, p_filesz))
    return data, segs


def vma_to_offset(vma, segs):
    for p_vaddr, p_offset, p_filesz in segs:
        if p_vaddr <= vma < p_vaddr + p_filesz:
            return p_offset + (vma - p_vaddr)
    return None


def symbol_addresses():
    names = {}
    for path in (LINKER_SCRIPT, ROOT / "config/symbols.txt"):
        for line in path.read_text().splitlines():
            m = re.fullmatch(r"\s*(\w+)\s*=\s*(0x[0-9a-fA-F]+)\s*;", line)
            if m:
                names[m.group(1)] = int(m.group(2), 16)
                continue
            m = re.fullmatch(r"\s*(0x[0-9a-fA-F]+)\s+(\w+)\s*(?:\n|$)", line)
            if m:
                names[m.group(2)] = int(m.group(1), 16)
    return names


def object_functions(obj):
    """Return (name, offset_in_text, size) for .text FUNC symbols."""
    out = subprocess.run([str(BIN / "mipsel-linux-gnu-readelf"), "-S", "-W", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    sec_names = {}
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\.\S+)", line)
        if m:
            sec_names[int(m.group(1))] = m.group(2)
    funcs = []
    out = subprocess.run([str(BIN / "mipsel-linux-gnu-readelf"), "-s", "-W", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        # Num: Value Size Type Bind Vis Ndx Name
        if len(parts) != 8 or not parts[0].endswith(":"):
            continue
        try:
            value = int(parts[1], 16)
            size = int(parts[2], 10)
        except ValueError:
            continue
        typ, bind, ndx, name = parts[3], parts[4], parts[6], parts[7]
        if typ != "FUNC" or bind != "GLOBAL" or ndx == "UND":
            continue
        if sec_names.get(int(ndx)) != ".text":
            continue
        funcs.append((name, value, size))
    funcs.sort(key=lambda f: f[1])
    unique = []
    for name, value, size in funcs:
        if name.endswith(".NON_MATCHING") or (unique and unique[-1][1] == value):
            continue
        unique.append((name, value, size))
    return unique


def text_data(obj):
    out = subprocess.run([str(BIN / "mipsel-linux-gnu-readelf"), "-S", "-W", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        if re.match(r"\s*\[\s*\d+\]\s+\.text\b", line):
            parts = line.split()
            return Path(obj).read_bytes()[int(parts[5], 16):int(parts[5], 16) + int(parts[6], 16)]
    raise RuntimeError(".text not found in " + str(obj))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", type=Path, help="compiled TU object to report on")
    parser.add_argument("candidate_elf", type=Path,
                        help="linked boot ELF containing that object (relocs resolved)")
    parser.add_argument("--all", action="store_true", help="list matching functions too")
    args = parser.parse_args()

    elf_bytes, segs = load_elf_bytes(ORIGINAL)
    cand_bytes, cand_segs = load_elf_bytes(args.candidate_elf)
    names = symbol_addresses()

    results = []
    for name, offset, size in object_functions(args.object):
        vma = names.get(name)
        if vma is None:
            results.append((name, size, None, "no-address-in-linker-script"))
            continue
        o_orig = vma_to_offset(vma, segs)
        o_cand = vma_to_offset(vma, cand_segs)
        if o_orig is None or o_cand is None or o_orig + size > len(elf_bytes) \
                or o_cand + size > len(cand_bytes):
            results.append((name, size, hex(vma), "vma-not-mapped"))
            continue
        original = elf_bytes[o_orig:o_orig + size]
        candidate = cand_bytes[o_cand:o_cand + size]
        diffs = [i for i in range(0, size, 4) if candidate[i:i + 4] != original[i:i + 4]]
        if not diffs:
            results.append((name, size, hex(vma), "match"))
        else:
            loc = ",".join(hex(vma + i) for i in diffs[:8])
            results.append((name, size, hex(vma),
                            f"{len(diffs)} word diffs @ {loc}" + ("..." if len(diffs) > 8 else "")))
    matched = sum(1 for r in results if r[3] == "match")
    print(f"total {len(results)} funcs, {matched} match, {len(results) - matched} differ/unknown")
    for name, size, vma, status in results:
        if args.all or status != "match":
            tag = "  OK  " if status == "match" else "  DIFF"
            print(tag, name, size, vma, status)
    return 0 if matched == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
