# LoadIRXModule (0x201520, code/game/init.cpp)

Matched 2026-10-09. 180 bytes, 0 diffs.

## Semantics

Boot-time IRX module loader, called 10x from `InitOnce__Fv` (0x201650,
initonce.cpp). Given an EE-side image (`src`, `size`):

1. `dst = sceSifAllocIopHeap(size)` — allocate an IOP heap buffer (cmd 1 of the
   0x80000003 IOP-heap service, client 0x158040).
2. Build a stack `SifDmaData { data = src, addr = dst, size = size, mode = 0 }`
   and start the transfer: `id = sceSifSetDma(&dma, 1)`.
3. If `id != 0`: poll `sceSifDmaStat(id)` while `>= 0` (bgez loop with three
   scheduler nops), then `r = sceSifLoadModuleBuffer(dst, 0, 0)` (veneer for
   `_sceSifLoadModuleBuffer` 0x11CB70, the 0x80000006 eeloadfile service),
   `if (!(r >= 0)) ok = 0;`, then `sceSifFreeIopHeap(dst)`.
4. Return `ok` (1, or 0 on load failure).

## SDK naming

Veneers at 0x118xxx-0x11Cxxx in `sce/lib.s` are raw `addiu $v1,N; syscall 0`
stubs. Named per Lombyte (`reference/Lombyte/src/sdk/sif_rpc/`,
`reference/Lombyte/config/us/symbol_addrs.txt`) and the Deadlocked PAL SDK
(`reference/dl/sdk/iopheap.c`):

| symbol | vram | note |
| --- | --- | --- |
| sceSifDmaStat | 0x118B10 | Lombyte asm veneer |
| sceSifSetDma | 0x118B20 | Lombyte asm veneer |
| sceSifInitIopHeap | 0x11C840 | binds 0x80000003, client 0x158040 |
| sceSifAllocIopHeap | 0x11C8C8 | IOP-heap cmd 1, returns IOP addr |
| sceSifFreeSysMemory | 0x11C938 | IOP-heap cmd 2, uses a0 only |
| sceSifFreeIopHeap | 0x11C9B0 | 0x10-frame veneer over 0x11C938 |
| sceSifLoadModuleBuffer | 0x11CD78 | veneer over _sceSifLoadModuleBuffer 0x11CB70 |

`SifDmaData { u32 data; u32 addr; s32 size; s32 mode; }` is the real SDK type
(layout confirmed against Lombyte `sce_sif_reset_iop.c`).

## Match-sensitive tail

Original tail (objdump of assets/boot_elf.elf):

```
201598: jal  0x11cd78      # r = sceSifLoadModuleBuffer(dst, 0, 0)
20159c: move a2, zero      # [delay]
2015a0: li   v1, -1
2015a4: move a0, s2
2015a8: slt  v1, v1, v0    # v1 = (r >= 0); r stays in v0
2015ac: jal  0x11c9b0      # sceSifFreeIopHeap(dst)  -- ONE argument
2015b0: movz s3, zero, v1  # [delay] if (v1 == 0) ok = 0
```

Two facts fixed the C shape:

- `sceSifFreeIopHeap` is called with a single argument (a0 = dst; a1/a2 hold
  stale values the veneer ignores). A 3-arg call makes EGC materialize the
  extra arguments and compute the sign of r as `nor/srl`.
- The `(r >= 0)` test is computed BEFORE the free call in RTL: r is consumed in
  v0 (no callee-saved spill) and the single slt result (v1) is reused by the
  movz that the scheduler parks in the free call's delay slot. Writing the if
  after the call (or with `r < 0`, which compiles to a movn of a second test)
  spills r to s0, re-computes the test, and grows the function to 196 bytes.

So the body is:

```c
int r = sceSifLoadModuleBuffer(dst, 0, 0);
if (!(r >= 0)) {
    ok = 0;
}
sceSifFreeIopHeap(dst);
```

Register map: src->s1, size->s0 (reused for id), dst->s2, ok->s3.

## Linkage

The boot ELF exports the unmangled `LoadIRXModule`; a C++ free function would
mangle to `LoadIRXModule__FiT0`, so the definition is pinned with
`asm("LoadIRXModule")` (DeleteMoby/RefreshPointLight precedent) rather than
claiming C linkage. The five SDK prototypes are `extern "C"` (syscall veneers
in generated sce/lib.s).
