# GetFrameTex__Fi (0x1FFA10, 0x220 B) — BLOCKED

## What the function does (semantics fully decoded, high confidence)
`u64 GetFrameTex(int fi)` — `fi` is a HUD frame-table index. Looks up
`fr = hudHeap.frames[fi]` (hPal,hTex), `pal = hudHeap.pals[fr.hPal]`
(8 B: ram u32 @+0, gsram u16 @+4), `tex = hudHeap.texs[fr.hTex]`
(8 B: ram u32 @+0, gsram u16 @+4, uLog u8 @+6, vLog u8 @+7). Reserves 16-byte
VU1 data-reference load slots in `gifLoadSlots` (base 0x18D040, slot n at
base+n*16, indexed by `gifLoadCnt`, 64 slots ending exactly at `effectTexs`
0x18D440) for the palette and/or texture when their `gsram==0`, advances
`textureCursor` past each (pal by 0x400, tex by 1<<(max(uLog,vLog)*2)),
increments `gifLoadCnt` if a slot was written, and returns a 64-bit GS TEX0
descriptor for the frame's texture:

```
tex0 = tex.gsram | (1<<(tbw+14)) | (fmt<<20) | (uLog<<26) | (vLog<<30)
     | ((u64)pal.gsram<<37) | (1<<34) | (1<<63)
tbw = max(0, uLog-6);  fmt = (tex.gsram < textureMemoryBase>>8) ? 0x1B : 0x13
```

Control flow (three sequential blocks, all tested against the ORIGINAL gsram
values; both cursor stores live in ordinary (non-likely) branch delay slots so
they execute unconditionally when their block runs, even if gifLoadCnt>=0x40):

- Block1 `if (pal.gsram==0 || tex.gsram==0)`: slot fields
  f00=pal.ram, f04=0, f06=0x3FF0, f08=pal.ram, f0E=0x3FF0, f0D=5, f0C=5
  (store order f00,f04,f06,f08,f0E,f0D,f0C; the +8 word is stored through a
  SECOND pointer register a0 while the rest use v1).
- Block2 `if (pal.gsram==0)`: `pal.gsram=tc>>8`; `textureCursor=tc+0x400`
  (unconditional); `if(gifLoadCnt<0x40){ slot f00=pal.ram,f04=0,f06=pal.gsram;
  wrote=1; }`.
- Block3 `if (tex.gsram==0)`: `m=max(uLog,vLog)` (lbu vLog; lbu uLog; sltu;
  beqz / move, NOT a ternary); `tex.gsram=tc>>8`; `textureCursor=tc+(1<<(m*2))`
  (unconditional); `if(gifLoadCnt<0x40){ slot f08=tex.ram,f0C=uLog,f0D=vLog,
  f0E=tex.gsram; wrote=1; }`.
- Epilogue `if(wrote) gifLoadCnt++;` then the staged TEX0 `or`-chain
  (`li 1; sllv; dsll 14` for the tbw bit; bit-34 and bit-63 constants
  materialized separately).

## Infrastructure change made this session (KEEP — verified clean)
GetFrameTex was split out of `game/hud_post_post2_post` into its own Splat
segment / TU `game/hud_post_post2_gftex` assembled by the SN assembler
(ps2eeas), with the trailing functions moved to `game/hud_post_post2_post2`
(still GNU). config/RC1.yaml boundaries at 0xffa10 (GetFrameTex start) and
0xffc30 (func_001FFC30 start); Makefile adds hud_post_post2_post2.o to the
GNU-assembler list; hud_post_post2_gftex.o uses the default SN flags. Also
named the real data: `gifLoadSlots = 0x18D040` (symbols.txt) and renamed
`HudFrameTex.pad_06[2]` to `{ u8 uLog; u8 vLog; }` (hud.h). Full-ELF parity
passes with the function retained as INCLUDE_ASM, and the only ELF diff vs
the original (while the C candidate was active) was confined to
0x1FFA20–0x1FFC2C, i.e. the split broke nothing else.

### Why the SN split (access-mode wall — SOLVED)
The original mixes in-window access modes for gifLoadCnt (0x15F458),
textureCursor (0x15EE74) and textureMemoryBase (0x15EE8C):
- self-based absolute (2-instr `lui r,%hi; lw r,%lo(r)`): every gifLoadCnt
  load, the gifLoadCnt store, the block-2 textureCursor load, the
  textureMemoryBase load;
- GPREL16 (1-instr `lw/sw r,off(gp)`): the block-3 textureCursor load (0x1FFB08
  `bnez` delay slot) and BOTH textureCursor stores (0x1FFAD4, 0x1FFB48 branch
  delay slots).

A GNU-assembler TU (gas) expands a plain named in-window `extern int` as
GPREL16 only; the self-based absolute form is reachable only via a
constant-address cast, which the project's hard policy forbids. Under SN,
plain unseeded in-window externs expand self-based absolute for ordinary
references and GPREL16 inside EGC's noreorder branch regions — exactly the
original pattern (the DoGifPaging mechanism). So the function MUST live in an
SN TU; it cannot match in the original GNU TU without forbidden addresses.

## The residual wall (EGC 2.95.2 register allocation) — BLOCKER
With the SN split (access modes correct) and the full best-form candidate
(integer-add frame lookup `fr=(HudFrame*)((int)heap->frames+fi*sizeof
(HudFrame))`, unconditional cursor updates, explicit `m=vLog; if(m<uLog)
m=uLog;` max, direct `gifLoadSlots[gifLoadCnt].fXX` subscripts, staged TEX0
accumulator, scoped `register HudHeap* heap asm("$3")` pin,
`register int wrote asm("$10")` pin), the candidate is 68/136 words off and
every residual word is the prologue register-allocation cascade:

| value | original | candidate |
|-------|----------|-----------|
| &hudHeap base | $3 (v1) | $3 (v1) [pinned, MATCHES] |
| frames (temp) | $2 (v0) | $5 (a1) |
| fr (result)   | $4 (a0) | $5 (a1) |
| pals          | $5 (a1) | $6 (a2) |
| texs          | $6 (a2) | $4 (a0) |
| wrote flag    | $10 (t2) | $10 (t2) [pinned, MATCHES] |

EGC keeps frames+fr both in a1 and texs in a0, one register "to the right" of
the original's v0/a0/a1/a2 layout, and the whole 136-instruction body re-colors
from there. This is the same "RA shifted from the first move, not fixed by
pinning" wall that blocked the sibling GIF-paging functions
(SetupGifPaging__Fi, DoGifPaging__Fv, GetEffectTex__Fii). The $3 heap and $10
wrote pins reproduce the first words and the wrote register but do not move the
struct-pointer allocation.

## last-resort-decompiler (GPT-5.6 Sol) — invoked 2026-10-09
Invoked with the full 128/136 GNU-TU dossier. Corrected the diagnosis: Wall 1
(access mode) is solvable with an SN-only TU split (plain unseeded in-window
externs), and prescribed the source forms (integer-add frame lookup,
unconditional cursor updates, explicit max, direct slot subscripts, staged
TEX0) plus a scoped $3 heap pin. All applied and mechanically diffed:
SN split + plain form = 92/136; + integer forms = 92; + $3 heap pin = 72; +
$10 wrote pin = 68/136. Confirmed the remaining wall is the EGC prologue RA
cascade and that, with the initial $3/$2/$5/$6 vs $5/$3/$6/$7 allocation
persisting after the prescribed source forms + pins, blocking is defensible.
No match is claimed.

## State
Retain INCLUDE_ASM in `game/hud_post_post2_gftex.cpp` (SN TU). Full-ELF parity
preserved. The gifLoadSlots symbol and uLog/vLog field names are retained as
durable, correct names for future work (referenced by this note even though no
matched C code uses them yet).
