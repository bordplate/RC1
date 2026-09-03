# voBufDelete__FP5VoBuf (code/game/movie/vobuf.cpp)

- Original: 8 bytes (`0x8`) at file offset `0x13E160` in `assets/boot_elf.elf`,
  vram `0x0023D1E0`. Object-relative `.text+0x50` in `vobuf.o`.
- Semantics: empty function taking one `VoBuf*` — the ring buffer is
  presumably owned/freed elsewhere (see caller below), so the "delete" hook
  does nothing.
- Original instruction sequence:
  ```
  jr     $ra
     nop
  ```
- Ghidra decompile of `FUN_0023d1e0`: `return;`.
- Sole xref: UNCONDITIONAL_CALL from vram `0x0023AAA0` (movie subsystem), so
  the symbol is referenced and must be emitted.

## Naming insight (old-GCC C++ mangling)

EGC 2.95.2 for EE uses the **old GCC (pre-Itanium) C++ ABI**, not Itanium.
Empirical test compile with identical flags:

```cpp
void free1(VoBuf* p) {}                    // -> free1__FP5VoBuf
void VoBuf::method1(VoBuf* p) {}           // -> method1__5VoBufP5VoBuf
```

So the binary symbol `voBufDelete__FP5VoBuf` is exactly the compiler-mangled
name of the **free function** `void voBufDelete(VoBuf* self)` — no extern "C"
and no manual mangled name needed. Per project guidance, define it as plain
C++ and let the compiler produce the symbol. (Members use the
`name__<size><Class><args>` form, so none of these movie hooks are member
functions.)

## C candidate

```cpp
void voBufDelete(VoBuf* self) {}
```

No delay-slot or scheduling traps: zero operations to schedule, EGC `-O2`
emits the canonical `jr $ra; nop` epilogue for any empty function.

Side note: the definition sits at file line 14, *above* where the local
`VoBuf` typedef used to be. The typedef was moved to the top of the file
(before `voBufCreate`) so the parameter type resolves; no other declarations
depend on the old ordering. This shifted later vobuf.cpp lines by +2, so the
`matched.json` key for `voBufIsEmpty` was corrected from line 27 to 29 in the
same commit.

## Verification (mechanical)

- `make` builds; `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- Test compile first confirmed EGC mangles `void voBufDelete(VoBuf*)` to
  exactly `voBufDelete__FP5VoBuf`.
- `objdump -d build/code/game/movie/vobuf.o`: symbol `voBufDelete__FP5VoBuf`
  at `.text+0x50`, size `0x8`, words `03e00008 00000000` — identical to the
  original slice (LE bytes `0800 e003 0000 0000`) at file offset `0x13E160`.
- No relocations introduced; function sits between `voBufCreate...NON_MATCHING`
  and `voBufReset__FP5VoBuf` exactly as in the original layout.

## Remaining vobuf.cpp targets (same family)

- `voBufIncCount__FP5VoBuf` / `voBufDecCount__FP5VoBuf` — small counters;
  old-GCC mangling means plain C++ free functions named e.g. `voBufIncCount`.
- `voBufGetData__FP5VoBuf` / `voBufGetTag__FP5VoBuf` — the pop pair around
  vram `0x23D2D8`; data one has a capacity guard (`beqzl -> break 7`).
- `voBufCreate__FP5VoBufP6VoDataP5VoTagi` — free function
  `void voBufCreate(VoBuf*, VoData*, VoTag*, int)`; allocation, likely needs
  the real allocator symbol.
