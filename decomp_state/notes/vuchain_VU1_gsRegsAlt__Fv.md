# VU1_gsRegsAlt__Fv (0x233C28, 0x5C) — MATCHED 2026-09-14

Appends the VU1 packet that streams the ALTERNATE GS register state block to
the VU1: `[0x30000003, 0x1DE3F0, 0, 0x50000003]` at the chain head, then
advances `vu1ChainHeadStore` to head+4. Data block `vu1GsRegsAlt` (0x1DE3F0,
12 words; differs from `vu1GsRegsNormal` at 0x1DE3C0 only in word 4:
0x0005380b vs 0x0005360b) named in config/symbols.txt.

Exact byte twin of matched VU1_gsRegsNormal__Fv (0x233BC8, 0x5C) except the
block address: same tag 3, same packet shape, same five self-based
`vu1ChainHead` loads, same GPREL delay-slot head store. Callers:
DrawDebugProfiler (0x1F39D0, 7x) and Transition_DefaultDraw__Fb (0x1EB410,
3x); the inlined twin in ResetGsRegisters__Fv (0x1F3868) sends the same block
inline before calling VU1_addGSregister. The draw pipeline sends the alt block
before draw batches and follows some with VU1_gsRegsNormal to restore the
normal state.

## Solution

`code/game/vuchain.cpp`, TU flag `-mno-split-addresses` (vuchain.o). The
barrier-pinned address RTL is identical to VU1_gsRegsNormal's (see that note
for the mechanism); only the signed low part changes: 0x1DE3F0 =
0x1E0000 - 0x1C10, so `address -= 0x1C10;` gives the original's
`lui 0x1E; addiu -0x1C10` signed split (0xE3F0 >= 0x8000 — the unsigned-fold
failure mode applies exactly as there).

```cpp
void VU1_gsRegsAlt() {
    volatile u32* packet = vu1ChainHead;
    asm volatile("" : : "r"(packet));
    u32 tag = VU1_DATA_REF_TAG | 3;
    asm volatile("" : : "r"(tag));
    u32 address = 0x001E0000;
    asm volatile("" : "+r"(address) : "r"(packet), "r"(tag));
    packet[0] = tag;
    address -= 0x1C10;
    vu1ChainHead[1] = address;
    vu1ChainHead[2] = 0;
    vu1ChainHead[3] = VU1_DATA_REF_END_TAG | 3;
    volatile u32* newHead = vu1ChainHead + 4;
    vu1ChainHeadStore = newHead;
}
```

## Dead tail func_00233C88 — EGC artifact, orphan retained

Unlike VU1_gsRegsNormal, the original has the standard vuchain dead-store
tail after the epilogue:

```
0x233C78  addiu $2, $2, 0x10
0x233C7C  jr $ra
0x233C80  sw $2, -0x5D00($28)   vu1ChainHeadStore  [delay slot, ALIVE]
0x233C84  nop                   .align pad
0x233C88  sw $2, -0x3FE0($28)   vu1ChainTail 0x162C20  [DEAD]
0x233C8C  nop
```

Unreachable (no jal/j target anywhere in the binary; Ghidra has no function
there) — same family as func_00233880 (VU1_addDataRef's tail, identical
instruction word and GPREL target) and the ~46 dead-tail fragments documented
in notes/help_msg_string__Fi.md.

The original source evidently stored the bumped head to vu1ChainTail as well,
and EGC dead-tailed that second store. Two forms probed with full builds:

1. VOID two-store (`vu1ChainHeadStore = newHead; vu1ChainTail = newHead;`):
   EGC keeps BOTH stores alive — `sw tail; jr ra; sw store (delay slot)` —
   no dead tail; the next function lands 4 bytes early. (EGC only dead-tails
   the second store when a return value forces v0 live across the epilogue.)
2. RETURN-VALUE form (`volatile u32* VU1_gsRegsAlt() { ...; return newHead; }`):
   fires the dead tail (`sw v1,-0x3FE0(gp)` after the jr) BUT EGC reserves v0
   for the return across the whole body: the tag moves v0->a1, the block
   address v1->v0, two head loads shift, plus a `move v0,v1` in the epilogue
   — 10-word body diff. The matched body is non-negotiable, so unusable.
   (Same failure mode as the VU1_addDataRef note: "pointer-return forms fire
   a tail store but break the matched body allocation".)

Last-resort GPT-5.6 Sol invoked 2026-09-14 on func_00233C88 itself: no C form
exists — a store immediately after `jr` is the executed delay slot, not dead;
the required tail sits after `jr; <delay-slot store>; .align nop`, where
source scheduling cannot place a reachable side effect; statements after a
`return` are deleted. The orphan INCLUDE_ASM(func_00233C88) is retained to
supply the 8 bytes (sw + nop; its `.align 3` supplies the 0x233C84 pad) and
the entry is blocked.

## Verification

- Candidate region 0x233C28-0x233C8F (body + epilogue + dead tail) compared
  word-for-word against assets/boot_elf.elf: identical.
- `make` + `cmp build/boot_elf.elf assets/boot_elf.elf` byte-for-byte.
- Count 697 -> 696 (VU1_gsRegsAlt matched; func_00233C88 remains as the
  blocked orphan, like func_00233880).
