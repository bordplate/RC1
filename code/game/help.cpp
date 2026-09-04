#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FD6E0);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FD748);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDC08);
INCLUDE_ASM("code/_generated/nonmatchings/game/help", func_001FDC90);

INCLUDE_ASM("code/_generated/nonmatchings/game/help", Help_FindIndex);

extern "C" int Help_FindIndex(int idx);

struct HelpMsg {
    char *text;
    int id;
    int f8;
    int fC;
};

extern struct HelpMsg *HelpMsgs;
extern char s_Paradox_this_message_does_not[];

char *msg_string(int idx) {
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
