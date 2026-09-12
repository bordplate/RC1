# readBufBeginGet__FP7ReadBufPPUc (vram 0x23B9D8, file 0x13C958, 72 bytes / 18 words) — MATCHED 2026-09-12

`int readBufBeginGet(ReadBuf* buf, u8** out)` in code/game/movie/readbuf.cpp.

## Semantics

Ring-buffer "prepare to read" accessor, sibling of the matched
readBufBeginPut/EndPut/EndGet. If `count != 0`, it stores the next read
position into `*out`:

```
*out = buf->data + ((putPos - count + capacity) % capacity)
```

i.e. the wrap-around offset of the first unread byte (putPos is where the
next WRITE goes, so the read head is `count` bytes before it, modded by
capacity). Returns `count` (bytes available). The single caller
(func_0023A3B8 at 0x23A5E8) uses the return value as the length
(`blez` test) and `*out` as a direct byte pointer.

## Why `*out = buf->data + x` is `self + x` in the machine code

ReadBuf begins with `u8 data[0x50000]`, so `buf->data + x == (u8*)buf + x`.
EGC therefore materializes the store base as the plain self pointer (`a3`),
not `self + 0x50000`. The field accesses (putPos/count/capacity at
0x50000/0x50004/0x50008) share the base `a2 = self + 0x50000` built once
with `lui v0,0x5; addu a2,a3,v0`.

## Codegen facts

- Frameless leaf: no `addiu sp`/`sq`/`lq` at all (18 words, two loads
  before the guard, one store, one return).
- The `% capacity` compiles to the signed `div zero, v1, a0; mfhi v0`
  sequence with the EGC div-by-zero debug guard `beq a0,zero,+8` and
  `break 0,7` (word CD010000) in its delay slot — the exact idiom already
  verified in the matched readBufEndPut__FP7ReadBufi and
  snd_GetDopplerPitchMod.
- The return `buf->count` is a fresh `lw v0, 4(a2)` in the `jr ra` delay
  slot: the earlier count load (word 5, used by the `beqz` guard) is
  clobbered by the subu/addu/div/mfhi chain, so EGC reloads from the still
  live `a2` base.
- The `if (buf->count)` guard is `lw v0,4(a2); beqz v0,ret; nop` — the nop
  delay slot is the EGC default here (no hoistable instruction available).

## Candidate (matched on first attempt)

```c
int readBufBeginGet(ReadBuf* buf, u8** out) {
    if (buf->count) {
        *out = buf->data + ((buf->putPos - buf->count) + buf->capacity) % buf->capacity;
    }
    return buf->count;
}
```

`decomp_probe.py` vs the generated .s: 72/72 bytes, zero differences.
Full build + `cmp build/boot_elf.elf assets/boot_elf.elf` clean; objdump of
0x23B9D8-0x23BA20 in the built ELF is instruction-identical to the original.
