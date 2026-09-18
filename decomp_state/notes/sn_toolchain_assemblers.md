# Authentic SN assembler experiments (2026-09-15)

User requested downloading and testing decomp.me's five SN compiler builds.
Research and integration were performed without subagents. Production C/C++
compilation defaults to `-snas` with SN 1.9.6.516; five translation units
retain GNU assembly for existing-source compatibility. Probes default to SN.
The full rebuild and boot ELF `cmp` pass; 685 nonmatching remain.

Update 2026-09-18: production now uses SN 1.9.6.516 with the same 2.73a
compiler. Compiler-isolation against the MPEG streaming sample reproduced
36/42 comparable functions with either GNU as or 1.9.6.516, versus 34/42 with
1.8.19.316; the newer assembler restored exact 272-byte viBufStopDMA and
824-byte viBufRestartDMA. It also reproduced the blocked Hud_GetIconIndex's
two missing NOPs. An isolated full build and an integrated-candidate build both
passed boot ELF parity. The compiler driver's `-v` option crashes 1.9.6.516
under Wine and was removed from the production C++ recipe.

## Finding

**The installed 2.73a compiler already works for the three symbolic-address
positive controls when paired with authentic `ps2eeas` instead of GNU as.**
No custom assembly expansion/postprocessor, numeric RAM addresses, load/store
aliases, or `-mno-split-addresses` were used in these positive probes.

The downloaded 2.73a cc1plus.exe has exactly the same SHA-256 as our installed
compiler. All five packages' GNU as binaries are identical to our installed
GNU assembler. The material addition is the SN assembler binary and `-snas`.
This demonstrates an authentic toolchain explanation for these cases, but
does not uniquely identify the exact original RC1 toolchain release.

## Measured results

All comparisons link objects at original addresses with original symbol
values, then compare function bytes (including size) with asset-validated
reference bytes. Default C++ flags: `-G8 -O2 -ffast-math -fno-exceptions`.

| Pipeline | DrawMobys (128 B) | ProcessMobyAnimData (68 B) | Menu callback (36 B) |
| --- | --- | --- | --- |
| All five compilers + bundled GNU as | 112 B, fail | 64 B, fail | 28 B, fail |
| 2.73a + bundled SN, via `-snas` | exact | exact | exact |
| 2.74 + borrowed 2.73a SN assembler | exact | exact | exact |
| 1.07 + borrowed 2.73a SN assembler | exact | exact | exact |
| 1.14 + bundled SN, via `-snas` | exact | exact | exact |
| 1.36 + bundled SN, via `-snas` | 128 B, fail | 68 B, fail | exact |

The combined archive lacks `ps2eeas` for 2.74 and 1.07. Their `-snas`
invocations fail explicitly with `cannot exec ps2eeas`. Their borrowed-SN
rows compile each build's own assembly with `-S`, then assemble that assembly
with the 2.73a package's SN binary; these are deliberately mixed pipelines.

**Assembler-isolation control:** feeding the SAME installed-2.73a-generated
`candidate.s` to each of the three downloaded SN assembler versions yields
exact matches for all three positives. Feeding it to any bundled GNU as
fails. Thus 1.36's own compiler output differs, but its assembler is capable
of reproducing the desired expansion on the older compiler's assembly.

Negative controls remain negative: the existing volatile-pointer symbolic
VU1_addDataRef probe is 84 B versus 76 B, and VU1_gsRegsNormal is 100 B versus
92 B with every tested SN pipeline. GNU produces 60 B and 76 B respectively,
also wrong. These are the existing research source forms, not an exhaustive
test of possible VU source declarations. Do not claim the VU family solved.

## Provenance and fingerprints

Manifest: https://github.com/decompme/compilers/blob/main/values.yaml

All archive names below are relative to:
`https://github.com/decompme/compilers/releases/download/compilers/`

| Archive | SHA-256 |
| --- | --- |
| ee-gcc2.95.2-273a.tar.gz | ee9d9a7fccb59aebfa78a5587f6f8059660b91f705acddbc292ad2243c8e562e |
| ps2_compilers.tar.xz | f337a65a4fbab5cf0e335efe266dcbc7cf69b19c665c6b3a8924736df151a02a |
| ee-gcc2.95.3-114.tar.gz | dbc2c8c764631788d4cbb4c848c3cb0002fded0f4a95bae39e6d8b794391a6cb |
| ee-gcc2.95.3-136.tar.gz | 3b6ae6897229ad005aaf1b0afaa1f3cb46e74b4c21a42e01130c07c0c598067f |

| Binary | Version | SHA-256 |
| --- | --- | --- |
| 2.73a cc1plus.exe (also installed) | SN 2.73a | 11f6abff04fa5aceaa11e565ad04904e642953959eb1c7b18c768db5ad264d80 |
| GNU as.exe (all five and installed) | 2.9-ee-991111b | 7f504e571215fead2a15a14520bd22dc13507a1f13b479ca8c06a165f4a886a4 |
| 2.73a ps2eeas.exe | 1.8.19.316 | c839dd63facabe7b76573c114056be61eaa7b3aa2329dd546930ab8f98898c76 |
| 1.14 ps2eeas.exe | 1.9.6.516 | 44bcd9aaa229d8a453730142792d542e56da14761e9677cffcd1a183f603836d |
| 1.36 Ps2EeAs.exe | 1.9.25.758 | cb5adda955e64626564212ef7e0c1434708c4e1ef423344a92ec8033306ed3aa |

## Reproduce

Downloaded archives, extracted binaries, detailed command logs and JSON word
diffs are under `/tmp/opencode/sn-toolchains`. The stand-alone tar.gz packages
are extracted into `273a/`, `114/`, and `136/`; the combined archive is extracted
at the root (giving `ee-gcc2.95.2-274/`, `ee-gcc2.95.3-107/`, etc.).

From the repository root, with those archives extracted:

```sh
source .venv/bin/activate
python tools/research_symbolic_addresses.py --out /tmp/opencode/sn-toolchains/baseline
python tools/research_sn_toolchains.py --out /tmp/opencode/sn-toolchains
```

The second harness supports `--builds 274 107` for a subset. It writes
per-build `test.log` and per-selection fingerprints/results JSON. `native`
means each compiler with GNU as; `snas` means the compiler driver with `-snas`;
`fixed-*` means assembling the baseline assembly unchanged; `borrowed-snas`
is the explicitly mixed pipeline described above. Direct SN assembler syntax:

```sh
WINEPREFIX="$PWD/tools/wineprefix" WINEDEBUG=-all tools/wine/bin/wine \
  /tmp/opencode/sn-toolchains/273a/lib/gcc-lib/ee/2.95.2/ps2eeas.exe \
  -G8 -o /tmp/opencode/sn-toolchains/draw.o \
  /tmp/opencode/sn-toolchains/baseline/draw/candidate.s
```

## Production pipeline

`make setup-snas` uses `tools/install_sn_assembler.py` to install only the
verified assembler from the 2.73a archive. It verifies both SHA-256 hashes;
`--archive PATH` permits offline installation. The compiler and SDK headers
remain those from the existing compiler installation. Missing SN assembly
binaries are installed automatically by make, and C/C++ objects depend on
the assembler file so replacing it triggers a rebuild.

`tools/run_ee_compiler.py` serializes compiler-driver invocations using an OS
file lock. Concurrent Wine/SN compiles intermittently failed to open the shared
`code/include/labels.inc` (observed in skyfunc and pause_sched); the serialized
driver completes the clean `make -j2` build. Native GNU cross-assembly remains
parallel. The wrapper propagates the compiler's exit status and releases the
lock when the process exits.

The default flags are `-G8 -O2 -ffast-math -fno-exceptions -snas`. SN rejects
the previous `-Wa,-EL -Wa,-Icode/include` flags, so these replace `-snas` only
for explicit GNU comparisons and the five compatibility TUs below. Existing
per-TU optimization flags remain in place. Standalone `.s` files continue
through the GNU cross assembler, and linking/binary conversion are unchanged.

An all-SN build of the unmigrated source does not match. It grows `.core_text`
into `.core_data` and also changes game functions. Full parity requires the
explicit assembler overrides listed here.

| GNU compatibility TU | Measured function changes with SN (GNU → SN bytes) |
| --- | --- |
| 989snd/ee/989snd.c | snd_SendCurrentBatch 276 → 304; snd_PrepareReturnBuffer 28 → 36; snd_PostMessage 56 → 64; several other scalar-access helpers grow |
| game/draw.cpp | draw_resetTextureDmaState (func_001F0B88) 60 → 48 |
| game/hud.cpp | hud_updateMessageTimer 24 → 32 |
| game/mobyutil.cpp | moby_getActiveObject / moby_getSecondaryObject each 48 → 52 |
| game/movie/vobuf.cpp | voBufCreate 76 → 72 |

The whole source object is the override boundary. All other C/C++ TUs use
SN. These are compatibility settings for current source forms, not proof
of which assembler Insomniac used for each original TU. Future migrations
can remove an override after correcting source/declarations and verifying
the affected function(s) plus full binary parity. No generated words are patched.

## menu.cpp migrated to SN (2026-09-15)

`game/menu.cpp` now compiles and assembles with SN; its GNU override was
removed from the Makefile. A per-function diff of the SN object linked into
the boot image (validated 65/65 on the then-GNU build) showed exactly one
mismatch: `menu_isSelectionCountZero__Fv` (0x206b78), 14 differing bytes out
of the whole 1.38 MB image. Its 4-byte plain in-window extern load expanded
to an absolute `lui/lw` pair under SN instead of the original single
GPREL16 `lw v0,-28316(gp)`. The SN form is 16 code bytes versus GNU's 12 +
4 alignment nops, so `menu.o`'s `.text` size is unchanged (0x1ea0) and no
layout shifts.

**ps2eeas single-pass memory-symbol rule** (probe-verified with hand-written
`.s`, SN 1.8.19.316, `-G8`): a memory-symbol reference (`lw/sw r,sym`)
expands to a single GPREL16 access when EITHER
- the reference sits inside a `.set noreorder`/`.set nomacro` region, OR
- an `.extern sym, N` declaration already appeared earlier in the file.
Otherwise it expands to an absolute self-based `lui/lw` pair (HI16/LO16),
regardless of any later `.extern`. EGC emits all `.extern` declarations at
the end of the file, so every memory-symbol reference not inside one of EGC's
noreorder/nomacro blocks (branches, `jal` delay slots) expands absolute under
SN. That is why `DrawMobys`'s tail store (inside a noreorder block) matched
under SN while `menu_isSelectionCountZero`'s load (a plain first
instruction) did not.

Fix (`menu_isSelectionCountZero__Fv`): seed the declaration inside the
function with `asm volatile(".extern menuSelectionCount, 4");` before the
return statement. SN accepts the duplicate end-of-file `.extern` EGC still
emits, GNU as accepts it too, and both assemblers then produce the original
`lw; jr ra; sltiu` shape. Verified under both assemblers: the GNU `menu.o`
was re-verified after the edit, and the SN `menu.o` links to a byte-for-byte
boot ELF. No generated words are patched.

The per-function diff used for this (and for future TU migrations) is
`tools/tu_assembler_diff.py`: compile the TU with the candidate assembler
(`make probe ASSEMBLER=snas PROBE_SOURCE=... PROBE_OUT=...`), swap the
object into `build/`, run the link step, then diff the linked candidate
against the original:

```sh
python3 tools/tu_assembler_diff.py build/code/game/menu.o build/boot_elf.elf
```

It exits non-zero while any function differs, printing the differing word
offsets per function.

Verification: `make clean && make split && make -j2`, followed by
`cmp build/boot_elf.elf assets/boot_elf.elf`, passes with these defaults.
This covers existing INCLUDE_ASM blocks, section layout, and all existing
matches. The compiler/probe suite passes all 12 tests, including three exact
SN symbolic-address controls. Its historical GP/RPC controls explicitly use
GNU assembly, matching their production TU overrides. An extern-only `.sdata`
attribute by itself did not restore their SN GP access.

The research matrix's `native` label means the historical GNU pipeline,
not the production default. `research_symbolic_addresses.py` explicitly
selects GNU for its baseline and model experiment, preserving its controls.
