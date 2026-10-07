#include "common.h"
#include "levelmem.h"

// Current VU1 chain buffer base in main memory, set by vuChain_getCurrent.
extern int currentVuChain;

// Largest boot-asset CD stream sector count the memcard code will place in
// the level-memory stream regions; larger counts are rejected.
#define MEMCARD_STREAM_MAX_SECTORS 0x20000

// Computes the destination addresses of a boot-asset CD stream read from its
// sector count: for each of the two level-memory regions, the region base
// plus the current VU chain base minus the count. The memcard save/restore
// state machine passes bootAssets' second record size, reads the stream into
// pDest1, and uses the result's layout to place the save data.
// Symbol override: the callers in memcard.cpp still reference the
// address-based generated label for this routine.
int memcard_ComputeStreamDest(unsigned int sectors, int* pDest1, int* pDest2)
    asm("memcard_ComputeStreamDest");
int memcard_ComputeStreamDest(unsigned int sectors, int* pDest1, int* pDest2) {
    if (sectors > MEMCARD_STREAM_MAX_SECTORS) {
        *pDest1 = 0;
        *pDest2 = 0;
        return -1;
    }
    *pDest1 = levelMem.field_0x04 + currentVuChain - sectors;
    *pDest2 = levelMem.field_0x08 + currentVuChain - sectors;
    return 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FD748);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDC08);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDC90);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", Help_FindIndex);

// C linkage: the matching helper remains supplied by generated assembly at its
// original unmangled entry point.
extern "C" int Help_FindIndex(int idx);

struct HelpMsg {
    char* text;
    int id;
    int f8;
    int fC;
};

extern struct HelpMsg* HelpMsgs;
extern char s_Paradox_this_message_does_not[];

char* msg_string(int idx) {
    int i = Help_FindIndex(idx);
    if (i >= 0)
        return HelpMsgs[i].text;
    return s_Paradox_this_message_does_not;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDD50);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDD58);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", Help_Update);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", Help_DrawPrompt);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FE980);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FECC8);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FED30);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FEE30);
