# func_002079D8 — MATCHED (byte-for-byte)

VRAM 0x002079D8, 48 bytes (12 insns). Source: `code/game/menu.cpp`
(Splat segment `menu`, `-fno-schedule-insns`).

Menu item availability predicate, function-pointer table vram 0x19FF70
(entry 0x1A0010). No direct `jal`/`j` — called only via the table. The
availability-predicate dispatcher has not been identified, so the exact
menu item and the meaning of the int/float args are unconfirmed; the
address-based name is retained (a batch rename can follow once the
dispatcher is known).

## Semantics

```
return (b >= 321) || (63.5f <= e);
```

where `b` is the 2nd int arg and `e` is the 3rd float arg.

Signature is `(int, int, float, float, float)`: the gate int is `$a1`
(2nd) and the compared float is `$f14` (3rd — EGC packs float args into
64-bit pairs `f12:f13, f14:f15`, so the 3rd lands in `$f14`). The 1st int
and the 1st/2nd floats are unused; they exist only to push the used args
into the right registers.

## Exact original 12 insns (objdump ground truth)
```
0x2079D8  slti   $5, $5, 321        ; a1 = (b < 321)
0x2079DC  beq    $5, $0, .L         ; b>=321 -> .L   [delay: li v0,1]
0x2079E0  li     $2, 0x1
0x2079E4  lui    $1, 0x427e         ; 63.5f
0x2079E8  mtc1   $1, $f0
0x2079EC  nop                        ; COP1 hazard
0x2079F0  c.le.s $f0, $f14          ; C1 = (63.5 <= e)
0x2079F4  nop
0x2079F8  bc1fl  .L                  ; likely branch (0x45020001)
0x2079FC  move   $2, $0
0x207A00  jr     $ra                ; .L
0x207A04  nop
```

## Correction to the earlier "dead float test" reading

The prior version of this note concluded the float test was dead
(effectively `return b >= 321`) and that the local EGC DCE'd it. That was
WRONG: `bc1fl` is a LIKELY branch — it has NO delay slot. The `move v0,0`
at 0x2079FC is the branch's not-taken path, not a delay slot, so the float
comparison is live and changes the result. (The `nop` at 0x2079F4 is the
`c.le.s` delay slot; the `nop` at 0x2079EC is the `mtc1` hazard slot.)

## Why the flat C form failed, and the matching form

The local EGC, given the flat form `if (threshold <= value) result = 0;`
(or any `if (cond) r = K;` / nested / De Morgan / ternary variant, at
-O1/-O2, with/without -ffast-math, -fno-schedule-insns/2), emits the
inverted COP1 likely branch `bc1tl` (0x45030001) where the original has
`bc1fl` (0x45020001) — a one-bit (T/F) difference on an otherwise
byte-identical 12-insn body. No flag or comparison-operator variation
flips it.

The matching form (from `last-resort-decompiler`, GPT-5.6 Sol) uses an
empty then-branch so the `c.le.s` result stays live on the likely branch:

```cpp
int result = 1;
if (gate < 321) {
    float threshold = 63.5f;
    asm volatile("nop" : : "f"(threshold));
    if (threshold <= value) {
    } else {
        result = 0;
    }
}
return result;
```

This compiles byte-for-byte to the original 48 bytes (verified against
`assets/boot_elf.elf` at file offset 0x108958 and in the built `menu.o`).
The `asm volatile("nop" : : "f"(threshold))` is the mtc1 -> c.le.s FPU
hazard (same idiom as the matched siblings `menu_isItemAvailable*Height`).

last-resort GPT-5.6 Sol used: confirmed the empty-then-branch form and the
`(b >= 321) || (63.5f <= e)` semantics; verified locally byte-for-byte.
