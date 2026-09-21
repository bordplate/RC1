#include "common.h"
#include "types.h"
#include "menu.h"

// The boot font table is a large block at 0x137B80. The 16-byte head is the
// debug font image (LoadDebugFont, bloaders.cpp); the 8-byte {src, size}
// records much further in describe the streams showDebugFont plays. The
// NTSC and non-NTSC tables sit 0x20 apart.
typedef struct {
    u8 pad[8];
    u32 src;
    u32 size;
} DebugFontLoadInfo;
extern DebugFontLoadInfo debugFontLoadInfo __attribute__((section(".data")));

// Offsets from debugFontLoadInfo of the {src, size} stream records
// showDebugFont plays.
#define DEBUG_FONT_NTSC_SRC 0x1A98
#define DEBUG_FONT_NTSC_SIZE 0x1A9C
#define DEBUG_FONT_NON_NTSC_SRC 0x1A78
#define DEBUG_FONT_NON_NTSC_SIZE 0x1A7C

// Audio state block at 0x13E550. pauseSoundVolume (0x13E5A0) is offset 0x50
// of this same block; the byte at 0x6B (0x13E5BB) is a playback state flag
// set by showDebugFont and the space clone.
typedef struct {
    u8 pad[0x6B];
    u8 playbackFlags;
} AudioState;
extern AudioState audioState __attribute__((section(".data")));

// Bits of audioState.playbackFlags; the space sound-update code reads them to
// stop and restart the music around debug-font playback.
#define AUDIO_PLAYBACK_FLAG_STARTING 0x8
#define AUDIO_PLAYBACK_FLAG_FINISHED 0x10

// Level memory map at 0x1940C0, written by the level loader. MemSlots
// (0x1940C4) and hudHeapBase (0x1940CC) are its 0x04/0x0C fields; 0x1C holds
// the decode buffer base used to position the movie/audio sub-buffers.
typedef struct {
    u32 field_0x00;
    u32 field_0x04;
    u32 field_0x08;
    u32 field_0x0C;
    u32 field_0x10;
    u32 field_0x14;
    u32 field_0x18;
    u32 decodeBufBase;
} LevelMem;
extern LevelMem levelMem __attribute__((section(".data")));

// Offsets within the level decode buffer of the movie video and audio
// sub-buffers the decode setup is positioned on.
#define LEVEL_DECODE_VIDEO_OFFSET 0x100000
#define LEVEL_DECODE_AUDIO_OFFSET 0x400000

// In-window scalar globals: plain externs (no .data) keep the -G8 small-data
// bare pseudo that ps2eeas expands to the original's self-based absolute
// lui/load and lui at/store (see bloaders_LoadDebugFont.md).
extern u32 NTSCProgressive;
extern u32 decodeMode;
extern u32 frameBufferBase;
extern u32 GameMode;

// decodeMode (0x15EED8) values: the movie decode loop gates on it, startlevel
// initializes it to -1, 0 is normal play, 2 selects the debug-font movie.
#define DECODE_MODE_NORMAL 0
#define DECODE_MODE_DEBUG_FONT 2

// GameMode (0x15F604) values: the level loop skips normal updates while it is
// nonzero; 1 is pinned by showDebugFont while the movie plays, 3 by
// pause_scheduleInput.
#define GAME_MODE_NORMAL 0
#define GAME_MODE_DEBUG_FONT 1

// Callee prototypes. Return types are declared for codegen even when ignored.
void sound_StopAllSounds(void);
void music_Stop(void);
// C linkage: handwritten fast function in game/fastfunc; scales a frame count
// by the frame-rate factor at 0x15ED68 (1.0f in the boot ELF, an identity).
extern "C" int func_001F96F8(int frames);
// The definition is mangled FadeToBlack__FiUi (int, unsigned int) but no boot
// caller materializes a1; declare one arg and pin the symbol so the call sets
// only a0, leaving a1 as the original's leftover.
void FadeToBlack(int frames) asm("FadeToBlack__FiUi");
// Fade durations for showDebugFont in frames (FadeToBlack takes one GS alpha
// step per frame).
#define DEBUG_FONT_FADE_OUT_FRAMES 0xC
#define DEBUG_FONT_FADE_IN_FRAMES 4
// C linkage: defined in the C sound-library TU (989snd_post.c), so the symbol
// is unmangled. Stream-safe CD session sync/wait; when no stream-safe session
// is active it delegates to the raw command-queue check func_00120C30.
extern "C" int snd_StreamSafeCdSync(int mode);
// C linkage: the implementation is the generated nonmatching assembly in the
// memcard region (memcard_Update.s), whose entry point is the unmangled label.
// The memcard I/O state machine, pumped in showDebugFont's settle loop.
extern "C" int memcard_Update(void);
// C linkage: the implementation is the generated nonmatching assembly in the
// movie region, whose entry point is the unmangled label. Movie/audio decode
// setup: positions the video/audio decode sub-buffers on the movie decode
// buffer and starts the decode (writes movieDecodeBuf, zeroed on exit). No
// original identifier is recoverable from the stripped boot ELF.
extern "C" void func_0023A3B8(int src, int size, int video, int audio, int flags);
// C linkage: the implementation is in the generated SCE SDK library
// (sce/lib.s), whose entry point is an unmangled C label. CD streaming
// command-queue sync/check: mode 0 blocks until the queue drains, a nonzero
// mode polls and reports whether a read is pending.
extern "C" int func_00120C30(int mode);
// C linkage: the implementation is in the generated SCE SDK library
// (sce/lib.s), whose entry point is an unmangled C label. Returns a GS
// (graphics-synth) status flag, GS_CSR bit 0x13, read directly or through the
// display-state wrapper; the passed argument is unused by the implementation.
extern "C" int func_00122298(int arg);
// C linkage: the implementation is in the generated SCE SDK library
// (sce/lib.s), whose entry point is an unmangled C label. Busy-waits until
// every DMA/GIF/VIF channel reports idle (a wait-for-all-transfers sync); the
// passed arguments are unused by the implementation.
extern "C" int func_00120558(int arg0, int arg1);
// C linkage: the implementation is in the generated SCE SDK library
// (sce/lib.s), whose entry point is an unmangled C label. Sets or clears the
// vblank interrupt handler (interrupt channel 2) via AddIntcHandler2 /
// RemoveIntcHandler and returns the previous handler; 0 removes it.
extern "C" int func_00122E68(int (*callback)(int));
// Sends the debug-font texture to the frame buffer: GS texture format, log2
// width and height, and transfer mode (1 transfers now, 0 appends to the
// current VU chain).
void Hud_sendTexture(char* dest, int base, int format, int uLog, int vLog,
                     int mode);
// Texture transfer parameters for the 64x64 debug-font image: 16-bit color
// with 4-bit shared alpha (PSMCT16SH4 = 0x1B).
#define DEBUG_FONT_TEX_FORMAT 0x1B
#define DEBUG_FONT_TEX_U_LOG 6
#define DEBUG_FONT_TEX_V_LOG 6
#define DEBUG_FONT_TEX_MODE_IMMEDIATE 1
int vsync_callback(int arg);

// Plays boot stream table entry `index`: stops audio, fades out, waits for the
// menu to settle, runs the movie decode on the level decode buffer, refreshes
// the HUD, and fades back in. No-op for a negative index.
void showDebugFont(int index) {
    if (index < 0)
        return;

    // The original holds the font-record address in two registers (a0 32-bit,
    // v0 64-bit via an explicit move) and loads fontSize before fontSrc. This
    // EGC build cannot reproduce that split from natural pointer arithmetic
    // (it keeps the address in one register and reverses the load/s-reg
    // coupling), so the address is pinned to a0/v0 with inline asm. See
    // decomp_state/notes/bmain_showDebugFont__Fi.md.
    register u32 fontSize asm("$17");
    register u32 fontSrc asm("$18");
    if (NTSCProgressive) {
        register u32 scaled asm("$3") = index * 8;
        register u8* base asm("$2") = (u8*)&debugFontLoadInfo;
        register u8* p asm("$4");
        asm volatile("addu %0,%1,%2" : "=r"(p) : "r"(base), "r"(scaled));
        register u8* q asm("$2");
        asm volatile("daddu %0,%1,$0" : "=r"(q) : "r"(p));
        fontSize = *(u32*)(p + DEBUG_FONT_NTSC_SIZE);
        fontSrc = *(u32*)(q + DEBUG_FONT_NTSC_SRC);
    } else {
        register u32 scaled asm("$3") = index * 8;
        register u8* base asm("$2") = (u8*)&debugFontLoadInfo;
        register u8* p asm("$4");
        asm volatile("addu %0,%1,%2" : "=r"(p) : "r"(base), "r"(scaled));
        register u8* q asm("$2");
        asm volatile("daddu %0,%1,$0" : "=r"(q) : "r"(p));
        fontSize = *(u32*)(p + DEBUG_FONT_NON_NTSC_SIZE);
        fontSrc = *(u32*)(q + DEBUG_FONT_NON_NTSC_SRC);
    }

    decodeMode = DECODE_MODE_DEBUG_FONT;
    audioState.playbackFlags |= AUDIO_PLAYBACK_FLAG_STARTING;
    FlushCache(0);
    sound_StopAllSounds();
    music_Stop();
    FadeToBlack(func_001F96F8(DEBUG_FONT_FADE_OUT_FRAMES));
    GameMode = GAME_MODE_DEBUG_FONT;
    FlushCache(0);
    sound_StopAllSounds();
    music_Stop();
    snd_StreamSafeCdSync(0);

    while (menuStateData.field_0xD4 >= 3 || menuStateData.field_0xDC >= 0)
        memcard_Update();

    u32 decBase = levelMem.decodeBufBase;
    func_0023A3B8(fontSrc, fontSize, decBase + LEVEL_DECODE_VIDEO_OFFSET,
                  decBase + LEVEL_DECODE_AUDIO_OFFSET, 0);
    func_00120C30(0);
    func_00122298(0);
    func_00120558(0, 0);
    func_00122E68(vsync_callback);

    // 0x1000000 is the EE VRAM/GS base (hardware region), not a data symbol.
    Hud_sendTexture((char*)0x1000000, frameBufferBase, DEBUG_FONT_TEX_FORMAT,
                    DEBUG_FONT_TEX_U_LOG, DEBUG_FONT_TEX_V_LOG,
                    DEBUG_FONT_TEX_MODE_IMMEDIATE);
    decodeMode = DECODE_MODE_NORMAL;
    FadeToBlack(DEBUG_FONT_FADE_IN_FRAMES);
    GameMode = GAME_MODE_NORMAL;
    audioState.playbackFlags |= AUDIO_PLAYBACK_FLAG_FINISHED;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/bmain", startlevel__Fv);
