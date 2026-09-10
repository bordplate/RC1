# snd_StreamSafeCdGetError (0x0012EEF0)

## What it does

```c
int snd_StreamSafeCdGetError(void) {
    if (!snd_cdStreamActive)
        return func_00121630();
    return snd_cdStreamInfo.error;
}
```

Safe-CD error getter for the 989snd streaming-CD path. If the CD stream is
active (`snd_cdStreamActive`, 0x15EC8C) it returns the cached CD error code from
the stream-info struct (`snd_cdStreamInfo.error`, 0x137B10); otherwise it falls
back to the SCE library's raw CD error/status getter `func_00121630` (0x121630,
no-arg, returns int, -1 on error) — the same raw-CD fallback the sibling
`snd_StreamSafeCd*` functions use.

`snd_cdStreamActive` (0x15EC8C, boot value 0) is written by
`snd_InitVAGStreamingEx` (0x12EB20) with the IOP init result and read as
`if (!x)` by every SafeCd function in the 0x12EB24-0x12EF2C region (CdRead,
CdSync, CdBreak, GetError, Callback). The `snd_cdStreamInfo` struct (0x137B00,
in the 0x137280 buffer at +0x880) holds the safe-CD state at 0x00 (zeroed by
`snd_StartSoundSystem`, set by CdRead, compared to 1 by CdSync) and the CD error
at 0x10 (cleared by CdRead, returned here).

## Codegen notes

- **Gate global is 0x15EC8C, not 0x15EE8C (Gp-name trap).** The `lw
  v0,-32628(gp)` at 0x12EEF4 is an in-gp-window plain-extern access whose base
  decodes to **s0 ($28)**, not gp (objdump mislabels it `(gp)`; confirmed
  `lw $2,-0x7F74($28)` encodes to word 0x8F82808C). The GPREL16 offset is
  `0x15EC8C - 0x166C00 = -0x7F74` (imm 0x808C). The tempting 0x15EE8C label is
  0x200 too high and yields imm 0x828C (a one-word mismatch). Declaring
  `extern int snd_cdStreamActive;` (no section attribute) reproduces the
  s0-relative access exactly.
- **`snd_cdStreamInfo` (0x137B00) must be `volatile`.** The error field is at
  offset 0x10, out of the gp window, so EGC materializes the base with
  `lui v1,0x13; addiu v1,v1,0x7B00; lw v0,16(v1)`. Without `volatile` EGC folds
  the load (hoists the `lui` and moves the `lw` into the `beqz` delay slot),
  giving a 13-word shape instead of the original 14. Both a non-`volatile`
  struct and a non-`volatile` `int[5]` array form mismatch; only the
  `volatile` struct matches.
- **Relocations:** `R_MIPS_GPREL16 snd_cdStreamActive`, `R_MIPS_HI16/LO16
  snd_cdStreamInfo` (signed split 0x13/0x7B00), `R_MIPS_26 func_00121630`.
  No `.sdata`, no unexpected GPREL16.
- Symbols `snd_cdStreamActive` (0x15EC8C) and `snd_cdStreamInfo` (0x137B00)
  were added to **both** `config/symbols.txt` and `config/linker_aliases.ld`;
  a data symbol present in only `symbols.txt` does not appear in the generated
  `.ld` and will not link (existing named data symbols all live in both files).

## Verification

- Function slice 0x12EEF0..0x12EF28 (14 words) byte-identical to the original;
  SHA1 of the slice equal on both ELFs.
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf`: byte-identical. Count 726 -> 725.
