# func_002169C0 (code/game/stream.cpp) — matched 2026-09-07

## Semantics

IOP-event callback for the music transition state. The music range handlers
(`func_00215440` / `0x215518` / `0x215600` / `0x2156D8` / `0x2157D0` /
`0x2158A0` in music.cpp) each store their three arguments into
`D_001516D0` (MusicState) fields +0x54 (arg1), +0x58 (arg2), +0x56 (arg3),
set +0x5A = 1, and register this callback through
`func_0012EC08 -> func_0012E6E0` (989snd IOP RPC, msg 0x2c/0x1c), passing
the callback address and a pointer to `D_001516D0 + 0x50` as stack args.
That is why Ghidra shows only DATA xrefs (the `sw t3, 0x8(sp)` pointer
stores in the handlers) and no direct jal.

When the IOP event fires, the driver calls `func_002169C0(param_1, param_2)`
with `param_2 = D_001516D0 + 0x50`:

- `*(int*)(D+0x50) = param_1` (recorded; on the `param_1 == 0` path this
  stores 0 — see codegen note 1, the store runs on BOTH branch paths)
- `param_1 != 0`: if `*(s16*)(D+0x5A) == 1` set it to 2 (state 1 -> 2)
- `param_1 == 0`: replay the stored transition
  `func_00215970(D+0x54, D+0x58, D+0x56)` (the music value/arg2/arg3
  dispatcher, still INCLUDE_ASM in music.cpp)

## Original body (file offset 0x117940, vram 0x002169C0, 0x5C bytes)

```
addiu sp,sp,-0x10
dsll32 a1,a1,0          # sign-extend (int)param_2 in place
dsra32 a1,a1,0
beqz  a1, .L00216A10    # param_2 == 0 -> epilogue
    sq  ra, 0(sp)
beqz  a0, .L002169F8    # param_1 == 0 -> replay path
    sw  a0, 0(a1)       # *(int*)p = param_1  (delay slot: runs on BOTH paths)
lh    v1, 0xA(a1)       # *(s16*)(p+0xA)
addiu v0, 0, 1
bne   v1, v0, .L00216A14
    lq  ra, 0(sp)       # epilogue lq hoisted into bne delay slot
addiu v0, 0, 2
b     .L00216A14
    sh  v0, 0xA(a1)     # *(s16*)(p+0xA) = 2
.L002169F8:
lui   v0, %hi(D_001516D0)
addiu v0, v0, %lo(D_001516D0)
lh    a2, 0x56(v0)      # arg3
lh    a0, 0x54(v0)      # arg1
jal   func_00215970
    lh  a1, 0x58(v0)    # arg2 (delay slot)
.L00216A10:
lq    ra, 0(sp)
.L00216A14:
jr    ra
    addiu sp, sp, 0x10
```

## Replacement (stream.cpp)

```cpp
typedef struct {
    u8 pad_0x54[0x54];
    s16 field_0x54;
    s16 field_0x56;
    s16 field_0x58;
} MusicTransState;

extern MusicTransState D_001516D0 __attribute__((section(".data")));

extern "C" void func_00215970(int param_1, int param_2, int param_3);

extern "C" void func_002169C0(int param_1, long param_2) {
    int p = (int)param_2;
    if (p) {
        *(int*)p = param_1;
        if (param_1) {
            if (*(s16*)(p + 0xA) == 1) {
                *(s16*)(p + 0xA) = 2;
            }
        }
        else {
            func_00215970(D_001516D0.field_0x54,
                          D_001516D0.field_0x58,
                          D_001516D0.field_0x56);
        }
    }
}
```

`D_001516D0` is also declared in music.cpp as `MusicState` (fields 0x40/0x5C/
0x78 used by `music::Unpause`); the local `MusicTransState` covers the
+0x54/0x56/0x58 region that TU needs. Layouts are compatible; consider
merging into one named struct once the music handlers are decompiled.

## Codegen findings (EGC 2.95.2, project flags)

1. **Store hoisting decides the schedule.** With the store inside the
   `if (param_1)` body, EGC filled the `beqz a0` delay slot with `li v0,1`
   and deferred `sw a0,0(a1)` to the `bne` delay slot (3-word schedule
   mismatch vs the original). Writing the store ABOVE the branch (valid,
   since EGC's own output executes it on both paths — it stores 0 on the
   replay path, which is inert: `func_00215970` never reads `D+0x50`)
   makes EGC place it in the `beqz a0` delay slot and match exactly.
   Semantically the C is equivalent to the binary's real execution, not
   just to a "cleaner" reading of it.

2. **Second param must be `long`** with `int p = (int)param_2` to get the
   in-place `dsll32/dsra32` sign-extension + `beqz a1` (same recipe as
   func_00216990). A plain `int` param compiles to a bare 32-bit `beqz`
   with no extension pair.

3. **s16 fields emit `lh`; u16 emits `lhu`.** The three call-arg loads
   (`lh a2,0x56 / lh a0,0x54 / lh a1,0x58`) require signed `s16` struct
   fields. The `*(s16*)(p+0xA)` load/store are signed by the cast.

4. **Call-arg order vs load order.** C arg order (f54, f58, f56) compiles
   to machine order `lh a2(0x56); lh a0(0x54); jal; lh a1(0x58) in delay`
   with one shared `lui/addiu` base — EGC materializes the a2 arg first,
   a0 second, and parks a1 in the `jal` delay slot, exactly as original.

5. **Epilogue duplication is automatic.** The `lq ra` appears both in the
   `bne` delay slot and at the merge (for the replay path); no C trick
   needed once the body schedule is right.

## Verification (mechanical)

- `build/code/game/stream.o` func_002169C0: 23/23 words match the original
  (independently re-verified by decomp-verifier, 2026-09-07: object vs
  reference .s vs raw `assets/boot_elf.elf` bytes at file offset 0x117940).
- Relocs: R_MIPS_HI16/LO16 on D_001516D0 (resolve to `lui 0x15 / addiu
  0x16D0`), R_MIPS_26 on func_00215970 (resolves to 0x215970).
- `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` — identical.
- Splat reclassified: `func_002169C0.s` now under
  `code/_generated/matchings/game/stream/`; status count 759 -> 758.

## Follow-ups

- `func_00215970` (music value dispatcher) and the `func_00215440` handler
  family would let this callback and the MusicState +0x50..0x68 region get
  real names (transition value / arg2 / arg3 / state).
- Merge `MusicTransState` (stream.cpp) and `MusicState` (music.cpp) into a
  single named struct in a shared header once the handlers are decompiled.
