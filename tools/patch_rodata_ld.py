#!/usr/bin/env python3
"""Place decompiled switch jump tables at their original data-segment address.

EGC compiles dense switches with jump tables in the object's .rodata section.
The original layout keeps those tables INSIDE the data segment at fixed
addresses, which Splat otherwise models as raw data blobs. Each such jtbl hole
is carved out as its own segment in config/RC1.yaml, and this script (run
before every link) rewrites the generated SCUS_971.99.ld to match the current
state of each owning object:

- The object has a non-empty .rodata: place it in the segment's output section
  at the hole, guarded by a top-level ASSERT that the section ends exactly at
  the hole's end, and drop the .rodata from the .text output section's rodata
  region.
- The object has no .rodata (whole TU still INCLUDE_ASM): restore the raw blob
  entry and the .text-region line.

The script is idempotent per hole: it detects the current .ld state via the
hole's top-level ASSERT marker and only rewrites when it disagrees with the
object. Stdlib only (minimal ELF32 section parse).
"""

import os
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
LD = os.path.join(REPO, "SCUS_971.99.ld")
INDENT = "        "
# Anchor in the .text rodata region where an object's .rodata line is (re)inserted.
TEXT_BEFORE = INDENT + "build/code/game/framebuf.o(.rodata);\n"

# Each carved-out jtbl hole. The blob path is the raw-data object Splat
# generates for the segment; the rodata path is the owning TU's .rodata input.
HOLES = [
    {
        "name": "freeze",
        "short": "freeze.o",
        "obj": os.path.join(REPO, "build", "code", "game", "freeze.o"),
        "blob": "build/code/_generated/build/data/data_freeze.data.o(.data)",
        "rodata": "build/code/game/freeze.o(.rodata)",
        "vram_start": 0x1E78D0,
        "vram_end": 0x1E78F0,
    },
    {
        "name": "help",
        "short": "help.o",
        "obj": os.path.join(REPO, "build", "code", "game", "help.o"),
        "blob": "build/code/_generated/build/data/data_help.data.o(.data)",
        "rodata": "build/code/game/help.o(.rodata)",
        "vram_start": 0x1E7A20,
        "vram_end": 0x1E7A40,
    },
]


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
    shstr_off = struct.unpack_from(
        "<I", d, e_shoff + e_shstrndx * e_shentsize + 16
    )[0]
    for nameoff, stype, size in shdrs:
        end = d.index(b"\x00", shstr_off + nameoff)
        if d[shstr_off + nameoff : end] == b".rodata" and stype == 1:
            return size
    return 0


def assert_line(hole):
    return (
        'ASSERT(ADDR(.data_%s) + SIZEOF(.data_%s) == 0x%06x, '
        '"%s .rodata must exactly fill the 0x%06x hole");\n'
        % (hole["name"], hole["name"], hole["vram_end"], hole["short"], hole["vram_start"])
    )


def patch(ld, hole, size):
    blob_line = INDENT + hole["blob"] + ";\n"
    rodata_line = INDENT + hole["rodata"] + ";\n"
    # Drop the .text-region line FIRST so the replace below cannot hit the
    # copy being inserted into the .data_<name> section.
    if rodata_line in ld:
        ld = ld.replace(rodata_line, "", 1)
    if blob_line not in ld:
        print("patch_rodata_ld: %s line missing from %s" % (hole["blob"], LD))
        sys.exit(1)
    ld = ld.replace(blob_line, rodata_line, 1)
    # Append the top-level ASSERT after the SECTIONS closing brace.
    if not ld.endswith("\n"):
        ld += "\n"
    ld += assert_line(hole)
    print(
        "patch_rodata_ld: placed %s at 0x%06x (%d bytes)"
        % (hole["rodata"], hole["vram_start"], size)
    )
    return ld


def unpatch(ld, hole):
    a = assert_line(hole)
    if a not in ld:
        print("patch_rodata_ld: ASSERT marker missing from %s" % LD)
        sys.exit(1)
    blob_line = INDENT + hole["blob"] + ";\n"
    rodata_line = INDENT + hole["rodata"] + ";\n"
    ld = ld.replace(a, "", 1)
    if rodata_line not in ld:
        print("patch_rodata_ld: %s line missing from %s" % (hole["rodata"], LD))
        sys.exit(1)
    ld = ld.replace(rodata_line, blob_line, 1)
    ld = ld.replace(TEXT_BEFORE, TEXT_BEFORE + rodata_line, 1)
    print(
        "patch_rodata_ld: restored %s (no .rodata in %s)" % (hole["blob"], hole["obj"])
    )
    return ld


def main():
    if not os.path.exists(LD):
        return 0
    with open(LD) as f:
        ld = f.read()

    changed = False
    for hole in HOLES:
        size = rodata_size(hole["obj"])
        patched = assert_line(hole) in ld

        if size > 0 and not patched:
            ld = patch(ld, hole, size)
            changed = True
        elif size == 0 and patched:
            ld = unpatch(ld, hole)
            changed = True
        else:
            print(
                "patch_rodata_ld: %s .ld state matches %s (rodata=%d)"
                % (hole["name"], hole["obj"], size)
            )

    if changed:
        with open(LD, "w") as f:
            f.write(ld)
    return 0


if __name__ == "__main__":
    sys.exit(main())
