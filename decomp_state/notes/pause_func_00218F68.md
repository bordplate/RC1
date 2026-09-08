# func_00218F68 (0x218F68)

## What it does

Writes the pause-screen entry state. Five independent constant stores:

```cpp
D_001D5BF0.f0   = 0x2D;   // 0x218F68  base_hi in $a0
D_001D5BF0.f110 = 0;      // 0x218F7C  base_addr in $v1
GameMode        = 3;      // 0x218F84  GM_hi in $at, GM_const in $a1
D_001D5BF0.fC   = 0;      // 0x218F88  base_addr in $v1
D_001D5BF0.f10  = 0;      // 0x218F90  base_addr in $v1 (jr delay slot)
```

D_001D5BF0 is a 0x114-byte struct (fields f0@0, fC@0xC, f10@0x10,
f110@0x110) at 0x1D5BF0, outside the gp window, accessed with absolute
HI16/LO16. GameMode is the u32 at 0x15F604 (== `GameMode`, in-window but
accessed absolutely here).

## The match

```cpp
typedef struct {
    u32 f0; u8 pad4[8]; u32 fC; u32 f10; u8 pad[0xFC]; u32 f110;
} PauseScreenState;
extern "C" PauseScreenState D_001D5BF0 __attribute__((section(".data")));

extern "C" void func_00218F68(void)
{
    register PauseScreenState* base asm("$3") = &D_001D5BF0;
    D_001D5BF0.f0 = 0x2D;
    register u32 mode asm("$5") = 3;
    base->f110 = 0;
    *(u32*)0x15F604 = mode;
    base->fC = 0;
    base->f10 = 0;
}
```

## Codegen / why it was hard

With default flags EGC hoists the GameMode store to position 2 (right after
f0), giving store order [f0, GM, f10, f110, fC-delay] — wrong. The original
keeps source order [f0, f110, GM, fC, f10-delay], which only comes out under
`-fno-schedule-insns`.

But even with that flag the register allocation differed: EGC put base_hi in
$v1 and base_addr in $a0, and used $v0/$v1 for the GameMode store, whereas the
original uses base_hi=$a0, base_addr=$v1, GM_hi=$at, GM_const=$a1. Binding
`base` to $v1 (`asm("$3")`) forces the shared %hi temporary into $a0 and
yields the $a0->$v1 address build; binding `mode` to $a1 (`asm("$5")`) gives
`li $a1,3`, and the constant-address GameMode store then expands through $at.
Declaring `mode` before `base->f110 = 0` lands its load at 0x218F78. This
register-binding + scheduler-flag pattern is the same one used in
menu_callbacks.cpp (see menu_func_002088A8).

## Splat boundary split

`-fno-schedule-insns` cannot be applied to the whole pause.o: doing so breaks
the already-matched func_0021A318 (0x21A318) and the func at 0x2223D8. So the
pause Splat segment was split at exact function boundaries (config/RC1.yaml):

- `game/pause`        0x119c90..0x119ee8  (func_00218D10, PauseAllSounds, func_00218F50)
- `game/pause_sched`  0x119ee8..0x119f18  (func_00218F68, this function)
- `game/pause_post`   0x119f18..0x1286c0  (func_00218F98 .. end, 134 functions)

pause.cpp keeps the first three; pause_sched.cpp is new (this function + the
PauseScreenState/D_001D5BF0 decls); the rest moved to pause_post.cpp with its
INCLUDE_ASM paths retargeted to `.../game/pause_post`. Makefile assigns
`-fno-schedule-insns` to pause_sched.o only. Rerunning `make split` moved
func_00218F68.s to `_generated/matchings/game/pause_sched/` and regenerated the
linker script so pause.o, pause_sched.o, pause_post.o link in order.

## Verification

- decomp_probe (flags=-fno-schedule-insns): 44/44 bytes, 0 differences.
- Full `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: identical. Count 740 -> 739.
