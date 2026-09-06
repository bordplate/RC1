# Compiler Blocker Workflow

Use this before repeating source permutations or blaming a custom compiler.
All commands run from the repository root. No subagent is required.

## Establish A Baseline

```sh
source .venv/bin/activate
pip install -r requirements.txt
make split
make -B -j2
cmp build/boot_elf.elf assets/boot_elf.elf
python tools/decomp_status.py
```

Do not proceed from an unexplained failing baseline. On 2026-09-06 the initial
build lacked a generated file; split repaired that, but stale sound-library
objects still caused parity failure. A forced rebuild with unchanged source
restored parity. Flags passed on the make command line are not dependency
tracked. The Makefile now tracks its own changes, userconfig.mk, top-level
headers, and same-stem generated assembly, but this is not a substitute for
the final clean build.

## Reproduce One Function

Keep a standalone candidate OUTSIDE `code/`, so it is not linked accidentally.
Use natural C++ names, e.g. `PutDispBuffer`, not `PutDispBuffer__Fv`.
`extern "C"` is appropriate for original unmangled C symbols, not as a way to
hand-spell a C++ mangled name. Verify parameter types and ABI from callers.

Two positive controls (both exit 0):

```sh
python tools/decomp_probe.py decomp_state/probes/menu_edge.cpp \
  decomp_state/probes/reference/menu_edge.s menu_pointIsClockwise \
  --flags=-fno-schedule-insns --out /tmp/opencode/check-edge
python tools/decomp_probe.py decomp_state/probes/vibuf_tag.cpp \
  decomp_state/probes/reference/vibuf_tag.s scTag2 \
  --out /tmp/opencode/check-tag
```

Negative control: rerun the edge probe without `--flags` in a different output
directory. It must exit 1 and report four reordered instruction positions.
The following unresolved FP probe must also exit 1:

```sh
python tools/decomp_probe.py decomp_state/probes/menu_threshold.cpp \
  code/_generated/nonmatchings/game/menu/func_00207690.s menu_threshold \
  --out /tmp/opencode/check-fp
```

The probe uses Makefile EEGCC/WINE/include/default flags, with experiment flags
appended. It deliberately does NOT inherit production per-file overrides:
pass `--flags=-fno-schedule-insns` for menu.cpp's current configuration.
Compiler output remains under the experiment directory: candidate.s, .o,
.elf, .log and .json, plus snapshots of the source and original reference.
The log records commands and compiler/linker output. A failed rerun invalidates
the prior JSON match result; do not trust leftover object files after failure.
The JSON includes differing byte positions, sizes, flags, and source/boot
hashes. Save or summarize results in durable notes; /tmp is not durable memory.

The script resolves external symbols from config/symbols.txt and split's
undefined-symbol scripts, links at the original function address with the
verified gp, and compares every byte. Unknown externs fail instead of masking
relocations. The reference's raw instruction bytes must also agree with the
original boot image. Padding after `endlabel` is excluded from function size;
the final full-image comparison covers padding and surrounding layout.
Use one function per probe; additional defined helpers/data can make standalone
layout unrepresentative. Symbol spelling changes and full-TU interactions must
still be validated in the real build.

GP correction from the deeper retest: a default ld script can override a
command-line `--defsym _gp=...`. The probe now writes its OWN linker script
with `_gp = 0x166c00` and verifies that symbol in the resulting ELF. Earlier
no-GP positive controls did not catch this. Run the GP control and the larger
RPC positive/negative controls too:

```sh
python -m unittest discover -s tools -p 'test_decomp_*.py'
```

This now includes actual compiler/linker integration tests, not just Python
unit tests. It requires the local toolchain, original asset, and a completed
split. A compilation error cannot count as a successful negative control;
tests require a completed JSON diff with the expected number of differences.

## Diagnose The First Difference

1. Check semantics before code generation. Decode instruction words as LE.
   `dsll32` means shift by 32+sa; `dsrl32` means shift right by 32+sa.
   A left shift alone is not zero extension. MIPS immediates use their actual
   instruction width, not an assumed 8-bit signed interpretation.
2. Check ABI and types: signed versus unsigned, 32/64-bit fields, natural
   C++ mangling, float argument registers, actual return use, and gp address.
   In this toolchain `long` is 64 bits and `long long` is 128 bits.
   Check callee returns even when ignored: the snd_batch control proves that
   declaring an int-returning RPC call void shifts later register allocation.
3. Inspect candidate.s AND the assembled object. `li.s` is a macro, `#nop`
   is only a comment. A missing hazard NOP can be an assembler issue even
   when the source-level compiler scheduling appears correct.
4. Test one relevant pass at a time: `-fno-schedule-insns` (pre-register
   allocation), `-fno-schedule-insns2` (post-register allocation), then both.
   Preserve the best source and exact differences; do not run hundreds of
   equivalent permutations once the controlling pass is identified.
5. Test flags at translation-unit scope before project-wide scope. 989snd is
   a separate library; its flag requirements do not prove menu's requirements.
   Any flag that changes existing matches needs an explanation and full parity.
   Do not introduce arbitrary per-function compiler hacks just to hide a diff.
6. Escalate to a compiler-version/assembler investigation only with a minimal
   correct reproducer, stage-specific evidence, exact versions and controls.
   Failure of some source forms does not prove no C form can match. An alternate
   toolchain should first run both positive controls, then representative already
   matched functions, then the entire clean build. Never patch compiled words.

Get supported options from the actual local executables, not modern GCC docs:

```sh
env WINEPREFIX="$PWD/tools/wineprefix" WINEDEBUG=-all tools/wine/bin/wine \
  tools/cc/lib/gcc-lib/ee/2.95.2/cc1plus.exe --help
env WINEPREFIX="$PWD/tools/wineprefix" WINEDEBUG=-all tools/wine/bin/wine \
  tools/cc/ee/bin/as.exe --help
```

Local versions: driver 2.9-ee-991111b/r4, C++ 2.95.2 SN BUILD v2.73a,
assembler 2.9-ee-991111b. If an RTL dump is needed, use a standalone probe
with `-da` (or selected `-d` passes); keep dump files out of game source.
The build's `-v` output shows the actual driver, cc1plus and assembler commands.

## Integrate And Verify

Only after a positive byte comparison, replace the selected INCLUDE_ASM.
Give unnamed functions descriptive names in source and config/symbols.txt;
rerun split and force a rebuild after renaming. Keep existing unrelated edits.
Do not commit generated references under code/_generated or build output.

```sh
source .venv/bin/activate
make clean && make split && make -j2
cmp build/boot_elf.elf assets/boot_elf.elf
python -m unittest discover -s tools -p 'test_decomp_*.py'
python tools/decomp_status.py --count
python tools/decomp_status.py --complete
```

`--complete` MUST fail while any nonmatching INCLUDE_ASM remains, including
blocked entries and retained dead tails. A successful fallback-based build
is not proof that a proposed C replacement matches. Blocker lookups ignore
historical line numbers, so moving source cannot silently unblock a target.

Record a concise matched note or concrete remaining blocker with the candidate,
flags, first diff, and next discriminating experiment. Correct or supersede
bad historical claims explicitly. Send exactly one brrr.py notification per
finished attempt as described in AGENTS.md. Commit only when requested.

## Current Results

- snd_SendCurrentBatch: matched all 276 bytes using DEFAULT flags after
  correcting sceSifCallRpc's return type to int and two GP global addresses.
  The wrong void callee declaration reproduces the epilogue blocker exactly.
- VU1_addDataRef: symbolic .data loads plus -mno-split-addresses match the
  first 68 bytes. Only final mixed absolute/GP addressing remains unresolved;
  this disproves the old "cannot generate these loads" compiler claim.

- menu_pointIsClockwise (formerly func_00208818): matched with menu-only
  pre-RA scheduling disabled. func_002089A8 uses equivalent E0/state/DC source
  store order under that flag. Full boot parity passes.
- scTag2: matched with DEFAULT flags by correcting the 64-bit tag expression.
  No special compiler required.
- func_00207690: corrected threshold is +190, not -66; still missing one FP
  hazard NOP. See notes/compiler_retests_2026_09_06.md.
- func_002088A8: post-RA scheduling disabled gets the correct order with a
  local struct pointer, leaving two register differences. Not yet integrated.
- PutDispBuffer__Fv: natural C++ name verified; tested scheduler/alias flags
  still fail prologue order. See the same retest note.

Other notes also deserve semantic review: the vsync note calls dsll32/dsrl32
"sign-extension", but that pair zero-extends. This observation is not a match
claim. Dead-tail entries require parent-function analysis, not standalone C
functions that fabricate an epilogue or copy original instructions.
