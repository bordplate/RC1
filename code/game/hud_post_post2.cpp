#include "common.h"
#include "types.h"
#include "hud.h"

extern int hudMessageTimer;

// C linkage: strlen is an unmangled core.text SDK string helper (0x001166CC);
// setMessageText calls it to bound the incoming message length.
extern "C" unsigned int strlen(const char* str);
// C linkage: memcpy is an unmangled SDK memory helper. EGC inlines this small
// constant-size call as unaligned ldl/ldr (two 64-bit moves) plus a 3-byte
// lb/sb tail; the pointer args must stay u8* (a u64* cast would emit aligned
// ld/sd and miss the original by 4 words).
extern "C" void* memcpy(void* dst, const void* src, unsigned int n);
// C linkage: func_001165B8 is a resident core.text SDK SIMD null-terminated
// string copy (dst=a0, src=a1); setMessageText uses it to copy str into the
// buffer.
extern "C" void func_001165B8(u8* dst, u8* src);
extern u8 messageTextBuffer[];
extern u8 messageTooLongLabel[];

#define MESSAGE_TEXT_MAX_LEN 0x50
#define MESSAGE_TOO_LONG_LABEL_LEN 19

// Unreachable dead tail before setMessageText (0x1FF5E8-0x1FF657): twelve
// `addiu sp,sp,N` units (0x60,0x20,0x60,0x40,0x40,0x90,0x10,0x90,0x10,0x20,
// 0x10,0x130) with interleaved nops. Nothing reaches 0x1FF5E8 (0 jal/j/branch/
// data references; no Ghidra function), so it is not a function; the original
// compiler emitted these bytes ahead of the next function, so they are
// preserved here as exact words. The real function follows at 0x1FF658.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FF5E8, 0x70\n"
    "glabel func_001FF5E8\n"
    "    .word 0x27bd0060\n"
    "    .word 0x00000000\n"
    "    .word 0x00000000\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x00000000\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0060\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0040\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0040\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0090\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0090\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0130\n"
    "    .word 0x00000000\n"
    "endlabel func_001FF5E8\n"
    "    .set reorder\n"
    "    .set at\n"
);

void setMessageText(char* str);
void setMessageText(char* str) {
    if (strlen(str) < MESSAGE_TEXT_MAX_LEN) {
        memcpy((void*)messageTextBuffer, (void*)messageTooLongLabel, MESSAGE_TOO_LONG_LABEL_LEN);
    }
    func_001165B8((u8*)messageTextBuffer, (u8*)str);
}

// Unreachable dead tail after setMessageText (0x1FF6D8-0x1FF760): eighteen
// `addiu sp,sp,N` units (0x10,0x20,0xB0,0x120,0x70,0xD0,0x20,0x20,0x50,0xC0,
// 0x30,0x40,0x20,0xB0,0xA0,0xD0,0x70,0x30) with interleaved nops. Nothing
// reaches 0x1FF6D8 (0 jal/j/branch/data references; no Ghidra function), so
// it is not a function; the original compiler emitted these bytes after the
// preceding RTL, so they are preserved here as exact words. The trailing nop
// pads to the 8-aligned hud_updateMessageTimer entry at 0x1FF768.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001FF6D8, 0x8C\n"
    "glabel func_001FF6D8\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00b0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0120\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0070\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00d0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0050\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00c0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0030\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0040\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0020\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00b0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00a0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00d0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0070\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0030\n"
    "endlabel func_001FF6D8\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

void hud_updateMessageTimer(void) {
    if (hudMessageTimer != 0) {
        hudMessageTimer--;
    }
}
