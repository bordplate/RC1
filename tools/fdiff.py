#!/usr/bin/env python3
"""Function word diff: original boot ELF vs built boot ELF at a VMA."""
import struct, sys

def read_phdr_range(path, vma, size):
    with open(path, 'rb') as f:
        data = f.read()
        e_phoff, = struct.unpack_from('<I', data, 0x1C)
        e_phentsize, = struct.unpack_from('<H', data, 0x2A)
        e_phnum, = struct.unpack_from('<H', data, 0x2C)
        for i in range(e_phnum):
            off = e_phoff + i * e_phentsize
            (p_type, p_offset, p_vaddr, p_paddr,
             p_filesz, p_memsz, p_flags, p_align) = struct.unpack_from('<IIIIIIII', data, off)
            if p_type == 1 and p_vaddr <= vma < p_vaddr + p_memsz and vma + size <= p_vaddr + p_filesz:
                f.seek(p_offset + (vma - p_vaddr))
                return f.read(size)
        raise SystemExit(f"no PT_LOAD for {hex(vma)} in {path}")

VMA = int(sys.argv[1], 16)
N = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x61C
orig = read_phdr_range('assets/boot_elf.elf', VMA, N)
cand = read_phdr_range('build/boot_elf.elf', VMA, N)
diffs = [i for i in range(0, N, 4) if orig[i:i+4] != cand[i:i+4]]
print(f"{len(diffs)} word diffs of {N//4}")
for d in diffs:
    o = struct.unpack('<I', orig[d:d+4])[0]
    c = struct.unpack('<I', cand[d:d+4])[0]
    print(f"0x{VMA+d:06X} (+{d:03X}): orig {o:08X}  cand {c:08X}")
