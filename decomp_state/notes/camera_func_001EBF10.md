# camera.cpp func_001EBF10 — Camera_switchToNewCam (0x1EBF10, 0x300, 192 instrs)

Status: BLOCKED (2026-09-23) — EGC 2.95.2 register-allocation / block-schedule wall.
INCLUDE_ASM retained (full-ELF parity preserved). See blocked.json entry.

## Identity
- Called only from UpdateAllCameras__Fi (0x1EC420) after the priority check picks a
  winner slot. Deadlocked names this `Camera_switchToNewCam(UpdateCam*, bool)`
  (RC1 predates the trailing bool param).
- Stages the switch to pNewCam: dispatches on the current camera's `deactivate`
  (s16, UpdateCam+0x7E) and the import-camera flag byte, optionally copies the
  transform, retargets currentCamera's UpdateCam slots + control buffers, updates
  the CamBlender state.

## Verified data layout (now in code/include/camera.h + camera.cpp)
- UpdateCam (0xA0): mtx0/1/2 @0x00/0x10/0x20, posQuad @0x30 (each CameraQuad =
  `unsigned int __attribute__((mode(TI)))`, 16 bytes), lPos[3] f32 @0x64, control
  u32 @0x70, activation{blendSpeed f32 @0x78, priority @0x7C, activate u8 @0x7D,
  deactivate s16 @0x7E}, importCameraIdx @0x84, collMode @0x86, funcIdx @0x8C,
  active u8 @0x8E.
- struct Camera (0x3A0): pos/posY/posZ f32 @0x140, pCurrentUpdCam u32 @0x180,
  pLastUpdCam u32 @0x184, CamBlender blender @0x270, camTimer u32 @0x398.
- CamBlender: state s16 @0x00, reqType u8 @0x03, reqQuatInterpAdd f32 @0x18,
  reqPosInterpAdd f32 @0x24, reqInterpFrames u32 @0x84. camTransState=0x1871b0 ==
  &currentCamera.blender.
- Data (config/linker_aliases.ld): importCameraTable=0x15EF90 (u32 ptr table),
  camControlWork=0x189650 (0x280-byte buffer), occlCamStaged=0x18C32C.
- ImportCamera { f32 pos[3]; f32 rot[3]; u32 pVar @0x1C; } — flag byte =
  *(u8*)(pVar+0x1D).
- func_001FA6D0 (fastfunc.s:1608): `cvt.w.s $f12,$f12; mfc1 $v0,$f12` (float arg in
  $f12, extern "C"). Probe-verified O32 float passing.
- Constants: 0x3C9374BC=0.018f, 0x3C23D70A=0.01f.

## Verified semantics (C form, byte-correct logic)
D = pCurCam->activation.deactivate; M = pImport->pVar ? *(u8*)(pVar+0x1D) : 0.
- D==4: pNewCam->active=1; END.
- D==2: M==1?blend : M==5?polar : stateUpd.
- else: M==1?blend : M==5?polar : dPath.
- dPath: (D==3 | D<=5 | M==3 | M==6)?{ copy 8B@0x00,0x10,0x20,0x30 cur->new;
  activate=2; (D==5|M==6)?{blend-setup, level==1->0.01f; stateUpd} : END }
  : { active=1; END }.
  NOTE: the dPath constants are D==3 and M==3 (NOT D==1/M==1 — M==1 is dead in
  dPath). This was a real semantic bug in the first C draft, caught by the
  last-resort-decompiler via delay-slot register tracing ($2=3, $3=5 at 0x1EC054).
- blend: reqType=0; adds=speed>0?speed:0.018f; stateUpd.
- polar: reqType=2; reqInterpFrames=speed>0?func_001FA6D0(speed):0x28; stateUpd.
- stateUpd: blender.state=(state==0)?1:2; END.
- END: pCurCam deactivate/activate/active=0; pLastUpdCam=pCurCam;
  FastMemCopy(camControlWork,camControlWork-0x280,0x280); pLast->control=work;
  pCurrentUpdCam=pNewCam; pNewCam->control=work-0x280; Camera_runSetupToNewCam(pNewCam);
  camTimer=0; BackupCurrentCam(); if(occlCamStaged==0) currentCamera.posQuad=
  pNewCam->posQuad; lPos=pos.x/y/z.
- The transform copy is 4x8-byte lq/sq at 0x00/0x10/0x20/0x30 (32 bytes), NOT
  4x16-byte CameraQuad copies — EGC 2.95.2 lowers the mode(TI) struct copy to a
  single 8-byte lq/sq here. Matches the original either way.

## What the C form achieved (all verified against the original asm)
- Frame 0x70 (s0-s5+ra) — forced by a `CameraQuad* newPosQuad=&pNewCam->posQuad;`
  local used in the dPath copy + END block (6th saved reg).
- currentCamera hi+%lo addressing form — forced by a `Camera* cameraAlias=
  &currentCamera; asm volatile("" : "+r"(cameraAlias));` used ONLY in the polar
  block + the initial pCurrentUpdCam load. Result: prologue materializes both
  s0=full(cam) and s4=hi(cam); blend/stateUpd/dPath-blend use `addiu base,hi,%lo`;
  polar + END use the full base. This matches the original's structure (s0=full,
  s1=hi).
- dPath emits beq/beql/beq/bnel like the original.
- state s16 emits `lh` (not `lhu`).
- Fresh field read `pCurCam->activation.deactivate==5` emits the original's `lh`
  D reload.

## The unmatchable gap (176 vs 192 instrs; the blocker)
- s-register PERMUTATION: original s1=hi cam, s4=pCurCam, s3=pNewCam+0x30, s5=full
  cam (END); the C form puts hi cam in s4, pCurCam in s3, pNewCam+0x30 in s5. EGC's
  callee-saved numbering is not controllable from C source.
- dPath `deactivate<=5`: EGC emits `slti D,6; bnel` (2 instrs) where the original
  has `beql D,5` (1 instr).
- Net: ~16-instruction gap from register allocation + block scheduling.

## Tried (all parity-safe, none closed the gap)
- Struct refactor (camera.h shared with draw.cpp) — passes parity, kept.
- newPosQuad local (frame fix), dPath 4-branch gotos, s16 state, fresh D reload,
  cameraAlias (addressing-form fix) — all applied, all verified above.
- Scoped-register-transfer pinning (pCurCam->$20/s4, newPosQuad->$19/s3, AGENTS.md
  technique): WORSE — frame grew 0x70->0x80, added s6, redundant daddu. Reverted.
- -fno-schedule-insns on camera.o (diagnostic): 174 instrs (worse). Not a fix.

## Escalations
- expert (GPT-6 Astra) invoked 2026-09-23: ranked hypotheses (address CSE/lifetime,
  fresh-load, pseudo live-ranges, flags); recommended the cameraAlias partition and
  the pinning. The alias fixed the addressing form; the pinning was tested and
  reverted (worse).
- last-resort-decompiler (GPT-5.6 Sol) invoked 2026-09-23: corrected the dPath
  constants (D==3/M==3, a real semantic bug) and recommended the cameraAlias split.
  Both applied and mechanically diffed; the s-reg permutation + slti-vs-beql gap
  remains. A durable allocator blocker is warranted.

## Re-attempt notes
- If a future EGC build or flag set is available, the C form (above) is the starting
  point — the semantics, frame, and addressing form are all correct; only the
  s-reg permutation and the one slti-vs-beql test remain.
- The struct definitions + externs in camera.cpp (ImportCamera, camControlWork,
  importCameraTable, occlCamStaged, currentLevelId, func_001FA6D0) are the verified
  data layout for this function and are kept for re-attempt even though the
  function is currently INCLUDE_ASM.
