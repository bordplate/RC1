#include "common.h"
#include "types.h"
#include "hud.h"

extern int hudMsgCount;
extern int hudMsgFade;
extern int hudMsgY;
extern u8 hudMsgText1[];
extern u8 hudMsgText2[];
extern int GameModeGp;
extern int hudMsgCountGp;
extern int hudMsgFadeGp;
extern int occlChainActive;
// Shared frames->timer scale used across the boot code (bmain.cpp, freeze.cpp);
// still a nonmatching placeholder in code/_generated/game/fastfunc.s.
extern "C" int func_001F96F8(int frames);
// HUD message printer (x, y, color, text); INCLUDE_ASM placeholder in
// hud_post_post2_post.cpp, called only from here.
extern "C" void func_00201200(int x, int y, u32 color, u8* text);

// Seed the GPREL16 aliases before use: this is an SN-assembled TU, and a bare
// `lw/sw r,alias` only expands GP-relative if a matching `.extern alias,4`
// appeared earlier in the file (otherwise ps2eeas emits an absolute self-based
// lui/lw pair, which would not match the original's GPREL16 sites).
asm(".extern GameModeGp, 4\n"
    ".extern hudMsgCountGp, 4\n"
    ".extern hudMsgFadeGp, 4");

void Hud_DrawChannels(void) asm("func_001FF780");

void Hud_DrawChannels(void) {
    HudChanSlot* slot;
    int i;
    int t;
    u32 color;

    if (hudHeap.field_30 != 0) {
        hudHeap.field_30 = 0;
        return;
    }
    if (occlChainActive != 0) {
        hudHeap.field_30 = 0;
        return;
    }
    hudHeap.vuField_0C = HUD_VU_FIELD_INIT;
    slot = hudChanSlots;
    i = HUD_SLOT_COUNT - 1;
    do {
        if (slot->e) {
            ((void (*)(HudChanSlot*))slot->e)(slot);
        }
        i--;
        slot++;
    } while (i >= 0);
    if (hudMsgCount == 0 && hudMsgFade == 0)
        goto force100;
    if (GameModeGp != 0)
        goto force100;
    if (hudMsgCount != 0) {
        t = hudMsgFade + (HUD_MSG_FADE_MAX / func_001F96F8(HUD_MSG_FADE_STEP_FRAMES));
        hudMsgFadeGp = t;
        if (t > HUD_MSG_FADE_MAX) hudMsgFadeGp = HUD_MSG_FADE_MAX;
    } else {
        t = hudMsgFade - (HUD_MSG_FADE_MAX / func_001F96F8(HUD_MSG_FADE_STEP_FRAMES));
        hudMsgFadeGp = t;
        if (t < 0) hudMsgFade = 0;
    }
    color = ((u32)hudMsgFade << 24) + HUD_MSG_COLOR_RGB;
    func_00201200(HUD_MSG_DRAW_X, hudMsgY, color, hudMsgText1);
    if (hudMsgText2[0] != 0 && hudMsgCount > HUD_MSG_COUNT_RESET) {
        func_00201200(HUD_MSG_DRAW_X, hudMsgY, color, hudMsgText2);
    }
    if (hudMsgCount != 0) hudMsgCount--;
    if (hudMsgCount == HUD_MSG_COUNT_RESET) hudMsgCountGp = 0;
    return;
force100:
    hudMsgY = HUD_MSG_IDLE_Y;
    return;
}
