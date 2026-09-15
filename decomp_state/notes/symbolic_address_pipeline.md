# Symbolic addresses in the RC1 build pipeline

## Verified pipeline

C/C++ compilation defaults to EEGCC 2.95.2 SN 2.73a with SN `ps2eeas` 1.8.19.316,
selected with `-snas`. Five compatibility TUs retain GNU assembly; the verified
production build uses those explicit overrides and passes full-image parity.
See [sn_toolchain_assemblers.md](sn_toolchain_assemblers.md) for installation,
archive fingerprints, version comparisons, and reproduction commands.

Ordinary symbolic scalar C++ matches the original bytes in these controls:

| Function | Original | GNU assembler | SN assembler |
| --- | ---: | ---: | ---: |
| DrawMobys | 128 B | 112 B, mismatch | 128 B, exact |
| ProcessMobyAnimData | 68 B | 64 B, mismatch | 68 B, exact |
| menu_post_openInventory (0x208E68) | 36 B | 28 B, mismatch | 36 B, exact |

The same unchanged compiler assembly gives those results with SN versions
1.8.19.316, 1.9.6.516, and 1.9.25.758. The 2.73a cc1plus binary is identical
to the one already installed. No RAM-address constants or postprocessor are
needed for these symbolic probes. Existing production implementations still
contain older source workarounds; source migrations need their own full-TU
and linked-byte verification.

## Compiler scheduling versus assembler expansion

Plain small scalar declarations produce symbolic macros like:

```asm
lw      $4,D_0015FF18
li      $6,-1
lw      $5,D_0015FF14
.set noreorder
.set nomacro
jal     MobyProc
li      $7,1
.set macro
.set reorder
lw      $4,vu1ChainHead
lw      $3,D_00160F08
slt     $3,$3,$4
.set noreorder
.set nomacro
beq     $3,$0,label
sw      $2,D_0015FF14
```

With `.extern NAME, 4` metadata, GNU as reduces all five accesses to GP-relative
instructions. SN instead emits the original four self-based absolute load
pairs while retaining the single GP-relative store in the branch delay slot.
The SAME global is loaded absolutely and stored GP-relatively. A uniform
`.data` declaration or an address-cast macro does not explain both accesses.

The verified mechanism is ps2eeas's single-pass expansion: a memory-symbol
reference becomes a GPREL16 access only when `.extern NAME, N` already
appeared earlier in the file OR the reference is inside a
`.set noreorder`/`.set nomacro` region (here, the branch-delay-slot block
around the store); otherwise it is an absolute self-based `lui/lw` pair.
EGC emits all `.extern` declarations at the end of the file, so the four
out-of-block loads see no prior declaration and expand absolute, while the
in-block store expands GPREL. See
[sn_toolchain_assemblers.md](sn_toolchain_assemblers.md) for the probe matrix
and the menu.cpp `.extern`-seeding fix that forces an out-of-block load to
GPREL.

For ProcessMobyAnimData, the array address for FastMemCopy remains compiler-
split and schedulable, the first scalar MobyAnimProc argument is expanded to
an absolute load, and the second remains GP-relative in the call delay slot.

The compiler sees declaration size, section and flags, not the eventual RAM
address. The assembler emits relocations; the linker resolves their addresses
and checks range. `$28` is GP, while `$s0` is register 16. An address hidden in
an enum or define is still a non-relocatable numeric address.

## Controls and limitations

The historical GNU controls establish why changing compiler source alone was
insufficient for the original probes:

- `-mno-gpopt` or `-Wa,-O0`: DrawMobys remains 112 B versus 128 B.
- Compiler `-G8`, GNU assembler `-Wa,-G0`: DrawMobys becomes 132 B; its
  delay-slot store expands incorrectly. ProcessMobyAnimData becomes 72 B
  because its delay-slot load expands too. Neither is a solution.
- `.sdata` externs alone do not establish that SN will choose GP addressing.
  The historical GP/RPC unit controls explicitly retain GNU assembly, as do
  their production TUs. GP references outside delay slots CAN be forced with
  an in-function `asm volatile(".extern sym, N");` seed before the reference
  (menu.cpp, 2026-09-15); the remaining production TUs still need that
  per-function treatment before migration.
- The symbolic volatile-pointer VU1_addDataRef probe is 84 B versus 76 B with
  SN, and VU1_gsRegsNormal is 100 B versus 92 B. These source forms still need
  work; the assembler switch does not solve every VU scheduling issue.
- GP accesses exist outside delay slots elsewhere in the original. Do not
  extrapolate a universal rule that all GP accesses must be delay-slot accesses.

## Historical expansion-model experiment

`tools/research_symbolic_addresses.py` explicitly compiles with GNU assembly
and separately tests a reference-independent expansion model. It recognizes
small `.extern` symbols, expands unsplit memory macros outside `.set noreorder`
to absolute pairs, and leaves noreorder/already-split accesses alone — a
two-pass approximation of the single-pass rule above that is exact for these
controls because EGC's `.extern` declarations sit at the end of the file.
The three positive controls match and the two VU controls fail. This model is
retained as a reproducible experiment; production uses the authentic SN
binary.

```sh
source .venv/bin/activate
python tools/research_symbolic_addresses.py --out /tmp/opencode/symbolic-addresses
```

`candidate.*` records the GNU baseline, `model.*` the experimental expansion,
and `summary.json` records both plus the overlay comparisons. Reference bytes
are validated against the boot asset. All final comparisons are unmasked.

## Overlays: link-time variation is sufficient

The normal-level assets are ET_EXEC ELF files. DrawMobys belongs to replaceable
level text, not resident `core.text`. The same model-generated relocatable
DrawMobys object links to exact 128-byte copies in all 19 normal overlays.

| Corresponding item | Boot / start | Novalis | Kerwan |
| --- | ---: | ---: | ---: |
| DrawMobys | 0x20D460 | 0x264E18 | 0x23EBA8 |
| Gate | 0x18A2D8 | 0x16A498 | 0x16A118 |
| Moby chain head cell | 0x15FF14 | 0x15FFD4 | 0x15FFD4 |
| Moby data cell | 0x15FF18 | 0x15FFD8 | 0x15FFD8 |
| VU1 head cell | 0x160F00 | 0x1611C0 | 0x1611C0 |
| VU1 limit cell | 0x160F08 | 0x1611C8 | 0x1611C8 |
| Diagnostic string | 0x1E8400 | 0x209F10 | 0x1E1B30 |

The four small-data cells are at the same addresses across all 19 normal
overlays, but differ from boot; the gate and string vary between levels. The
original GP store changes from word `0xAF829314` to `0xAF8293D4`, consistent
with the head cell moving by `0xC0` while `_gp` remains `0x166C00`.

Overlay searches mask address fields only to locate the corresponding function;
linking uses the corresponding operand addresses and comparison checks every
byte. This verifies function ranges, not complete overlay binaries. The
observed variation needs no numeric per-level C++ headers or runtime lookup.
