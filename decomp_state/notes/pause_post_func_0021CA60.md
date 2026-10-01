# func_0021CA60 → pause_stageSpriteLists (0x21CA60, 0x34 bytes)

Matched 2026-10-01. Clean build + `cmp build/boot_elf.elf assets/boot_elf.elf`
byte-identical; `tu_assembler_diff` pause_post.o 29/29 and pause_post2.o
121/121; count 609 → 608.

## Behavior

Pause-menu post-callback, entry 4 of the 0x1CF368 callback table
(0x21C7A0, 0x220B20, 0x21C9C8, 0x21CA60 — all still nonmatching except this
one). Copies the menu item's 8-entry sprite-list pointer array
(`mode->spriteLists`, +0x30..+0x4F) into the staging buffer
`pauseIopSpriteLists` (0x141EA0, zero-initialized 8×u32 in core.data) and
returns 0. No stack frame; only a0/a1/v0/v1.

Direction matters: the sibling `func_0021C9C8` (queue) copies the buffer BACK
into an item (global → `mode->spriteLists`) and then walks entries 0..7
counting up to 7 consecutive non-null lists (counter in `mode+0x50`, stored
mod 8). The card-screen setup `func_00226B08` (pause_post2.cpp:375, queue)
ships the buffer to IOP RAM 0x70000150 (`FUN_001f9838(0x70000150, 0x141EA0,
0x20)`) as part of a 0x1A0-byte IOP screen-data block (0x70000000 is the
EE's 16KB IOP RAM window), and later rewrites the buffer from the IOP copy
gated by the enable-flag bytes at 0x13D4C0.

## Layout established

`PauseSpriteListMode` (pause.h) is now `{ u8 pad[0x30]; u32
spriteLists[8]; }`. The previous form (`pad[0x34]; u32 spriteList;`) and its
"7-entry pointer array" comment were wrong: both this function and the
0x21C9C8 scan prove 8 entries (+0x30..+0x4F); the `li 7` count starts at
N-1 for an 8-iteration do-while. Element 1 (+0x34) is the list the select
callbacks use; the two existing users were updated to `spriteLists[1]`
(`pause_selectSpriteList` pause_post.cpp, `pause_setFirstSpriteTag`
pause_post2.cpp) — same offset/size, re-verified matching. `+0x50` (the
counter used by 0x21C9C8) is intentionally NOT in the struct yet; it belongs
to that sibling's investigation.

## Matching form

Default flags (`-G8 -O2 -ffast-math -fno-exceptions`, SN assembler), first
probe matched 52/52:

```cpp
int pause_stageSpriteLists(PauseSpriteListMode* mode) {
    u32* src = mode->spriteLists;
    u32* dst = pauseIopSpriteLists;
    int count = PAUSE_SPRITE_LIST_COUNT - 1;
    do {
        *dst = *src;
        src++;
        dst++;
        count--;
    } while (count >= 0);
    return 0;
}
```

EGC emits the original exactly: `lui v0,%hi(pauseIopSpriteLists)` then the
interleaved `addiu a0,a0,48`, `addiu a1,v0,%lo`, `li v1,7`, the
`lw/addiu -1/addiu 4/sw/nop/bgez` loop with the single `dst++` in the bgez
delay slot, and the `jr ra; move v0,zero` tail. `PAUSE_SPRITE_LIST_COUNT - 1`
folds to the same `li v1,7` (probe-verified); the constant names the 8-entry
array size shared by the struct field, the global, and the loop.

## Symbols

- `pauseIopSpriteLists = 0x00141EA0;` (data symbol; was D_00141EA0 — the
  generated .s of 0x21C9C8/0x226B08 now use the name).
- `pause_stageSpriteLists__FP19PauseSpriteListMode = 0x0021CA60;` — the
  MANGLED name is what goes in symbols.txt here: the generated data table
  (`code/_generated/build/data/data.data.s:110794`) emits `.word
  <symbol>` for the 0x1CF374 pointer, so the name must equal the exact
  linker symbol the C++ object defines (same convention as
  `menu_post_preparePageOpen__Fv`). An unmangled `pause_stageSpriteLists`
  entry instead fails the link with `undefined reference to
  'pause_stageSpriteLists'` because the object exports the mangled symbol.
  The original binary is stripped, so no C-linkage evidence exists; a
  natural C++ function is correct, not `extern "C"`.
