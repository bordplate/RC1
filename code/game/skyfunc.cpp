#include "common.h"
#include "sky.h"

// Assembly visibility is required for the adjacent generated sky-function
// fragment, which references this legacy label.
asm(".globl Label_0022AE54\n");

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyFunc_UNK_FUN_0022AC30);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyLevelGeneric___maybe);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", func_0022B288);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SetupSkyGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", DoSkyGifPaging__Fv);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", func_0022B688);

// Draws one sky shell layer: Gouraud when the layer's draw-mode flag is set,
// textured otherwise. Layers 0-3 are drawn by the level sky update.
void SkyDrawShell(int ishell) {
    sky_shell* shell;
    if (ishell < Sky->shellCnt) {
        shell = Sky->shells[ishell];
        // Match-sensitive: the truthiness test with Gouraud first is what
        // makes EGC emit beqz to the Textured else-branch with the Gouraud
        // jal as fallthrough; `if (flags == 0) Textured else Gouraud`
        // flips the branch to bnez with the Textured jal as fallthrough.
        if (shell->flags) {
            SkyDrawShellGouraud(shell);
        } else {
            SkyDrawShellTextured(shell);
        }
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyDrawShellTextured);

INCLUDE_ASM("code/_generated/nonmatchings/game/skyfunc", SkyDrawShellGouraud);
