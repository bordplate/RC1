# startlevel__Fv (0x1E9658, 0x45C, 279 words) — matched

## Correction and successful retry, 2026-09-22

The earlier conclusion of a compiler wall was too strong. The replacement in
`code/game/bmain.cpp` matches all 1116 bytes with the default EEGCC/SN flags.
The full integrated build also compares byte-for-byte with the original ELF.
No subagents or escalation were used in this retry (explicit user request).

### What unlocked the match

1. Reconstructing from assembly with signed `decodeMode`, signed decode-buffer
   arithmetic and `.data` movie records immediately fixed the old BSS-loop,
   negative-constant, movie-load, and decode-register differences. A simple
   `for (int i = 0; i < levelBssEnd - levelBssStart; ++i)` produces the two
   preheader copies and the original operand order without any asm.
2. **Constrain the initializer, not the entire loop variable lifetime.**

   ```cpp
   register int initialState asm("$16") = 0;
   int prevState;
   VU1_initChain();
   asm volatile("" : "=r"(prevState) : "0"(initialState));
   ```

   The zero-byte tied transfer coalesces `prevState` onto s0. The remaining
   five loop values naturally take s1..s5 and the frame stays 0x70. Putting
   the transfer BEFORE the call prevents the initializer from occupying its
   delay slot; making it nonvolatile lets EGC treat the empty asm as a delay
   instruction and SN rejects the following jal in that slot. Pinning
   `prevState` itself inhibits invariant motion (hoists a constant 1 into s5
   and loses the cached base+0x10). Pinning the initializer avoids that effect.
3. A short-lived `BootAssetTable*` pinned to v0 reproduces the bank-location
   load's base register. The table's shared type declares the +0x14E0 field;
   a raw char-array expression folds the offset into the symbol's HI/LO pair.
4. Sound-bank assignment uses volatile local views and volatile publication
   stores to reproduce the original ordering. A zero-byte memory barrier
   followed by the last ordinary bank store lets that store fill the
   transition call's delay slot. Making the last store volatile adds a NOP.
   No emitted asm instructions, numeric data addresses, patched bytes, or
   compiler flag changes are used.

### Layout and naming corrections established by this function

- `D_24135F` was **not a real data object**: it equals the ELF `.text` end
  (0x23D360) plus 0x3FFF. The source now rounds the linker-provided
  `text_VRAM_END` up to 0x4000 and adds the 0x2C0000 workspace size. This
  remains relocatable and emits the original symbolic HI/LO pair.
- `currentVuChainIndex` (0x15ED84) is `currentLevelId`: save/menu callers
  index per-level save data with it. startlevel saves it into `spaceLoadId`
  and writes -1. `vuChain_getCurrent` selects its chain by **level**, not by
  double-buffer index; its declaration/comment were corrected too.
- `streamState` (0x13C940) is the already-known `padState` used by
  `UpdatePad(PAD&)`. The shared PAD declaration now includes the reset fields
  and +0x1A4 pressed-buttons mask. The reset helper is `pad_resetState`.
- `NTSCProgressive` (0x15ED80) had its meaning reversed: InitOnce derives it
  from the disc region, and the GS reset call selects NTSC=2 for zero, PAL=3
  for nonzero. It is now `videoModePal`; boot movie and debug-stream names
  reflect the same mapping.
- `bootAssets` (formerly debugFontLoadInfo) is a resident asset table, shared
  by bloaders and bmain. The two sound-definition arrays live in the boot
  overlay data at 0x186100/0x1861E0; their +0x1C fields are 989snd bank handles.
- Memory-card polling checks card type/format, an existing save directory,
  and free space; it returns the localized warning-image selector.
- Related callees were named for their established behavior, with natural
  C++ linkage for game functions and C linkage for the SDK routines.

### Verification

- Standalone p14 and p15: 1116 original/candidate bytes, zero word differences.
- Integrated `make split && make -j2 && cmp`: passed with the replacement
  active, shared declarations, recovered function names and linker end symbol.
- Final clean `make clean`, `make split`, `make -j2`, and full ELF `cmp`
  pass. `tu_assembler_diff.py` verifies all 219 sized functions across bmain,
  bloaders, draw, framebuf, hud, memcard, pad, sound, stream, and vuchain.
  This includes the C replacements and the surrounding assembly functions.
- The per-TU checker initially misclassified two zero-sized interior GNU
  assembly labels in draw.o as missing functions. It now excludes only aliases
  covered by a sized function and rejects independent unsized entries. Two
  focused regression tests pass; no original/candidate bytes are ignored by
  the containing-function check or the full-image oracle.
- `SoundDef` is now shared in `code/include/sound.h`; its former field_0x1C
  is the bank handle. Existing range/volume/pitch fields and users are retained.
- Status count after the match: 678 nonmatching INCLUDE_ASM entries remain.

## Historical failed attempt (superseded by the correction above)

## The six callee-saved loop values

`prevState`(int), `base`(u32=0x500000), `state`(int), `frame`(int),
`bufA`(u32=base+0x10), `&streamState`(ptr) live across the state loop.
Original sregs: prevState=s0, base=s1, state=s2, frame=s3, bufA=s4,
&streamState=s5 (frame 0x70, save order `sq ra,s5,s4,s3,s2,s1,s0`).

The level base is `((u32)&D_24135F & 0xFFFFC000) + 0x2C0000` where
`D_24135F = 0x24135F` is a real linkable abs symbol in the level code region;
the `&D_24135F` form is the ONLY C form that stops EGC folding the address
into `lui a0,0x50` (every pure-constant form folds). The original keeps it as
`and v0,v0,v1; addu s1,v0,a1` (v0 intermediate, 0x2C0000 in a1).

## Rotation is an allocator tie-break (not source/type/flag)

Natural EGC 2.95.2 allocates the trio in a 3-cycle rotation:
base=s0, state=s1, prevState=s2 — the opposite of the original. Tried and
failed to fix from C:
- every declaration-order / type permutation of the six values;
- `-fno-schedule-insns` and `-fno-schedule-insns2`;
- the destructive `prevState = base+0x40; if (state==1) prevState = bufA;`
  (required for the `movz s0,s4,v0` regardless).

## Hard-register pinning: fixes rotation, breaks the frame

`register int prevState __asm__("$16") = 0;` (s0) +
`register int state __asm__("$18");` (s2) puts base naturally on s1 and fixes
the rotation, BUT forces an extra callee-saved register:
- two-pin alone: frame 0x80 (s6 hoisted), 45 diff blocks, 275 words.
- pin all six ($16-$21): frame 0xA0 (s7/s8 hoisted for &bootEntryTable /
  &loadingSoundBankMsg bases); also moves the base computation INTO s1 (early
  &D_24135F / &streamState materialization) and turns `bufA=base+0x10` into
  `ori s4,s1,0x10` (not `addiu`).
No subset gives the correct rotation AND the 0x70 frame.

## Other independent diffs (present even with the two-pin)

1. bss zero-loop: original has two preheader copies `move a2,v1; move a1,v0`
   and `addu v1,a0,a2`; the C `for`/`do-while` forms emit the count/pointer
   directly into a1/a2 and `addu v1,a2,a0`.
2. Signed constants: `decodeMode=-1`, `pendingSpaceLoad=-1`, and the
   `(dst+0x3F)&~0x3F` mask emit `lui/ori` (2 instrs) where the original has
   single `li v,-1` / `li v,-64` (unsigned `u32` decl vs signed `int`).
3. videoDecodeDst: held in dead s0 (prevState's reg) instead of the original
   a2; the original's a2 dies at the `func_0023A3B8` call.
4. intro-movie {src,size}: two independent self-based loads vs the original's
   single CSE'd base (`lui v0,0x14; addiu v1,v0,-27784`).

## Escalations

- expert (GPT-6 Astra): recommended the hard-register pinning.
- last-resort GPT-5.6 Sol used: top recommendation = pin only prevState=$16 +
  state=$18, shorten decodeDst lifetime, make decodeMode/pendingSpaceLoad/
  videoDecodeDst signed, `.data` the intro-movie structs, explicit bss
  do-while. Applied and mechanically tested: two-pin → 0x80 frame / 45 diffs;
  full combo → 0xA0 frame. Neither matches.

Conclusion: genuine EGC 2.95.2 codegen wall (global RA tie-break plus many
independent scheduling/splitting differences in a 279-instr function).
