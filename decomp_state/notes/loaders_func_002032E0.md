# loaders relocateObjectPointers (func_002032E0)

Address 0x002032E0, 0x58 (88) bytes, leaf (no frame, no calls). SN TU, default flags.

## What it does

Pointer-relocation fixup for a loaded data chunk.

- `obj = *(u32*)(base + index*4 + 0x48)` — a chunk pointer table entry.
- If `obj->firstReloc` (s32 at +0x14) is non-zero, make it absolute:
  `obj->firstReloc = (s32)(u8*)obj + obj->firstReloc`.
- For `relocCount` (u8 at +0x10) entries, add the obj base to each s32
  starting at +0x1C: `arr[i] = (s32)(u8*)obj + arr[i]`.

So loaded chunks store embedded pointers as offsets from the chunk base; after
placement this turns them into absolute addresses. The struct layout is:

```
+0x00  pad[0x10]
+0x10  u8  relocCount
+0x11  pad[3]
+0x14  s32 firstReloc   (offset from chunk base)
+0x18  pad[4]
+0x1C  s32 relocations[]
```

## Codegen notes

- The `relocCount` loop MUST be written as a do-while inside the `if`
  (body: `*arr = objbase + *arr; i++; arr++;` then `while (i < count)`).
  A plain `while (i < count)` hoists the `arr` address `addiu` above the
  `beqz` count test, producing a 3-word prologue diff (beqz/addiu order).
- The single `firstReloc` block is an `if` with an `addiu` (value = obj base
  + stored offset) then a conditional store.
- `u8 relocCount` is read with a sign-extending `andi`/`sll`/`dsrl32`
  (`(s32)(u8)obj->relocCount`); the loop compares signed.

## Symbol / rename

Original symbol was the Splat placeholder `func_002032E0`. Renamed to
`relocateObjectPointers` and registered in `config/symbols.txt`
(`relocateObjectPointers = 0x2032e0;`). The three callers
(pause_post2 func_002256E8 / func_00225E70 / func_00226848) are still
generated `INCLUDE_ASM`, so `make split` regenerated their `jal` targets to
the new name. The C definition keeps the unmangled label via
`asm("relocateObjectPointers")` (the generated callers reference the unmangled
symbol). Once those callers are decompiled, the `asm` label can be dropped in
favour of the natural name.

## Verification

- `tools/decomp_probe.py` byte-for-byte (88/88, no diffs).
- Clean `make clean && make split && make -j2` + `cmp build/boot_elf.elf
  assets/boot_elf.elf` passes.

Matched 2026-09-25.
