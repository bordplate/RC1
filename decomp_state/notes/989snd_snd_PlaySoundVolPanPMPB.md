# snd_PlaySoundVolPanPMPB (0x0012E308)

## What it does

```c
void snd_PlaySoundVolPanPMPB(int a, int b, int c, int d, int e, int f, int g, int h) {
    int buf[6];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    snd_SendIOPCommandNoWait(0x11, 0x18, buf, g, h);
}
```

989snd "play sound" wrapper (IOP command 0x11, 0x18 payload bytes): packs six
ints into a stack buffer and forwards the seventh and eighth parameters as the
IOP call's x/y callback slots (same role as b/c in the matched
`snd_SoundIsStillPlaying_CB` (0x19, 4, buf, b, c) family). Callers
(sound_update 0x22D660, func_0022EAA8 0x22EB94) pass e.g.
(id, vol, 1024, 0, 0, 0, 0, 0x14D940) or (s2, s1, *s7, s6, s3, s4, 0x22D940,
*(sp+540)) — g/h are live pointer/int values, not constants.

## Codegen notes

- **EGC 8-argument register ABI (useful, verified):** this EGC 2.95.2 build
  passes the 7th and 8th integer parameters in **t2/t3 ($10/$11)** — i.e. the
  register arg window is a0-a5, t0-t3, not the usual a0-a5 + stack. Evidence:
  the original wrapper forwards t2/t3 into the callee's x/y, and *both*
  callers load t2/t3 as their final call-setup (0x22D65C `addiu t2,v1,-8816`
  immediately before the jal; 0x22EB84-8C `move t2,zero; lui t3,0x14; addiu
  t3,t3,-6720` immediately before the jal). A standalone probe of the exact C
  above compiled with the project flags emitted all 18 words byte-identical —
  EGC even produced the odd-looking original sequence: `move v0,a3` (save arg4
  before a3 is reused), `sw t0,16(sp)`, `move a3,t2`, `sw t1,20(sp)` in the
  `jal` delay slot, `move t0,t3` (arg8 into a4) clobbering arg5's register
  after its buf store.
- Statement order is the natural `buf[0]..buf[5] = a..f`; no reordering needed.
- The `jal snd_SendIOPCommandNoWait` is an R_MIPS_26 on the `.text` section
  symbol (callee is the later INCLUDE_ASM in this same TU), so the object word
  is a placeholder; the linker materializes the section-absolute 0x0C04B9B8
  (0x12E6E0). Judged by the linked result, per the same-TU-jal observation in
  AGENTS.md.

## Verification

- Object 17/18 words identical vs 0x12E308; the 18th is the pending
  R_MIPS_26 → .text (+field) resolving to snd_SendIOPCommandNoWait.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
  Count 765 -> 764.
