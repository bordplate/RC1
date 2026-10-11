# Hud_sendTexture__FPciiiii (0x200B10, 0x170) — MATCHED 2026-10-11

VU1 texture transfer. Appends a 96-byte load-image def record (filled by
`sceGsSetDefLoadImage`) plus a data reference to `src` to the VU1 command chain,
or, when `immediate` is set, copies the image descriptor to the stack and
transfers the texture directly via `FlushCache(0); sceGsExecLoadImage(image, src)`.
Sole caller is `code/game/bmain.cpp:178`.

## Solution

`code/game/hud_post_post2_sendtex.cpp`, isolated into its own SN (ps2eeas) TU by a
Splat boundary split in `config/RC1.yaml`:

```
- [0xffc30,  cpp, game/hud_post_post2_post2]     # leading: func_001FFC30..func_00200958 (GNU)
- [0x100b10, cpp, game/hud_post_post2_sendtex]   # Hud_sendTexture (SN, default)
- [0x100c80, cpp, game/hud_post_post2_post3]     # tail: func_00200C80..draw_bootImage (GNU)
- [0x101520, cpp, game/init]
```

`hud_post_post2_post3.o` was added to the GNU-assembler list in the Makefile so the
moved tail keeps the same assembler it previously used in `hud_post_post2_post2.o`.
The new `sendtex.o` uses the default `-snas`.

```cpp
extern volatile u32* volatile vu1ChainHead;   // 0x160F00 (plain, NOT .data)
extern volatile u32* vu1ChainHeadStore;        // linker alias, same 0x160F00

void Hud_sendTexture(char* src, int dest, int format, int uLog, int vLog,
                     int immediate) {
    sceGsLoadImage lregs;
    sceGsLoadImage* image;
    int qwc, tbw, t;

    t = 1 << uLog;
    tbw = t >> 6;
    qwc = 1 << (uLog + vLog - 4);
    tbw = tbw > 0 ? tbw : 1;
    if (immediate == 0) {
        vu1ChainHead[0] = 0x10000006;
        vu1ChainHead[1] = 0;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = 0x50000006;
        u32* head = (u32*)vu1ChainHead;
        vu1ChainHeadStore = head + 0x1C;
        image = (sceGsLoadImage*)(head + 4);
    } else {
        image = &lregs;
    }
    sceGsSetDefLoadImage(image, dest, tbw, format, 0, 0, 1 << uLog, 1 << vLog);
    if (immediate == 0) {
        vu1ChainHeadStore[0] = qwc | 0x30000000;
        vu1ChainHead[1] = (u32)src;
        vu1ChainHead[2] = 0;
        vu1ChainHead[3] = qwc | 0x50000000;
        vu1ChainHeadStore = vu1ChainHead + 4;
    } else {
        FlushCache(0);
        sceGsExecLoadImage(image, src);
    }
}
```

## Why the SN-TU split was required

The original reads the VU1 chain head pointer (`vu1ChainHead`, 0x160F00) in a mixed
pattern that no single GNU-TU or `.data` declaration can reproduce:

- **Nine loads** of the head pointer, each a FRESH self-based `lui r,0x16; lw
  r,0xf00(r)` pair in a NORMAL (non-delay) position, before each of the nine
  packet stores.
- **Four accesses** (three head-update stores + one first-store of the post-call
  record) go through the plain alias `vu1ChainHeadStore` as GPREL (`sw/lw
  r,0xa300($28)`, gp=0x166C00), and all sit in branch/bnez DELAY SLOTS.

Under `-G8`, a PLAIN double-volatile head declaration makes EGC emit one bare
`lw r, vu1ChainHead` pseudo per access (no CSE). `ps2eeas` expands each bare
pseudo into a fresh self-based `lui/lw` pair in place — exactly the nine loads. The
plain alias expands to the GPREL form in the delay slots. A GNU-assembled TU
expands the same bare pseudo to GPREL (wrong), and a `.data` declaration makes EGC
emit an explicit `lui %hi; lw %lo` split that it then CSEs into ONE shared base
register across all nine accesses (wrong). EGC's `.s` is identical for GNU and SN
assemblers, so the CSE decision is at RTL level and only the assembler can fix the
plain form. Hence the function must live in its own default-SN TU. This is the
same plain double-volatile + plain-alias mechanism as the matched
`ResetGsRegisters__Fv` (draw_post) and `VU1_addDataRef__FPvi` (vuchain).

## The tbw clamp (the one real wall)

`tbw = (1 << uLog) >> 6` is then clamped. The original emits:

```
slt   v1, zero, t4    # v1 = (tbw > 0)
movz  t4, a3, v1      # if v1 == 0, t4 = 1
```

i.e. `tbw = (tbw > 0) ? tbw : 1` (clamp UP to at least 1; result = max(tbw,1)).
An earlier read of the `movz` direction as "clamp down" was wrong. The matching
C form is `tbw = tbw > 0 ? tbw : 1;` (variant L). Verified failures:

- `(tbw > 0) ? 1 : tbw` / `(0 < tbw) ? 1 : tbw` / `(tbw >= 1) ? 1 : tbw` (inline):
  EGC folds the compare to `slti v1, t4, 2` (a max-clamp), wrong predicate.
- separate `int pos = tbw > 0; tbw = pos ? 1 : tbw;`: correct `slt` but emits
  `movn` (funct 0x0b) instead of `movz`.

R5900 opcode quirk: `movz` = funct 0x0a and `movn` = funct 0x0b — the OPPOSITE of
standard MIPS. objdump (BFD 2.9-ee) and Splat both label word 0x00e3600a `movz`.

Register allocation: t5=uLog, a3=1 (format reused in the bnez delay slot),
t4=tbw, v0=(uLog+vLog-4) exponent, s2=qwc, s3=src, s1=immediate, s0=image,
t0=vLog, t1=immediate. Frame 0xB0 (lregs@sp, s0..s3, ra). Call targets:
sceGsSetDefLoadImage=0x122330, FlushCache=0x118A80, sceGsExecLoadImage=0x122658.

## Verification

The 368-byte object matches the reference with 0 real diffs (the only probe diffs
are unlinked-symbol artifacts: head `lui/lw` and GPREL `0` in the unlinked probe
vs `0x16`/`0xf00`/`0xa300` linked). Full `make` + `cmp build/boot_elf.elf
assets/boot_elf.elf` passes byte-for-byte.
