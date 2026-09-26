# sound_KillChannel__Fi (0x22d798, 84 bytes)

Releases a 3D sound channel given its slot index. Mirrors Deadlocked
`sound_KillByChannel`, but indexed rather than pointer-passed.

```cpp
void sound_KillChannel(int i) {
    if (i < 0) return;
    SoundChannelView* ch = (SoundChannelView*)((u8*)audioState + i * 0x70);
    u8 status = ch->status;
    if (status == 7) {           // active
        ch->pMoby = 0;
        ch->pAmbient = 0;
        ch->status = 0;          // free
    } else if (status != 0 && status != 6) {
        ch->status = 4;          // pending-kill
    }
}
```

## Channel layout (the +0x70 offset split)

The 0x70-byte channel slots are `SoundData` (see sound.cpp) starting at
`audioState + 0x70` (0x13E5C0); `sound_GetFade` and the channel-start routine
0x22D7F0 address a slot at `audioState + 0x70 + i*0x70` with the normal
SoundData field offsets. This routine instead keeps `base = audioState +
i*0x70` (one slot early) and bakes the +0x70 into every field offset, so the
fields land at absolute 0x74 (status), 0x88 (pMoby), 0x8C (field_0x1C/pAmbient).

The matching form indexes from the `audioState` base with an explicit 0x70 byte
stride and models the touched fields at their absolute offsets
(`struct SoundChannelView`, sound.cpp). A struct-member form
(`&audioState.channels[i]` with `channels` at offset 0x70) does NOT match:
EGC keeps the +0x70 in the base pointer (0x13E5C0) and uses the plain
SoundData offsets (0x04/0x18/0x1C), a different lui/addiu/offset encoding.

## Store order (EGC three-constant-store permutation)

The three zero-stores in the `status == 7` branch are independent constant
stores to a register base, which EGC emits in the fixed permutation
`stmt3; stmt1; jr $ra; <delay slot: stmt2>`. The original tail is
`status; pMoby; jr; <pAmbient>`, so the source order must be
`pMoby, pAmbient, status` (stmt1=pMoby, stmt2=pAmbient, stmt3=status).

## Verification

`tools/decomp_probe.py` matched byte-for-byte (0 diffs, 84 bytes); full
`make -j2` + `cmp build/boot_elf.elf assets/boot_elf.elf` passes. Both callers
(Transition_DoTransition 0x1EBADC, mobyfunc func_0020C940 0x20C998) were
INCLUDE_ASM and their `jal` was auto-renamed to `sound_KillChannel__Fi` on
`make split` after adding the symbol to config/symbols.txt.
