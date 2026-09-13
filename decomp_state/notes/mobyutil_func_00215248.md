# func_00215248 -> moby_freeVarSlots (code/game/mobyutil.cpp)

Free-capacity calculator built on the two pool counters from
notes/mobyutil_func_00215300.md.

- Original: 72 bytes (`0x48`) at vram `0x215248`, an unmangled symbol
  (callers `jal func_00215248` directly: the moby state machine at
  0x216C48 cases 4/5 and the debug HUD at 0x21EB20).
- Semantics:
  ```cpp
  enum {
      MOBY_POOL_MAX_SLOTS = 0x28,
      MOBY_POOL_SLOTS_PER_ENTRY = 4,
  };
  int moby_freeVarSlots(void) {
      int n = func_00215290() - MOBY_POOL_SLOTS_PER_ENTRY * func_00215300();
      if (n < 0)  n = 0;
      if (n > MOBY_POOL_MAX_SLOTS) n = MOBY_POOL_MAX_SLOTS;
      return n;
  }
  ```
  (Named constants per the style guide; both fold to the same literals,
  codegen verified unchanged.)
  `func_00215290` (still INCLUDE_ASM) counts the active entries of the
  first 0x38 bytes of pool 0x14BEC0 clamped to [0, 0x28]; `func_00215300`
  counts pool 0x13E520 clamped to [0, 0xA]. The free value is
  `active(0x14BEC0) - 4 * active(0x13E520)` clamped to [0, 0x28]; the max
  of the 0x13E520 counter (0xA) times 4 is exactly the 0x28 cap, i.e. each
  active 0x13E520 entry consumes 4 slots of the 0x14BEC0 pool.
- Uses: 0x216C48 uses the result as a slot-index cap for two state-entry
  conditions (`idx <= free`, `3 < free`); 0x21EB20 (debug HUD showing the
  "ON"/"O0"/"O1" labels) displays all three values: `func_00215290()`,
  `func_00215300() << 2`, and `moby_freeVarSlots()` at (0xF0, 0x1D/0x36/
  0x54).
- Naming: the pools' entity domains are runtime-loaded (same finding as the
  sibling note), so the pool address names are retained. The calculator's
  role (free slot count) is established, hence `moby_freeVarSlots` with an
  `asm("func_00215248")` label (unmangled symbol; see sibling note for the
  linkage reasoning).
- Match: matched on the first attempt with default mobyutil.o flags
  (`-G8 -O2`). Codegen EGC reproduces exactly:
  - first call `jal func_00215290` with the `sq s0,0(sp)` prologue save in
    its delay slot; second `jal func_00215300` with `move s0,v0` (saving
    the first result across the second call) in its delay slot;
  - `4 *` lowers to `sll v0,v0,2`, the subtraction to `subu s0,s0,v0`
    (result kept in s0);
  - the clamp tail matches the sibling's pattern with n in s0:
    `li v1,-1; slt v1,v1,s0; movz s0,zero,v1` (max) then
    `li v0,0x28; slti a0,s0,0x29; movn v0,s0,a0` (min), with `lq ra`
    hoisted before the comparisons and `lq s0` before `jr ra`;
  - 0x20 frame, ra at 0x10(sp), s0 at 0(sp).
- Verification: full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`
  byte-for-byte (also after `make split`, which moved the .s to
  matchings/ and dropped the `.NON_MATCHING` alias); decomp count 705 -> 704.
