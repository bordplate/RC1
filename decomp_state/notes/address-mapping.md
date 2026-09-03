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
  `env WINEPREFIX=tools/wineprefix WINEDEBUG=-all tools/wine/bin/wine
  tools/cc/bin/ee-gcc.exe -S -x c++ -G8 -O2 -ffast-math -fno-exceptions
  -Wa,-EL -Icode/include -Btools/cc/lib/gcc-lib/ee/2.95.2/` and compare the
  emitted `.s`. The `-B.../2.95.2/` path is required or cpp cannot be found.
- Makefile line 64 runs objcopy with identical in/out path
  (`build/boot_elf.elf`); verified harmless - the built file stays a full ELF
  and cmp passes byte-for-byte.
- Most targets live in `.cpp` files (830) vs `.c` (63).
