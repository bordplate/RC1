# Address / verification map (learned 2026-09-03)

## Coordinates

- The first number in generated `.s` comments (`/* <A> <B> <insn> */`) is the
  file offset inside `assets/boot_elf.elf`; `<B>` is the splat vram address.
- Splat segment "start" values in `config/RC1.yaml` are also file offsets.
  Example: `text` (level code) = file `0xe9c80..0x13e2e0`, vram base `0x1e8d00`;
  a function at file offset F in that segment has vram `0x1e8d00 + (F - 0xe9c80)`.
- The ELF has one PT_LOAD (`off 0x1000 -> vaddr 0x100080`), but the project,
  splat config and Ghidra all use the per-segment vram rebasing above.
- **Ghidra (headless MCP) address space == splat vram column.** e.g.
  `func_00207A08` is at `0x00207A08` in Ghidra, not the PHDR-derived value.

## Object-level verification of a single function

For a function in subsegment `[seg_start, ...)` (file offsets) at file offset F:
its position inside the compiled `.o` is `F - seg_start` within `.text`.
Check with a tiny ELF/symtab parser (no objdump in local toolchains):

- symbol st_value == `F - seg_start`, st_size == original size
- object bytes at that offset == original slice from `assets/boot_elf.elf`
- total `.text` size of the object == subsegment length
Then full oracle: `make && cmp build/boot_elf.elf assets/boot_elf.elf`.

## Compiler behaviour (EEGCC 2.95.2, `-G8 -O2 -ffast-math`)

- Empty `extern "C" void f(void) {}` -> `jr $ra; nop` (matches func_00208810).
- `extern "C" int f(void) { return 1; }` -> `jr $ra; addiu $v0,$0,1`,
  exactly 8 bytes, no extra alignment padding emitted (preceding asm blobs end
  on 8-byte boundaries so in-section placement lines up).
- Define matched functions with `extern "C"` directly in the `.cpp` at the spot
  of the removed `INCLUDE_ASM` line to preserve .text ordering.

## Caveats

- Tiny 2-instruction "functions" that are only stack tweaks without a `jr $ra`
  (e.g. several in `989snd`) look like split artifacts; verify against Ghidra
  before decompiling them. e.g. `func_00208028`/`func_00208500` in menu.cpp
  are mid-function fragments (`andi`/`sw` + nop, no entry/return).

## Delay-slot scheduling vs volatile stores (learned 2026-09-03, voBufReset)

- EGC 2.95.2 `-O2` moves an independent store into a `jr $ra` delay slot even
  when the source writes both stores first. If the original shows two stores
  in program order with a real `nop` after the branch, qualify BOTH stored
  fields `volatile` (one volatile is not enough). See
  `decomp_state/notes/vobuf_voBufReset.md`.
- Quick experiment harness: compile standalone variants with
  `env WINEPREFIX=/home/bordplate/Projects/RC1/tools/wineprefix WINEDEBUG=-all
  tools/wine/bin/wine tools/cc/bin/ee-gcc.exe -S -x c++ -G8 -O2 -ffast-math
  -fno-exceptions -Wa,-EL -Icode/include -Btools/cc/lib/gcc-lib/ee/2.95.2/` and
  compare the emitted `.s`. The `-B.../2.95.2/` path is required or cpp cannot
  be found; WINEPREFIX must be an absolute path. Also `mipsel-linux-gnu-objdump`
  IS available under `tools/mipsel-linux-gnu/bin/` for disassembling test objects.
- Makefile line 64 runs objcopy with identical in/out path
  (`build/boot_elf.elf`); verified harmless - the built file stays a full ELF
  and cmp passes byte-for-byte.
- Most targets live in `.cpp` files (830) vs `.c` (63).

## Stripped symbol table (learned 2026-09-03, func_0021A308)

- `assets/boot_elf.elf` has **no symtab/dynsym at all** (`nm`, readelf: no
  symbols). All `func_<addr>` names come from splat's flow-based splitting
  (boundaries after `jr $ra` epilogues), and the few real names in
  `config/symbols.txt` are the only authoritative name source. A small
  candidate starting with a full `jr $ra; ...` prologue-free body is therefore
  not proof of split-artifact status: check for data-pointer table entries
  pointing exactly at it (e.g. 0x21A308 has one at vram 0x1D1AA8 / file
  0x0D2A28, next to function start 0x2212B8) before dismissing it as a
  mid-function fragment.
- Splat/Ghidra vram-space GP is `0x166C00` (Makefile ld line: `--defsym
  _gp=0x166c00`; crt0 loads D_00166C00 into $gp). Ghidra's unresolved
  gp-relative labels are named `<type>Gp<offset-as-unsigned-hex>` (e.g.
  `uGpffff95b4` = gp-0x6A4C = 0x1601B4), so real addresses = 0x166C00 + offset.

## GP register / globals (learned 2026-09-03, func_001FF768)

- Runtime `$gp = 0x166C00`: crt0 (`code/_generated/sce/crt0.s`) does
  `lui/addiu a0,%hi/%lo(D_00166C00); daddu $gp,$a0,$zero`. `D_00166C00` is an
  auto splat dlabel inside `build/data/data.data.s` (the `data` segment, vram
  0x165480). All gp-relative offsets in nonmatching asm blobs are relative to
  this; Ghidra's gp-relative data resolution agrees (e.g. `-0x7308($gp)` ->
  `DAT_0015f8f8`).
- Link-time `_gp` must equal the same value for C code to emit matching
  gp-relative access: ps2 ld otherwise computes its own `_gp` and rejects out
  of range `R_MIPS_GPREL16` ("relocation truncated to fit"). Fix (committed):
  Makefile link line carries `--defsym _gp=0x166c00`. `SCUS_971.99.ld` is
  splat-regenerated on every `make split` and gitignored, so the Makefile is
  the only persistent place for this.
- To reference an existing global from C: add `NAME = 0xADDR;` to
  `config/symbols.txt`; splat emits a `dlabel NAME` in the data blob covering
  that vram (e.g. `D_0015F8F8` landed in `build/data/lit.lit4.s`). Declare it
  as plain `extern int NAME;` in the `.cpp` — NO section attribute, or EGC
  emits %hi/%lo instead of GPREL16 and instruction count changes.
- Globals reachable gp-relative are those within ±0x8000 of 0x166C00:
  roughly vram 0x15EC00..0x16E400 (core.bss tail, core.lit/lit, level bss +
  start of level data). Out-of-range globals need the usual %hi/%lo form.
- EGC emits `addu r,r,-1` in `.s` for int decrements; GAS re-encodes it as
  `addiu` in the object, so `x--` matches original `addiu ...,-1` bytes.
