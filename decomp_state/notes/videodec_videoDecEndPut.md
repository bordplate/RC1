# videoDecEndPut (0x0023CC10, file 0x13DB90, 0x1C bytes) — BLOCKED

## The problem: implicit a1 pass-through

```
addiu sp,sp,-0x10
sq ra,0(sp)
jal viBufEndPut__FP5ViBufi
addiu a0,a0,0x48      ; only arg setup: &self->vibuf
lq ra,0(sp)
jr ra
addiu sp,sp,0x10
```

viBufEndPut (0x23BF18, still INCLUDE_ASM in vibuf.cpp) genuinely consumes its
second argument: `daddu s2,a1,0` saved to the frame, then
`buf->field_0x14 += a1` (sw at 0x14) and `*(u64*)(buf+0x48) += a1`
(ld/daddu/sd at 0x48). So a1 is a real count.

But videoDecEndPut never sets a1. Both callers set it themselves right
before the call:

- videoCallback (read.cpp) 0x23B6F4: `daddu a1, s3, 0` then 0x23B6F8
  `jal videoDecEndPut`
- videoDecFlush 0x23CDA4: `daddu a1, v0, 0` (v0 = slti result of the
  BeginPut chunk-sum check) then 0x23CDA8 `jal videoDecEndPut`

The count therefore crosses the videoDecEndPut boundary in a1. The function
semantically takes (VideoDec*, int) but its mangled name is
`videoDecEndPut__FP8VideoDec` (one parameter).

## Why no C++ form reproduces it

- 1-arg declaration `void viBufEndPut(ViBuf*)` + 1-arg call: mangles the
  call to `viBufEndPut__FP5ViBuf` — wrong symbol (binary has the 2-arg
  mangled name at the jal).
- 2-arg declaration + call with a value: EGC must materialize a1 (li or
  load) — the original has no such instruction.
- Default argument (`int = 0`): the default is materialized by the caller
  (li a1,0) — also absent.
- There is no C++ language construct that says "pass through whatever the
  caller left in a1".

Original likely compiled from a declaration/definition mismatch (e.g. a
header declaring the 2-arg form while the .cpp definition omitted the
param, or a C++ class method whose `this`-adjusted call left a1 alone);
either way EGC 2.95.2 cannot regenerate it from well-formed C++.

Blocked pending a toolchain-level explanation; the 28-byte INCLUDE_ASM
stays to preserve parity.
