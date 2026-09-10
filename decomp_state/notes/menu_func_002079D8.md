# func_002079D8 — INVESTIGATED (likely blocked, not yet in blocked.json)

VRAM 0x002079D8, 48 bytes (12 insns). Source: `code/game/menu.cpp`
(Splat segment `menu`, `-fno-schedule-insns`). Still INCLUDE_ASM.

Menu predicate callback, function-pointer table vram 0x1A0000 (index 4,
entry 0x1A0010). No direct `jal`/`j` — called only via the table.

## Exact original 12 insns (objdump ground truth)
```
0x2079D8  slti   $5, $5, 321        ; a1 = (b < 321)
0x2079DC  beq    $5, $0, .L         ; b>=321 -> .L   [delay: li v0,1]
0x2079E0  li     $2, 0x1
0x2079E4  lui    $1, 0x427e         ; 63.5f
0x2079E8  mtc1   $1, $f0
0x2079EC  nop                        ; COP1 hazard
0x2079F0  c.le.s $f0, $f14          ; C1.F = (63.5 <= y)
0x2079F4  nop
0x2079F8  bc1f   .L                  ; C1.F==0 (y<63.5) -> .L  [delay: move v0,0]
0x2079FC  move   $2, $0
0x207A00  jr     $ra                ; .L
0x207A04  nop
```

## Semantics: the float test is DEAD
Trace (both `move v0,0` delay slots execute unconditionally):
- `b >= 321`  -> return 1
- `b < 321, y < 63.5`  -> return 0
- `b < 321, y >= 63.5` -> return 0   (v0 clobbered by bc1f delay slot)

So the function is effectively `return b >= 321;`. The `c.le.s`/`bc1f`
float comparison does not affect the result. Confirmed by tracing concrete
values (b=100,y=100 -> 0; b=100,y=10 -> 0; b=400 -> 1).

## Why it likely does not match with local EGC 2.95.2
Local EGC **DCE's** the dead float comparison in every C form tried:
- `if (b>=0x141) return 1; if (y<t) return 0; return 0;`  -> 40 bytes (test removed)
- `... if (y<t) return 0; else return 0;`                 -> 40 bytes
- `... return (y<t) ? 0 : 0;`                              -> 40 bytes
- `... return (int)(y<t) & 0;`                             -> 40 bytes
- `volatile float threshold`                               -> 56 bytes (adds swc1/lwc1 the original lacks)

Insomniac's compiler evidently kept the dead float test; the local EGC
2.9-ee-991111 eliminates it. No C form reproduces a dead float test.

## ABI note (reusable)
`slti $5` (2nd int -> a1) and `c.le.s $f0,$f14` (float in f14). Under packed
EGC the f14 float is the **3rd** float param, so the signature is
`(int, int, float, float, float)` using the 2nd int and 3rd float. This
register analysis is solid; the blocker is the dead-code DCE, not the ABI.

## Next step before recording a blocker
Per AGENTS.md, `last-resort-decompiler` must be invoked on this exact target
before adding it to `decomp_state/blocked.json`. Not yet done.
