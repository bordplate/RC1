# Hud_SendResidentBank__FiPcb (0x1FF128) — BLOCKED

Mangled `Hud_SendResidentBank__FiPcb` = `(int bank, char* ram, bool immediate)`.
Sends one bank's resident HUD textures from GS RAM. Deadlocked sibling:
`reference/dl/game_dl/hud.cpp` L131 `Hud_SendResidentBank(int bank, char *ram,
bool immediate)`. Caller: FUN_00202a98 @0x202be8 calls
`Hud_SendResidentBank(0, DAT_001940c8+0x60000, 1)`.

## Semantics (verified against Deadlocked + original objdump)

```
if (hudHeap.header->bankLoad[bank] == 0) LinkHudBank(0, ram);   // a0=0 in jal delay slot
gsloc  = (int)frameBufferBase;        // LOCAL, loaded once, never written back
low    = (bank == 0) ? 0 : hudHeap.header->texCount[bank - 1];
count  = hudHeap.header->texCount[bank];
for (i = low; i < count; i++) {
    gsram = gsloc >> 8;               // SIGNED sra, computed once per iter
    Hud_sendTexture((char*)texs[i].ram, gsram, 0x1B, texs[i].uLog,
                    texs[i].vLog, immediate);
    gsloc  += (1 << (texs[i].uLog + texs[i].vLog)) * 4;   // sllv(s7=1) then sll 2
    texs[i].gsram = gsram;            // sh in loop-back bnez delay slot
}
```

- `Hud_sendTexture(char*,int,int,int,int,int)` @0x200B10. `LinkHudBank(int,char*)` matched @0x1FEFC0.
- `frameBufferBase` @0x15EE88 (in GP window, gp=0x166C00, GPREL16 off -0x7D78) but the
  original accesses it ABSOLUTELY: `lui s4,0x16; lw s4,-4472(s4)` at the merge point 0x1FF184.
- The `gsloc` load is a plain absolute load at the bankLoad-check merge point, NOT in a delay slot.

## Data layout (verified)

- `hudHeap` @0x19A3E8: +0x18 `HudHeader* volatile header`, +0x24 `HudFrameTex* texs`, +0x28 pals.
- `HudHeader`: +0x14 palCount[8], **+0x34 texCount[8] (int)** = word offset **0xD** (NOT 0xC —
  the critical bug the last-resort agent caught), +0x74 bankLoad[8] (u32) = word offset 0x1D.
- `HudFrameTex` (8B): +0x00 u32 ram, +0x04 u16 gsram, +0x06 u8 uLog, +0x07 u8 vLog.
- Address form: original computes `(header + bank*4) + OFFSET` (base ptr first),
  NOT `header + (bank*4 + OFFSET)`.

## Original register map (frame 0xA0)

s0=bank→(uLog+vLog), s1=hi(&hudHeap), s2=gsloc>>8 (loop), s3=low/i, s4=gsloc,
s5=count, s6=&hudHeap full (loop), s7=1, s8=immediate.
Prologue save order: **s1, s8, s0, ra, s7, s6, s5, s4, s3, s2**.
immediate (`move s8,a2`) is in the bankLoad-bnez DELAY slot (0x1FF178).

## Progress

Loop body and total size (312 bytes) match byte-for-byte once the texCount word offset is 0xD,
the pointer locals are dropped, and a `size = 1 << (uLog+vLog)` local is added (the `sllv`+`sll 2`
increment then schedules exactly). `working/hud_sendresidentbank/candidate_v3.cpp` is the best
candidate: **312 bytes, 33-40 word diffs, ALL in the prologue** (loop from ~0x1FF1D0 matches).

## Blocker (durable prologue allocator tie-breaks)

The prologue differences are all EGC 2.95.2 register-allocation/scheduler choices that could not
be steered with source restructuring, flags, barriers, or pins:

1. **&hudHeap caching.** Original keeps the HI in `s1` and recomputes the full address into a
   temp (v0/a0) for each of the three header loads. The candidate caches the FULL in `s1` and the
   hi in `s2` (one extra saved register). This cascades into the save order.
2. **Save order.** Candidate `s8, s2, s1, s0, ra, s7, s6, s5, s4, s3` vs original
   `s1, s8, s0, ra, s7, s6, s5, s4, s3, s2`.
3. **gsloc load position.** Original: plain absolute load at the bankLoad-check merge point.
   Candidate: hoisted into the low-computation branch's delay slot as GPREL16 (`lw s4,0(gp)`).
4. **immediate save.** Original: `move s8,a2` in the bankLoad-bnez delay slot. Candidate: in the
   prologue.
5. **bank*4 register.** Original `a0`/`a1`; candidate `v0`/`v1`.

## Attempts (all nonmatching)

- probe2/3: `gsloc` as `u32` (before/after count) → 288/296 B, 75-76 diffs, `srl` (unsigned),
  gsloc GPREL16 in a delay slot.
- probe_data: `frameBufferBase` with `__attribute__((section(".data")))` → forces absolute but
  splits the lui/lw across delay slots and grows the frame to 0xB0 (75 diffs).
- probe_int: `gsloc` as `int` → correct `sra`, 296 B / 75 diffs.
- probe_pin: `register int gsloc asm("$20")` → 296 B / 74 diffs.
- probe_flag: `-fno-schedule-insns` 288/76, `-fno-schedule-insns2` 288/73.
- probe_cse: `-fno-cse-follow-jumps` 40, `-fno-cse-skip-blocks` 33, both 33 (best).
- probe_v2: named texCount fields + `size` local + input-only barrier `asm("" : : "r"(gsloc))`
  → loop matches but gsloc DUPLICATED (delay slot + merge), 320 B / 71 diffs.
- probe_v3: v2 minus the barrier → **312 B, 33-40 diffs** (best; loop matches).
- probe_v4: tied barrier `asm("" : "+r"(gsloc))` → 320 B / 78 (worse).
- probe_v5: flipped low-computation branch direction → 312 B / 42 (worse).

## Last-resort escalation

`last-resort-decompiler` invoked. Its concrete recommendations were applied: correct the texCount
word offset to 0xD (named fields), drop the pointer locals, add the `size` local, try both barrier
forms and the CSE flag matrix. Those fixed the loop and the size but not the prologue allocation.
Per the agent's own blocker standard (corrected natural-field/no-`t`/named-`size` candidate fails
under SN with both barrier forms and the CSE matrix), this is a durable prologue allocator blocker.
Note: hud.o is a GNU-assembler TU in the Makefile; a GNU probe (`--assembler gnu`) is 296 B and
also nonmatching, so the mismatch is not assembler-specific.

## Resume

If revisiting: the loop and size are solved (candidate_v3). Only the prologue allocation remains.
Look for a way to make EGC keep the &hudHeap hi in a single saved register (s1) and recompute the
full into a temp (the split-address form) instead of caching the full — that single change likely
resolves the save order and most of the prologue diffs. Also try steering the gsloc load out of the
low-computation delay slot and into the bankLoad merge point, and the immediate save into the
bankLoad-bnez delay slot.
