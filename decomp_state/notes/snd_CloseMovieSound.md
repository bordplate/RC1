# snd_CloseMovieSound (vram 0x12F0E0, file 0x30060, 0x24 bytes)

Sends SIF sound command 0x3C ("close movie sound") to the IOP via
`snd_SendIOPCommandAndWait(0x3C, 0, 0)` with no data payload. Replaced in
`code/989snd/ee/989snd.c` (placeholder was line 117) with:

```c
extern void snd_SendIOPCommandAndWait(int cmd, int count, void* data);

void snd_CloseMovieSound(void) {
    snd_SendIOPCommandAndWait(0x3C, 0, 0);
}
```

## Original body

```
27bdfff0  addiu $sp,$sp,-0x10
2404003c  li    $4,0x3C            # cmd
7fbf0000  sq    $31,0($sp)
0000282d  move  $5,$0              # count = 0
jal       snd_SendIOPCommandAndWait (vram 0x12E548)
0000302d    move  $6,$0            # data = NULL (delay slot)
7bbf0000  lq    $31,0($sp)
03e00008  jr    $ra
27bd0010    addiu $sp,$sp,0x10     (delay slot)
```

## Findings

- Splat's generated comments mislabel the EE `move` instruction (funct word
  0x...282d) as `daddu $r,$0,$0`. The raw bytes are identical to what EGC
  emits for zeroing/moving a register, so such "daddu r,$0" words in
  nonmatching asm are NOT an obstacle — they match EGC `move` output
  byte-for-byte. (Verify with objdump of assets/boot_elf.elf before trusting
  the mnemonic.)
- Callee signature: Ghidra shows `FUN_0012e548(undefined8 param_1,int,int)`
  but its prologue does `move s4,a0; move s0,a1` and the byte-copy loop reads
  the data pointer from a2 — the three args are separate 32-bit values
  (cmd, count, data*). The undefined8 grouping is a Ghidra artifact. Declaring
  the prototype `(int,int,int)` (or `(int,int,void*)`) reproduces the caller
  bytes; do NOT use `(long,int,int)` — that would also zero a3 and add a word.
- Sole caller is the small fn at vram 0x23AC90 (`jal 0x12F0E0; nop; ... li
  v0,1`) which passes no args and overwrites v0 afterward — `void` return is
  safe. No C source referenced it before this change.
- Callee behavior (for context): stages `count` bytes from data* into lit
  buffer DAT_0x133140, cache-flushes while a GP flag is set, waits on IOP via
  the 0x11B3B8/0x116078/0x12DC80/FlushCache poll thunks, then issues the SIF
  command block at lit 0x15EBC0 (with or without the staged data) and returns.

## Verification

- Standalone EGC test (-G8 -O2 -ffast-math -fno-exceptions, C) of the exact
  call produced all 9 words identical on first attempt.
- Build: 989snd.o objdump shows `T snd_CloseMovieSound` (no NON_MATCHING
  alias); body byte-identical to original ELF slice vram 0x12F0E0..0x12F104.
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes
  (md5 0c081d4e0353b419d95e5f95eb0ebacb both). decomp_status 856 -> 855.
