# func_002335A0 (code/game/vuchain.cpp) — matched

VRAM 0x2335A0, 0x2C bytes, no calls. Replaced `INCLUDE_ASM` with C and
verified byte-for-byte via object relocations, linked-ELF word diff, full
`cmp` parity, and the decomp-verifier subagent (2026-09-08).

## What it does

Looks up the current VU1 chain buffer pointer and caches a related table
entry:

```c
extern "C" int D_00160F0C;
extern "C" int D_001DDFB8[];

extern "C" int *func_002335A0(void) {
    int idx = *(int *)0x15ED84;
    if (idx >= 19) idx = 0;
    int *p = D_001DDFB8 + idx;
    D_00160F0C = *p;
    return p;
}
```

- `*(int *)0x15ED84` — current chain index (boot value 0).
- `D_001DDFB8` — 19-element table of main-memory VU chain buffer pointers
  (boot values 0x160000..0x1C0000, e.g. 0x1c0000, 0x160000, 0x1a0000, ...).
- `D_00160F0C` — stores the looked-up value (a pointer); sits inside the
  original `.lit` pool region (0x15EF00-0x161228), which the game reuses as
  general RAM. Boot value 0.
- Return: the table element pointer (`int *`); caller 0x219328 stores it to
  D_0015F5B8, which `VU1_initChain__Fv` later loads.

## Original words (ground truth, objdump of assets/boot_elf.elf)

```
3c020016 lui  v0,0x16
8c42ed84 lw   v0,-4732(v0)        # *(int*)0x15ED84, absolute load
3c04001e lui  a0,0x1e
2484dfb8 addiu a0,a0,-8264        # D_001DDFB8, signed %hi/%lo split
28430013 slti v1,v0,19
0003100a movz v0,zero,v1          # idx = (idx<19)?idx:0
00021080 sll  v0,v0,2
00441021 addu v0,v0,a0
8c430000 lw   v1,0(v0)
03e00008 jr   ra
af83a30c sw   v1,-0x5CF4(s0)      # D_00160F0C (GPREL vs gp=0x166C00)
```

## Key findings (persistent)

1. **The store base is s0, not gp.** Splat prints `sw $3,-0x5CF4($28)` and
   objdump mislabels the base `(gp)`. The word 0xAF83A30C decodes to
   rs=28 (s0); imm 0xA30C is signed -0x5CF4, so target = 0x166C00-0x5CF4
   = 0x160F0C. In this EGC build a plain `extern "C" int` global inside the
   gp window compiles to an **s0-relative** GPREL16 access
   (`lw/sw r, addr-0x166C00 (s0)`); the original text contains 764 such
   accesses; confirmed in previously matched functions
   (texResetCursor__Fv, space_func_0022E188, ...). The GPREL16
   relocation fills the offset exactly as for gp, so a plain extern matches
   whenever the original shows the s0 base. (My first parity run failed on
   one word because I used the wrong symbol D_00160F24 — 0x18 bytes away;
   D_00160F0C is the true target. Always decode the imm against gp to find
   the target address; don't trust the printed base label or a guessed
   symbol.)
2. **Out-of-window constant vs symbol address splitting.** For the
   0x1DDFB8 base (outside the gp window):
   - constant cast `(int *)0x1DDFB8 + idx` → EGC emits an UNSIGNED split
     `lui a0,0x1d; ori a0,a0,0xdfb8` (wrong vs original);
   - symbol reference (`extern "C" int D_001DDFB8[]`) → `%hi/%lo` relocs,
     assembler SIGNED split `lui a0,0x1e; addiu a0,a0,-0x2048` (matches).
   So when the original shows a signed hi/lo split for an out-of-window
   address, use a real symbol, not a cast. (Contrast with the gp-window
   case, where casts are needed to force absolute lui/lw — 0x15ED84 load
   here is a cast and matches.)
3. `D_001DDFB8` exists at `build/undefined_syms_auto.txt:441`; `D_00160F0C`
   and `D_0015ED84` are not in the auto files but are assigned in
   `build/SCUS_971.99.ld` (lines 8068, 3762), which is why plain externs
   link.

## Probes

- `decomp_state/probes/vu1_func_002335A0_v1.cpp` — cast-based base: 9/11
  words, unsigned-split mismatch on the lui/ori pair.
- `decomp_state/probes/vu1_func_002335A0_v2.cpp` — symbol base: 11/11.

## Commit

One commit: vuchain.cpp + this note (+ AGENTS.md observation). 736→735
nonmatching.
