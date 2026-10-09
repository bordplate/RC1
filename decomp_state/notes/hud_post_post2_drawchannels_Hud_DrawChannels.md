# Hud_DrawChannels (func_001FF780) — prep done, match pending

Status: PREP COMPLETE / MATCH PENDING. The function is isolated in its own
SN TU (Splat split) and its data is named, but the C body has NOT been matched
yet (still `INCLUDE_ASM` in code/game/hud_post_post2_drawchannels.cpp). This is
a hard target with several distinct codegen walls. Do not claim it matched.

## What is done (verified, parity-neutral)

1. **Splat boundary split** isolating func_001FF780 into its own SN TU:
   - config/RC1.yaml: boundaries at file 0xff780 and 0xff958 (the function
     region is 0x1ff780-0x1ff958, 0x1d8 bytes incl. the trailing nop).
   - New objects: `game/hud_post_post2_drawchannels.cpp` (SN, the target) and
     `game/hud_post_post2_post.cpp` (GNU, the rest: ghost func_001FF958,
     GetIconFrame, func_001FFC30, func_00201200, draw_bootImage, ...).
   - Makefile: `hud_post_post2_post.o` added to the GNU override list; the
     drawchannels TU uses default SN. The split is parity-neutral (all
     INCLUDE_ASM still active → full `cmp` MATCH).
   - The SN TU is REQUIRED: the function mixes GPREL and absolute accesses to
     the same in-window globals (the snd_BankLoadByLoc pattern), which a GNU TU
     cannot reproduce.
2. **Named data** (config/symbols.txt): hudMsgCount=0x15F680, hudMsgFade=
   0x15F684, hudMsgY=0x15F688, hudMsgText1=0x1993D8, hudMsgText2=0x199428.
3. **GPREL aliases** (config/linker_aliases.ld): GameModeGp=0x15F604,
   hudMsgCountGp=0x15F680, hudMsgFadeGp=0x15F684 (seeded `.extern sym,4` for
   the GPREL sites; the plain names expand self-based absolute for the abs
   sites).
4. **HudHeap extended** (code/include/hud.h): exposed +0x0C (vuField_0C, =sym
   `Hud`, written 0xFFFFFFF0) and added +0x30 (field_30 skip flag).

## Semantics (Ghidra + objdump)

Per-frame HUD draw-stage dispatcher (no args, void). Gated by
`drawEnableMask&0x80 && *(s0+0x40)` from DrawDebugProfiler (0x1F3E90).
1. `if (hudHeap.field_30) { hudHeap.field_30 = 0; return; }`
2. `if (occlChainActive) { hudHeap.field_30 = 0; return; }`
3. `hudHeap.vuField_0C = 0xFFFFFFF0;`
4. Loop 13 hudChanSlots (0x90 stride): `if (slot->e) slot->e(slot);` (fn ptr
   at +0x18, a0 = slot).
5. Fade block: `if ((count==0 && fade==0) || GameMode) { hudMsgY = 100;
   return; }` else ramp `fade` by `0x80/func_001F96F8(8)` per frame
   (count!=0: fade+=step, clamp hi 0x80; count==0: fade-=step, clamp lo 0),
   `color = (fade<<24)|0xF0F0F0`, `func_00201200(0x100, hudMsgY, color,
   hudMsgText1)` and (if `hudMsgText2[0] && count<=1000`) a second call.
6. Tail: `if (count) count--; if (count==1000) count = 0;`

Callees: func_001F96F8 (extern "C" int(int), = 1.0f + frames*factor, =9 boot,
bmain.cpp:63); func_00201200 (void(int x,int y,u32 color,u8* text), the
HUD-message printer, INCLUDE_ASM in hud_post_post2_post.cpp).

## Codegen walls (why it is hard)

- **(a) Mixed GPREL+absolute** to in-window globals (count: 5 abs + 2 GPREL;
  fade: 5 abs + 3 GPREL; GameMode GPREL-only; value/occlChainActive abs-only).
  Handled by the plain-name + Gp-alias split above, but the C must use the
  right name at EACH access site (see the access map in the research report).
- **(b) Branch-likely `bnel` vs `bne`**: the first skip branch (0x1FF79C) is
  `bnel` in the original with the `field_30 = 0` store in the likely-delay
  slot (dead on the taken path); the C `if (x) { x=0; return; }` emits `bne`.
  This is the first diff and cascades. Needs a C form / scheduler form that
  makes EGC pick the likely variant.
- **(c) Duplicated `i--`** in the slot loop (beql delay slot + post-jalr).
- **(d) `break 0,7`** in a beql delay slot (0x1FF830, after func_001F96F8);
  `asm("break 0,7")` as a statement-expression does NOT parse in this EGC —
  find the working form (or confirm EGC emits it from the C shape).
- **(e) Size-clamp double-stores** (increase: both GPREL; decrease: GPREL
  value + abs clamp-0 via $at).
- **(f) Dead `==1000` tail** (0x1FF900-954): the `count=0` store sits in a
  beql delay slot that always runs; both branch targets are byte-identical
  epilogue, so the store is effectively unconditional and the ==1000 test is
  semantically dead. The natural C does not capture this.

## Current candidate

working/func_001FF780/candidate.cpp — compiles, 448 B vs 468 B original, 76
word diffs (first diff at 0x1FF79C, the bnel/bne). Probed with:
`python3 tools/decomp_probe.py working/func_001FF780/candidate.cpp
code/_generated/nonmatchings/game/hud_post_post2_drawchannels/func_001FF780.s
func_001FF780 --define GameModeGp=0x15F604 --define hudMsgCountGp=0x15F680
--define hudMsgFadeGp=0x15F684 --define func_001F96F8=0x1F96F8
--define func_00201200=0x201200 --out working/func_001FF780/probeN`

## Next steps (for a fresh session)

1. Cracking wall (b) first: find the C form that makes EGC emit `bnel` for
   `if (x != 0) { x = 0; return; }` (try `if (x) return; x = 0;`, a volatile,
   or the -fno-schedule-insns flag) — this is the first diff.
2. Then (c) the loop double-dec, (d) the break form, (e) the clamp stores,
   (f) the dead tail.
3. The full research (data layout, access map, callee signatures) is in
   working/func_001FF780/RESEARCH.md.
