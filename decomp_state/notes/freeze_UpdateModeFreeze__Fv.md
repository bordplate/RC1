# UpdateModeFreeze__Fv (0x1FCE28, 0x8B8 = 558 words) — BLOCKED

Freeze/menu-mode state machine (7 outer `Freeze.mode` cases + a 23-way inner
`menuPostCallbackIndex` switch in mode 3). Sister of the blocked
`DrawDialogText__Fv` (same file, same `Freeze` global). INCLUDE_ASM at
`code/game/freeze.cpp:129`.

## Structure (fully decoded from `assets/boot_elf.elf` objdump)

Prologue (MATCHES): `addiu sp,-0x40`; saves `sq s1,16 / ra,48 / s2,32 / s0,0`;
`jal sound_update`; `if (Freeze.countdown) Freeze.countdown--;`. Then
`switch (Freeze.mode)` with a computed dispatch (jtbl_001E79A0 at vram 0x1E79A0,
data-seg foff 0xE8920 = VMA-0x80080). Both jtbls ARE emitted by the candidate
(Splat hex fields are the raw LE words, verified equal to boot_elf.elf).

Freeze (0x193300) high page is cached in **s1** (`lui v1,0x19; daddu s1,v1,0;`),
base in **v1/s0** (`addiu s0,s1,0x3300`). Fields: +0x04 countdown, +0x14
prevGameMode, +0x1C field_0x1C (state), +0x20 field_0x20, +0x24 field_0x24,
+0x28 restorePage, +0x00 mode.

Outer body text order (by address): **5, 4, 6, 1, 2, 0, 3**.
- mode 5 (0x1FCE84): `field_0x20++; if (func_001F96F8(0x5A)<field_0x20 && field_0x24) field_0x24--; if (func_001F96F8(0x78)<field_0x20 && (pressed&0x40) && currentLevelId==1){ GameMode=0; memcard_Save(0,-1); }`. The two calls use LITERALS 0x5A/0x78 — there is NO `frames` variable (the `a0=0x78` is hoisted into the first branch's delay slot and always executes).
- mode 4 (0x1FCF08): `if (pressed&0x10){ P=*(levelCamData+0x2080); v=P[0x34]; P[0x31]=0; P[0x94]=0; P[0x34]=v|1; func_001E93E8(); func_001E9440(levelCamData+0x1D00, levelCamData+0x1D10,0,1); GameMode=0; } else if (pressed&0x40) GameMode=0;` (P-ops are the fall-through).
- mode 6 (0x1FCF60): video-mode sub-state machine, `FadeToBlack(4)` (ONE arg, a0=4) at two sites, sets `requestedVideoMode` (u8, sb) 0/1.
- mode 1 (0x1FD09C): `D_0015EE48=2; if (pressed&0x10){ GameMode=0; levelCamData+0x160F |= 1; } if (pressed&0x40) GameMode=0;`
- mode 2 (0x1FD0E8): `D_0015EE48=2; if (pressed&0x40) GameMode=0;`
- mode 0 (0x1FD108): sequential if/else on field_0x1C: `==1`: `if(pressed&0x40) field_0x1C=2; else if(pressed&0x820) field_0x1C=3;` | `<2`(`==0`): `if(field_0x20<8) field_0x20++; else if(field_0x24<8) field_0x24++; else field_0x1C=1;` | `<4`: `if(field_0x24) field_0x24--; else if(field_0x20) field_0x20--; else { if(field_0x1C==2){ sprite=levelCamData+0x880; if(sprite!=-1){func_001FF570(sprite,0);sprite=-1;} func_001FF768(); if(func_001F96F8(0x1068)<levelCamData+0x19C){ if(currentLevelId==5)D_0015EE38++; else if(currentLevelId==0x10)D_0015EE3C++; } GameMode=0; func_001E9440(0x1D00,0x1D10,0,1);} else { GameMode=0; if((s16)rank(levelCamData+0x89A)>=3){ rank--(lhu/sh); bevel=*(levelCamData+0x890); bevel[0xBC]=3; } } }` | `>=4`: epilogue.
- mode 3 (0x1FD2B8): `field_0x20++; if (func_001F96F8(0x1E)<field_0x20 && field_0x24) field_0x24--; switch (menuPostCallbackIndex){...}` (jtbl_001E79C0, 23 entries, index `menuPostCallbackIndex-2`). Inner case text order: **2, 6, 13, 17/18/20/21, 16, 12→(3/5), 19, 23/24, 4, 9**; absent values → default → epilogue.

Epilogue (0x1FD698): `int mode = GameMode - 3; if ((u32)mode >= 2){ snd_ContinueAllSoundsInGroup(0x1D); music_Unpause__Fv(); snd_FlushSoundCommands(); }` (a TEMP — no store of `mode` back to GameMode). Then restore `ra/s2/s1/s0; jr ra`.

## What the candidate gets right

With default flags the candidate matches: the full prologue/frame (0x40), the
Freeze hi-page in s1, the countdown decrement, BOTH jtbls (23 + 7 entries,
correct targets), the outer case body order, the epilogue temp, the one-arg
`FadeToBlack(4)`, the `memcard_Save(0,-1)` gp-relative GameMode store, and the
rank/bevel signed-vs-unsigned loads in mode 0.

## The wall: pervasive fine-grained EGC 2.95.2 scheduling / RA tie-breaks

Best verified candidate (default flags) = **2240 bytes / 521 differing words**
(out of 558; 8 bytes too long). The structure is correct, but EGC makes a
different low-level choice in nearly every basic block, and the choices
accumulate:

1. **hi-page load temp**: original `lui v1,0x19; daddu s1,v1,0; addiu v1,v1,0x3300`
   (temp=v1); candidate `lui v0,0x19; daddu s1,v0,0; addiu v1,v0,0x3300`
   (temp=v0). End state identical (s1=hi, v1=base) but the differing temp at the
   first instruction seeds a register-allocation cascade through the body.
2. **delay-slot scheduling / register reuse**: e.g. mode 0 case 1 — the original
   hoists `pressed & 0x820` into the delay slot of the first branch, but the
   candidate schedules `li v0,2` (the `field_0x1C=2` constant) there and then
   computes `2 & 0x820` (v0 reused), which makes the `field_0x1C=3` path dead — a
   genuine logic difference, not just ordering.
3. **dead delay-slot load**: mode 3 emits an extra `lw v0,-32080(gp)` in a delay
   slot (0x1FD2DC) that the original's schedule does not reproduce.
4. **mode 4 pointer base**: candidate keeps `levelCamData+0x2080` (the P pointer)
   as the base register and derives `a0/a1 = base-0x380/-0x370`; original keeps
   `levelCamData` (base) and does `a0/a1 = base+0x1D00/+0x1D10`. Same addresses,
   different register materialization and body size.
5. Numerous per-case branch-direction and temp-register choices across the ~11
   inner case bodies.

## Attempts (all failed to converge)

- Applied every last-resort correction (case order 5,4,6,1,2,0,3; `u8[]`
  levelCamData to kill the ×4 offset scaling; signed `freeze_t` fields; epilogue
  temp no-store; `FadeToBlack(4)` one-arg; inner case order; sequential mode 0;
  signed `lh`/unsigned `lhu` rank; bevel pointer deref; `u8 requestedVideoMode`;
  `int memcard_Save(int,int)`).
- Removed the spurious `frames` local (mode 5 uses literals 0x5A/0x78) — cut
  8 bytes.
- Flipped mode 4 and mode 0 case 1 branch directions to the original's.
- `--flags=-fno-schedule-insns`: regresses to 2176 bytes / 519 diffs (too short).
- No `-mno-split-addresses` / section-attr / prototype / pin lever reproduces the
  original's per-block schedule; the function's 558-word body has too many
  independent tie-breaks to control from C source.

## Last-resort escalation

`last-resort-decompiler` (GPT-5.6 Sol) invoked once for this exact target. It
determined the target is **not** a single allocator wall but a set of concrete
source mistranslations (the 11 corrections above). All were applied; the
candidate went from a structural mismatch to 2240 B / 521 words but the
residual is the pervasive EGC 2.95.2 scheduling/RA tie-break class documented
above, which the recommendation's levers do not reach. No match. Reverted to
INCLUDE_ASM; full boot-ELF parity green. See blocked.json.

## Globals referenced (for a future attempt)

In-window (gp rel): GameMode 0x15F604, currentLevelId 0x15ED84, menuPostFlags
0x15EEB4, menuPostCallbackIndex 0x15EEB0, D_0015EE48/38/3C, bootLevelActive
0x15F5E8, requestedVideoMode 0x16034C. Out-of-window (absolute, need `.data`):
padState.pressedButtons 0x13CAE4, D_0013E05A 0x13E05A (u16), levelCamData
0x13F350 (u8 base), pauseCurrentActionList 0x1D5BF8, Freeze 0x193300. levelCamData
byte-offsets: +0x880 sprite-id, +0x19C frame-target, +0x890 bevel ptr, +0x89A
rank u16, +0x160F flag, +0x1D00/+0x1D10 func_001E9440 args, +0x2080 → P.
