# func_002075F8 — MATCHED

VRAM 0x002075F8, 52 bytes (13 insns). Source: `code/game/menu.cpp`
(Splat segment `menu`, `-fno-schedule-insns`).

Menu predicate callback, jump table vram 0x19FF70, entry 0x19FFA0.
Returns `1` iff `x < 224` and `y <= 38.0f`; `0` otherwise. The gated menu
item is unconfirmed, so the address-based name is retained.

```
extern "C" int func_002075F8(int x, float unused1, float unused2, float y) {
    if (x >= 224)
        return 0;
    float threshold = 38.0f;
    asm volatile("nop" : : "f"(threshold));
    return y <= threshold;
}
```

Match-critical:
- ABI `(int, float, float, float)`: the compared float is the **3rd** float
  param. EGC 2.95.2 packs float args into 64-bit pairs (f12:f13, f14:f15),
  so the 3rd lands in `$f14`, exactly where the original reads it
  (`c.le.s $f14,$f0`). A 2-float signature would put it in `$f13` and miss.
- `asm volatile("nop" : : "f"(threshold))` forces the materialization
  `lui at,0x4218; mtc1 at,$f0` followed by the required EE COP1 hazard `nop`
  before `c.le.s`. EGC otherwise emits the compare directly after `mtc1`
  (no nop) and misses.
- `return y <= threshold;` emits `c.le.s $f14,$f0; nop; bc1t L; li v0,1;
  move v0,zero; L: jr ra`, sharing the single epilogue with the
  `if (x >= 224) return 0;` branch (`beqz L` with `move v0,zero` in its delay
  slot).

First-probe byte-exact match (52/52). Full boot ELF `cmp` byte-for-byte.
Sibling precedent: func_00207690 (same file, same idiom).
