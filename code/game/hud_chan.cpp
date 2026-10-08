#include "common.h"
#include "types.h"
#include "hud.h"

extern u32 GameMode;
// func_001FF418 is still an INCLUDE_ASM placeholder in hud_post whose ELF
// symbol is the unmangled address label; the asm override targets that label.
void func_001FF418(HudChanSlot*) asm("func_001FF418");

// Records a pending request for HUD channel `chan` (low nibble = slot index,
// high bits = mode). When GameMode is 5 and the slot is neither 2 nor 0 there
// is no channel to service, so it returns 0. If the slot's pending fields
// already match the request, the existing serial is returned unchanged.
// Otherwise the request is stored, a fresh serial is assigned from
// hudHeap.nextSlotSerial, the slot is marked pending, and func_001FF418
// commits it when `mode & slot->mode & 0x20` holds. Returns the slot serial.
//
// The slot address is byte-indexed (hudChanSlots + idx * 0x90) rather than a
// struct subscript: EGC then keeps the 0x90*idx product in v1 (li v1,0x90;
// mult v1,idx,v1) exactly as the original, instead of giving the product a
// t-register that displaces idx/mode/id.
int Hud_SetChannelPending(int chan, int id, int fn, int d, int e, int c, int b) {
    int idx = chan & 0xF;
    int mode = chan & 0xFFF0;
    HudChanSlot* slot = (HudChanSlot*)((u8*)hudChanSlots + idx * 0x90);
    if (GameMode == 5 && idx != 2 && idx != 0)
        return 0;
    if (slot->pendC == c && slot->pendB == b && slot->pendId == id && slot->pendMode == mode
        && slot->pendFn == fn && slot->pendD == d && slot->pendE == e)
        return slot->serial;
    slot->pendC = c;
    slot->serial = hudHeap.nextSlotSerial++;
    slot->pendB = b;
    slot->pendId = id;
    slot->pendFn = fn;
    slot->pendD = d;
    slot->pendE = e;
    slot->pending = 1;
    slot->pendMode = mode;
    slot->field_7C = 0;
    slot->field_70 = 0;
    if (mode & slot->mode & 0x20)
        func_001FF418(slot);
    return slot->serial;
}

// Trailing alignment padding (4 bytes) the original emits after this function
// before func_001FF418: the C body is 0x10C but the Splat region is 0x110.
asm("nop");
