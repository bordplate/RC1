# snd_StopAllStreams (code/989snd/ee/989snd.c) — MATCHED 2026-09-05

`void snd_StopAllStreams(void)` at vram `0x0012EBD0` (file offset `0x2FB50`), 0x2C
bytes (plus 0x4 bytes of dead tail, see below). Sends IOP sound-system command
0x34 (52, "stop all streams") with no data through the no-wait path.

## Identity / context

- Sole caller: 0x215FC4, an unconditional `jal` inside `FUN_00215EE8`
  (0x215EE8–0x21604F, still INCLUDE_ASM) with no arguments.
- Sibling family: same shape as `snd_StopAllSounds` (0x12E990, cmd 0x18),
  `snd_ResetMovieSound` (cmd 0x3d, wait path), etc. — thin wrappers around
  `snd_SendIOPCommandNoWait` / `snd_SendIOPCommandAndWait`.
- Prototype already present in the file (line 4):
  `extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);`
  — the callee is still INCLUDE_ASM'd in the same translation unit (line 77).

## Replacement

```c
void snd_StopAllStreams(void) {
    snd_SendIOPCommandNoWait(0x34, 0, 0, 0, 0);
}
```

No mangling involved (pure C file, unmangled symbol).

## Codegen facts that matter

- 0x10 frame: `addiu sp,-0x10; li a0,0x34; sq ra,0(sp); move a1,a2,a3,zero;
  jal; move t0,zero; lq ra; jr ra; addiu sp,0x10`. EGC hoists the `li a0,0x34`
  above the `sq` (constant-load scheduling), matching the original exactly.
- The `jal` word needs a relocation (see AGENTS.md note on section-symbol
  R_MIPS_26 for calls to same-TU later-defined functions): the candidate
  object emits `0c00032e` with an R_MIPS_26 on the `.text` section symbol
  (field = callee's section offset 0xCB8 >> 2). ps2-elf-ld resolves it to
  `0c04b9b8` at link time = the original word (section VMA 0x12DA28 + 0xCB8
  = 0x12E6E0 = snd_SendIOPCommandNoWait). All 10 non-relocated words are
  byte-identical pre-link; the linked ELF word was verified equal.
- This is the same reloc pattern as every other wrapper in the file
  (snd_StopSound, snd_PauseVAGStream, ...) — identical mechanism, not a fluke.

## The dead tail (func_0012EC00) — blocked, INCLUDE_ASM retained

```
0x12EBD0: ...
0x12EBF4: lq   ra, 0(sp)
0x12EBF8: jr   ra
0x12EBFC:      addiu sp, sp, 0x10    (delay slot)
0x12EC00: addiu sp, sp, 0x10         <- unreachable, file 0x2FB80
0x12EC04: nop
```

A dead second deallocate of the parent's own 0x10 frame, at the 8-aligned
address right after the epilogue — the classic dead-tail artifact (see
help_msg_string__Fi.md for the systemic ~46-fragment analysis).

- Deadness: Ghidra has no function at 0x12EC00 and no xrefs to it; nothing
  jumps there.
- Not regenerable with local EGC 2.95.2 v2.73a: the standalone experiments in
  `/tmp/opencode/tail_exp/` (t1–t10) show EGC's only dead-code tail mechanism
  is the dead *store* (two stores before `return` → second store lands after
  `jr ra`). No C form produces a dead `addiu sp,sp,N`: the simple wrapper
  (t2) compiles byte-identical for the 11-word body but emits no tail at all,
  and no double-deallocate construct found reproduces the extra dealloc.
- Therefore the orphan `INCLUDE_ASM(..., func_0012EC00)` is retained to supply
  the 4 bytes, and the queue entry is blocked (precedent: func_001FDD50,
  func_00233880).

## Verification

- Candidate object slice (11 words) vs original 0x2C bytes at file 0x2FB50:
  all words equal except the jal (reloc, expected pre-link); linked word
  `0c04b9b8` equals the original.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` → byte-for-byte.
- `make split` moved `snd_StopAllStreams.s` to
  `code/_generated/matchings/989snd/ee/989snd/`; count 808 → 807.
