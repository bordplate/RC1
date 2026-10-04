# func_001FB440 (setupEffectDrawBufferGs) — 0x1FB440, size 0x23C (143 words)

BLOCKED 2026-10-04. Retain INCLUDE_ASM; full-ELF parity preserved.

## What it does

C-linkage. Builds the occlusion effect-buffer GS draw-env state packet and
appends it to the VU1 command chain. Called by `setupEffectDrawBuffer`
(0x1F7888, draw_post_post.cpp) as `func_001FB440(log2Width, log2Height,
base << 13)` (a0/a1/a2; a3/a4/a5 unused).

Prologue computes the packed texture descriptor `effectBufTexDesc` and the
width/height/PSM/base fields in `occlCamParamBase`, zeroes the pre-zeroed
chain region with `FastMemZero16(0xF0)`, appends a VIF direct-data record
(10 qwords of GS state), calls the SDK builder `sceGsSetDefDrawEnv`
(0x121FC8) to emit the draw-env GIF block, patches the buffer base into the
finished block, and advances `vu1ChainHead`.

## Verified / solved pieces

- **Descriptor shift idiom:** the original keeps `1 << max(log2Width - 6, 1)`
  as a RUNTIME shift — `addiu t,v0,-6; slt v1,zero,t; movz v0,a3(1),v1;
  sllv v0,a3,v0` (movz KEEPS v0 when v1 != 0, so v0 = max(t,1); sllv = the
  shift). C form: `int tbw = log2Width - 6; if (tbw <= 0) tbw = 1; int
  shift = 1 << tbw;`. A folded select (`tbw > 0 ? tbw : 1`) does NOT match.
- **effectBufTexDesc = 0x15EED0:** the descriptor store is GPREL
  (`sd v0, -32048(gp)` = 0x166C00 - 0x7D30) in the `FastMemZero16` jal delay
  slot. 0x15EED0 sits in the GAP (0x15EE60-0x15EF00) between core.lit
  (0x260 bytes, 0x15EC80-0x15EE60) and .lit (0x15EF00). It must be a linker
  alias in `config/linker_aliases.ld` (`effectBufTexDesc = 0x0015EED0;`),
  NOT a `config/symbols.txt` symbol (a symbol there makes Splat extend the
  generated core.lit past 0x260 and break parity). The C decl is a plain
  `extern u64 effectBufTexDesc;`.
- **vu1ChainHeadStore = 0x160F00** (linker alias, GPREL store only inside a
  delay slot); `vu1ChainHead` is the plain `volatile u32* volatile` (absolute
  loads/stores).
- **occlCamParamBlock fields** (camera.h): 0x160 effectBufW (s16, 1<<log2W),
  0x162 effectBufH (s16, 1<<log2H), 0x164 effectBufPsm (s16), 0x166
  effectDrawBufBaseGs (s16, gsBase>>13), 0x16C effectBufPsmMt (u16, low
  nibble << 24), 0x16E effectBufBaseGs (s16).
- **Builder:** `sceGsSetDefDrawEnv(u64* dst, short psm, short w, short h,
  short qwc, short flags)` at 0x121FC8, returns int, called with (dst,
  psm, w, h, 3, 0). The EE/R5900 ABI passes the 5th/6th int args in t0/t1
  ($8/$9), which is why the original emits `li t0,3; move t1,zero`.
- **Payload constants (verified 64-bit literals):** A0@head+0x10 =
  0x1000000000000008ULL; q0@head+0x90 = 0x1000000000000001ULL;
  q4@head+0xB0 = 0x4400000000008001ULL.

## The wall — body scheduling (63 instruction-word diffs in the best state)

Three real residuals (the rest is a 1-word shift cascading from #2):

1. **Record-write order** (words ~64-65): two 64-bit stores to the freshly
   allocated VU1 region. Original stores the LARGER value (0x1000...0008)
   first from base `v0=head`, the SMALLER (14) second from base `v1=head+16`
   (`sd a1,16(v0); sd s3,8(v1)`). EGC canonicalizes both onto one base and
   emits small-first regardless of source order or distinct `head`/`next`
   expressions.

2. **Builder-call delay slot** (words ~72-75) — the dominant one. Original:
   `move a0,s2` (a0=dst, BEFORE the call); `jal 0x121fc8`; `sw s2,-23808(gp)`
   (the `vu1ChainHeadStore` store IN the jal delay slot, GPREL16, 1
   instruction). Candidate: `jal`; `move a0,s2` (in the delay slot); `lui
   at,0x16; sw s2,3840(at)` (the store AFTER the call, ABSOLUTE, 2
   instructions). The 1-vs-2 instruction store is what shifts every later
   word by one through the end. ps2eeas expands `sw r,sym` to a single
   GPREL16 only inside a noreorder/delay-slot region (or after a preceding
   `.extern sym,N`); outside it the store is a self-based absolute `lui/sw`.
   So the store is GPREL iff EGC schedules it INTO the jal delay slot — and
   EGC instead spends that slot on the arg-setup `move`. Pinning the first
   arg to a0 (`register u64* d asm("$4") = dst;`) did not free the slot.

3. **lh vs lhu** (word ~77): original `lh v0,358(s0)` (signed 16-bit load of
   effectDrawBufBaseGs at 0x166); candidate `lhu v1,358(s0)`. The value is
   immediately `andi r,r,0x1ff`, so EGC zero-extends. A signed-int local with
   an input asm barrier (`int db = ...; asm volatile("" : : "r"(db));`) was
   the last-resort form, but it disturbed the prologue allocation.

## What was tried

- Prologue: the `1 << max(log2Width-6,1)` runtime-shift idiom (movz/sllv)
  matched; a h-first-compute / w-first-store local arrangement fixed the
  t0=log2Width allocation and the w/h store order (prologue words 0-30 and
  the w/h stores 31-35 matched except data-address artifacts).
- Record stores: large-first / small-first source order, and the last-resort
  distinct `head`/`next` base form — EGC reorders regardless.
- Builder delay slot: source reordering, and pinning the builder's first arg
  to a0 — the `move a0` stays in the delay slot, the store stays
  absolute-after-call.
- last-resort-decompiler (GPT-5.6 Sol) recommendations: distinct head/next
  bases (neutral), moving `vu1ChainHeadStore` before the call (broke the
  matched prologue — frame shrunk to -64, register saves reshuffled, 117
  diffs), and a signed-int input barrier for the lh (disturbed the
  prologue). None closed the body without regressing the prologue.
- Flag diagnostics (`-fno-schedule-insns`, `-fno-schedule-insns2`,
  `-mno-split-addresses`) were not pursued globally; the prologue already
  matched under default flags and the call-delay-slot fill requires the
  default scheduler.

## Notes on the 16-byte ELF growth observed while iterating

When the candidate function was 3 words (12 bytes) longer than the original,
the built boot ELF came out 16 bytes larger and malformed (section headers
past end of file, objdump empty). The framebuf .text is pinned to 0x1260, so
a longer function pushes the trailing INCLUDE_ASM functions past the pinned
section end and corrupts the link. This is a symptom of the function not
matching, not a separate bug: a byte-exact function (0x23C) links cleanly.
Do not read the 16-byte growth as an independent defect.

## Re-attempt start point

A matching candidate needs, in one consistent allocation: (a) the prologue
with t0=log2Width and the -80 frame; (b) the two record stores kept in
large-first order on the original's (v0, v1) bases; (c) the `move a0` for
the builder BEFORE the jal and the `vu1ChainHeadStore` store as a single
GPREL in the jal delay slot; (d) a signed `lh` for effectDrawBufBaseGs.
Items (a) and (d) interact with (b)/(c) through register allocation, which
is the EGC 2.95.2 default-scheduler/RA wall.
