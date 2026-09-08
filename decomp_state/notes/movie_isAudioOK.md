# isAudioOK (vram 0x23A790, file 0x13B710, 44 bytes / 11 words) — BLOCKED 2026-09-08

Free C function in code/game/movie/movie.cpp (line 11, currently INCLUDE_ASM).
Unmangled symbol `isAudioOK`, `int isAudioOK(void)`.

## Semantics (confirmed)

Load the 32-bit value stored at global 0x16120C, add constant 0xD9100, call
`audioDecIsPageFull` (matched; `extern "C" int audioDecIsPageFull(_AudioDec* self)`
in code/game/movie/audiodec.cpp) with the sum as the single argument, and return
its result. Ghidra FUN_0023a790 agrees. The only caller (func_0023A3B8 at
0x23A654: `jal isAudioOK; nop; beq v0,zero,...`) consumes v0, so the return type
is int and the callee prototype must be `int`.

0x16120C lives in the `.lit` section (vram 0x15EF00-0x161228), INSIDE the gp
window (gp=0x166C00). It holds a base pointer that movie code combines with
+0xD9xxx offsets (see the repeated `lui v0,0x16; lw v0,0x120c(v0); lui a0,0xd;
ori a0,0x91xx; jal X; addu a0,v0,a0` idiom throughout func_0023A3B8). A Splat
label `D_0016120C` already exists (code/_generated/build/data/lit.lit4.s).
`proceedAudio__Fv` (0x23ABA0) is a byte-identical twin except it calls
audioDecSend — it is blocked by the same codegen issue.

## Exact original (11 words)

```
0:  1600023C  lui   v0, 0x16              # %hi(0x16120C)
4:  0C12428C  lw    v0, 0x120C(v0)        # %lo(0x16120C); VALUE ENDS IN v0
8:  0D00043C  lui   a0, 0xD               # 0xD9100 hi
C:  F0FFBD27  addiu sp, sp, -0x10         # prologue (4th insn!)
10: 00918434  ori   a0, a0, 0x9100        # 0xD9100 lo
14: 0000BF7F  sq    ra, 0(sp)
18: B8EB080C  jal   audioDecIsPageFull
1C: 21204400  addu  a0, v0, a0            # arg = base + 0xD9100 (jal delay slot)
20: 0000BF7B  lq    ra, 0(sp)
24: 0800E003  jr    ra
28: 1000BD27  addiu sp, sp, 0x10
```

The load pair (lui v0; lw v0) AND the constant-hi (lui a0) are hoisted BEFORE the
prologue. The constant-lo (ori a0) and ra-save (sq ra) are AFTER it. The loaded
value is in v0 (self-overwriting base), and the final add is `addu a0, v0, a0`.

## The codegen conflict (why it blocks)

The original needs two properties that EGC 2.95.2 only ever emits together in
opposing pairs:

1. **Self-overwriting load, value in v0, load-pair adjacent, `addu a0,v0,a0`** —
   produced ONLY by the CONSTANT-CAST form (`*(int*)0x16120C`). Because the
   address is a constant (no symbol reloc), EGC may overwrite the base register
   with the value. But this form SCHEDULES THE PROLOGUE FIRST and the load LAST:
   `[addiu sp, lui a0, sq ra, ori a0, lui v0, lw v0, jal, addu]` (default,
   -fno-schedule-insns, -G0, and both no-sched flags all agree; both-no-sched
   gives `[addiu sp, sq ra, lui v0, lw v0, lui a0, ori a0, ...]` — still
   prologue-first). Never hoisted.

2. **Load hoisted before the prologue** — produced ONLY by the SYMBOL form
   (`extern "C" int D_0016120C __attribute__((section(".data"))); (u8*)D_0016120C
   + 0xD9100`). EGC hoists 3 insns before the prologue: `[lui v0, lui a0, lw v1,
   addiu sp, ori a0, sq ra, jal, addu a0,v1,a0]`. But the value goes to **v1**
   and the order is interleaved (lui a0 before the lw), because the symbol
   reference forces the base register to stay stable across the HI16/LO16
   reloc pair, so the value cannot reuse the base register.

A plain `extern "C" int D_0016120C;` (no section attr) is in-window, so EGC uses
a single GP-relative `lw` (1 insn) — wrong (original is lui+lw, 2 insns).
section(".data") (or a constant cast) is what forces the absolute lui+lw.

So: cast-form = right registers (v0, `addu a0,v0,a0`) but no hoist; symbol-form =
right hoist but wrong register (v1) + interleaved order. No form yields both.

## Forms/flags tried (project EGC 2.95.2, -G8 -O2 -ffast-math -fno-exceptions)

- Cast, 1-expr / 2-stmt / 3-stmt / int-temp / explicit-result-temp: prologue
  first, load last, v0. (probes isAudioOK_v1..v5, v7, v11 fA/fD/fF/fH, /tmp iA)
- Cast + -fno-schedule-insns / -fno-schedule-insns2 / both / -G0: same, no hoist.
- Symbol section(".data"), scalar / int-array[0] / `int b=Gsym` /
  `*(int*)&Gsym` / 2-stmt / 3-stmt: hoisted but v1 + interleaved. (v3, v7, v11
  fB/fC/fE/fG, /tmp iB)
- Symbol + -fno-schedule-insns2: value moves to v0 but prologue goes first.
- Symbol + both no-sched: `[addiu sp, sq ra, lui v0, lw v0, lui a0, ori a0, ...]`
  (v0, adjacent pair, but prologue first).
- `register int base asm("$2") = D_0016120C;` (last-resort form): matches words
  4..11 exactly (`addiu sp` onward, value v0, `addu a0,v0,a0`), but the 3-word
  prefix is `[lui v1, lui a0, lw v0,0(v1)]` vs original `[lui v0, lw v0,0(v0),
  lui a0]` — base register v1 (not v0) and const-hi interleaved before the
  self-load.

last-resort-decompiler (GPT-5.6 Sol) independently tested zero-byte asm
dependencies, fixed-register variants, -m[no-]split-addresses, strict aliasing,
speculative-load scheduler flags, and optimization toggles, and concluded with
HIGH confidence that current EGC cannot emit the exact schedule from an ordinary
source form; it recorded the same unresolved 3-word prefix mismatch.

## Verdict

BLOCKED. EGC 2.95.2 ties the self-overwriting v0 load (constant address) and the
pre-prologue hoist (symbol address) to mutually exclusive address kinds. Needs a
different compiler/flag not yet identified in this project. `proceedAudio__Fv`
is blocked by the identical issue (same 11 words, only the jal target differs).
Both INCLUDE_ASM placeholders are retained to preserve parity.
