#!/usr/bin/env python3
"""Scan the boot ELF for any reference to a target address.

Usage: python3 tools/deadness_scan.py 0x12E198

Checks, per the AGENTS.md dead-tail rule:
  1. jal / j instructions in the executable text sections whose
     section-relative target is the address.
  2. Relative conditional/unconditional branches (major ops 4-7, 10-13)
     whose PC+4+signext(imm16)*4 target is the address.
  3. 32-bit (and 64-bit) words equal to the address in every
     non-text, non-NBITS section (jump tables, lit pools, data).

Exit status: 0 = no references found (dead), 1 = references found,
2 = usage/parse error.
"""

import struct
import sys

ELF = "assets/boot_elf.elf"
TEXT_SECTIONS = ("core.text", ".text")
# 6-bit major opcodes: j=2, jal=3, regimm(beqz/bnez)=1, beq=4, bne=5,
# blez=6, bgtz=7, beql=12, bnel=13, blezl=14, bgtzl=15
BRANCH_OPS = {4, 5, 6, 7, 12, 13, 14, 15}
REGIMM = 1
JAL = 3
J = 2


def load_sections(path):
    data = open(path, "rb").read()
    e_shoff = struct.unpack_from("<I", data, 0x20)[0]
    e_shentsize = struct.unpack_from("<H", data, 0x2E)[0]
    e_shnum = struct.unpack_from("<H", data, 0x30)[0]
    e_shstrndx = struct.unpack_from("<H", data, 0x32)[0]
    # ELF32_Shdr: name(0) type(4) flags(8) addr(12) offset(16) size(20)
    #             link(24) info(28) addralign(32) entsize(36)
    shdrs = []
    for i in range(e_shnum):
        b = e_shoff + i * e_shentsize
        sh_name = struct.unpack_from("<I", data, b + 0)[0]
        sh_type = struct.unpack_from("<I", data, b + 4)[0]
        sh_addr = struct.unpack_from("<I", data, b + 12)[0]
        sh_offset = struct.unpack_from("<I", data, b + 16)[0]
        sh_size = struct.unpack_from("<I", data, b + 20)[0]
        shdrs.append((sh_name, sh_type, sh_addr, sh_offset, sh_size))
    shstr_name = shdrs[e_shstrndx][0]
    shstr_off = shdrs[e_shstrndx][3]
    shstr_size = shdrs[e_shstrndx][4]
    names = data[shstr_off : shstr_off + shstr_size]
    secs = []
    for sh_name, sh_type, sh_addr, sh_offset, sh_size in shdrs:
        end = names.index(b"\0", sh_name)
        name = names[sh_name : end].decode()
        secs.append((name, sh_addr, sh_offset, sh_size, sh_type))
    return data, secs


def signext16(v):
    return v - 0x10000 if v & 0x8000 else v


def scan_text(data, secs, by_name, target, refs):
    for name in TEXT_SECTIONS:
        addr, off, size, _ = by_name[name]
        end = addr + size
        for pc in range(addr, end - 3, 4):
            word = struct.unpack_from("<I", data, off + (pc - addr))[0]
            op = word >> 26
            if op in (JAL, J):
                t = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
                if t == target:
                    refs.append(f"{name}: {op_name(op)} at 0x{pc:06X} -> 0x{t:06X}")
            elif op == REGIMM and (word >> 16) & 0x1F in (4, 5):
                t = pc + 4 + (signext16(word & 0xFFFF) << 2)
                if t == target:
                    refs.append(f"{name}: beqz/bnez at 0x{pc:06X} -> 0x{t:06X}")
            elif op in BRANCH_OPS:
                t = pc + 4 + (signext16(word & 0xFFFF) << 2)
                if t == target:
                    refs.append(f"{name}: branch(op {op}) at 0x{pc:06X} -> 0x{t:06X}")


def op_name(op):
    return {JAL: "jal", J: "j"}.get(op, f"op{op}")


def scan_data(data, secs, target, refs):
    for name, addr, off, size, sh_type in secs:
        if sh_type == 8:  # SHT_NOBITS
            continue
        if name in TEXT_SECTIONS:
            continue
        if size < 4 or size % 4 != 0:
            continue
        for i in range(0, size - 3, 4):
            w = struct.unpack_from("<I", data, off + i)[0]
            if w == target:
                refs.append(f"{name}: u32 at 0x{addr + i:06X} == target")
        for i in range(0, size - 7, 4):
            q = struct.unpack_from("<Q", data, off + i)[0]
            if q == target:
                refs.append(f"{name}: u64 at 0x{addr + i:06X} == target")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    target = int(sys.argv[1], 16)
    data, secs = load_sections(ELF)
    by_name = {n: (a, o, s, t) for n, a, o, s, t in secs}
    refs = []
    scan_text(data, secs, by_name, target, refs)
    scan_data(data, secs, target, refs)
    print(f"target 0x{target:06X}: {len(refs)} reference(s)")
    for r in refs:
        print("  " + r)
    return 1 if refs else 0


if __name__ == "__main__":
    sys.exit(main())
