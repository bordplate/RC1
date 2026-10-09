#include "common.h"
#include "types.h"
#include "hud.h"

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
