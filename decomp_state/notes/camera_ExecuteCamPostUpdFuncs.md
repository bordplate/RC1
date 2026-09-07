# ExecuteCamPostUpdFuncs (0x1EBCF0, 108 bytes) — BLOCKED at 106/108

`code/game/camera.cpp:5`. Semantics:

```c
for (i = 0; i < CamPostUpdRoutineCnt; i++)   // cnt @ 0x15EF8C
    CamPostUpdRoutines[i]();                 // array @ 0x1892B0 (fn-ptr array)
CamPostUpdRoutineCnt = 0;
```

Loop var `i` in **s0**, running array base in **s1**, called ptr in **v1**.
Separate orphan dead-tail `func_001EBD60` (camera.cpp:7) is KEPT regardless.

## Best candidate: 106/108 (`decomp_state/probes/camera_execpostupd_v2.cpp`)

```c
extern "C" int CamPostUpdRoutineCnt __attribute__((section(".data")));
typedef void (*CamPostUpdFunc)(void);
extern "C" CamPostUpdFunc CamPostUpdRoutines[];

extern "C" void ExecuteCamPostUpdFuncs(void) {
    int i;
    for (i = 0; i < CamPostUpdRoutineCnt; i++)
        CamPostUpdRoutines[i]();
    CamPostUpdRoutineCnt = 0;
}
```

Compiled with `--flags=-mno-split-addresses`. Verified:

```sh
python tools/decomp_probe.py decomp_state/probes/camera_execpostupd_v2.cpp \
  code/_generated/nonmatchings/game/camera/ExecuteCamPostUpdFuncs.s \
  ExecuteCamPostUpdFuncs --flags=-mno-split-addresses --out /tmp/opencode/cam-v2-nosplit
# -> sizes 108 108, ndiff 2
```

Everything matches EXCEPT the two array-base words:

| addr | original | candidate |
|------|----------|-----------|
| 0x1EBD10 | `1900023c` lui **v0**,0x19 | `1900113c` lui **s1**,0x19 |
| 0x1EBD14 | `b0925124` addiu s1,**v0**,lo | `b0923126` addiu s1,**s1**,lo |

## Why

- Counter `0x15EF8C` is **inside** the gp window. `section(".data")` forces lui/lw
  (not gp-relative). Under `-mno-split-addresses` the counter high is NOT cached:
  it is re-materialized 3x (front / back-edge / store) and the front load is
  **hoisted before the saves** with frame 0x30 — matches the original exactly.
- Array `0x1892B0` is **outside** the gp window. Under `-mno-split-addresses` EGC
  emits the `la $17,sym` pseudo for it, which the assembler expands to the
  **fused** form `lui $17,hi; addiu $17,$17,lo` (high in the destination s1).
  The original is the **explicit/unfused** form `lui v0,hi; addiu s1,v0,lo`.
- Without the flag (v2 plain), the array is correctly explicit (high v0), but the
  counter high is **cached into s2** (`move s2,v1`, frame 0x40; back-edge/store
  reuse s2 with no re-lui) — wrong.

So the counter needs `-mno-split-addresses` (re-lui + prologue hoist) while the
out-of-gp-window array needs plain mode (explicit `la`); a single per-TU flag
cannot satisfy both, and because the array is outside the gp window,
`-mno-split-addresses` always fuses it via `la`.

## Tried (all in `decomp_state/probes/`)

- `camera_execpostupd.cpp` (v1, constant-address cast counter + symbol array):
  the counter load is a single RTL `movsi` from a const_int; the reload pass
  parks it AFTER the three saves + `PROLOGUE_END` with anti-deps on the saves,
  so it is never hoisted → 6 prologue-word diffs (load after saves; `i=0` in the
  blez delay slot). Body + tail match.
- `camera_execpostupd_v2.cpp` (symbol counter `.data` + symbol array): plain = 27
  diffs (counter s2-cache, frame 0x40); **with `-mno-split-addresses` = 2 diffs**
  (array `la` fusion). Best result.
- `camera_execpostupd_v12.cpp` (symbol counter + constant-address array, NOSPLIT):
  10 diffs — the constant array base reshuffles the loop registers (i→s2 etc.).
- `camera_execpostupd_v13.cpp` (last-resort register-asm `asm("$2")` high +
  do-while, NOSPLIT): 10 diffs — changing the loop structure reshuffles i/base
  registers.
- `camera_execpostupd_v14.cpp` (volatile counter, plain): 30 diffs (frame 0x4c,
  extra s2/s3 saves).
- Flags on the plain form: `-fno-schedule-insns`/`-fno-schedule-insns2` (9/13
  diffs, body regs change), `-fno-gcse` (29 diffs); `-fno-gcse-lm` and
  `-fno-cprop-registers` are rejected by EGC 2.95.2.

## Escalation

`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-07. It independently
confirmed the 106/108 NOSPLIT route (symbolic counter `.data` +
`-mno-split-addresses` fixes the whole counter prologue) and that the only
residual is the array-base high register. Its recommended register-asm /
inline-asm fixes for the array base change the loop register allocation (10
diffs) and do not produce a match. Recorded as blocked.
