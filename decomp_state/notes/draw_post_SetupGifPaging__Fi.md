# SetupGifPaging__Fi (0x1F4280, 0x114 = 69 words) — BLOCKED

File: `code/game/draw_post_post.cpp`. Blocked 2026-09-29. Retained as
`INCLUDE_ASM`; full boot-ELF parity preserved.

## Semantics
Reserves the 16-byte VU1 data-reference slot that `DoGifPaging` fills
(`gifPageMarkerA`), advances `vu1ChainHead` past it, resets `textureCursor` to
`textureCursorEnd`, clears the effect-texture array (`effectTexs`, `sd zero`
0x10-stride) and the gif load-slot counter `gifLoadCnt`. Unless `noHud` is set
it invalidates HUD GS RAM: texture slots at/above the texture-pool start
(`texs[i].gsram = 0` when `gsram >= textureMemoryBase>>8`) and all palette slots.
Deadlocked reference: `X:\rcb\code\stable\game\draw.cpp` `SetupGifPaging__Fi`.

## Verified layout (kept in `code/include/hud.h`)
- `hudHeap` (0x19A3E8): header+0x18 (volatile ptr), iconTable+0x1C,
  texs+0x24 (plain ptr), pals+0x28 (plain ptr).
- `HudHeader`: palCount[8]@+0x14 (palCount[4]=+0x24), texCount[8]@+0x34
  (texCount[4]=+0x44), bank_load[8]@+0x74.
- `HudFrameTex`/`HudFramePal`: 8 bytes, `u16 gsram` at +4.
- `gifLoadCnt = 0x15F458` (a 16-byte slot counter, slot = cnt*16, base 0x18D020;
  NOT 0x15F478). GPREL16 store in the first guard's delay slot -> `.extern`-
  seeded alias `gifLoadCntGp` in `config/linker_aliases.ld`.

## Match status
Head block (vu1ChainHead load/store, textureCursor, gifLoadCntGp GPREL store),
EffectTex memset loop, BOTH HUD loop BODIES, and the epilogue all match
byte-for-byte in the best C form. 9 residual word-diffs remain, all in the two
HUD-loop PREAMBLES.

## Root cause (branch-target confirmed)
- Orig loop1 guard (word 31, `blez v0,+0x13`) targets word 51 (the loop2
  `addiu a1,t0,%lo(hudHeap)`); orig loop2 guard (word 54, `blez v1,+0xc`)
  targets word 67 (fn end).
- Mine loop1 guard targets word 52 (the early `j=0`); loop2 guard -> word 67.
- Because the counter zero-init is hoisted EARLY, the loop1 guard skips word 51,
  so EGC emits an `addiu a1,t0,%lo(hudHeap)` in the loop1 guard DELAY SLOT as a
  delay-fill that is LIVE on the taken (skip) edge to set up loop2 (NOT dead).
- The single root cause is the counter-init position: EGC 2.95.2 schedules the
  independent `move <ctr>,zero` BEFORE the guard; the original placed it in the
  plain guard's DELAY SLOT. The a1=heap and the branch target are consequences.

## Coupling (cannot get both from a plain C form)
- counter-init BEFORE count-load in RTL  => plain `blez` + counter early.
- counter-init AFTER guard in RTL        => `blezl` (likely) + counter in
  guard-delay (but loop2 heap-base flips a1->a0).

## Variants tested (reloc-masked 69-word diff vs assets/boot_elf.elf)
1. for / while / do-while (init before if)      -> 15 diffs (counter early).
2. init declared+init INSIDE the if             -> 9 diffs BUT guard=blezl + loop2 base a1->a0.
3. hybrid (decl before if, assign inside)       -> 9 diffs (same as #2).
4. `register ... asm("$13")` pin                -> 22 (a1 is $5 not $13; wrong reg).
5. direct `hudHeap.texs[i]` subscript           -> 15 (same as #1).
6. count captured in local `int c=..; if(c>0)..`-> 15 (no change).
7. #6 + `asm volatile("": :"r"(c))` barrier     -> 25 (worse).
8. TU flags -fno-schedule-insns/2, -fno-delayed-branch -> counter still early.
9. **DEPENDENCY-ANCHORED asm** (last-resort rec):
   `int c=hudHeap.header->texCount[4]; int i; asm("daddu %0,$0,$0":"=r"(i):"r"(c));
   if (c>0) do{...}while(i<hudHeap.header->texCount[4]);`
   -> **9 diffs (best)**. Counter-init moves word27->word31 (right BEFORE the
   guard, not in the guard-delay); guard stays plain `blez`; loop BODIES still
   match. EGC emits the `daddu` as a `#APP/#NO_APP` block placed BEFORE the
   `blez`; the guard DELAY SLOT gets the regular `addiu a1,t0,%lo`. EGC will not
   place an opaque asm into the branch delay slot. Adding the counter pin
   `register int i asm("$6")` regressed to 17 (body allocation disrupted).

Residual 9 (word: mine vs orig): 31 i=0/blez, 32 blez/i=0, 33 a1heap/a3heap,
34 a3heap/texs, 35 texs/nop, 54 j=0/blez, 55 blez/j=0, 56 nop/pals, 57 pals/nop.

## Escalation
- expert (GPT-6 Astra) consulted 2026-09-29: confirmed the a1=heap is a live
  delay-fill (not dead); recommended the count-capture, empty-asm, scheduler-flag
  and counter-pin experiments — all tested, none closed the gap.
- last-resort-decompiler (GPT-5.6 Sol) invoked 2026-09-29: recommended the
  dependency-anchored nonvolatile `daddu` asm (Variant 1) + optional counter/heap
  pins (Variants 2/3). Variant 1 (no pins) = 9 diffs (best); pins regressed.
  Concluded no TU flag remains; a blocker is defensible.

## Blocker
EGC 2.95.2 (default scheduler, `-G8 -O2 -ffast-math -fno-exceptions -snas`)
places the independent loop-counter zero-init before the plain guard and will not
occupy the guard's delay slot with an opaque asm, whereas the original places the
counter in the delay slot. No C form, asm form, register pin, or tested TU flag
produces (plain blez + counter-in-delay-slot + original loop2 a1 base) while
preserving the already-matching loop bodies. A regular (non-asm) count-dependent
zero-init that EGC would lower to `move a2,zero` (delay-slot-eligible AND
anchored) does not exist in the forms probed; any count-dependent expression
lowers to a shift/sub/mul, not the original zero-move.
