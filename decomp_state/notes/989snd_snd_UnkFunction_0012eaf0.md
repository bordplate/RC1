# snd_UnkFunction_0012eaf0 (0x0012EAF0)

## What it does

```c
extern int D_0015ECC4;

void snd_UnkFunction_0012eaf0(void) {
    D_0015ECC4 = 1;
}
```

Sets `D_0015ECC4` (0x15ECC4 = gp − 0x7F3C) to 1 and returns. The global is the
989snd **RPC-in-flight / return-pending flag** in the sound command batching
machinery:

- `snd_FlushSoundCommands` (0x12DDDC) only calls `snd_SendCurrentBatch` while
  the flag is 0 — a pending return blocks new batches.
- `snd_SendIOPCommandNoWait` (0x12E820-82C) clears the flag before issuing a
  batch (with a `$t1` "cleared" marker) and re-sets it to 1 afterward
  (0x12E8B0-8B8), since the new RPC is in flight again.
- The paired `snd_UnkFunction_0012eb00` (0x12EB00, matched earlier) clears the
  flag and flushes.

All six callers use the (cleared-then-set) trio, e.g. FUN_00216828 at
0x216878: `snd_UnkFunction_0012eb00(); snd_FlushSoundCommands();
snd_UnkFunction_0012eaf0();` — reset the channel, then mark the return path
armed. Sites: 0x1EB9A4 (movie setup), 0x216878/0x2168F8/0x21692C (game
sound/update), 0x22D6B4, 0x22EB5C. None use a return value, consistent with
the leftover `v0 = 1`.

The pinned symbol name `snd_UnkFunction_0012eaf0` (config/symbols.txt:42) is
kept, same as the paired 0012eb00.

## Codegen notes

- Void function with a single constant store: EGC hoists `li v0,1` to the top
  of the body and the GP-relative store reuses it — the documented
  void-constant-store shape (contrast: an int-returning version would
  materialize the store into v1 plus a separate `li v0`, which the 4-word
  original does not have).
- `D_0015ECC4` is the pre-existing plain `extern int` at 989snd.c:17 (no
  section attribute, no cast). 0x15ECC4 is inside the gp window
  (gp = 0x166C00, ±32K), so EGC emits the store as
  `sw $v0, -0x7F3C($gp)` with R_MIPS_GPREL16 — same idiom as the matched
  `snd_UnkFunction_0012eb00` in this file.
- No frame: no locals, no calls, no return value.

## Verification

- Original words (LE): `0x24020001` (li v0,1), `0x03E00008` (jr ra),
  `0xAF8280C4` (sw v0,-0x7F3C(gp)), `0x00000000` (nop).
- Built object disassembly of the function: 4/4 words identical; GPREL16 store
  reloc resolves to the observed 0x80C4 displacement at link.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
- Nonmatching count 764 → 763. decomp-verifier PASS.
