# fontTextSubmit (func_001F0BD0) — matched 2026-09-25

Address 0x1F0BD0, 116 bytes (29 words). Font text request submitter.
Matched with the draw.cpp GNU-TU flags (`-G8 -O2 -ffast-math
-fno-exceptions`, `--assembler=gnu`); full boot ELF parity passes (cmp
clean, count 645).

## What it does

`fontTextSubmit(x, y, field_0x08, text)` records one font-text draw
request and appends the formatted text to the shared font data buffer:

1. `index = fontTextSlotIndex` (0x15F004, .lit, initial 0).
2. Slot `&fontTextSlots[index]` (16-byte slots at 0x18AB00) receives
   `{x, y, field_0x08, fontTextCursor}` (field_0x0C = `textStart`).
3. `fontTextSlotIndex = index + 1`.
4. `count = sprintf(fontTextCursor, fontTextFormat, text) + 1;`
5. `fontTextCursor += count;`

`fontTextCursor` (0x15F000, .lit, initial 0x18A300) is a `char*` into the
2KB font data buffer at 0x18A300 (auto-named `D_0018A300` in
data.data.s, zero-filled). `fontTextFormat` (0x15F008, .lit) holds the
word 0x7325, whose little-endian bytes `25 73 00 00` are the C string
`"%s"` — the original passes the address of that inline format word to
sprintf. The `+ 1` in step 4 accounts for sprintf's NUL terminator, so
the cursor always lands on the first byte past the whole block
(text + NUL).

## Callee: sprintf at 0x116248

0x116248 is the SDK `sprintf` (Lombyte's mdebug-recovered US name list
puts `sprintf = 0x00116248`; that list is confirmed in our address space
by independent anchors — `memcpy = 0x115248` is verifiably memcpy code,
`FlushCache = 0x118A80` and `scePad2Read = 0x124BD8` match our
symbols.txt). The function entry is 0x116248; the `pref` at 0x116244
belongs to the gap after func_001161E8's epilogue (Splat now emits it as
its own 4-byte nonmatching range). The SDK implementation is
context-based: it builds a 0xE0-byte stack struct (fields: str, 0x7FFFFF,
0x7FFFFF, u16 0x208, *D_0012F76C at +0x54), calls the engine
func_00116DA8 (get-or-init the global context object, then func_00116E20),
and finishes with `sb zero, 0(str)`. The return value is the number of
characters written. The body is generated SDK code (code/_generated/
glibc.s) and is not a decompilation target.

## Caller

Sole boot-ELF caller is the centering wrapper func_001F0C48
(alabel func_001F0C50, still INCLUDE_ASM): it walks `text`, sums per-char
widths from the width table at 0x189DC0 (`lw` of `widthTable[ch - 0x20]`
for chars in `0x20..0x5F`, else default 0x20), then calls
`fontTextSubmit(x - total/2, y, a2, text)` where a2 is passed through
untouched (no boot caller of the wrapper exists, so a2's meaning is
unknown — that is field_0x08). The wrapper has no callers in the boot
ELF; the whole cluster is used from level overlays. The 8 bytes at
0x1F0C48–0x1F0C4C (`addiu sp,sp,0x30; nop`) are a dead tail inside the
wrapper's Splat range and stay in its INCLUDE_ASM.

## Naming / symbols

New config/symbols.txt entries (draw region):

- `sprintf = 0x00116248;`
- `fontTextCursor = 0x0015F000;` (char*)
- `fontTextSlotIndex = 0x0015F004;` (int)
- `fontTextFormat = 0x0015F008;` (const char[] = "%s")
- `fontTextSlots = 0x0018AB00;` (FontTextSlot[])

The `fontTextSlots` dlabel in data.data.s spans 0x18AB00–0x18C318: the
array sits in a run of zero data with no visible bound (it continues
zero to 0x18C300, where a 6-pointer table to D_0015F30x sits, then
`occlCamState` at 0x18C318). The declared `FONT_TEXT_SLOT_COUNT` (20) is
inferred (matches the `drawTextureDmaState` count at 0x18A2B0 just
before); it only appears in the C declaration and affects no codegen.

`fontTextSubmit` uses an `asm("func_001F0BD0")` symbol override: the
generated caller (func_001F0C48.s) references the Splat placeholder name.

## Codegen notes (draw.cpp is a GNU-assembler TU)

- Statement order matters: the cursor store
  (`fontTextSlots[index].textStart = fontTextCursor;`) must precede the
  index store (`fontTextSlotIndex = index + 1;`). With the index store
  first, EGC schedules `sw index` at 0x1F0C0C and `sw cursor` at 0x1F0C18
  (8-word diff); the original is the reverse.
- The tail needs the explicit local
  `int count = sprintf(...) + 1; fontTextCursor += count;`. Writing
  `fontTextCursor = fontTextCursor + (sprintf(...) + 1);` makes EGC
  re-associate to `(cursor + 1) + ret` (`addiu v1,v1,1` instead of the
  original's `addiu v0,v0,1` before the `lq ra`).
- Direct subscripts (`fontTextSlots[index].x = x;` etc.) reproduce the
  original's re-materialized address pair (`addu t0,v1,v0` then
  `addu t1,v0,v1`, operands reversed, then `move a0,t1` / `move v0,a0`).
  A local `FontTextSlot* slot = &fontTextSlots[index];` with `slot->x = x;`
  lets EGC reuse one address register (104 bytes, 10-word diff).
- `extern const char fontTextFormat[];` (unspecified size, so not
  small-data) makes `&fontTextFormat` materialize as the original's
  absolute `lui a1,0x16; addiu a1,a1,-0x1008` even though 0x15F008 is
  inside the GP window.
- Plain `extern char* fontTextCursor;` and `extern int fontTextSlotIndex;`
  (no section attribute) compile to the bare pseudo form that GNU as
  expands to GPREL16 (`lw t3,-0x7C00(gp)`, `lw/sw t2,-0x7BFC(gp)`),
  matching the original.
- The variadic `extern "C" int sprintf(char*, const char*, ...);`
  declaration compiles identically to a fixed 3-arg prototype (all three
  args register-passed; no caller-save difference) — verified by probe.
- The `asm("...")` override must sit on a separate declaration before the
  definition (cfront parse error when attached to the definition).

## Uncertainties

- `field_0x08`: passed through from the (overlay-only) caller; no boot
  evidence for its meaning. Named by offset.
- Slot count 20: inferred, see above.
- The SDK sprintf's `sb zero, 0(str)` (NUL at the block head) and the
  context engine (func_00116DA8/0x116E20) are not fully traced; the
  return-value = character-count reading follows from the caller's
  `cursor += ret + 1`.

## Probe history

- probe1: `slot` pointer form — 104 bytes, address CSE'd (wrong).
- probe2: direct subscripts, single-statement tail — 116 bytes; residual
  t1/t2 swap + tail re-association (8 words).
- probeA: `count` local — tail fixed; t1/t2 swap remained.
- probeB: cursor store moved before index store — **match**.
- probeC: sprintf/char* naming — **match** (codegen identical to B).
