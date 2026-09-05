# snd_StopSound (vram 0x12E368, file 0x2F2E8, 0x30 bytes) — MATCHED

Stops one VAG/sound stream: builds a 1-word payload on the stack and sends
IOP sound command 0x15 ("stop sound", 4-byte payload = stream id) via
`snd_SendIOPCommandNoWait(0x15, 4, &data, 0, 0)`. Replaced in
`code/989snd/ee/989snd.c` (placeholder was line 45) with:

```c
extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);

void snd_StopSound(int id) {
    int data = id;
    snd_SendIOPCommandNoWait(0x15, 4, &data, 0, 0);
}
```

## Original body (raw words, LE)

```
27bdffe0  addiu $sp,$sp,-0x20
24050004  li    $5,4                # count
afa40000  sw    $4,0($sp)           # data[0] = id
03a0302d  move  $6,$sp              # data ptr = sp
7fbf0010  sq    $31,0x10($sp)
24040015  li    $4,0x15             # cmd
0000382d  move  $7,$0               # x = 0
jal       snd_SendIOPCommandNoWait (vram 0x12E6E0)
0000402d    move  $8,$0             # y = 0 (delay slot)
7bbf0010  lq    $31,0x10($sp)
03e00008  jr    $ra
27bd0020    addiu $sp,$sp,0x20      (delay slot)
```

## Findings

- Callers (Ghidra xrefs): 0x002160FC (music_UpdateStream) and 0x0022D4E4
  (sound_update) both pass exactly one int (the stream id) in $a0 and ignore
  the return, so the signature is `void snd_StopSound(int)`.
- The 5-arg callee takes (cmd, count, void* data, x, y): the prologue at
  0x12E6E0 moves a0..a4 into s-registers and the byte-copy loop reads the
  payload from a2.
- Sibling `snd_SetMasterVolume` (0x12E208) is the 2-word-payload twin of the
  same shape — next easy target in this family.
- Three source forms (`int data = id; &data`, `int data[1]; data[0] = id`,
  `int data[1] = { id }`) all compiled byte-identical under local EGC
  2.95.2 -G8 -O2; the plain scalar form was kept.
- Verified: build ELF slice 0x2F2E8..0x2F318 == assets slice (12 words,
  e0ffbd27 04000524 0000a4af 2d30a003 1000bf7f 15000424 2d380000
  b8b9040c 2d400000 1000bf7b 0800e003 2000bd27); full make +
  cmp build/boot_elf.elf assets/boot_elf.elf passes; decomp_status 811->810.
