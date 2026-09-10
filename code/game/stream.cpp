#include "common.h"
#include "types.h"

typedef struct {
    u8 pad_0x08[0x08];
    s16 field_0x08;
    u8 field_0x0A;
    u8 pad_0x0B[0x49];
    s16 field_0x54;
    s16 field_0x56;
    s16 field_0x58;
} MusicTransState;

extern MusicTransState musicTransition __attribute__((section(".data")));

// C linkage: the sound-library stream-safe CD routines are unmangled generated
// entry points called directly by the original stream callback.
extern "C" int snd_StreamSafeCdGetError(void);
// Returns int, not void: the live int return keeps v0 claimed across the call,
// so stream_breakTransitionCd's post-call constant is allocated to v1; a void
// prototype breaks the byte match (see notes/stream_func_002166E8.md).
extern "C" int snd_StreamSafeCdBreak(void);

// Symbol override: this recovered helper occupies an address-based generated
// entry point whose four pause_post callers jal it by name.
void stream_breakTransitionCd(void) asm("func_002166E8");

void stream_breakTransitionCd(void) {
    if (musicTransition.field_0x08 != 0) {
        snd_StreamSafeCdBreak();
        musicTransition.field_0x0A = 1;
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216728);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216788);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", Load);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_002168A8);

void stream_updateCdStatus(int status) {
    if (status != 1) {
        return;
    }
    musicTransition.field_0x08 = 0;
    int err = snd_StreamSafeCdGetError();
    if (err) {
        musicTransition.field_0x08 = 2;
    }
}

void stream_updateBufferState(int state, long buffer) {
    int p = (int)buffer;
    if (p && state && *(s16*)(p + 0xA) == 2) {
        *(s16*)(p + 0xA) = 3;
    }
}

// Symbol override: the still-assembly music dispatcher at 0x00215970 selects
// and starts the stream path for a transition request.
extern void music_startTransitionStream(int track, int mode, int transition)
    asm("func_00215970");

void stream_setBufferState(int state, long buffer) {
    int p = (int)buffer;
    if (p) {
        *(int*)p = state;
        if (state) {
            if (*(s16*)(p + 0xA) == 1) {
                *(s16*)(p + 0xA) = 2;
            }
        } else {
            music_startTransitionStream(musicTransition.field_0x54,
                                         musicTransition.field_0x58,
                                         musicTransition.field_0x56);
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

extern StreamState streamState __attribute__((section(".data")));

void stream_resetState(void) {
    streamState.field_18e = 0;
    streamState.field_190 = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00217038);
INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00217048);
