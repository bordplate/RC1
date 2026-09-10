# snd_SetSoundParams_CB (0x0012E4C0)

## What it does

```c
void snd_SetSoundParams_CB(int a, int b, int c, int d, int e, int f, int g, int h) {
    int buf[6];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    snd_SendIOPCommandNoWait(0x21, 0x18, buf, g, h);
}
```

989snd "set sound parameters" wrapper (IOP command 0x21, 0x18 payload bytes):
byte-identical twin of the matched `snd_PlaySoundVolPanPMPB` (0x12E308,
command 0x11) — same 6-int stack buffer, same forwarding of the 7th/8th
parameters (t2/t3) as the IOP call's x/y slots. The only difference in the
original is the command constant: `addiu a0,0,0x21` vs `addiu a0,0,0x11`.

Callers: the sound-update loop (FUN_0022CA50, call at 0x22D61C) and
0x21658C. The 0x22D61C call passes
(volume, flags, pan, uVar19, iVar11, iVar10, 0x22DDD8, local_e4) — g is a
data-table pointer constant and h an entity pointer, mirroring the
PlaySoundVolPanPMPB call shape (g=0x22DD90, h=&DAT_0013E5C0+iVar18).

## Codegen notes

- EGC 8-argument register ABI applies (7th/8th int args in t2/t3 — see
  notes/989snd_snd_PlaySoundVolPanPMPB.md): the original's `move a3,t2`,
  `move t0,t3`, and `sw t1,20(sp)` in the jal delay slot are all reproduced
  by EGC from this exact C.
- The `jal snd_SendIOPCommandNoWait` is an R_MIPS_26 on the `.text` section
  symbol (callee is the later INCLUDE_ASM in this same TU); judged by the
  linked result (0x0C04B9B8 → 0x12E6E0), per the same-TU-jal observation in
  AGENTS.md.

## Verification

- Standalone probe (`decomp_state/probes/snd_setsoundparams.c`) via
  `tools/decomp_probe.py`: 72/72 bytes, 18/18 words, zero differences.
- Linked-ELF function words (0x12E4C0..0x12E508) compared equal to the
  original.
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: identical. Count 728 -> 727.
