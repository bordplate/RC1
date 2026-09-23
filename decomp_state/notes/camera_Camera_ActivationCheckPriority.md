# Camera_ActivationCheckPriority (0x1EC210) — BLOCKED

Target: `code/game/camera.cpp` — `int Camera_ActivationCheckPriority(UpdateCam* pCam,
UpdateCam* pCompare)` (C linkage, unmangled; called by `UpdateAllCameras__Fi` via a
`jal` + R_MIPS_26). Original is 114 words (0x1EC210..0x1EC3D8). Sibling
`func_001EBF10` (Camera_switchToNewCam) is blocked on the same register-allocation wall.

## Semantics (Ghidra + Deadlocked ref reference/dl/game_dl/camera.cpp:957)
1. `if pCam->activation.priority (u8 @pCam+0x7C) == 0: return 0`
2. `pf = lvlCamVtbl[pCam->funcIdx(+0x8C)].activationCheck(+0x4)`; if pf:
   `r = pf(pCam,pCompare)`; `if r==-1 return 0; if r==1 return 1`.
3. `switch (pCam->activationType (int @pCam+0x74))`:
   - 0:      `if !pCompare return 1; if pCompare->deactivate(+0x7E)!=0 return 1;
             return pCompare->priority < pCam->priority`
   - 1,2:    `if pCam->activate(+0x7D)==0 return 0;` then same as 0
   - 3,5,6:  `return 0`
   - 4:      `pVar=importCameraTable[pCam->importCameraIdx(+0x84)].pVar(+0x1C);
             if pCompare && pCompare->deactivate==0 && pCam->pri<=pCompare->pri return 0;
             return func_00214720(&heroCamData.pos(0x13F3D0), *(int*)(pVar+0xC))!=0`
   - 7:      `if pCam->collMode(+0x86)!=heroCamData.desiredCam(0x1415D4) return 0;
             if pCompare->deactivate==0 && pCam->pri<=pCompare->pri return 0;
             if pCam->collMode!=3 return 1;
             pVar=importCameraTable[idx].pVar; grindIdx=*(int*)(pVar+0x24);
             if grindIdx>=0: if heroCamData.pPath(0x13F8B0)!=grindPaths[grindIdx].pPath(+0x10)
                           return 0; if heroCamData.eopTime(0x13F8C0)!=0 return 0; return 1;
             return 1`
   - default(>=8): `return 0`

`func_00214720` (code/game/mobyutil.cpp, INCLUDE_ASM) = Deadlocked `MB_IsPntInCuboid`.

## Confirmed layout (kept in source — the UpdateCam field renames are committed)
- `UpdateCam+0x74` = `int activationType` (switch value), `+0x7C` = `u8 priority`,
  `+0x7D` = `u8 activate`, `+0x7E` = `s16 deactivate`. priority/activate are UNSIGNED
  (loaded `lbu`, compared `sltu`); deactivate is `s16` (`lh`).
- heroCamData @0x13F350 (core.data): +0x80 pos(vec4), +0x560 pPath, +0x570 eopTime,
  +0x2284 desiredCam. GrindPath entries are 0x20 bytes (index scales `sll 5`), pPath +0x10.
- importCameraTable @0x15EF90 (absolute self-based lui/lw), grindPaths @0x15EF00
  (GP-relative `lw r,-0x7CC0(gp)`, needs `.extern` seed), lvlCamVtbl (absolute).
- Original regs: s1=pCam, s0=pCompare, s2=(u8*)pCam+0x74, $6=heroCamData base; frame 0x40.
  The `addiu s2,s1,116` sits in the first `beqz` delay slot.
- 8-entry jump table @0x1E7730 (in the generated `.data`, data.data.o): case0=0x1ec2b0,
  case1=case2=0x1ec2a4, case3=case5=case6=0x1ec3b8, case4=0x1ec2d4, case7=0x1ec330.
  Dispatch: `lui v0,0x1e; addiu v0,v0,0x7730`.

## BLOCKER A — switch jump table lands in an orphan .rodata (infrastructure)
This is the FIRST `switch` in all decompiled (non-ASM) C, so there is no project
precedent. EGC (EEGCC 2.95.2 SN 2.73a; -G8 -O2 -ffast-math -fno-exceptions -snas) emits
the 8-case jump table as a NEW `.rodata` section (32 bytes) inside camera.o. The Splat
linker script (SCUS_971.99.ld) references only `camera.o(.text)` — there is no
`camera.o(.rodata)` input — so it is an ORPHAN section: the link yields a MALFORMED ELF
("section extending past end of file", objdump "file format not recognized"), the file
grows 48 bytes, and ~280KB of scattered byte diffs appear. The original table bytes live
in data.data.o(.data) @0x1E7730; my compiler generates its OWN table @0x24D370 and
references that (wrong section AND wrong address — word 30/32: orig lui 0x1e/addiu
0x7730 vs mine lui 0x24/addiu 0xd370).

Expert (GPT-6 Astra) rated the structural fix as: Splat data boundary split at the table
(file offsets 0xE86B0..0xE86D0 / VRAM 0x1E7730..0x1E7750) so the output `.data` becomes
`data_before.data.o(.data); camera.o(.rodata); data_after.data.o(.data)`, with the table
bytes removed from data.data.o, plus linker ASSERTs on the slot. Invasive (changes the
generated data split + link), and only pays off if the body also matches. Not applied.

## BLOCKER B — body codegen mismatch (register allocation / CFG scheduling)
Even ignoring the table pointer, the body does not match. Best candidate (u8* base form)
was 118 words vs 114 (4 extra) with ~73 word-diffs:
- Prologue store/move/base-creation SCHEDULING (words 1-17): orig
  `sq s2,32 / sq s1,16 / sq s0,0 / move s1,a0 / sq ra,48 / move s0,a1 / lbu / beqz /
  addiu s2,s1,116`; EGC reorders these (s2 store late, move s0 late).
- Register allocation of the two priority loads (orig: pCompare->pri->v1 via s0+0x7C,
  pCam->pri->v1->v0 via s2+8, then `sltu v0,v0,v1`; EGC swaps the registers).
- 4-word size growth (partly GrindPath stride: a 0x14 struct gives `li 20; mult` where
  the original uses `sll idx,5`; fixed by a 0x20 struct, but the total still exceeded).
- linked-branch deltas (bltz vs bltzl, beqz vs beqzl).
NOTE: word 48 is `sltu` in BOTH (funct 0x2b) — an earlier read of it as `slt` was wrong;
the delta there is register allocation, not signedness. A standalone EGC probe of
`u8 a,b; return a<b;` yields sltu, so the in-function compare is a pure allocation issue.

## Attempts (all non-matching; parity-safe, none regressed the build)
1. Natural struct-access C: no s2 base, frame 0x30 — 111 diffs.
2. `char* act` base: induced s2/0x40 prologue but SIGNED -> `lb` loads + slt canonicalization.
3. `u8* act = (u8*)pCam+0x74` + `switch(*(int*)act)` + plain u8 compares: fixed lb->lbu,
   best body (73 diffs, 118 words) but introduced the .rodata orphan (BLOCKER A).
4. last-resort-decompiler (GPT-5.6 Sol) recommended: GrindPath 0x20 (applied; gave the
   sll-5 stride), move `act` before the priority test + input-only barrier, reconstruct a
   shared boolean-normalization block via `int ret;`+`break` for cases 0/1/2/4, cache
   `collMode` in case 7, repolarity the case-7 tail. Applied all -> WORSE (90 diffs,
   120 words): the `ret`+`break` shape does not reproduce the original CFG and adds
   instructions. Reverted.
5. -fno-schedule-insns: did not remove the linked-branch deltas.

## Escalations (both invoked on this exact target, 2026-09-23)
- expert (GPT-6 Astra): the .rodata placement is addressable via the data split (BLOCKER A
  fix above); the sltu was a misdiagnosis (already sltu); recommended the GrindPath/collMode/
  shared-normalization source fixes.
- last-resort-decompiler (GPT-5.6 Sol): corrected word 48 to sltu, reversed the documented
  priority-load allocation, gave the four source fixes + the exact .data split boundaries
  (0xE86B0..0xE86D0). Source fixes applied and mechanically diffed -> worse (90/120). The
  .data split was NOT applied (invasive; the body does not match anyway).

## Resolution
Retain INCLUDE_ASM (full-ELF parity preserved). The UpdateCam field renames
(activationType / u8 priority / u8 activate) are correct and kept. To revisit: (1) solve
BLOCKER A with the .data split so the generated table owns 0x1E7730, then (2) close the
BLOCKER B register-allocation/CFG-scheduling gap — the same wall as func_001EBF10.
