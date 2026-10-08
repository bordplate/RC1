# func_001FE980 (code/game/help.cpp) — BLOCKED, scheduler/allocator-permutation wall

Date: 2026-10-08. VRAM 0x1FE980, size 0x344 (836 bytes). last-resort GPT-5.6 Sol invoked.

## What it does
In-level Help-message box renderer. `switch(g_helpState.state)` over states 1..7
dispatching through the jump table `jtbl_001E7A70` (7 entries:
0x1FE9F4, 0x1FEA38, 0x1FEA54, 0x1FEB3C x3, 0x1FEC28, 0x0). Per state:

- state 1 (0x1FE9F4): growing box. `h = counter*4+8`; store halfW_cur=halfH_cur=h;
  `DrawUIFrame(cy-h, cy+h, cx-h, cx+h, 0x60)`.
- state 2 (0x1FEA38): `Help_DrawPrompt()`.
- state 3 (0x1FEA54): clamp+box+icon. `w=((halfW_src-32)*counter)/8+32`,
  `h=((halfH_src-32)*counter)/8+32` (round-toward-zero via the slt/addiu/movn/sra idiom);
  store halfW_cur=w, halfH_cur=h; `DrawUIFrame(...,0x60)`; `alpha=max(0,8-counter)*16`
  (the `*16`+`<<24` split interleaves the call), `color=(alpha<<28)|0x808080`,
  `tex=GetEffectTex(4)`, `DrawTexturedQuad(cx-32,cy-32,0x40,0x40,0,0,0x40,0x40,color,tex)`.
- states 4/5/6 (0x1FEB3C): full box+text. store halfW_cur=halfW_src, halfH_cur=halfH_src;
  `DrawUIFrame(...,0x60)`; RELOAD state; `color=0x80FFA888`; if state==4
  `color=0xFFA888|(counter<<29)`; else if state==6 `color=0xFFA888|((4-counter)<<29)`;
  `FontSetWindow(&window,0xF0,0x1E0,0x2C,0x1D4,0x100,cy,0x10,3)`;
  `FontPrintWindowMedium(&window,color,text,-1)`.
- state 7 (0x1FEC28): shrinking box. `hh=((curHalfH-8)*counter)/8`,
  `hw=((curHalfW-8)*counter)/8`, `alpha=(8-counter)*12`, `h=curHalfH-hh`,
  `w=curHalfW-hw`, `DrawUIFrame(cy-h,cy+h,cx-w,cx+w,alpha)`.

Gate (prologue): `state!=0 && ready_gate(field_0x30)!=0 && (flag1!=0 || flag0!=0)`;
every case additionally gated by `flag1` (`andi $2,$5,0xFF; beqz`). flag1 lives in a1.

## Verified data layout
- g_helpState @ 0x1996D0 (.data): +0x00 state(u32), +0x04 counter, +0x08 halfW_src,
  +0x0C halfH_src, +0x10 cx, +0x14 cy, +0x18 halfW_cur, +0x1C halfH_cur, +0x20 msgIndex,
  +0x2C msgCount, +0x30 ready_gate.
- HelpMsgs @ 0x15F6A0: array of { char* text; int id; int f8; int fC; }.
- sceneHelpMsgFlag0 = 0x15EE1C (byte), sceneHelpMsgFlag1 = 0x15EE1D (byte). BOTH in the
  gp window. **Mixed address mode:** flag1 is read GPREL here (`lbu $2,-0x7DE3($28)`)
  but ABSOLUTE in sibling Help_DisplayMessage (0x1FDD58) — needs a same-address
  `.extern`-seeded alias (sceneHelpMsgFlag1Gp) in config/linker_aliases.ld.
- Callee addrs: DrawUIFrame=0x1F5F18 (5 int), Help_DrawPrompt=0x1FE898 (void),
  GetEffectTex__Fii=0x1F44B8 (u64 return), DrawTexturedQuad=0x1F5450 (8 int + 2 u64 on
  caller stack 0(sp)/8(sp)), FontSetWindow=0x1F7668 (9 int), FontPrintWindowMedium=0x1F75F0.

## ABI (verified from matched sibling Help_DrawPrompt__Fv, same TU)
EGC 2.95.2 passes int args 1-6 in a0-a5 ($4-$9), args 7-8 in $10/$11, args 9+ on the
stack. u64 args (DrawTexturedQuad color/tex) on the caller stack. EE `mult rd,rs,rt` is
3-operand. The matched sibling uses the identical GetEffectTex + u64-color +
DrawTexturedQuad pattern, so the u64 ABI is NOT the wall.

## last-resort GPT-5.6 Sol (invoked 2026-10-08) — its 7 corrections
1. Drop the explicit `if (state>7) return;` (the switch's `state-1<7` bounds check is the
   only one the original has).
2. Never use the hoisted `state` after dispatch — reload `g_helpState.state` in the
   4/5/6 body (original does this at 0x1FEB80). Frees s1 for the cached high page.
3. In the clamp case store w/h BEFORE DrawUIFrame.
4. Reload counter/cx/cy after calls, not as hoisted locals across them.
5. Use a 32-bit (u32) color local, not `unsigned long` (original builds the animated
   alpha with 32-bit `sll`, candidate's `dsll` proves a wrong 64-bit mode).
6. Restore the MISSING state-6 color path `0xFFA888|((4-counter)<<29)` (real decode bug —
   states 4/5/6 share one body and 4 AND 6 both special-case the color).
7. Remove `(short)cy` from FontSetWindow (callee takes int; the cast added sll/sra).

Result: candidate went 207 diffs / 0xB0 frame / 8 callee-saved regs → **164 diffs / 0x70
frame / s0-s2+ra**. The FRAME and STRUCTURE now MATCH the original: s1 holds
%hi(g_helpState)<<16 re-derived per case, counter/cx/cy reloaded per case, 32-bit sll
color, the slt/addiu/movn/sra round-toward-zero clamp, and the per-case andi gate are all
reproduced in the .s.

## Why it stays blocked (residual 164 diff words, best of 6 variants = cand4)
Pure scheduler / register-assignment tie-breaks EGC 2.95.2 will not surrender:
- (a) Prologue gate (11w): the original loads flag1 into **v0** (`lbu $2`) and sets
  a1=v0 in the bnez delay slot; EGC loads flag1 straight into **a1** (`lbu $5`). A v0
  pin (cand6) and a gate restructure (cand5) both make it WORSE (199).
- (b) Per-case `andi $2,$5,0xFF` (EGC emits a bare `beq a1,$0`).
- (c) counter/cx/cy/halfW/halfH register choices and store ordering across cases 1-6.
- (d) Resulting branch-target cascade (candidate 832B vs original 836B).
- Flags `-fno-schedule-insns`, `-fno-schedule-insns2`, `-mno-split-addresses` all
  INCREASE the diff (173/173/195). Default scheduler is the best.

Wall class = scheduler/allocator-permutation, same family as the blocked effects drawers
(drawEffectRibbon 0x1EDC50, 0x1EE338/0x1EE328, 0x1EE008, 0x1EE4B0): semantics + multiset
+ frame all correct, per-instruction RA/scheduling uncontrollable by source, pins, or
flags. The last-resort's own blocker threshold was a frame mismatch (s0-s2+ra); the frame
now matches, so this is the harder residual-body form of the same wall.

## If revisited (secondary, independent of the wall)
- Jump table: needs a Splat TU split — move func_001FE980 + func_001FECC8 +
  func_001FED30 + func_001FEE30 to a new `game/help_msg` TU (text split at rom 0xFFF900
  = VRAM 0x1FE980), and a jtbl "hole" at VRAM 0x1E7A70 (rom 0xE99F0, 0x20) in
  tools/patch_rodata_ld.py HOLES (split data_suffix2 vram 0x1e7a40 into 0x30 / 0x20 / rest).
  Mechanical + parity-preserving with INCLUDE_ASM intact.
- Mixed address mode: add `sceneHelpMsgFlag1Gp = 0x0015EE1D;` to config/linker_aliases.ld
  and seed it in-function with `asm volatile(".extern sceneHelpMsgFlag1Gp, 1");`
  (precedent: 989snd snd_BankLoadByLoc).
- Best candidate body was working/help_func_001FE980/cand4.cpp (since cleared); its key
  shape is the per-case reload + u32 color + state-6 path above.
