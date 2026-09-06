# mpegError__FP7sceMpegP18sceMpegCbDataErrorPv (vram 0x23D080, file 0x13E000, 40 bytes) — MATCHED

SCE Mpeg decode-thread error callback registered with the video decoder.
Prints the message-string pointer stored at offset 4 of the callback data
(`*(int*)((char*)err + 4)`) through the `.lit` format string at D_00161220
(`"%s\n"`, the last label of `.lit`, 0x15EF00-0x161228) via the printf import
thunk STUB_printf (vram 0x1E93B0, kept as INCLUDE_ASM in
code/_generated/game/stub.s), then returns 1 (decode thread continues).
Ghidra (FUN_0023d080) agrees: `FUN_001e93b0(0x161220, *(u32*)(p2+4)); return 1;`.

Matched 2026-09-06. Final form:

```cpp
extern char D_00161220[];
extern "C" void STUB_printf(const char* fmt, ...);

int mpegError(struct sceMpeg* mpeg, struct sceMpegCbDataError* err, void* user) {
    STUB_printf(D_00161220, *(int*)((char*)err + 4));
    return 1;
}
```

Verification:

- `tools/decomp_probe.py` on the final source: 40/40 bytes identical to the
  original at 0x23D080 (candidate recompiled into its own .elf at the original
  address with runtime GP 0x166C00; unrelated TU externs passed with
  `--define` from their Splat name-encoded addresses, experiment-only).
- Callee prototype: the return value of STUB_printf is discarded, and both
  `void STUB_printf(const char*, ...)` (project convention, movie.cpp:20 /
  mobyfunc.cpp:21) and `int STUB_printf(char*, ...)` produce identical bytes
  here (negative probe: the `void` form also matches 40/40). The `void`
  convention is used.
- Data declaration: D_00161220 is a `.lit` label defined in
  code/_generated/build/data/lit.lit4.s, exactly like D_001611F8 used by the
  matched ErrMessage (movie_errmessage.md). A plain `extern char D_00161220[];`
  (no section attribute) compiles to the original `lui/addiu` absolute load
  and matches; an earlier WIP form with `section(".data")` also matched (the
  attribute is ignored for the extern; the definition's `.lit4` section wins)
  but is not the project idiom.
- Full build + `cmp build/boot_elf.elf assets/boot_elf.elf` passes with the
  candidate.
- Symbol name pinned: config/symbols.txt:407
  (`mpegError__FP7sceMpegP18sceMpegCbDataErrorPv = 0x23d080`).

Original shape (9 words): 0x10 frame, arg1+4 lw in the `jal` delay slot,
`lui/addiu` of D_00161220 into $4 before the `sq $ra`, hoisted
`addiu $2,$0,1` before `jr $ra`, standard epilogue.
