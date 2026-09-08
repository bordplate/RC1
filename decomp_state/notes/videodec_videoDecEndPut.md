# videoDecEndPut (0x0023CC10, file 0x13DB90, 0x1C bytes) — MATCHED 2026-09-08

## What it does

Single-call wrapper: `viBufEndPut(&self->vibuf);` (vibuf at VideoDec+0x48).

```
addiu sp,sp,-0x10
sq ra,0(sp)
jal viBufEndPut__FP5ViBufi
addiu a0,a0,0x48      ; only arg setup: &self->vibuf (hoisted into delay slot)
lq ra,0(sp)
jr ra
addiu sp,sp,0x10
```

## The implicit a1 pass-through

viBufEndPut (0x23BF18, still INCLUDE_ASM in vibuf.cpp) genuinely consumes its
second argument: it saves a1, does `buf->ipuBp (0x14) += a1` and
`*(u64*)(buf+0x48) += a1`. So a1 is a real count — but videoDecEndPut never
sets it. Both callers load a1 themselves right before the call:

- videoCallback (read.cpp) 0x23B6F4: `daddu a1, s3, 0` then 0x23B6F8
  `jal videoDecEndPut`
- videoDecFlush 0x23CDA4: `daddu a1, v0, 0` (v0 = cpy2area return) then
  0x23CDA8 `jal videoDecEndPut`

The count therefore crosses the videoDecEndPut boundary in a1. The function
semantically takes (VideoDec*, int) but its mangled name is
`videoDecEndPut__FP8VideoDec` (one parameter).

## The form that matches (supersedes the 2026-09-05 blocker)

The earlier blocker concluded "no C++ form reproduces this" because it
considered only plain declarations. The missing mechanism is an asm label on
a 1-parameter declaration:

```cpp
void viBufEndPut(ViBuf* buf) asm("viBufEndPut__FP5ViBufi");

void videoDecEndPut(VideoDec* self) {
    viBufEndPut(&self->vibuf);
}
```

The 1-arg call makes EGC set up only a0 (`addiu a0,a0,0x48` hoisted into the
`jal` delay slot — the standard single-call-wrapper shape from
stash_func_00232CE0), and the asm label redirects the reference to the
2-parameter mangled symbol defined by vibuf.cpp's INCLUDE_ASM, so the
R_MIPS_26 resolves to 0x23BF18 exactly as in the original.

Probe evidence (decomp_state/probes/):

- videodec_endput_v1.cpp — 2-param decl + `viBufEndPut(&self->vibuf, 0)`:
  EGC emits `addiu a0,a0,0x48` BEFORE `sq ra` plus `daddu a1,$0,$0`
  (2d280000) — 8 words, no match (negative control).
- videodec_endput_v2.cpp — 1-param decl, no asm label: identical 7-word code
  but references `viBufEndPut__FP5ViBuf` (wrong symbol, probe link rejects it).
- videodec_endput_v3.cpp — 1-param decl + asm label: probe
  `"match": true`, 0 differences; nm on candidate.o shows
  `U viBufEndPut__FP5ViBufi` / `T videoDecEndPut__FP8VideoDec`.

Verification: clean `make clean && make split && make -j2`, function bytes
0x23CC10..0x23CC2C equal to assets/boot_elf.elf, full
`cmp build/boot_elf.elf assets/boot_elf.elf` passes,
decomp_status 744 nonmatching.

Note: viBufEndPut keeps its 2-arg mangled definition in vibuf.cpp; only the
videodec.cpp declaration is the 1-param asm-labeled form (no header conflict —
vibuf.h declares no prototypes).
