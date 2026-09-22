# startlevel__Fv (0x1E9658, 0x45C, 279 words) — blocked

Boot/main-menu level entry. Reverted to `INCLUDE_ASM`. A full C candidate
reproduces most of the body but leaves ~45 distributed diff blocks. See
`blocked.json` entry `code/game/bmain.cpp:5:startlevel__Fv` for the durable
blocker. This note records the reusable codegen findings so a future attempt
does not repeat the matrix.

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
