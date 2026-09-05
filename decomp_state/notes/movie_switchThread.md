# switchThread (0x0023A770, file 0x13B6F0, 0x1C bytes)

## What it does

Yields the EE thread: calls `func_001188C0(1)`, which is a handwritten
libSCE stub (code/_generated/sce/lib.s) that runs `syscall 0x2B`
(sched_yield) after `addiu $v1, $0, 0x2B` — the syscall number is the 5th
syscall argument; the `1` in a0 is unused by the stub. Ghidra names the
stub `RotateThreadReadyQueue`.

## Callers

All five call sites use `nop` delay slots and none reads v0 after the call,
so the return type is void:

- readMpeg__FP8VideoDecP7ReadBufP7StrFile (0x23A5DC, 0x23A6B8, 0x23A6E0)
- decBs0__FP8VideoDec (0x23CF10, 0x23D000)
- mpegNodata__FP7sceMpegP13sceMpegCbDataPv (0x23D0B0)

The symbol is UNMANGLED in the original (included asm references
`jal switchThread`), so it must be defined as `extern "C"` even though the
callers are C++ methods.

## Implementation

```cpp
extern "C" void func_001188C0(int);

extern "C" void switchThread() {
    func_001188C0(1);
}
```

This is the documented single-call wrapper shape (see AGENTS.md): 0x10
frame, `sq/lq $ra` at 0(sp), argument load `addiu $4, $0, 1` hoisted into
the `jal` delay slot. The callee prototype MUST declare the int parameter
(a `(void)` prototype would emit a `nop` delay slot). func_001188C0 is
defined by code/_generated/sce/lib.s (assembled directly, not INCLUDE_ASM),
so a plain extern "C" declaration links it.

## Verification

- Object disassembly of the built movie.o: 7/7 instruction words identical
  to the original (27bdfff0 7fbf0000 [jal] 24040001 7bbf0000 03e00008
  27bd0010); jal carries R_MIPS_26 on func_001188C0, resolving to the same
  fixed address as the original 0x0C046230.
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
- decomp_status count 807 -> 806.
