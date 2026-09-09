#include "common.h"
#include "types.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_002166E8);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216728);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216788);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", Load);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_002168A8);

typedef struct {
    u8 pad_0x08[0x08];
    s16 field_0x08;
    u8 pad_0x0A[0x4A];
    s16 field_0x54;
    s16 field_0x56;
    s16 field_0x58;
} MusicTransState;

extern MusicTransState D_001516D0 __attribute__((section(".data")));

extern "C" int snd_StreamSafeCdGetError(void);

extern "C" void func_00216950(int param_1) {
    if (param_1 != 1) {
        return;
    }
    D_001516D0.field_0x08 = 0;
    int err = snd_StreamSafeCdGetError();
    if (err) {
        D_001516D0.field_0x08 = 2;
    }
}

extern "C" void func_00216990(int a0, long a1) {
    int p = (int)a1;
    if (p && a0 && *(s16*)(p + 0xA) == 2) {
        *(s16*)(p + 0xA) = 3;
    }
}

extern "C" void func_00215970(int param_1, int param_2, int param_3);

extern "C" void func_002169C0(int param_1, long param_2) {
    int p = (int)param_2;
    if (p) {
        *(int*)p = param_1;
        if (param_1) {
            if (*(s16*)(p + 0xA) == 1) {
                *(s16*)(p + 0xA) = 2;
            }
        }
        else {
            func_00215970(D_001516D0.field_0x54,
                          D_001516D0.field_0x58,
                          D_001516D0.field_0x56);
        }
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216A20);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216A80);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216AD0);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216B28);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216B68);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216BC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216C30);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216C48);

typedef struct {
    u8 pad[0x18E];
    u16 field_18e;
    u32 field_190;
} StreamState;

extern StreamState D_0013C940 __attribute__((section(".data")));

extern "C" void func_00217020(void) {
    D_0013C940.field_18e = 0;
    D_0013C940.field_190 = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00217038);
INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00217048);
