# OcclAppendRectsHi__FiiiiUlUii (func_00200F90, 0x200F90, 0x180 = 96 words) — MATCHED 2026-10-12

## Purpose
VU1-chain packet appender, the 7-arg sibling of matched `OcclAppendRects`
(0x200E08) and the blocked `func_00200C80`:

```
void OcclAppendRectsHi(int x0, int y0, int x1, int y1,
                       u64 attr, u32 cornerHi, int scale)
```

Leaf function with a 0x10 frame (saves s0). Appends the same 16-word (0x40
byte) inline VU1 GS solid-sprite (rectangle fill) record to the chain through
the in-window global `vu1ChainHead` (u32* @ 0x160F00) that `OcclAppendRects`
does: header tags 0x10000003 / 0 / 0 / 0x50000003, then the payload
GIFTAG 0x4400000000000001, GIFTAG param 0x4410, attr-lead 0x46, the `attr`
word, and two packed (x,y) corners.

Differences from the matched sibling:
1. The HIGH 32 bits ORed into each packed corner come from the runtime
   `cornerHi` argument (`dsll32 zext` of the arg register, per branch)
   instead of the constant 0xFFFFF000. The sole boot caller
   (0x219B14, in the 0x2196B8 region) passes 0, so on the boot path the
   corners are zero-extended the same way the sibling constant masks them;
   the parameter generalizes it for overlay callers.
2. A `0x10` frame: x1 lands in s0, which is spilled (`sq $16,0(sp)` /
   `lq $16,0(sp)`). The sibling has no frame.
3. The intermediate and final `vu1ChainHead` advance stores are self-based
   absolute (`lui at; sw`) instead of the sibling's GPREL-via-alias form —
   the same plain `u32* head` / `u32* next` local form reproduces both.

Packed corner:
```
raw  (scale != 0): (x+minX-8)  | ((u64)(y+minY-8)  << 16) | ((u64)cornerHi << 32)
x16  (scale == 0): (16x+minX-16) | ((u64)(16y+minY-16) << 16) | ((u64)cornerHi << 32)
```
minX/minY = `occlViewParams.{minX,minY}` @ +0x10/+0x14 (base 0x13E500,
camera.h). The 8 / 16 offsets are one-pixel and half-pixel corner offsets in
the 16.0 fixed-point occlusion view space (minX/minY pre-scaled <<4 by
InitViewContext).

## ABI (definitive, byte-verified)
7 params; the EGC register-arg window is a0-a3, t0-t3 (NOT a0-a5+stack) and
the `u64 attr` occupies ONE 64-bit argument register:

| arg     | reg      | use                                            |
|---------|----------|------------------------------------------------|
| x0,y0,x1,y1 | a0-a3 ($4-$7) | corner arithmetic                    |
| attr    | a4/t0 ($8) | `sd $8,0x28(head)` — full 64-bit store      |
| cornerHi| a5/t1 ($9) | `dsll32 $5,$9,0` zext, ORed as corner high |
| scale   | t2 ($10) | `beqz $10` — 0 => x16 path (branch target), nonzero => raw (fall-through) |

Note this refines the sibling's note: the 6-arg `OcclAppendRects` mode arg
sits in $9 for the same reason (one 64-bit slot for the u64), not "overlapping
the u64 high word".

## Match-critical forms
1. Double-volatile `extern volatile u32* volatile vu1ChainHead;` — EGC must
   re-read the head before every packet store; a plain or single-volatile
   declaration CSEs the later loads (same wall as the matched sibling).
2. Payload stores through a PLAIN (non-volatile) `u32* head`: EGC schedules
   the `attr` store into the `beqz` delay slot and the second-corner store
   into the unconditional `b` delay slot; a volatile head leaves both slots
   as nops.
3. `u64 high = (u64)cornerHi << 32;` declared INSIDE each branch body (not
   before the if) so the zext is emitted once per path in the original's
   registers, not CSEd/hardened into one.
4. `u32 cornerHi` (not `int`): the original zexts (`dsll32 r,r,0`), so a
   signed type would risk a sign-extending pair.
5. SN-assembled TU (ps2eeas): the self-based `lui;sw` advance stores expand
   from bare pseudos only under ps2eeas; GNU assembly would emit GPREL16 and
   change the size.

## Integration
Splat boundary split in `config/RC1.yaml` (the tail segment at 0x100F90 was
split at the dead-tail boundary 0x101110):

```
- [0x100e08, cpp, game/hud_post_post2_post3_occl]        # OcclAppendRects (SN)
- [0x100f90, cpp, game/hud_post_post2_post3_tail_occl]   # OcclAppendRectsHi (SN, default)
- [0x101110, cpp, game/hud_post_post2_post3_tail]        # dead tail + rest (GNU)
- [0x101520, cpp, game/init]
```

`hud_post_post2_post3_tail_occl.o` uses the default `-snas` (no private
flags, not in the GNU list); the remaining tail TU keeps GNU assembly as
before. Symbol added to `config/symbols.txt`:
`OcclAppendRectsHi__FiiiiUlUii = 0x00200F90;` (mangling: 4x int `i`, u64 =
unsigned long `Ul`, u32 `Ui`, int `i`).

## Dead tail func_00201110 (0x201110, 0x14)
Unreachable multi-unit dead tail after the function: three stack
deallocations `addiu sp,sp,0x20 / 0x30 / 0xF0` with nops, matching no live
frame of the parent (whose epilogue deallocates 0x10). Deadness scan finds no
references; Ghidra has no function at or containing 0x201110. Emitted as
inline byte-preserving `asm` in `hud_post_post2_post3_tail.cpp` (the TU
Splat associates with 0x101110), `effects.cpp` raw-tail precedent. Not a
decompilation target, not a blocker.

## Verification
`tools/decomp_probe.py` on the final source form: 384 bytes, 0 word diffs vs
`code/_generated/nonmatchings/game/hud_post_post2_post3_tail/func_00200F90.s`.
Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
Nonmatching count 526 -> 524 (function + dead tail).
