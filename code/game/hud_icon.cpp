#include "common.h"
#include "types.h"

typedef struct {
    u16 id;
    u16 len;
    u16 start;
    u8 animType;
    u8 speed;
} HudIconDef;

typedef struct {
    u8 pad[0x10];
    u32 heapCursor;
    u32 heapEnd;
    u32 pad_18;
    u32 iconTable;
} HudHeap;

extern HudHeap hudHeap __attribute__((section(".data")));

int Hud_GetIconIndex(int iconId) {
    // The peeled element-0 checks must compile to beq (return-as-jump), which
    // EGC only emits for the inverted "value != end && value != iconId" body;
    // the early-return form inverts them to bne. The register pins force the
    // a1<->v1 table/constant swap across prologue->loop. SN assembly is
    // required (ps2eeas inserts the two loop filler nops; GNU omits them).
    register HudIconDef *first asm("$5") = (HudIconDef*)hudHeap.iconTable;
    register int index asm("$6") = 0;
    register int value asm("$2") = first->id;
    register int firstEnd asm("$3") = 0xffff;
    if (value != firstEnd && value != iconId) {
        register HudIconDef *icon asm("$3") = first;
        register int end asm("$5") = 0xffff;
        ++icon;
        for (;;) {
            value = icon->id;
            ++index;
            if (value == end)
                break;
            if (value != iconId) {
                ++icon;
                continue;
            }
            break;
        }
    }
    return index;
}
