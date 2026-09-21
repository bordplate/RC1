#include "common.h"
#include "types.h"
#include "menu.h"

// The boot font table is a large block at 0x137B80. The 16-byte head is the
// debug font image (LoadDebugFont, bloaders.cpp); the 8-byte {src, size}
// records much further in describe the streams showDebugFont plays. The
// NTSC/non-NTSC tables sit 0x20 apart (0x1A98/0x1A78).
typedef struct {
    u8 pad[8];
    u32 src;
    u32 size;
} DebugFontLoadInfo;
extern DebugFontLoadInfo debugFontLoadInfo __attribute__((section(".data")));

// Audio state block at 0x13E550. pauseSoundVolume (0x13E5A0) is offset 0x50 of
// this same block; the byte at 0x6B (0x13E5BB) is a playback state flag set by
// showDebugFont and the space clone (bit 3 = starting, bit 4 = finished).
typedef struct {
    u8 pad[0x6B];
    u8 playbackFlags;
} AudioState;
extern AudioState audioState __attribute__((section(".data")));

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

// In-window scalar globals: plain externs (no .data) keep the -G8 small-data
// bare pseudo that ps2eeas expands to the original's self-based absolute
// lui/load and lui at/store (see bloaders_LoadDebugFont.md).
extern u32 NTSCProgressive;
extern u32 decodeMode;
extern u32 frameBufferBase;
extern u32 GameMode;

// Callee prototypes. Return types are declared for codegen even when ignored.
void sound_StopAllSounds(void);
void music_Stop(void);
extern "C" int func_001F96F8(int frames);
// The definition is mangled FadeToBlack__FiUi (int, unsigned int) but no boot
// caller materializes a1; declare one arg and pin the symbol so the call sets
// only a0, leaving a1 as the original's leftover.
void FadeToBlack(int frames) asm("FadeToBlack__FiUi");
extern "C" int snd_StreamSafeCdSync(int mode);
extern "C" int memcard_Update(void);
extern "C" void func_0023A3B8(int src, int size, int video, int audio, int flags);
extern "C" int func_00120C30(int mode);
extern "C" int func_00122298(int arg);
extern "C" int func_00120558(int arg0, int arg1);
extern "C" int func_00122E68(int (*callback)(int));
void Hud_sendTexture(char* dest, int base, int c0, int c1, int c2, int c3);
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
        fontSize = *(u32*)(p + 0x1A9C);
        fontSrc = *(u32*)(q + 0x1A98);
    } else {
        register u32 scaled asm("$3") = index * 8;
        register u8* base asm("$2") = (u8*)&debugFontLoadInfo;
        register u8* p asm("$4");
        asm volatile("addu %0,%1,%2" : "=r"(p) : "r"(base), "r"(scaled));
        register u8* q asm("$2");
        asm volatile("daddu %0,%1,$0" : "=r"(q) : "r"(p));
        fontSize = *(u32*)(p + 0x1A7C);
        fontSrc = *(u32*)(q + 0x1A78);
    }

    decodeMode = 2;
    audioState.playbackFlags |= 0x8;
    FlushCache(0);
    sound_StopAllSounds();
    music_Stop();
    FadeToBlack(func_001F96F8(0xC));
    GameMode = 1;
    FlushCache(0);
    sound_StopAllSounds();
    music_Stop();
    snd_StreamSafeCdSync(0);

    while (menuStateData.field_0xD4 >= 3 || menuStateData.field_0xDC >= 0)
        memcard_Update();

    u32 decBase = levelMem.decodeBufBase;
    func_0023A3B8(fontSrc, fontSize, decBase + 0x100000, decBase + 0x400000, 0);
    func_00120C30(0);
    func_00122298(0);
    func_00120558(0, 0);
    func_00122E68(vsync_callback);

    // 0x1000000 is the EE VRAM/GS base (hardware region), not a data symbol.
    Hud_sendTexture((char*)0x1000000, frameBufferBase, 0x1B, 6, 6, 1);
    decodeMode = 0;
    FadeToBlack(4);
    GameMode = 0;
    audioState.playbackFlags |= 0x10;
}

INCLUDE_ASM("code/_generated/nonmatchings/game/bmain", startlevel__Fv);
