# FontQueueVUState (0x1F76A0) — matched 2026-10-03

`extern "C" void FontQueueVUState(void)` in `code/game/draw_post_post.cpp`.
122 instructions, 0x1E8 bytes, frame 0x80. Matched with default `-G8 -O2
-ffast-math -fno-exceptions -snas` (no private TU flag). Full boot ELF parity
passes.

## What it does
Builds the per-frame font VU1 state:
1. `draw_loadViewMatrixW(viewRows, 1024.0f)` + `FastVecScale(viewRows+12,
   &currentCamera.pos, -1024.0f)` + `viewRows[15]=1.0f` — the VU-side view rows.
2. On first use (`fontState != 7`): `VU1_addDataRef(fontVUProgram,
   fontVUProgramTag)` streams the font VU program, then `fontState = 7`.
3. Appends a 0xF0-byte record to the `vu1ChainHead` linked chain: the two view
   matrices (each followed by `m[14] += fontDepthBias`), the clip quad and hvdf
   quad (128-bit copies from `viewCtx`), the fog intensities, the constant
   words, and a queue-length tag OR-ed into the head.
4. `VU1_gsRegsFont()` and stores the new chain tail (`vu1ChainHeadStore`).

The moving base `s1` is recomputed per region: m1 (+0x20), m2 (+0x60), clip dst
(+0xB0), hvdf dst (+0xC0), tail (+0xF0). `s0` caches the record base.

## Key codegen findings (EGC 2.95.2, default flags)
- **fontVUProgramTag two-register split**: the u16 tag must be declared
  `extern u16 fontVUProgramTag __attribute__((section(".data")));`. A plain
  scalar extern emits a bare pseudo `lhu $5,fontVUProgramTag` whose end-of-file
  `.extern` makes ps2eeas expand it self-based (`lui a1; lhu a1,0(a1)`). The
  `.data` section attribute forces a genuine RTL split (`lui $2; lui $4;
  lhu $5,%lo($2)`) — base in v0, value in a1 — matching the original's call
  setup where the sibling `fontVUProgram` pointer loads self-based into a0.
- **128-bit quad materialization**: `*(CameraQuad*)p = *clip/hv` folds to
  `sq imm(s0)` unless the destination pointer is tied with
  `asm volatile("" : "+r"(p))` and the source pointer is pinned
  (`clip`→a1, `hv`→v1). The tied barrier defeats the DImode offset folding.
- **hvdf temp register overlap**: the original loads the hvdf quad as
  `lq v0,0(v1)` — the 128-bit temp (v0:v1) OVERLAPS the source pointer (v1).
  EGC avoids the overlap and otherwise uses a2:a3. Pinning the temp to the
  v0:v1 pair (`register CameraQuad hvq asm("$2"); hvq = *hv;`) reproduces the
  original. The clip quad (src a1) does not overlap, so its temp naturally
  lands in v0:v1.
- **Phase boundaries**: a tied register+memory clobber
  (`asm volatile("" : : : "$2","$3","$5","memory")`) after the hvdf store
  stops the tail constant loads (W0xE0/E4/E8) from hoisting into the quad
  region, and a plain memory barrier after the last zero store keeps the head
  load from interleaving with the record stores.
- **Tail constants + RMW**: W0xE0/E4/E8 pin to v1/v0/a1. The queue-length RMW
  (`head[0] |= (((tail-head)>>4)-1)`) needs `diff` as a SIGNED int (→ `sra`,
  not `srl`), pinned to v0, with `word` pinned to v1 and tied barriers to
  reproduce the exact `subu; lw; sra; addiu; or; sw` order (the word load sits
  right after the subu).
- **Constant-store permutations**: the three zero stores and the
  [0xE4]/[0xE8]/[0xE0]/fog stores needed specific source orders to land in the
  original's sequence (see the matched source); EGC emits a fixed permutation
  for independent constant stores.

## Verification
`make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes; the 122-word
function range 0x1F76A0..0x1F7888 compares byte-for-byte.
