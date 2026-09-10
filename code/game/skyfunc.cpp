#include "common.h"

// Assembly visibility is required for the adjacent generated sky-function
// fragment, which references this legacy label.
asm(".globl Label_0022AE54\n");

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyFunc_UNK_FUN_0022AC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyLevelGeneric___maybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", func_0022B288);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SetupSkyGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", DoSkyGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", func_0022B688);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyDrawShell__Fi);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyDrawShellTextured);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyDrawShellGouraud);
