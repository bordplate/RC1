# SetupFS_AA_buffer__Fiiiiii (0x1FA978) — BLOCKED (EGC prologue-save order + 6-block RA/constant-interleave wall)

`code/game/framebuf.cpp`, C++ mangled, 0x904 (2308) bytes / 577 instructions.
Anti-aliasing framebuffer + GS state setup; called with
(drawW, drawH, dispW, dispH, dispOfsX, dispOfsY).

## What it does (confirmed from ground-truth objdump)

1. 0xA0 frame; saves s0-s8+ra. Loads `displayBase`/`zbufBase`/
   `frameBufferBase` (0x15EE80/84/88), `>>13`. `s0 = occlCamParamBase`
   (0x151780). Stores 14 s16 fields into the struct (fsAABuff layout:
   disp/draw envs + two 128-bit GIF tags + 16-qword drawLarge/drawSmall
   arrays at +0x00..0x14F, dimension/PSM/fbp s16 fields at +0x150..0x170).
   `aaBuffPtr` (0x15EEB8) = &occlCamParamBase.
2. C-call `func_00121AC8` (SetDefDispEnv): (p, 0, sx, sy, ofx, ofy).
   Lombyte recovered declaration: `extern s32 sceGsSetDefDispEnv();`.
3. `env+0x10` (disp.dispfb) 64-bit RMW: `(old & ~0x1FF) | (dispFBP & 0x1FF)`.
4. `func_00121FC8` (SetDefDrawEnv): (env+0x40, drawPSM, drawW, drawH, 3, zPSM).
5. `env+0x40` (drawLarge.frame1) RMW; `env+0x50` (drawLarge.zbuf1) =
   `drawFBP | (zPSM & 0xF) << 24`.
6. `giftagDrawLarge` (env+0x30/0x38): `por v0,zero,zero; sq v0,48(aaBuffPtr)`
   128-bit zero, then two 64-bit RMWs:
   `field_8 = (old & ~0xF) | 14`;
   `field_0 = ((((old & ~0x7FFF) | 8) | 0x8000) & ~0xF) | (0x8000 << 45)`.
7. `func_00121FC8`: (env+0xD0, auxPSM, dispW, dispH, 0, 0).
8. `env+0xE0` (drawSmall.zbuf1) = `0x8000 << 17`; `env+0xD0` RMW.
9. `giftagDrawSmall`: same idiom as 6.
10. Six GS data blocks (u64 arrays), four with 16-iteration loops:
    - `aaDisplayRegs` 0x151B60: 12-qword header + 16*4 loop
      (i*drawW, (i+1)*drawW, dispW/dispH/drawW/drawH multiplies)
    - `aaBlurRegs` 0x151DF0: 10-qword header + 16*4 loop (running accums)
    - `aaClearRegs` 0x151900: 12-qword header + 16*4 loop (running accums)
    - `aaClearBlackRegs` 0x152040: 11-qword header + 16*2 loop
    - `gsStateFadeColor` 0x13CD10: 16*2 loop (drawH-based)
    - `gsStateFade` 0x13CE10: 16*2 loop (dispH-based)

## Data symbols named (config/symbols.txt, kept after the block)

aaBuffPtr=0x15EEB8, aaDisplayRegs=0x151B60, aaBlurRegs=0x151DF0,
aaClearRegs=0x151900, aaClearBlackRegs=0x152040. The generated
nonmatching assembly now references these names (core.lit/core.data
dlabels); pure relabel, no codegen effect.

## Best candidate reached (2026-10-03/04, two sessions)

Full C implementation with: `fsAABuff` struct (128-bit `GifTag128`
mode-TI GIF tags), `volatile s32 aaBuffPtr` re-read per statement group
`(fsAABuff*)(u32)aaBuffPtr`, register pins s0-s8/a0-a5/t0-t7 for the
args/shifts, `asm volatile` tied barriers, inline asm
`li $30,-1; dsrl $30,$30,4` for the 64-bit `~0xF` mask, and
`$(OBJ_DIR)/game/framebuf.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns`.
Result: 2168 bytes (542 instr) vs original 2308 (577); 514/542 word
diffs in the overlapping range. Reverted to INCLUDE_ASM (parity
restored, `cmp` passes).

## The residual wall (concrete)

1. PROLOGUE SAVE ORDER (compiler-generated, not source stores). Original:
   `addiu sp,sp,-160`, three base loads + `li v0,49` + s0 setup, then
   `sq s8,128(sp)` FIRST, interleaved `sra`/`move`/`sll`, `s7,s6,s5,s4,s3,
   s2,s1` descending, and `sq ra,144(sp)` LAST at +0x6C, immediately after
   `sll a2,t4,0x10`. EGC 2.95.2 emits the identical frame, identical save
   set at identical slots, but pins `sq ra,144(sp)` at +0x2C (immediately
   after `sq s0,0(sp)`) and shifts s8..s1 down by one slot — in every
   source form and flag combination tried (pins, volatility, statement
   order, empty barriers, -fno-schedule-insns, -fno-schedule-insns2, -G0,
   -mno-split-addresses, default). The same region's `sh` field stores are
   permuted (orig zFBP-then-drawW vs mine drawW-then-zFBP) and the
   `aaBuffPtr` store uses `sw s0` (orig) vs `sw a0` (mine, because EGC
   performs `move a0,s0` earlier). Note the save order is source-dependent
   in principle — matched FontPrintWindowSmall saves ra last with
   descending s-registers — but no source form found reproduces THIS
   function's full allocation (s0-s8 all live, six later blocks).
2. Per-statement-group base-register choice for aaBuffPtr (v1/a1/a0/v0
   varies by group in the original) and its reload cadence: reproducible
   per group, not across all six blocks simultaneously.
3. 64-bit constant chains in t5/t7 interleaved with the loop-setup
   stream: e.g. aaDisplayRegs[0] = (((0x8116<<16)|0x8000)<<31)|1 is
   `li t7,0x8116; dsll t7,0x10; ori t7,0x8000; dsll t7,0x1f; ori t7,1`
   (64-bit `dsll`, not dsll32) interleaved with `li t1,71; li t2,5;
   li t3,0x8000; dsll t3,0x11; ori t3,0x261; li t0,20`. Each chain is
   reproducible in isolation (probe working/.../probe/ti.c) but embedding
   it shifts the surrounding allocation and breaks the match.
4. The `por v0,zero,zero; sq v0,48(aaBuffPtr)` 128-bit giftag-zero idiom
   interleaved with the two-RMW fixup and the next call's arg setup.

Individually reproducible idioms do not compose: changing the hard-register
lifetimes needed for the constant chains/giftag idiom perturbs the
prologue save schedule and all six loop allocations.

## Last-resort escalation (required before blocking)

`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-10-04 with the full
dossier (working/.../dossier.md: ground-truth objdump, current candidate,
residual categories, all attempted forms). One concrete recommendation
returned: change the `func_00121AC8` callee prototype from `void` to
`int` (Lombyte: `extern s32 sceGsSetDefDispEnv();`; project-verified
callee-return-type effect, 2026-09-06 snd_SendCurrentBatch), keep the
current source and `-fno-schedule-insns`, test alone. MECHANICALLY TESTED:
rebuilt framebuf.o + full relink; the candidate is byte-identical to the
void-prototype build (same 2168-byte size, same 514/542 word diffs, full
function disassembly diff = 0 lines changed). The recommendation failed;
the decompiler's block confirmation (prologue-save schedule as the
irreducible wall) stands.

## Verification of the revert

`make split && make` (framebuf.cpp back to INCLUDE_ASM, Makefile flag
removed): `cmp build/boot_elf.elf assets/boot_elf.elf` passes.

## References

- working/framebuf_SetupFS_AA_buffer/ (dossier.md, orig_fn.txt,
  mine_fn.txt, fn_diff_now.txt, probes/) — cleared after this block; the
  durable record is this note.
- decomp_state/notes/framebuf_func_001FA860.md (blocked sibling,
  last-resort 2026-10-03)
- decomp_state/notes/framebuf_func_001FA958.md (dead tail after the
  sibling, inline-asm preserved)
- reference/Lombyte/src/assembly/textbin/fun_001fa978.c (m2c, non-matching)
- reference/dl/game_dl/framebuf.cpp (Deadlocked evolved version)
- tools/fdiff.py + tools/fpatch.py (added this session: raw word diffs and
  parseable-candidate splicing for non-matching-state builds, whose
  section-header table is truncated by design)
