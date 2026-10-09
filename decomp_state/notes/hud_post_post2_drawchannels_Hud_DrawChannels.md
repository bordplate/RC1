# Hud_DrawChannels (func_001FF780) — MATCHED

Status: MATCHED (2026-10-09). The C body in code/game/hud_post_post2_drawchannels.cpp
replaces the INCLUDE_ASM; the full 0x1D8 region (0x1FF780-0x1FF958, code + trailing
padding) is byte-identical to assets/boot_elf.elf, and `cmp build/boot_elf.elf
assets/boot_elf.elf` is an EXACT MATCH.

## Semantics (verified by the byte match)

Per-frame HUD draw-stage dispatcher (no args, void). Gated from
DrawDebugProfiler (0x1F3E90).
1. `if (hudHeap.field_30) { hudHeap.field_30 = 0; return; }`
2. `if (occlChainActive) { hudHeap.field_30 = 0; return; }`
3. `hudHeap.vuField_0C = HUD_VU_FIELD_INIT;` (0x00FFFFF0)
4. Loop over the 13 hudChanSlots (0x90 stride): `if (slot->e) slot->e(slot);`
   (callback fn ptr at +0x18, a0 = slot).
5. Fade block: `if ((hudMsgCount==0 && hudMsgFade==0) || GameModeGp) {
   hudMsgY = HUD_MSG_IDLE_Y; return; }` else ramp `hudMsgFade` by
   `HUD_MSG_FADE_MAX / func_001F96F8(8)` per frame (count!=0: fade+=step,
   clamp hi to HUD_MSG_FADE_MAX; count==0: fade-=step, clamp lo to 0).
   Then `color = (hudMsgFade<<24) + HUD_MSG_COLOR_RGB`,
   `func_00201200(HUD_MSG_DRAW_X, hudMsgY, color, hudMsgText1)`, and a second
   call with hudMsgText2 `if (hudMsgText2[0] && hudMsgCount > HUD_MSG_COUNT_RESET)`.
6. Tail: `if (hudMsgCount) hudMsgCount--; if (hudMsgCount==HUD_MSG_COUNT_RESET)
   hudMsgCount = 0;` (the reset store is hudMsgCountGp, the GPREL alias).

Callees: func_001F96F8 (extern "C" int(int), frames->timer scale); func_00201200
(void(int x,int y,u32 color,u8* text), the HUD message printer, INCLUDE_ASM in
hud_post_post2_post.cpp).

## Key codegen findings (what made it match)

The prior "codegen walls" in the old note were mostly misdiagnoses. The actual
fixes, all verified by the byte match:

1. **Correct constant `0x00FFFFF0`** (not 0xFFFFFFF0): `lui v0,0x00ff; ori v0,v0,
   0xfff0`. Getting the top byte right (0x00) ALSO resolved the prologue
   delay-slot swap (EGC's scheduling of the two independent lui's in the
   occlChainActive beqz delay slot is sensitive to the constant). Named
   HUD_VU_FIELD_INIT.
2. **Color is ADDITION not OR**: `color = (fade<<24) + 0xF0F0F0` (addu), not
   `|` (or). Named HUD_MSG_COLOR_RGB.
3. **Second-message condition is `count > 1000`** (not `<= 1000`): the original
   branches `bnez (count<1001) -> skip`, i.e. the second line is shown while the
   countdown is still above the reset point. Named HUD_MSG_COUNT_RESET.
4. **Plain `hudMsgY` (not a .data alias) for the force100 store AND the second
   message y-arg.** The force100 block and the msg2 call setup sit OUTSIDE
   noreorder, so the -G8 small-data bare pseudo expands self-based ABS (via $at),
   matching the original. A `.data` alias forces an explicit split (`lui $v1; sw
   $v0,off($v1)`) whose separate hi load EGC hoists into the GameMode `bnel`
   delay slot, flipping the branch to `bnel`+hoisted-yhi and using $v1 instead of
   $at. The plain name gives the original's `li 100` in the delay slot + `lui $at`
   in the block. (The GPREL sites still need the seeded `.extern` aliases:
   GameModeGp / hudMsgCountGp / hudMsgFadeGp.)

Data / symbols (all pre-existing): hudMsgCount=0x15F680, hudMsgFade=0x15F684,
hudMsgY=0x15F688, hudMsgText1=0x1993D8, hudMsgText2=0x199428 (config/symbols.txt);
GameModeGp=0x15F604, hudMsgCountGp=0x15F680, hudMsgFadeGp=0x15F684
(config/linker_aliases.ld, seeded `.extern sym,4` in the source).

Constants named in code/include/hud.h (HUD_MSG_* / HUD_VU_FIELD_INIT); the
#defines are preprocessor substitutions and are parity-neutral.

## TU

Isolated SN TU (Splat split at file 0xff780 / 0xff958); the SN assembler is
required because the function mixes GPREL and absolute accesses to the same
in-window globals. See config/RC1.yaml, Makefile (default SN for this object).
