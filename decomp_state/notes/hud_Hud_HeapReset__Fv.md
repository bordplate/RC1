# Hud_HeapReset__Fv (code/game/hud.cpp) — matched 2026-09-05

## Semantics

HUD bump/texture heap allocator reset. The HUD owns a small first-fit bump
heap; `Hud_HeapAlloc` (0x001FF288) calls `Hud_HeapReset` lazily when the
cursor field is still 0, and then hands out 16-byte-aligned chunks
`cursor += (size + 0xf) & ~0xf` while `size <= end - cursor`.

Globals (both outside the GP window, absolute via
`build/undefined_syms_auto.txt`):

- `D_001940CC` — heap start, set earlier by the level init at 0x002015D8
  (`DAT_001940cc = DAT_00160f0c + 0x240000 + DAT_00160f0c`), i.e. a pointer
  into the level's heap area.
- `D_0019A3E8` — HUD state struct; only +0x10/+0x14 matter here:
  - +0x10 (0x19A3F8) heap cursor
  - +0x14 (0x19A3FC) heap end
  The struct's +0x00/+0x04 words are a texture-cache entry counter written by
  0x001FEE88/0x001FF308; the struct's real name awaits those funcs.

Body: `heapEnd = D_001940CC + 0x64000; heapCursor = D_001940CC;`
(heap size 0x64000 = 400 KB).

## Original body (file offset 0x1001E0, vram 0x001FF260, 10 words)

```
3c040019  lui   $4, %hi(D_001940CC)
3c030006  lui   $3, 0x6
8c8540cc  lw    $5, %lo(D_001940CC)($4)
3c02001a  lui   $2, %hi(D_0019A3E8)
34634000  ori   $3, $3, 0x4000        # 0x64000
2442c1e8  addiu $2, $2, %lo(0x19A3E8) # -0x5c18
00a31821  addu  $3, $5, $3
ac450010  sw    $5, 0x10($2)         # cursor = D_001940CC
0800e003  jr    $ra
ac430014    sw  $3, 0x14($2)         # end = D_001940CC + 0x64000 (delay)
```

## Replacement (hud.cpp)

```cpp
typedef struct {
    u8 pad[0x10];
    u32 heapCursor;
    u32 heapEnd;
} HudHeap;

extern "C" int D_001940CC __attribute__((section(".data")));
extern "C" HudHeap D_0019A3E8 __attribute__((section(".data")));

void Hud_HeapReset(void) {
    D_0019A3E8.heapEnd = D_001940CC + 0x64000;
    D_0019A3E8.heapCursor = D_001940CC;
}
```

## Codegen findings

1. Both globals sit OUTSIDE the gp window (gp=0x166C00, ±32K), so
   `section(".data")` externs make EGC emit the plain absolute
   lui/lw + lui/addiu pair with R_MIPS_HI16/LO16 relocs, exactly like the
   original (lo 0x40CC positive for D_001940CC, lo 0xA3E8 negative so the
   base gets the +1 hi adjust and the addiu imm is -0x5C18 — the linker
   relocs handle this).
2. Two non-constant stores (one loaded value, one addu-computed value)
   sharing one %hi/%lo base: EGC again emits them in REVERSE source order —
   first statement's store in the `jr $ra` delay slot, second before the jr.
   Same behavior as the two-CONSTANT-store case in stream_func_00217020, so
   the reverse-order rule holds for computed store data too. Source
   [end, cursor] -> machine [cursor main, end delay], matching the original
   on the first try.
3. `extern "C"` on the FUNCTION breaks the match: the generated asm of
   `Hud_HeapAlloc__FUiPcT1i` (still INCLUDE_ASM in the same file) references
   the mangled `Hud_HeapReset__Fv`. Plain C++ `void Hud_HeapReset(void)`
   mangles to that name under EGC's cfront scheme (no explicit label needed
   for this shape).

## Verification (mechanical)

- objdump of build/code/game/hud.o `Hud_HeapReset__Fv`: 10 instructions,
  identical sequence/registers to the original; relocs HI16/LO16
  D_001940CC at +0x0/+0x8 and HI16/LO16 D_0019A3E8 at +0xC/+0x14.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- Raw 20-byte slice at file offset 0x1001E0 built == original
  (`1900043c 0600033c cc40858c 1a00023c 00406334 ...`).
- No symbol renamed; the mangled name is identical to the placeholder's, so
  no stale-include hazard.

## Follow-ups

- `Hud_HeapAlloc` (0x001FF288, the other caller of this heap) is a trivial
  15-word leaf and the natural next target; it will confirm the cursor/end
  field names.
- Naming the D_0019A3E8 struct properly waits on 0x001FEE88 / 0x001FF308
  (texture cache init/lookup).
