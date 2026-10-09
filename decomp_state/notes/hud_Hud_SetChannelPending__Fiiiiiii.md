# Hud_SetChannelPending__Fiiiiiii (code/game/hud_chan.cpp) — matched 2026-10-09

Was `func_001FF308` (0x1FF308, 0x10C = 268 bytes). HUD channel pending-request
slot allocator. Long-standing blocker (working/hud_func_001FF308, 15 candidates +
5 probes) on a register-allocation tie-break; resolved by the byte-indexed slot
address form plus moving the function to its own SN TU.

## Semantics

`int Hud_SetChannelPending(int chan, int id, int fn, int d, int e, int c, int b)`.
`chan` low nibble = slot index (`idx`), high bits = `mode` (`chan & 0xFFF0`).
`slot = &hudChanSlots[idx]` (13 slots of 0x90 at 0x199B60).

1. Early `return 0` when `GameMode == 5 && idx != 2 && idx != 0` (no channel to
   service in that mode for those slots).
2. If all 7 pending fields already equal the request, `return slot->serial`
   (unchanged).
3. Else store the request into the pending block, `slot->serial =
   hudHeap.nextSlotSerial++`, `slot->pending = 1`, `slot->field_7C = 0`,
   `slot->field_70 = 0`, and when `mode & slot->mode & 0x20` call
   `Hud_CommitChannel(slot)` (commits pending→active and invokes the callback;
   now decompiled, was func_001FF418). `return slot->serial`.

Param→register map (all seven arrive in registers; fn/d are never spilled):
a0=chan, a1=id→t5, a2=fn, a3=d, a4=e→t0, a5=c→t1, t2=b. The 5th/6th/7th args
(e, c, b) are the PARAMS in a4/a5/t2 — the working-dir notes' "uninitialized
locals" reading was wrong (Lombyte's error, carried into the notes).

## Register-allocation tie-break (the blocker)

Original prologue:
```
andi $11,$4,0xf      # idx -> t3
li   $3,144          # 0x90 -> v1
mult $3,$11,$3       # product -> v1  (li/mult keep 0x90 AND product in v1)
addiu $29,$29,-0x20
lui  $2,0x1a; sq $16,0($29); addiu $2,$2,-0x64a0   # slot base -> v0
andi $12,$4,0xfff0   # mode -> t4
lui  $4,0x16; lw $4,-2556($4)  # GameMode (0x15F604) -> a0, SELF-BASED
move $13,$5          # id -> t5
addu $16,$3,$2       # s0 = slot = base + product
```
EGC 2.95.2 gave the `idx*0x90` product a T-REGISTER (t0/t3) in every
struct-subscript formulation, displacing idx/mode/id up (t1..t6) — 15+ candidate
failures. The product must land in v1 (a value register) with idx/mode/id in
t3/t4/t5.

## The fix: byte-indexed slot address

Writing the slot as a struct subscript (`&hudChanSlots[idx]`) or with a named
`off`/`slotBase` local makes EGC allocate the product a t-reg. Writing it
byte-indexed — the matched `sound_KillChannel` idiom —
`HudChanSlot* slot = (HudChanSlot*)((u8*)hudChanSlots + idx * 0x90);`
makes EGC keep the product in v1 (`li v1,0x90; mult v1,idx,v1`) and idx/mode/id
in t3/t4/t5, byte-for-byte. The array must stay a plain scalar/byte base (no
`.data` section attribute) so the base is a two-instr `%hi/%lo` split into v0,
and `GameMode` must be a plain in-window `extern u32` (no `.data`) so SN expands
it self-based.

## SN TU requirement + Splat split

Both the self-based GameMode load and the v1 product need the SN assembler
(ps2eeas); the GNU TU emits a GPREL GameMode load and a different allocation
(GNU probe: 59 diffs). hud.cpp is one of the five GNU-compat TUs, so the
function was split into its own default-SN TU via Splat boundaries at file
offsets 0xFF308 / 0xFF418 (vram 0x1FF308 / 0x1FF418):
- `game/hud` (0xfefc0..0xff308): unchanged (LinkHudBank..Hud_HeapAlloc + 3 nops).
- `game/hud_chan` (0xff308..0xff418): NEW, default SN, holds the decompiled
  function + 1 trailing `asm("nop")` (region 0x110 vs body 0x10C).
- `game/hud_post` (0xff418..0x101520): the former tail (func_001FF418..
  draw_bootImage + hud_updateMessageTimer), added to the GNU override list.
`.ld` order verified: hud.o, hud_chan.o, hud_post.o. The still-assembly caller
func_001FF480's `jal` now relocates to `Hud_SetChannelPending__Fiiiiiii`
(Splat renamed it from symbols.txt); the Hud_InitBanks call was updated in
hud_pre.cpp (its old `asm("func_001FF308")` override removed).

## Shared-struct rename (same change)

The decompilation establishes the HudChanSlot layout, so the placeholder field
names in code/include/hud.h were renamed (no other TU uses the field names —
only the base address — so this is codegen-neutral): the 0x20-0x38 block is the
pending request (pendId/pendMode/pendB/pendC/pendFn/pendD/pendE); the 0x04-0x18
block is the committed/active values (mode/b/c/fn/d/e, named for the argument
each holds). `Hud_CommitChannel` (the committer, ELF label func_001FF418; now
decompiled, see notes/hud_post_Hud_CommitChannel.md) copies pending→active and
calls `slot->fn`.

## Verification (mechanical)

- Probe (working/hud_func_001FF308/probe_p6_snas, SN, project flags):
  268/268 bytes, 0 diffs. (GNU probe: 59 diffs — confirms SN required.)
- Full `make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` PASSES
  (byte-identical).
- Per-function: objdump of built vs original at 0x1FF308-0x1FF418 — all 67
  instruction words identical.
- active count 556 → 555.
