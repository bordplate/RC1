# videoDecBeginPut (0x0023CBF0, file 0x13DB70, 0x1C bytes)

## What it does

Forwards to viBufBeginPut on the embedded ViBuf: `viBufBeginPut(&self->vibuf, data, size, data2, size2)`. viBufBeginPut locks the buffer sema, splits the available put-space into two (data,len) chunks written through the four out-pointers, and unlocks.

## Mangled-name decoding (important)

This EGC cfront encodes repeated parameter types as **Tn with n = 0-based index of the first parameter**: `videoDecBeginPut__FP8VideoDecPPUcPiT1T2` = (VideoDec*, u8**, int*, **T1=u8****, **T2=int**) — NOT (..., VideoDec*, u8**) which is what 1-based reading would give. Cross-check with `cpy2area__FPUciT0iT0iT0i` = (u8*, i, u8*, i, u8*, i, u8*, i). Empirically confirmed by test compiles: the 1-based guess (VideoDec*, u8**, int*, VideoDec*, u8**) both mangles differently (T0T1) AND trips a cfront parser bug ("type ViBuf is not a base type for VideoDec") when the call passes a u8** variable argument; the 0-based guess mangles exactly and compiles cleanly. See AGENTS.md mangling note.

## Implementation

```cpp
int viBufBeginPut(ViBuf* buf, u8** data, int* size, u8** data2, int* size2);

int videoDecBeginPut(VideoDec* self, u8** data, int* size, u8** data2, int* size2) {
    return viBufBeginPut(&self->vibuf, data, size, data2, size2);
}
```

Passthrough wrapper: a1-a4 forwarded untouched (no codegen), only a0 = self+0x48
emitted into the jal delay slot. 0x10 frame, sq/lq ra at 0(sp). viBufBeginPut
is still INCLUDE_ASM in vibuf.cpp; the declaration here links to it.

## Verification

- Object: 7/7 words identical (27bdfff0 7fbf0000 [jal] 24840048 7bbf0000
  03e00008 27bd0010); R_MIPS_26 on viBufBeginPut__FP5ViBufPPUcPiT1T2 resolves
  to the original jal word 0x0C08EF88 (target 0x23BE20).
- Full `make` + `cmp build/boot_elf.elf assets/boot_elf.elf`: identical.
- decomp_status count 806 -> 805. decomp-verifier MATCH.
