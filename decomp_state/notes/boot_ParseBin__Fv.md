# boot_ParseBin__Fv (0x12D8F8, 0xDC = 55 words) — BLOCKED

`StartLevelPtr ParseBin()` in `code/game/boot.cpp`. Parses the level-load table
reached from `levelRoot` (0x15EE4C, `.data`, value 0 in the boot ELF), copies
each chunk payload to its destination, and returns the first captured `entry`
pointer (the next level's start function). `main` calls it in a loop to chain
levels.

## Data model
```c
struct LevelLoad { u8* dst; u32 len; u32 src_off /*unused*/; void* entry; }; // 16-byte header + payload
struct LevelRoot { u32 first_offset; };
```
First chunk = `levelRoot + *levelRoot`; payload `src = (u8*)chunk + 0x10`,
`next = src + len`; copy 8-byte when `len`, `src`, `dst` are all 8-aligned else
4-byte.

## Original structure (verified objdump)
Per iteration, execution order is **setup (TOP) → entrypoint → copy → advance**:
- Prologue (lean, NO nop): `lui v1; lw v1` (levelRoot→v1), `move t1,0` (entrypoint=0),
  `lw v0,0(v1)` (first), `b TOP`, `addu t0,v0,v1` (base→t0, in the b delay).
- **TOP** (single block, TWO predecessors: the prologue's `b` and the advance
  fall-through): `move a3,t0` (chunk), `addiu t0,a3,16` (src), `lw a1,0(a3)` (dst).
- **entrypoint**: `bnez t1,TAIL` (subsequent-first); `move a0,t0` (s→a0, in the b delay);
  `b L910`; `lw t1,12(a3)` (set entrypoint first time, delay). TAIL: `lw v0,12(a3)`;
  `beql t1,v0,L914` (continue if equal); `jr ra` / `move v0,t1` (return if not).
- **copy**: `lw v0,4(a3)` (len→v0), `move v1,v0` (len copy→v1), three `bnezl` alignment
  tests (len&7, src&7, dst&7 — De Morgan `&&` chain) each with `addu v1,a1,v1` (word
  bound, rematerialized; branch-likely so only the taken edge executes it), `addu
  a2,a1,v1` (qword bound→a2), `move a0,t0` (s→a0), `beq a1,a2` (qword skip, `move v1,a1`
  qword d in delay), qword loop `ld/sd/addiu v1,bne/addiu a0` (s=a0,d=v1,bound=a2, TWO
  nops), word `beql a1,v1` (word skip, len reload in delay), word loop `lw/sw/addiu
  a1,bne/addiu a0` (s=a0,d=a1,bound=v1, TWO nops).
- **advance**: `lw v0,4(a3)` (len reload), `addu t0,t0,v0` (src += len, in-place on t0),
  then fall through to TOP.

Key registers: levelRoot→v1, base/src loop-carried in a SINGLE t0 (ping-pongs
base↔src), chunk→a3, dst→a1, entrypoint→t1, len in v0 (load) then copied to v1,
word bound→v1, qword bound→a2, s→a0 (both loops), qword d→v1 (copy of dst),
word d→a1 (dst's own register). The word path consumes pre-loaded `dst`(a1)/`s`(a0)
directly; only the 64-bit path introduces a destination copy (v1) — asymmetric
iterator lifetimes.

## Why it is blocked
The high-level structure (single two-predecessor TOP, two separate bounds, three
`bnezl` alignment tests, word d=a1 / qword d=v1, correct iteration order) is
reproducible in C. The irreducible gap is **EGC block ordering + register
allocation**, which no C form or flag reproduces:

1. **Block ordering.** The original places the single TOP setup block LAST in
   memory (after the copy body), reached by `b` from a lean prologue. EGC always
   emits it FIRST (setup-first) for the structurally-correct `while(true)` form
   (p16: single TOP, two bounds, three bnezl, word d=a1 — but TOP at the start,
   fall-through). The do-while form (p21) gets copy-first memory layout but
   changes the iteration order (copy executed before the setup) and DUPLICATES the
   setup (initial in the prologue + loop at the end), so it never yields a single
   TOP block. The copy-first + single-TOP combination the original has is not
   reachable: it requires the loop head to be the setup (while-true) AND placed
   after the body, which is EGC's block-ordering decision, not a C construct.
2. **Register allocation.** Original: levelRoot→v1, base→t0 (single register
   ping-pong), len→v0 then copy→v1, s→a0, qword bound→a2, advance as `addu t0,t0,v0`
   in-place. Every probe puts levelRoot/base in a0, s in a2, qword bound in a3, and
   the advance in two registers (a0/t0). The len v0→v1 copy and the single-t0
   advance are the same underlying tie-break the original's allocator made and the
   local EGC makes differently.
3. **Prologue / nops.** Original prologue is 6 words with no nop and only the base
   compute + `b TOP`; each copy loop body has TWO nops. Probes carry an extra nop
   and/or the initial setup in the prologue.

## Probes (flags `-G8 -O2 -ffast-math -fno-exceptions -snas`)
Diffs are `tools/decomp_probe.py <source> code/_generated/nonmatchings/game/boot/ParseBin__Fv.s ParseBin__Fv --out <dir>`
which LINKS the probe with the game's symbol table + runtime GP (so relocations
resolve to the true machine code) and compares the linked bytes to the original
55 words (220 bytes). Best: **p21 (do-while) = 43/55** (matches 12: the qword/word
loop cores — sd/sw, d-increments, loop nops, loop/skip branches). p16 (while, single
TOP) = 54/55. p22 (last-resort rotated-while, TOP in a comma-expression condition)
= 45/55. p24 (last-resort ternary condition) = 54/55. (An early unlinked `.o` byte
diff muddied the register picture by showing pending relocations as address
mismatches; `decomp_probe.py` is the authoritative comparison.)

Exhausted: if/else-if chains, `||`/De Morgan `&&` alignment chain (the `&&` chain is
required & correct; OR-form is wrong), goto (fails C++ compile: jumps cross
initialization), advance as `chunk=src+len` / `chunk=src+chunk->len` / `src+=len;
chunk=src` / single loop-carried `src` doubled as chunk, top-of-function vs in-loop
decls, separate `base`/`chunk`/`p` loop var, per-branch bounds (EGC CSEs to one),
for-loop form, do-while with copy-first + setup-at-end, entrypoint as body statement
vs loop condition, plain extern vs `.data` section attribute.
Flags: `-fno-schedule-insns` (no change), `-fno-schedule-insns2` (no change),
`-fno-delayed-branch` (52 words, confirms the three delay-slot bounds are the
scheduler's taken-edge duplication of one `addu`), `-mno-split-addresses` (fixes the
levelRoot load to a self-based `lui v1; lw v1` for the `.data` symbol but does not
change the layout/RA).

## Tooling
Use `tools/decomp_probe.py` for function-level diffs: it reads the expected bytes
from the Splat reference `.s` (verified against the boot image), compiles the
candidate via `make probe`, LINKS it with the game's symbol table and runtime GP
(`_gp = 0x166c00`) so relocations resolve to true machine code, and byte-compares
the linked `.text` to the original. Do NOT diff an unlinked `.o` against the boot
image (pending relocations masquerade as address mismatches), and do NOT rely on
objdump text (it collapses nop pairs into `...` and right-justifies the offset
column). VMA 0x12D8F8 maps to boot-image file offset 0x2E878 (segment Off 0x1000 /
VA 0x100080).

## Escalations (this exact target)
- decomp-researcher: full report — structure reproducible, no codebase idiom to copy
  (`FastMemCopy` 0x1f98d0 is handwritten 16-byte unrolled asm, no alignment tiers),
  Deadlocked reference confirms the idiom family but is a later padded variant.
- expert (GPT-6 Astra, one-shot): corrected the branch-likely delay-slot
  misread, identified the asymmetric iterator lifetimes (word path uses pre-loaded
  dst/s directly; only the 64-bit path copies the destination), provided the exact
  C form (tested as p16).
- last-resort-decompiler (GPT-5.6 Sol, invoked 2026-09-22): recommended a rotated
  `while` with the TOP setup as a comma-expression loop condition and the
  entrypoint assignment as the first body statement (p22) plus a ternary-condition
  variant (p24). Both tested with the `.data` symbol + `-mno-split-addresses`; both
  produced a DIFFERENT CFG (the entrypoint test fused into the first alignment
  branch's delay slot, `beq t1,0` first-pass-first check) and did not improve on
  p21's 43/55 — p22/p23 = 47, p24 = 54. The recommendation was based on treating the
  entrypoint check as the loop condition, but the original keeps it as a separate
  body block after TOP, so the rotated-condition form cannot reproduce the original
  CFG. No match.

INCLUDE_ASM retained; the generated asm is overlay-safe. See
`decomp_state/blocked.json` entry `code/game/boot.cpp:23:ParseBin__Fv`.
