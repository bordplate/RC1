#!/usr/bin/env python3
"""Splice the built ELF's bytes for a VMA range into a copy of the
original ELF and write a parseable candidate ELF for disassembly.

The built file in non-matching states has a truncated/corrupt section
header table (the Splat section-headers blob shifts with content size),
which makes BFD reject it outright. But its file layout otherwise mirrors
the original (sections pinned by the Splat linker script), so the raw
bytes of an unchanged region can be spliced into a copy of the original
ELF, which keeps its valid header and section table.

Usage: python3 tools/fpatch.py <vma> <size> [output]
  e.g. python3 tools/fpatch.py 0x1FA978 0x904
Writes build/boot_elf.fpatch.elf (or the given output).
"""
import struct
import sys
from pathlib import Path

root = Path(__file__).resolve().parent.parent
orig_path = root / "assets" / "boot_elf.elf"
built_path = root / "build" / "boot_elf.elf"
out_path = Path(sys.argv[3]) if len(sys.argv) > 3 else root / "build" / "boot_elf.fpatch.elf"

vma = int(sys.argv[1], 16)
size = int(sys.argv[2], 16)

orig = bytearray(orig_path.read_bytes())
built = built_path.read_bytes()

# Map VMA to file offset through the original section table.
e_shoff, = struct.unpack_from('<I', orig, 0x20)
shentsize, = struct.unpack_from('<H', orig, 0x2E)
shnum, = struct.unpack_from('<H', orig, 0x30)
off = None
for i in range(shnum):
    o = e_shoff + i * shentsize
    (sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size) = \
        struct.unpack_from('<IIIIII', orig, o)
    if sh_type == 1 and sh_addr <= vma < sh_addr + sh_size:
        if vma + size > sh_addr + sh_size:
            raise SystemExit(f"range {hex(vma)}+{hex(size)} exceeds section "
                             f"{hex(sh_addr)}+{hex(sh_size)}")
        off = sh_offset + (vma - sh_addr)
        break
if off is None:
    raise SystemExit(f"no PROGBITS section for {hex(vma)}")

if off + size > len(built):
    raise SystemExit(f"built file too short: need {off+size}, have {len(built)}")

orig[off:off + size] = built[off:off + size]
out_path.write_bytes(bytes(orig))
print(str(out_path))
