# pause_stepAngle01 (func_0021E1F8, code/game/pause_post2.cpp)

- 56 bytes (0x38) at vram `0x21E1F8`. `void pause_stepAngle01(PauseStepObject*)`,
  bound to the address-based entry via `asm("func_0021E1F8")` symbol override
  (the generated function-pointer tables reference the address-based name).
- Semantics: per-frame step callback for the pause-screen step object. It
  advances the object's angle (float at +0x48) by 0.01, wrapping the result
  into [-100.0, 100.0) via FastAddRots.

## Object and family

The step object is allocated by `func_00225490` (still INCLUDE_ASM) for the
pause gadget/manipulator screens (func_0021DF98, LoadHandGadget). Established
fields: camera-relative display floats at +0x10/+0x14/+0x18 (derived from
`currentCamera` +0x140/+0x144/+0x148 in func_0021DF98), u16 at +0x34,
pointer at +0x44, **float at +0x48** (initialized to 100.0 by func_0021DF98),
per-frame callback pointer at +0x74, pointer at +0x78. func_0021E698 also
writes a float into +0x48 (from a sub-object at [+0x78]+0x38 or cam+0x140),
so +0x48 is a generic wrapping float the step callbacks integrate.

The two functions stored at +0x74 in this family are the 0.01f/0.02f twins:
func_0021E1F8 (this one) and func_0021F120. Both are 14-instruction wrappers
around FastAddRots; the only differences are the constant (0x3C23D70A =
0.01f vs 0x3CA3D70A = 0.02f) and the frame.

## FastAddRots (0x1FA580, game/fastfunc, handwritten asm)

`f0 = f12 + f13; if (f0 < 100.0) f0 -= 200.0; else if (!(f0 < -100.0))
f0 += 200.0;` — i.e. wrap into [-100.0, 100.0). The mangled symbol is
`FastAddRots__Fff` = C++ free function with **two** float parameters (cfront
drops the return type); the callee materializes its own ±100.0 limits into
f14/f15 on entry, so a third float argument is never consumed. Deadlocked
reference usage confirms the angle/rotation semantics
(`g_TransitionClankLightAng = FastAddRots(g_TransitionClankLightAng, step)`).
Declared C++-linkage in pause_post2.cpp as `float FastAddRots(float, float)`
(mangles to `FastAddRots__Fff`), so a two-arg C call passes exactly f12/f13
as the original does — declaring three params would make EGC set f14 and emit
extra instructions.

## Codegen

First-try match, default TU flags (pause_post2.o is `-G0`):

```cpp
typedef struct {
    u8 pad[0x48];
    float angle;
} PauseStepObject;

void pause_stepAngle01(PauseStepObject* obj) {
    obj->angle = FastAddRots(obj->angle, 0.01f);
}
```

- The `0.01f` constant (`lui v0,0x3C23; ori v0,0xD70A`) is materialized into
  `$v0` right after the frame allocate and **before** the `sq $ra,0x10(sp)` /
  `sq $s0,0(sp)` prologue stores, then `mtc1 v0,f13` (the second float arg).
- `daddu s0,a0` + `sq s0`/`lq s0` keep the object pointer callee-saved across
  the call (0x20 frame).
- The `lwc1 f12,0x48(s0)` (first float arg) is scheduled by EGC into the
  `jal FastAddRots` delay slot; the `swc1 f0,0x48(s0)` store follows the call.
- Void return: no `daddu v0,zero` — v0 is left holding the float result,
  exactly as in the original (a `return 0;` would add an extra word).

## Verification

- `tools/decomp_probe.py`: 56/56 words match, zero differences.
- `tools/tu_assembler_diff.py build/code/game/pause_post2.o build/boot_elf.elf`:
  121/121 match.
- Clean build (`make clean && make split && make -j2`) + `cmp
  build/boot_elf.elf assets/boot_elf.elf`: byte-identical.
- `tools/decomp_status.py --count`: 626 -> 625.

## Next

func_0021F120 (the 0.02f twin, 56 bytes) is a direct follow-up: same form
with `0.02f` and a `pause_stepAngle02` name; its `asm` override target is
`func_0021F120`.
