# voBufCreate__FP5VoBufP6VoDataP5VoTagi (0x0023D190, 0x4C bytes)

Matched 2026-09-13.

## Function
Initializes a `VoBuf` and zeroes its tag slots. Sets the struct fields in this
order (matching the store sequence): `count = 0`, `data = data`, `tags = tags`,
`capacity = size`, `head = 0`. If `size > 0`, it zeroes the leading word of each
of `size` tag slots, where slot *k* is `self->tags + k * 0x138C0` bytes.

## Key codegen finding: integer add, not pointer arithmetic
The loop computes the slot address as `addu v0, a1, v0` — i.e. **offset + tags**
with the byte OFFSET as the `rs` (first) operand and the reloaded `self->tags`
as the `rt`. Writing it as pointer arithmetic, `(u8*)self->tags + offset`, makes
EGC canonicalize to `addu v0, v0, a1` (tags as `rs`) — a single-word mismatch at
0x23D1C0. The matching form casts to integers so the offset is the base:

```c
*(u32*)((u8*)(offset + (u32)self->tags)) = 0;
```

This also keeps `self->tags` reloaded from memory each iteration (the compiler
does not CSE it across the loop because the store could alias the field), which
is exactly what the original does (`lw v0, 4(a0)` at the top of the loop).

## Loop shape
`blez size` guard, then a counted loop with the decrement (`addiu a3, -1`) early
in the block and the stride add (`addu a1, a1, v1`, a1 = byte offset, v1 =
0x138C0) in the `bnez` delay slot. Natural `int i = size; while (i) { ...; i--; }`
reproduces it; no peeling/rotation occurs.

## Types
`VoData` and `VoTag` are referenced only by pointer in this function (stored into
`VoBuf::data`/`VoBuf::tags` as `void*`); their full layouts are not yet
determined, so they are declared as empty structs solely to produce the correct
cfront mangled name `voBufCreate__FP5VoBufP6VoDataP5VoTagi`.
