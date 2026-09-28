# findMapSlot (func_002050A0, code/game/map.cpp)

- 68 bytes (0x44) at vram `0x2050A0`. `int findMapSlot(int key)`, cfront-mangled
  `findMapSlot__Fi` (verified with `nm` on map.o).
- Semantics: scan the five-slot map pool at `mapSlotPool` and return the index
  of the first slot whose key equals `key` **and** whose pointer is non-zero;
  otherwise -1.

## Data layout (base `D_001A00F0` -> `mapSlotPool`)

Investigated through the callers (map.cpp `0x2050E8`, `0x205000`, pause
`0x21BE60`) and the Lombyte reference (`src/textbin/fun_002050a0.c`):

- `0x224` `currentId` (map/level id; callers pass `currentId + 0x100` as the key).
- `0x228` `pauseMemoryCardState` (already a named symbol in symbols.txt).
- `0x278` `void* slotPtr[5]` — pointer to each slot's map data (copied as a
  pointer in `0x205000`; null means the slot is free).
- `0x28C` `s32 slotKey[5]` — the slot key; `findMapSlot` compares against this.
  Set to -1 on free, `^ 0x1000` / `| 0x1000` on the pause path.
- `0x2A4` `s32` value words (parallel array, not needed by this function).

`slotPtr[i]` sits exactly `MAP_SLOT_COUNT` (5) words before `slotKey[i]`, so the
original's `lw v0,-0x14(v1)` (v1 = `&slotKey[i]`) reads the backing pointer as
`p[-MAP_SLOT_COUNT]`.

## Codegen notes

- Two-step base+offset load: the prologue is `lui v0,%hi(base); move a1,0;
  addiu v0,v0,%lo(base); addiu v1,v0,0x28C`. A direct struct-member access
  (`s32* p = mapSlotPool.slotKey;`) makes EGC fold the base and offset into one
  `lui`/`addiu` that drops `%lo(base)` (wrong address) plus an extra `nop`.
  Going through a pointer (`struct MapSlotPool* pool = &mapSlotPool; s32* p =
  pool->slotKey;`) keeps the base a live register value, so EGC emits the
  correct two-step `lui`+`addiu` (base) then `addiu` (+0x28C).
- `count` is declared first so EGC schedules `move a1,zero` second, matching the
  original prologue order.
- The single `count++` is duplicated by EGC into the taken-delay slots of both
  `beqzl` (p[-COUNT]==0) and `bnel` (*p!=key); the `p++` lands in the `bnez`
  loop-back delay slot. Standard branch-likely taken-delay duplication — one
  source increment each, no pinning needed.

## Replacement (map.cpp, at the old INCLUDE_ASM site)

```cpp
#define MAP_SLOT_COUNT 5
struct MapSlotPool {
    u8 pad[0x278];
    void* slotPtr[MAP_SLOT_COUNT];
    s32 slotKey[MAP_SLOT_COUNT];
};
extern struct MapSlotPool mapSlotPool;

int findMapSlot(int key) {
    int count = 0;
    struct MapSlotPool* pool = &mapSlotPool;
    s32* p = pool->slotKey;
    do {
        if (p[-MAP_SLOT_COUNT] != 0 && *p == key)
            return count;
        count++;
        p++;
    } while (count < MAP_SLOT_COUNT);
    return -1;
}
```

`mapSlotPool = 0x1A00F0` and `findMapSlot__Fi = 0x2050A0` added to
config/symbols.txt. Renaming `D_001A00F0` -> `mapSlotPool` also updates the
references in the sibling INCLUDE_ASM functions (map, menu, help) — they all
resolve to the same linker symbol.

## Verification

Function bytes 0x2050A0..0x2050E3 compare equal to the original (68/68). Clean
`make clean && make split && make -j2` + `cmp build/boot_elf.elf
assets/boot_elf.elf` passes.
