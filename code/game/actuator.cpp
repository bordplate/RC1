#include "common.h"

// The original .text section starts with 8 padding bytes (two nops) before
// the first actuator function; spimdis split them into a phantom 4-byte
// "function" (func_001E8D00) plus one trailing nop. Nothing jumps here.
asm("nop");
asm("nop");

INCLUDE_ASM("code/_generated/nonmatchings/game/actuator", actuator_CalcPower);

// Unreachable dead tail after actuator_CalcPower (0x1E8D08): one
// `addiu sp,sp,0x20`; the unit size does not match the parent's 0xC0 frame
// (see the 989snd dead addiu-sp tail family). Nothing reaches 0x1E9120
// (0 jal/j/branch/data references; no Ghidra function), so it is not a
// function; the original compiler emitted this byte after the parent's
// RTL, so it is preserved here as an exact word. The trailing nop pads to
// the 8-aligned texResetCursor__Fv entry at 0x1E9128.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001E9120, 0x4\n"
    "glabel func_001E9120\n"
    "    .word 0x27bd0020\n"
    "endlabel func_001E9120\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);

extern int textureCursor;
// Base of the VRAM texture pool; textureCursor allocates from it.
extern int textureMemoryBase;
// Allocator counter cleared with the cursor at level start; no boot-ELF
// readers, likely maintained by level overlay code. Exact role unconfirmed.
extern int textureAllocCounter;

void texResetCursor(void) {
    textureCursor = textureMemoryBase;
    textureAllocCounter = 0;
}

// These padding instructions preserve the original boundary before the next
// generated actuator function.
asm("nop");
asm("nop");

// Unreachable dead tail after texResetCursor__Fv (0x1E9128): four
// `addiu sp,sp,N` units (0x40, 0xA0, 0x10, 0x130) with interleaved nops;
// the parent is a frameless leaf, so none of the unit sizes matches its
// frame (see the 989snd dead addiu-sp tail family). Nothing reaches
// 0x1E9148 (0 jal/j/branch/data
// references; no Ghidra function), so it is not a function; the original
// compiler emitted these bytes after the parent's RTL, so they are
// preserved here as exact words. The trailing nop pads to the 8-aligned
// bloaders .text entry (LoadPifAsPSMT8H) at 0x1E9168.
asm(
    ".section .text\n"
    "    .set noat\n"
    "    .set noreorder\n"
    "    .align 3\n"
    "    nonmatching func_001E9148, 0x1C\n"
    "glabel func_001E9148\n"
    "    .word 0x27bd0040\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd00a0\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0010\n"
    "    .word 0x00000000\n"
    "    .word 0x27bd0130\n"
    "endlabel func_001E9148\n"
    "    .word 0x00000000\n"
    "    .set reorder\n"
    "    .set at\n"
);
