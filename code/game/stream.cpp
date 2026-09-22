#include "common.h"
#include "types.h"
#include "pad_state.h"

typedef struct {
    u8 pad_0x08[0x08];
    s16 field_0x08;
    u8 field_0x0A;
    u8 pad_0x0B[0x15];
    s16 field_0x20;
    u8 pad_0x22[0x32];
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

// VAG track-buffer field offsets (the buffer is a raw pointer the sound
// system passes to each state callback): the flag at +0xA and an associated
// entry at +0x10.
#define VAG_BUFFER_OFF_FLAG 0xA
#define VAG_BUFFER_OFF_0x10 0x10
// VAG track buffer flag states (s16 at VAG_BUFFER_OFF_FLAG): which track path
// owns the buffer - queued for playback, transition/start-body, or ready once
// the sound system reports the buffer active.
#define VAG_BUFFER_FLAG_QUEUED 1
#define VAG_BUFFER_FLAG_TRANSITION 4
#define VAG_BUFFER_FLAG_READY 8

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216A20);

// VAG buffer state callback for a music transition: the sound system reports
// each track buffer's state. A nonzero state on a queued buffer promotes its
// flag to VAG_BUFFER_FLAG_TRANSITION and, when the buffer carries a nonzero
// entry (VAG_BUFFER_OFF_0x10), raises the transition's field_0x20; a cleared
// state resets the flag to 0.
// Symbol override: the still-assembly music dispatcher loads the callback by
// its address-based generated label.
void stream_setTransitionVagBufferState(int state, long buffer)
    asm("func_00216A80");

void stream_setTransitionVagBufferState(int state, long buffer) {
    int p = (int)buffer;
    if (p) {
        *(int*)p = state;
        if (state) {
            s16 old = *(s16*)(p + VAG_BUFFER_OFF_FLAG);
            if (old == VAG_BUFFER_FLAG_QUEUED) {
                *(s16*)(p + VAG_BUFFER_OFF_FLAG) = VAG_BUFFER_FLAG_TRANSITION;
                if (*(s16*)(p + VAG_BUFFER_OFF_0x10)) {
                    musicTransition.field_0x20 = old;
                }
            }
        } else {
            *(s16*)(p + VAG_BUFFER_OFF_FLAG) = 0;
        }
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216AD0);

// VAG buffer state callback passed to snd_PlayVAGStreamByLocEx_CB by
// music_StartTrack: the sound system reports each track buffer's state.
// The state is always recorded on the buffer; a nonzero state promotes a
// queued buffer to ready, a cleared state resets the flag to 0.
// Symbol override: music_StartTrack (still assembly) loads the callback by
// its address-based generated label.
void stream_setVagBufferState(int state, long buffer) asm("func_00216B28");

void stream_setVagBufferState(int state, long buffer) {
    int p = (int)buffer;
    if (p) {
        *(int*)p = state;
        if (state) {
            if (*(s16*)(p + 0xA) == VAG_BUFFER_FLAG_QUEUED) {
                *(s16*)(p + 0xA) = VAG_BUFFER_FLAG_READY;
                return;
            }
        } else {
            *(s16*)(p + 0xA) = 0;
        }
    }
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216B68);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216BC0);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216C30);

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00216C48);

void pad_resetState(void) {
    padState.field_18e = 0;
    padState.field_190 = 0;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00217038);
INCLUDE_ASM("code/_generated/nonmatchings/game/stream", func_00217048);
