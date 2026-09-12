# snd_StreamSafeCdBreak (vram 0x12EEA8, file 0x2FE28, 68 bytes) — matched 2026-09-12

`code/989snd/ee/989snd.c` (C). Matched implementation:

```c
extern int snd_cdStreamActive;
extern int func_001216C8(void);

int snd_StreamSafeCdBreak(void) {
    if (!snd_cdStreamActive)
        return func_001216C8();
    snd_SendIOPCommandNoWait(0x37, 0, 0, 0, 0);
    return 1;
}
```

Semantics: if the stream-safe CD session is inactive, defer to the SCE library
CD break routine (`func_001216C8`, 0x1216C8, generated sce/lib.s; SDK identity
unresolved, address-based name retained with a comment per the sibling
precedent). Otherwise send IOP command 0x37 (break the active CD streaming
read) through `snd_SendIOPCommandNoWait` and return 1.

- `snd_cdStreamActive` (0x15EC8C, `.lit`, inside the gp window) is a plain
  `extern int`; EGC emits the s0-relative GPREL16 load `lw v0,-0x7F74($28)`,
  the same access mode the matched siblings snd_StreamSafeCdGetError /
  snd_StreamSafeCdCallback use.
- Only caller: 0x216708 (game code, different TU), so the definition was moved
  from the old INCLUDE_ASM slot (before the `SndCdStreamInfo` declaration
  block) to just after it, where `snd_cdStreamActive` is already declared.
  No same-TU caller exists, so no same-TU jal reloc ordering changed.

## EGC findings

- Branch direction: EGC makes the LAST-written path the fallthrough. The form
  `if (snd_cdStreamActive) { call; return 1; } return f();` compiles to
  `bne v0,0,ifbody` with the else branch (f) first — the mirror image of the
  original. The early-return form `if (!snd_cdStreamActive) return f();`
  reproduces the original `beqz v0, else` with the IOP call as fallthrough,
  `sq ra` in the branch delay slot, then `b end` / `li v0,1` in that delay
  slot, `jal f; nop` in the else, shared epilogue.
- The 5-arg call schedule (`addiu a0,0,0x37; move a1,zero; move a2,zero;
  move a3,zero; jal; move t0,zero` in the delay slot) is identical to the
  matched snd_StopAllStreams.

## Verification (mechanical)

- `decomp_probe.py` standalone (defines snd_cdStreamActive=0x15EC8C,
  snd_SendIOPCommandNoWait=0x12E6E0, func_001216C8=0x1216C8): 68/68 bytes,
  0 differences.
- `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf`: pass.
- `python3 tools/decomp_status.py --count`: 716 -> 715.
