#!/usr/bin/env python3
"""Function word diff: original boot ELF vs built boot ELF at a VMA.

The built file is laid out to mirror the original file (the Splat linker
script pins every section's ROM position), so both sides are read through
the ORIGINAL ELF's section table. That keeps the diff working in
non-matching states, where the built file's own section headers are
truncated/corrupt and its program headers are the linker's, not the
original's.

Note: if an earlier function in the same section changed size, everything
after it shifts; diff such a candidate function-by-function from its own
start address with a size no larger than the original's.
"""
import struct, sys

def section_map(path):
    data = open(path, 'rb').read()
    e_shoff, = struct.unpack_from('<I', data, 0x20)
    shentsize, = struct.unpack_from('<H', data, 0x2E)
    shnum, = struct.unpack_from('<H', data, 0x30)
    segs = []
    for i in range(shnum):
        off = e_shoff + i * shentsize
        (sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size) = \
            struct.unpack_from('<IIIIII', data, off)
        if sh_type == 1:  # PROGBITS
            segs.append((sh_addr, sh_offset, sh_size))
    return data, segs

def read_range(data, segs, vma, size):
    for sh_addr, sh_offset, sh_size in segs:
        if sh_addr <= vma < sh_addr + sh_size and vma + size <= sh_addr + sh_size:
            off = sh_offset + (vma - sh_addr)
            return data[off:off + size]
    raise SystemExit(f"no PROGBITS section for {hex(vma)}")

VMA = int(sys.argv[1], 16)
N = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x61C
orig_data, orig_segs = section_map('assets/boot_elf.elf')
cand_data = open('build/boot_elf.elf', 'rb').read()
orig = read_range(orig_data, orig_segs, VMA, N)
cand = read_range(cand_data, orig_segs, VMA, N)
if len(cand) < N:
    print(f"warning: candidate shorter than requested ({len(cand)} < {N}); "
          f"comparing {len(cand) & ~3} bytes")
    N = len(cand) & ~3
diffs = [i for i in range(0, N, 4) if orig[i:i+4] != cand[i:i+4]]
print(f"{len(diffs)} word diffs of {N//4}")
for d in diffs:
    o = struct.unpack('<I', orig[d:d+4])[0]
    c = struct.unpack('<I', cand[d:d+4])[0]
    print(f"0x{VMA+d:06X} (+{d:03X}): orig {o:08X}  cand {c:08X}")
