# VU1_addDataRef__FPvi (0x233830, 0x4C) — BLOCKED

Bump allocator for the VU1 command chain. `D_00160F00` holds the bump pointer
V. Writes a 0x10-byte slot `[x|0x30000000, p, 0, 0]` at [V..V+0xC], then advances
both `D_00160F00` and (dead-tail) `D_00162C20` to V+0x10.

## Exact original codegen (objdump of assets/boot_elf.elf)
```
lui  v1,0x16 ; lw v1,3840(v1)      ; V = *(int*)0x160F00   (self-based, load #1)
lui  v0,0x3000 ; or a1,a1,v0 ; sw a1,0(v1)   ; [V+0] = x|0x30000000
lui  v0,0x16 ; lw v0,3840(v0) ; sw a0,4(v0)  ; [V+4] = p     (load #2)
lui  v1,0x16 ; lw v1,3840(v1) ; sw 0,8(v1)   ; [V+8] = 0     (load #3)
lui  a0,0x16 ; lw a0,3840(a0) ; sw 0,0xC(a0) ; [V+0xC] = 0   (load #4)
lui  v0,0x16 ; lw v0,3840(v0) ; addiu v0,v0,0x10  (load #5)
jr   ra
  sw   v0,-23808(gp)   ; D_00160F00 = V+0x10  (GPREL, delay slot, ALIVE)
nop
sw   v0,-16352(gp)     ; D_00162C20 = V+0x10  (GPREL, DEAD TAIL = func_00233880)
nop
```

## The blocker
The 5 head-pointer loads are **self-based and re-materialized per statement**:
`lui r,0x16; lw r,3840(r)` (address 0x160F00 split into lui-hi + lw-lo offset,
target reg == base reg), repeated 5x, NOT hoisted into a shared base register.
The two final stores are **GPREL** (small-data symbols D_00160F00 / D_00162C20).

So the same address 0x160F00 is read with full 32-bit self-based addressing and
written with gp-relative addressing. The C structure that reproduces the 5
separate (non-CSE'd) loads + correct register pattern (v1,v0,v1,a0,v0) + offsets
+ the two GPREL stores is the "recompute per statement" form:
```cpp
*(int*)(*(int*)0x160F00 + 0)  = x | 0x30000000;
*(int*)(*(int*)0x160F00 + 4)  = (int)p;
*(int*)(*(int*)0x160F00 + 8)  = 0;
*(int*)(*(int*)0x160F00 + 12) = 0;
int n = *(int*)0x160F00 + 0x10;
D_00160F00 = n;
D_00162C20 = n;
```
But local EGC 2.95.2 (both -O1 and -O2, -G8) **always hoists** the constant
address into a dedicated base register when it is reused: it emits
`lui v1,0x16; ori v1,v1,0xf00` once, then `lw X,0(v1)` per load (3-instr full
constant, base reused, load regs v0,a1,v0,a0,v0). It never emits the
original's 2-instr self-based re-materialized form.

## Forms tried (all in /tmp/opencode/vu1exp, -G8, -O2 and -O1)
- plain `extern int` (small-data) -> GPREL 1-instr loads (CSE'd to 1 load for the
  local-pointer form, 5 GPREL loads for the recompute form). Wrong access mode.
- constant `(int)*(int*)0x160F00` -> hoisted full constant (lui+ori) into v1. Wrong.
- `__attribute__((section(".data")))` int -> hoisted base (lui a2 + lw X,0(a2)). Wrong.
- struct (32B, non-small-data) field access -> hoisted base. Wrong.
- -O1 -> same hoisting as -O2.

Conclusion: the self-based non-hoisted load pattern is an EGC version/flag
difference; the local toolchain cannot regenerate it. Byte-for-byte match not
achievable with the current compiler. Both the function and its dead tail
func_00233880 are blocked. The vuchain family shares this pattern (see
VU1_addGSregister, func_00233C28, VU1_setScissor, etc.) — expect the same
blocker unless the exact original compiler flags are identified.
