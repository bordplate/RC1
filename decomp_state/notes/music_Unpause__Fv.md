# music_Unpause__Fv (vram 0x216088, file 0x117008, 28 bytes)

Semantics: clears the three paused/channel-state halfwords of the music state
struct at D_001516D0 by setting them to 4. Sibling `music_Pause__Fi`
(vram 0x216050) sets the same fields (0x40, 0x5C, 0x78) plus their +2
neighbors (0x42, 0x5E, 0x7A) to 0x8000/0 under a `beqz` guard on one field.
So the struct is three (state, value) u16 pairs at 0x40/0x42, 0x5C/0x5E,
0x78/0x7A; 4 = "unpaused" state code for the SIF music driver. Callers:
jal at 0x1FD6B8 and 0x22FB48.

## Original body

```
3C021500  lui   v0, %hi(D_001516D0)
24030004  addiu v1, $0, 4
244216D0  addiu v0, v0, %lo(D_001516D0)
A443005C  sh    v1, 0x5C(v0)
A4430040  sh    v1, 0x40(v0)
0800E003  jr    ra
A4430078   sh   v1, 0x78(v0)      (delay slot)
```

## Candidate (committed)

`code/game/music.cpp`: struct `MusicState` with u16 fields at exactly
0x40/0x5C/0x78 (pads between), extern instance at D_001516D0
(section ".data"), plus:

```cpp
class music {
public:
    void Unpause() asm("music_Unpause__Fv");
};

void music::Unpause() {
    D_001516D0.field_0x40 = 4;
    D_001516D0.field_0x78 = 4;
    D_001516D0.field_0x5C = 4;   // source order 40, 78, 5C - see below
}
```

The class name `music` and the asm-label trick are needed because this EGC
build (v2.73a) uses old cfront mangling: a real method `music::Unpause()`
mangles to `Unpause__3music`, and a plain free function
`void music_Unpause__Fv()` mangles to `music_Unpause__Fv__Fv`. Neither
yields the binary's PS2-style `music_Unpause__Fv`. Putting
`asm("music_Unpause__Fv")` on the in-class declaration emits exactly that
symbol (GCC function asm label; EGC 2.95.2 accepts it on the in-class
declaration but NOT on the out-of-class definition). The codegen itself is
a true C++ instance method (`this` unused, identical allocation to a free
function for this body).

## EGC store-ordering rule (3 independent constant stores, one base)

All six statement permutations were compiled standalone (scratch fuzzer);
machine order is ALWAYS `stmt3; stmt1; jr ra; <delay: stmt2>`:

| source order (40/5C/78) | emitted |
|---|---|
| 40, 5C, 78 | 78, 40, [jr] 5C |
| 40, 78, 5C | 5C, 40, [jr] 78   <- target, committed |
| 5C, 40, 78 | 78, 5C, [jr] 40 |
| 5C, 78, 40 | 40, 5C, [jr] 78 |
| 78, 40, 5C | 5C, 78, [jr] 40 |
| 78, 5C, 40 | 40, 78, [jr] 5C |

Generalizes the known 2-store reversal (src A,B -> B,[jr]A): with three
constant stores sharing one %hi/%lo base, the third statement emits first,
the first second, and the second lands in the `jr $ra` delay slot. Base is
always $v0 (lui/addiu), the constant in $v1 (addiu).

## Verification

- music.o objdump: symbol `T music_Unpause__Fv`, 7 words identical to the
  original above (relocs R_MIPS_HI16/LO16 D_001516D0 resolve to 0x15/0x16D0).
- Function is exactly 0x1C bytes; next symbol music_UpdateStream follows at
  +8 with one nop pad word, matching the original layout (0x2160A4 pad,
  0x2160A8 next label).
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes;
  decomp_status --count 857 -> 856.
