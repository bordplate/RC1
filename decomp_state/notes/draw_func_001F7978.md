# func_001F7978 (draw.cpp) — matched 2026-09-06

Void wrapper, 0x2C bytes, at VMA 0x1f7978 (file 0xF88F8). Calls three
zero-arg draw-init helpers in order:

    PutDrawBufferLarge()   -> PutDrawBufferLarge__Fv  0x1fb2d0 (framebuf.cpp)
    InitViewContext()      -> InitViewContext__Fv     0x1f2c60 (this TU)
    UpdateViewContext()    -> UpdateViewContext__Fv   0x1f2d98 (this TU)

All three return void (confirmed in Ghidra).

## Implementation

    void PutDrawBufferLarge();
    void InitViewContext();
    void UpdateViewContext();

    extern "C" void func_001F7978(void) {
        PutDrawBufferLarge();
        InitViewContext();
        UpdateViewContext();
    }

## Key points

- The linker script pins the PLAIN symbol `func_001F7978` (no `__Fv`), and the
  single caller (pause `func_002196B8`, `jal func_001F7978` @ 0x219b34)
  references that plain name. So the definition must be `extern "C"` to emit the
  unmangled symbol — same precedent as the other `func_XXXX` matches in draw.cpp
  (func_001F21B0/F8, func_001F61E8/F8, ...).
- The callees are declared as plain C++ (`void f();`, NOT `extern "C"`), so each
  call site mangles to `<name>__Fv`, matching the INCLUDE_ASM-defined symbols.
  A mix of `extern "C"` outer wrapper + C++ callees is fine: only the outer
  symbol is unmangled; the call relocations still target the mangled names.
- `PutDrawBufferLarge__Fv` is in a different TU (framebuf.cpp), so it is `U`
  (undefined) in draw.o and resolved at link; the other two are `T` in draw.o.
- EGC codegen: 0x10 frame (`addiu sp,sp,-0x10; sq ra,0(sp)`), three `jal` each
  with a `nop` delay slot, then `lq ra; jr ra; addiu sp,sp,0x10`. No return value,
  no argument setup, so no register-allocation subtleties.

## Verification

- draw.o disassembly: 11 instructions identical to the generated original;
  first jal carries `R_MIPS_26 -> PutDrawBufferLarge__Fv`, other two same-section.
- Final-ELF region 0x1f7978..+0x2C byte-for-byte identical, including resolved
  jal target words `0c07ecb4 / 0c07cb18 / 0c07cb66`.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes.
- decomp_status count 767 -> 766.
