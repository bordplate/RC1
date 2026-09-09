# func_001F2070 (0x1F2070, 308 bytes) — matched 2026-09-09

Perspective projection helper in code/game/draw.cpp. Called from
FUN_001EDC50 (effects) as `jal 0x1F2070` with `a0 = out[4] float buffer,
a1 = world/vector pointer`. Projects a 3D vector through the camera and
emits screen-space coordinates.

## Semantics

```
v   = identity-ish 16-float matrix (func_001F9FC8), v[12..14] = -cam.f14x * 1024.0f
m   = func_001FA378(m, cam.matrix @D_00186F40+0x40, v)
s4  = func_001F9A68(s4, vec, 1024.0f); s4[3] = 1.0f
r4  = func_001F9D20(r4, s4, m)
scale = D_0018CF10.f00 / r4[3]
out[2] = r4[2] * 0.0009765625f        // = r4[2] / 1024
r4[0]  = r4[0] * scale + 2048.0f      // stored back to stack
r4[1]  = r4[1] * scale + 2048.0f      // stored back to stack
out[0] = r4[0] * 16.0f
out[1] = r4[1] * 16.0f
```

`D_00186F40` and `D_0018CF10` are two camera-like globals (struct Camera,
0x1B4 bytes: f00, pad, matrix[16] @0x40, pad, f140/f144/f148, pad, f1B0).
The fastfunc callees live in code/_generated/game/fastfunc.s.

## Constants

Floats verified by unpacking the lui words (`struct.unpack('<f')`):
1024.0f (0x44800000), 1.0f (0x3F800000), 2048.0f (0x45000000),
0.0009765625f = 1/1024 (0x3A800000), 16.0f (0x41800000).
An earlier pass misdecoded these as 1000/32/10/0.1 by hand; the Ghidra
disassembly constants were correct all along.

## EGC quirks found

1. Reverse-order allocation: source must assign `r4[0]` before `r4[1]` and
   `out[0]` before `out[1]` for EGC to emit the original machine order
   (r4[1] stack store and out[1] store first). Statement order
   `out[2]; r4[0]; r4[1]; out[0]; out[1];` gives 308/308 with 0 diffs
   (probe v4). The "natural" order r4[1]-first produces a 6-word
   register-swap diff.
2. Prologue 0x7FB? words: this toolchain assembles `sq $17,176($sp)` as
   0x7FB100B0 (op 0x1F minor 0x1D); objdump/EER alias $17 as "s1". The
   words are byte-identical to the original — do not chase the mnemonic.

## Dead tail func_001F21A8 (retained orphan)

After the parent's epilogue the original contains, 8-aligned:

```
1f21a4: nop
1f21a8: sw  $zero, 0x1B0($v0)   # v0 = 0x190000 = %hi(D_0018CF10) from 0x1F2120
1f21ac: nop
```

Target 0x1901B0 = D_0018CF10 + 0x32A0, zeroed .data, no project symbol.
Unreachable (no jal/j/xref; Ghidra has no function there). This is a
single-instruction store with no setup, reusing a stale %hi temporary —
EGC's post-`jr` spill zone only accepts setup-free stores, and no C
expression yields exactly one `sw $zero,0x1B0($v0)` on the cached v0:

- field f1B0 (v1/v5): `addiu v0,v0,%lo(D)` fixup + live store (316B)
- field @0x12A0 / @0x32A0, same or separate struct, int/float
  (v6/v11/v12 + last-resort probe): full-base fixup + live store
- separate symbol / int array at 0x1901B0 (v7/v10): new reg + own lui,
  live store
- constant-address cast `*(int*)0x1901B0 = 0` (v9): `at` base, live lui+sw,
  perturbs the f1/f2 allocation of the divisor load (14 diffs)
- page-anchor `D_00190000 ± off` representation, all statement positions,
  `-fno-schedule-insns` / `-fno-schedule-insns2` (last-resort): CSEs the
  complete first address; still live stores, 316B

Precedent for the mechanism (live vs dead): func_001F5AB0 ends with a
setup-having store (lui at; sw v0,%lo(D_00160F00)(at)) live before the jr
and a setup-free store (sw v0,0(gp)) dead after it.

last-resort GPT-5.6 Sol (2026-09-09) confirmed: the intermediate value
v0 = 0x190000 has no corresponding C pointer value — symbolic accesses
materialize the complete address or create another HI16 temporary — and
no tested source/scheduling form emits anything after the `jr` delay slot.
Recommendation (adopted): match the parent without the store statement,
retain the orphan INCLUDE_ASM (it supplies the 8 bytes; the two nops come
from the .align 3 boundaries), and block func_001F21A8 as a dead tail.

Probes: decomp_state/probes/draw_func_001F2070_v4.cpp is the matched form
(v1–v3, v5–v12 are comparison variants).
