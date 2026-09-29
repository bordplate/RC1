# func_001F4650 — ExecuteDrawCallbacks (BLOCKED: 2-byte independent-`lui` order wall)

Target: `code/game/draw_post_post.cpp` (INCLUDE_ASM at line 552),
`code/_generated/nonmatchings/game/draw_post_post/func_001F4650.s`, 0x78 bytes, vram 0x1F4650.

## Semantics
Executes the **primary** per-draw-phase callback list. For `i` in `0..drawCallbackCount`:
`drawCallbackFuncs[i](drawCallbackArgs[i])`. Natural name `ExecuteDrawCallbacks`, linked to the
Splat placeholder via `void ExecuteDrawCallbacks(void) asm("func_001F4650");` (same pattern as the
already-matched `AddDrawCallback2` at 0x1F47B8).

Three byte-identical 0x78-byte sibling clones exist (same reversed-`lui` pattern, all still
nonmatching): `func_001F46C8` (list 3, D_0018DF40/D_0018E040), `func_001F4740` (list 4),
`func_001F4808` (list 2). Matching one form matches all four.

## Register map (original)
`i`→s0($16), `funcs`→s1($17), `args`→s2($18), `f` (callback ptr)→v1($3), `count`→v0($2),
`arg`→a0($4). Frame 0x40; saves s0@0, ra@0x30, s2@0x20, s1@0x10 (s1 save in the `blez` delay slot).
Globals: `drawCallbackCount`=0x15F464 (in GP window → self-based abs lui/lw);
`drawCallbackFuncs`=0x18DB40, `drawCallbackArgs`=0x18DC40 (out of GP window → split
`lui` hi / `addiu` lo). `drawCallbackCount` must be `extern volatile int` (reloaded each iteration).

## Best verified candidate — 118/120 bytes match
`working/ExecuteDrawCallbacks/probe_expert_v2.cpp` (production default flags, SN assembler):

```cpp
register int i asm("$16") = 0;
if (drawCallbackCount > 0) {
    u32* args = drawCallbackArgs;
    u32* funcs = drawCallbackFuncs;
    asm volatile("" : "+r"(funcs));
    register u32 f asm("$3");
    f = *funcs;
    for (;;) {
        i++;
        asm volatile("" : "+r"(i));
        u32 arg = *args;
        funcs++;
        ((void (*)(u32))f)(arg);
        args++;
        register int more asm("$2") = (i < drawCallbackCount);
        asm volatile("" : "+r"(more));
        if (!more) break;
        f = *funcs;
    }
}
```

Every byte matches EXCEPT the emitted order of two independent setup `lui`s:

```
0x1f4674  original: lui v1,%hi(drawCallbackFuncs)   candidate: lui v0,%hi(drawCallbackArgs)
0x1f4678  original: lui v0,%hi(drawCallbackArgs)    candidate: lui v1,%hi(drawCallbackFuncs)
```

Same registers (Args→v0, Funcs→v1), same following `addiu`s (s2=args @0x1f467c, s1=funcs
@0x1f4680), only the two high-half loads swapped. The original emits the two high-half
**producers** in REVERSE order of their **consumers** ([Funcs,Args] producers vs [Args,Funcs]
consumers); our EGC emits producers in consumer order.

## Load-bearing pins/barriers (all required; do not remove)
- `register int i asm("$16") = 0;` — fixes the prologue (sq s0 first, `i=0` before the `blez`,
  correct s2/s1 save slots). A plain `int i=0` mis-allocates i→s2 and reorders the saves.
- `for(;;){...; if(...) break;}` — emits the loop branch as opcode 0x54 (BNE/"bnel" on R5900),
  which matches. A `do{...}while(...)` emits 0x05 (standard BNE) and does NOT match.
- `asm volatile("" : "+r"(i));` after `i++;` — keeps `i++` (addiu s0) before the arg load
  (lw a0,0(s2)); without it EGC hoists the load above the increment.
- `register u32 f asm("$3");` + `asm volatile("" : "+r"(funcs));` — puts `f` in v1 (without the
  pin, f→v0 and collides with the per-iteration count load in v0, and there is no f-reload in the
  bne delay slot) and makes the f-load use the materialized s1 base `lw v1,0(s1)`.
- `register int more asm("$2") = (i < drawCallbackCount);` + `"+r"(more)` — makes the loop-tail
  `slt` reuse the just-loaded count register v0 (original) instead of v1, so the `bne` branches on
  v0 and f(v1) is left for the delay-slot reload.

## Tried and ruled out (~40 probes)
- Scheduling flags: `-fno-schedule-insns`, `-fno-schedule-insns2`, both, `-fno-regmove` — none
  change the `lui` order (the latter two break the prologue).
- Declaration order of args/funcs: no effect on the `lui` order.
- Assignment order: `funcs`-first flips the `lui` order to [Funcs,Args] (correct) BUT also swaps
  the high-half temps (Funcs→v0, Args→v1) and flips the `addiu` order — not a match.
  `args`-first keeps temps/addiu correct but gives the wrong `lui` order.
- Early-return guard `if (drawCallbackCount <= 0) return;`: no effect on the `lui` order.
- Moving `f = *funcs;` earlier (between/before the assignments; direct `drawCallbackFuncs[0]`):
  breaks other bytes.
- Register pins on the pointers (`register u32* args asm("$18")` etc.): breaks the temps.
- Barriers: `+r(funcs)` vs input-only `"r"(funcs)` vs none vs a `"memory"` barrier between the
  assignments: none produce the exact [Funcs(v1), Args(v0)] order with correct temps/addiu.
- Typed function pointer `DrawCallbackProc f = (DrawCallbackProc)*funcs;` instead of u32+cast: no effect.
- Last-resort joint-dependency forms: `asm volatile("" : "+r"(funcs) : "r"(args))`,
  `"+r"(funcs),"+r"(args)`, `"+r"(args),"+r"(funcs)`, and input-only
  `"r"(args),"r"(funcs)` / `"r"(funcs),"r"(args)` each followed by `"+r"(funcs)`: all still the
  same 2-byte order.

## Last-resort escalation
`last-resort-decompiler` (GPT-5.6 Sol) invoked 2026-09-29 with the full 2-byte dossier. It
confirmed the "v0 still busy" hypothesis is unlikely (the guard's v0 dies at `blez`), assessed
that register allocation follows the low-half consumers (Args→v0, Funcs→v1) while scheduling
independently reverses the two ready high-half producers, and recommended (in order): the joint
zero-byte dependency `"+r"(funcs) : "r"(args)`; the `PARALLEL` operand-order variants; an
input-only joint dependency; and a `-fno-regmove` probe. All were applied and mechanically tested
— none closed the 2 bytes. It concluded the operation is unrepresentable in pure C with this EEGCC
build: C/empty-asm can constrain the complete address but cannot order one split-address
`%hi`-half producer independently of its `%lo` consumer; only emitted inline assembly or a
different compiler/codegen revision could enforce it.

## Classification
Same documented class as `camera_UpdateAllCameras__Fi` ("two independent absolute-address `lui`s
whose HI emit order the original decouples from declaration/register order … 2 swapped hi `lui`s
can remain a blocker even with loop+tail fully matched"). Systematic across all four executor
clones → original-codegen behavior, not a misdecode.

## Disposition
Retain `INCLUDE_ASM`; full-ELF parity preserved (build/boot_elf.elf == assets/boot_elf.elf).
Re-attempt only with a compiler/codegen revision change or an inline-assembly workaround for the
two high-half loads (the latter is out of style for a matching decompilation and not applied).
