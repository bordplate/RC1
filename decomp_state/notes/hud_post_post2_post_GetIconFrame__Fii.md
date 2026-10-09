# GetIconFrame__Fii (0x001FF960, 172 bytes) — MATCHED

Status: matched (2026-10-09). Full clean build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.

## Function
Given an icon id and a frame-within-icon index, returns the absolute
frame-table index (the icon's `start` + `frame`) to use when drawing that
frame, or 0 when the frame is unavailable. A frame is unavailable when the icon
is not in the icon table (terminator id 0xFFFF), the frame index is out of the
icon's `len` range, or the frame's palette/texture `ram` entry is not yet
re-based by `LinkHudBank` (bit 31 of the `ram` word still set — see
`HUD_RAM_ENTRY_FLAG` / `HUD_RAM_ENTRY_MASK` in hud.h / hud.cpp).

Established from the Deadlocked reference (`reference/dl/game_dl/hud.cpp:192`,
`GetIconFrame`) and the caller `DrawGalacticMap` (0x1FD748, which calls it with
`GetIconFrame(0xE,0xF)`). This is also the source of the `HudFrame` type
(`{s16 hPal; s16 hTex;}`, DL `frame_t`) and the `hudHeap.frames` field
(+0x20, previously `pad_20`).

Semantics (C):
```
index = Hud_GetIconIndex(icon)          # icon-table index (or terminator index)
entry = iconTable + index * 8
if entry->id == 0xFFFF or frame >= entry->len: return 0
fi = entry->start + frame
if pals[frames[fi].hPal].ram < 0:            return 0   # pal not re-based
if (texs[frames[fi].hTex].ram & 0x80000000): return 0   # tex not re-based
return fi
```

## Original ground truth (objdump of assets/boot_elf.elf)
```
1ff960: addiu sp,sp,-0x20
1ff964: sq    s0,0(sp)
1ff968: sq    ra,0x10(sp)
1ff96c: jal   Hud_GetIconIndex
1ff970: move  s0,a1
1ff974: lui   v1,0x19
1ff978: sll   v0,v0,3            # index * 8
1ff97c: addiu a2,v1,0x3e8        # &hudHeap (0x19A3E8)
1ff980: li    a0,0xffff
1ff984: lw    v1,0x1c(a2)        # iconTable
1ff988: addu  v1,v0,v1           # entry = iconTable + index*8  (rs = index*8)
1ff98c: lhu   v0,0(v1)           # entry->id
1ff990: beq   v0,a0,0x1ffa0c     # A: id == 0xffff -> exit
1ff994: move  v0,zero            #   (delay) ret = 0
1ff998: lhu   v0,2(v1)           # entry->len
1ff99c: slt   v0,s0,v0           # frame < len ?
1ff9a0: beqz  v0,0x1ffa0c        # B: out of range -> exit  (NON-likely)
1ff9a4: move  v0,zero            #   (delay) ret = 0
1ff9a8: lhu   v0,4(v1)           # entry->start
1ff9ac: lui   t0,0x8000          # 0x80000000 (hoisted above the pal load)
1ff9b0: lw    a0,0x20(a2)        # frames
1ff9b4: addu  a3,v0,s0           # fi = start + frame
1ff9b8: lw    a1,0x28(a2)        # texs
1ff9bc: sll   v1,a3,2
1ff9c0: addu  a0,v1,a0           # &frames[fi]
1ff9c4: lh    v0,0(a0)           # hPal
1ff9c8: sll   v0,v0,3
1ff9cc: addu  v0,v0,a1           # &pals[hPal]
1ff9d0: lw    v1,0(v0)           # pals[hPal].ram
1ff9d4: bltzl v1,0x1ffa0c        # C: pal ram < 0 -> exit  (LIKELY)
1ff9d8: move  v0,zero            #   (delay)
1ff9dc: lh    v0,2(a0)           # hTex
1ff9e0: lw    a0,0x24(a2)        # pals
1ff9e4: sll   v0,v0,3
1ff9e8: addu  v0,v0,a0           # &texs[hTex]
1ff9ec: lw    v1,0(v0)           # texs[hTex].ram
1ff9f0: move  v0,zero
1ff9f4: and   v1,v1,t0           # tex ram & 0x80000000
1ff9f8: movz  v0,a3,v1           # ret = (tex flag set) ? 0 : fi
1ff9fc: lq    ra,0x10(sp)
1ffa00: lq    s0,0(sp)
1ffa04: jr    ra
1ffa08: addiu sp,sp,0x20
1ffa0c: nop
```

## Matched source (code/game/hud_post_post2_post.cpp)
```cpp
int GetIconFrame(int icon, int frame) {
    int index = Hud_GetIconIndex(icon);
    HudIconDef* entry = (HudIconDef*)(index * sizeof(HudIconDef) + hudHeap.iconTable);
    if (entry->id != HUD_SLOT_RESET_ICON_ID && frame < entry->len) {
        int start = entry->start;
        register unsigned int mask asm("$8");
        mask = HUD_RAM_ENTRY_FLAG;
        int fi = start + frame;
        return (int)hudHeap.pals[hudHeap.frames[fi].hPal].ram >= 0
            ? ((hudHeap.texs[hudHeap.frames[fi].hTex].ram & mask) == 0
                ? fi : 0)
            : 0;
    }
    return 0;
}
```

## Match-sensitive constructs (all verified required via tools/decomp_probe.py)
1. **Integer-form entry pointer** `(HudIconDef*)(index * sizeof(HudIconDef) +
   hudHeap.iconTable)`. A pointer subscript `((HudIconDef*)hudHeap.iconTable) +
   index` compiles to the same `addu` with the operands swapped (`addu
   v1,v1,v0` vs the original `addu v1,v0,v1` at 0x1ff988). The integer form
   keeps `index*8` in the `rs` slot.
2. **`register unsigned int mask asm("$8"); mask = 0x80000000;`** The 0x80000000
   constant must land in $8 (t0) with its `lui t0,0x8000` hoisted to 0x1ff9ac
   (above the `frames`/`texs` base loads) and the final test as `and v1,v1,t0`
   in the tail. Without the $8 pin EGC picks a different register / hoist point.
3. **Nested-ternary grouping of the validity checks.** This is the piece that
   takes the last 2 words. The two exit tests B (`frame >= len`) and C (`pal ram
   < 0`) emit `beqz` (NON-likely) and `bltzl` (LIKELY) in the original
   respectively. Writing the checks as a flat `if (...) return fi; ... return 0;`
   (or `&&`-chained guards) makes EGC emit `beqzl` for B and `bltz` for C — the
   exact opposite -l selection — a 2-word residual. Grouping them as the nested
   ternary `palOk ? (texOk ? fi : 0) : 0` reproduces the original's exit-value
   RTL (`move v0,zero` in the branch delay slots + `movz v0,a3,v1` tail) and
   therefore its -l bits. This is an RTL/control-flow-provenance tie-break in
   EGC's delay-slot/likely selection, not a semantic difference (both forms are
   behaviorally identical; the original binary carries the same annulled-delay-
   slot shape).

## Tried (all via tools/decomp_probe.py, standalone, project default flags)
- A: `if/else-if` with a `ret` local — 176 B, ret allocated to t0, ~15 word diffs.
- B: separate early-return guards — 176 B, B test inverted to `bnezl`.
- C: `&&`-guarded + inner pal check + ternary — 172 B, 14-word diff (addu
  operands, both -l bits, mask reg/hoist).
- D: C + pointer entry subscript — identical to C.
- E: C + integer entry add — fixed the addu, 13-word diff.
- F: E + split `int start` + `$8` mask pin — **2-word diff** (only the B and C
  -l bits).
- G: F with nested `if`s instead of `&&` — same 2-word diff.
- h2 (expert): `if (palOk && texOk) return fi;` + trailing `return 0;` — 176 B, 22 diffs.
- h3 (expert): `register int result asm("$2")` accumulator — 26 diffs.
- **h1 (expert): nested ternary `palOk ? (texOk ? fi : 0) : 0` — 0 diffs, MATCH.**
- `sizeof(HudIconDef)` in place of the literal 8 re-verified as a match (the
  committed form).

## Shared changes (same commit)
- `code/include/hud.h`: added `HudFrame` (`{s16 hPal; s16 hTex;}`), renamed
  `HudHeap.pad_20` (u32) to `HudHeap.frames` (`HudFrame*`), added
  `#define HUD_RAM_ENTRY_FLAG 0x80000000`, and the `GetIconFrame` prototype.
  `pad_20` was referenced nowhere else, so the rename is layout-neutral
  (u32 and pointer are both 32-bit under this ABI).
