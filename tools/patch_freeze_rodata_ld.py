#!/usr/bin/env python3
"""Place freeze.o's dense-switch jump tables at their original data-segment address.

EGC compiles the dense switch in mode_freezeInit (and, once decompiled,
DrawDialogText / UpdateModeFreeze) with jump tables in the object's .rodata
section. The original layout keeps those tables INSIDE the data segment at
0x1E78D0+, which Splat otherwise models as one raw blob (data.data.o).
config/RC1.yaml carves the jtbl hole out of the blob as the data_freeze
segment, and this script (run before every link) rewrites the generated
SCUS_971.99.ld to match the current state of build/code/game/freeze.o:

- freeze.o has a non-empty .rodata: place freeze.o(.rodata) in the .data_freeze
  output section at the hole, guarded by a top-level ASSERT that the section
  ends exactly at the hole's end (0x1E78F0), and drop the .rodata from the
  .text output section's rodata region.
- freeze.o has no .rodata (whole TU still INCLUDE_ASM): restore the raw blob
  entry (data_freeze.data.o) and the .text-region line.

The script is idempotent: it detects the current .ld state via the top-level
ASSERT marker and only rewrites when it disagrees with the object. Stdlib
only (minimal ELF32 section parse).
"""

import os
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
LD = os.path.join(REPO, "SCUS_971.99.ld")
OBJ = os.path.join(REPO, "build", "code", "game", "freeze.o")

HOLE_START = 0x1E78D0
HOLE_END = 0x1E78F0
BLOB = "build/code/_generated/build/data/data_freeze.data.o(.data)"
RODATA = "build/code/game/freeze.o(.rodata)"
INDENT = "        "
# Inline input line (inside the .data_freeze output section).
RODATA_LINE = INDENT + RODATA + ";\n"
BLOB_LINE = INDENT + BLOB + ";\n"
# Top-level size assertion. ASSERT is a top-level command in GNU ld and this
# ld rejects it INSIDE the SECTIONS block, so it is appended at end-of-file
# (after the closing brace of SECTIONS). Kept on ONE line (this ld parser
# mishandles newlines inside the command).
ASSERT_LINE = (
    'ASSERT(ADDR(.data_freeze) + SIZEOF(.data_freeze) == 0x%06x, '
    '"freeze.o .rodata must exactly fill the 0x%06x hole");\n'
    % (HOLE_END, HOLE_START)
)
# .text output section: freeze.o(.rodata) sits between these two inputs.
TEXT_BEFORE = INDENT + "build/code/game/framebuf.o(.rodata);\n"


def rodata_size(path):
    try:
        with open(path, "rb") as f:
            d = f.read()
    except OSError:
        return 0
    if d[:4] != b"\x7fELF" or d[4] != 1:
        return 0
    e_shoff = struct.unpack_from("<I", d, 0x20)[0]
    e_shentsize = struct.unpack_from("<H", d, 0x2E)[0]
    e_shnum = struct.unpack_from("<H", d, 0x30)[0]
    e_shstrndx = struct.unpack_from("<H", d, 0x32)[0]
    shdrs = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        nameoff, stype, _flags, _addr, _offset, size = struct.unpack_from(
            "<6I", d, off
        )
        shdrs.append((nameoff, stype, size))
    if e_shstrndx >= len(shdrs):
        return 0
    # Section header field order: name, type, flags, addr, offset, size, ...
    shstr_off = struct.unpack_from("<I", d, e_shoff + e_shstrndx * e_shentsize + 16)[0]
    for nameoff, stype, size in shdrs:
        end = d.index(b"\x00", shstr_off + nameoff)
        if d[shstr_off + nameoff : end] == b".rodata" and stype == 1:
            return size
    return 0


def patch(ld, size):
    # Drop the .text-region line FIRST so the replace below cannot hit the
    # copy being inserted into the .data_freeze section.
    if RODATA_LINE in ld:
        ld = ld.replace(RODATA_LINE, "", 1)
    if BLOB_LINE not in ld:
        print("patch_freeze_rodata_ld: %s line missing from %s" % (BLOB, LD))
        sys.exit(1)
    ld = ld.replace(BLOB_LINE, RODATA_LINE, 1)
    # Append the top-level ASSERT after the SECTIONS closing brace.
    if not ld.endswith("\n"):
        ld += "\n"
    ld += ASSERT_LINE
    with open(LD, "w") as f:
        f.write(ld)
    print(
        "patch_freeze_rodata_ld: placed %s at 0x%06x (%d bytes)"
        % (RODATA, HOLE_START, size)
    )


def unpatch(ld):
    if ASSERT_LINE not in ld:
        print("patch_freeze_rodata_ld: ASSERT marker missing from %s" % LD)
        sys.exit(1)
    ld = ld.replace(ASSERT_LINE, "", 1)
    if RODATA_LINE not in ld:
        print("patch_freeze_rodata_ld: %s line missing from %s" % (RODATA, LD))
        sys.exit(1)
    ld = ld.replace(RODATA_LINE, BLOB_LINE, 1)
    ld = ld.replace(TEXT_BEFORE, TEXT_BEFORE + RODATA_LINE, 1)
    with open(LD, "w") as f:
        f.write(ld)
    print(
        "patch_freeze_rodata_ld: restored %s (no .rodata in %s)" % (BLOB, OBJ)
    )


def main():
    if not os.path.exists(LD):
        return 0
    with open(LD) as f:
        ld = f.read()

    size = rodata_size(OBJ)
    patched = ASSERT_LINE in ld

    if size > 0 and not patched:
        patch(ld, size)
    elif size == 0 and patched:
        unpatch(ld)
    else:
        print(
            "patch_freeze_rodata_ld: %s .ld state matches %s (rodata=%d)"
            % ("SCUS_971.99", OBJ, size)
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
