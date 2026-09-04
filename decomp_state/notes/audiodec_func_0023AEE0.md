# audioDecIsPageFull (was func_0023AEE0) @ 0x0023AEE0, size 0x10, movie/audiodec.cpp

Original asm (file offset 0x13BE60):

    lw   $2, 0x50($4)
    slti $2, $2, 0x1000
    jr   $31
         xori $2, $2, 1        # delay slot -> return !(a < 0x1000)

Semantics: returns `self->sentPos >= 0x1000` for the `_AudioDec` object
(movie context ptr D_0016120C + 0xD9100). Field layout evidence (Ghidra):

- audioDecReset (0x23AD10) and audioDecCreate zero offset 0x50.
- sendADPCM (0x23AFC0) does `*off50 += 0x400` per block copied into the
  0x1000-byte SPU page, and computes free page space as `0x1000 - *off50`.
- Sole caller is isAudioOK (movie.cpp, 0x23A790), which forwards the result:
  in readMpeg (0x23A460) `if (isAudioOK())` gates startDisplay__Fi(1) +
  audioDecStart. Note the call uses the jr delay slot for the pointer add,
  so Ghidra shows isAudioOK as discarding the value; it does not.

Replacement (code/game/movie/audiodec.cpp):

    typedef struct _AudioDec {
        u8 _pad[0x50];
        int sentPos;
    } _AudioDec;

    extern "C" int audioDecIsPageFull(_AudioDec* self) {
        return self->sentPos >= 0x1000;
    }

Signed `int` is required: unsigned would compile to sltu, original uses
slti. The inversion lands in the jr $ra delay slot exactly like the
original (slti + xori).

Renamed func_0023AEE0 -> audioDecIsPageFull and added
`audioDecIsPageFull = 0x0023aee0;` to config/symbols.txt (// movie/audiodec.cpp
section). Splat then regenerated isAudioOK.s with `jal audioDecIsPageFull`.

Build gotcha: the Makefile has no dependency from C/C++ objects to the
generated .s files they INCLUDE_ASM, so renaming a symbol leaves other
translation units' objects stale (link failed with undefined
`func_0023AEE0` in movie.o). Fix: touch every source file whose INCLUDE_ASM
blocks reference the renamed symbol (here code/game/movie/movie.cpp), then
rebuild.

Verification: objdump of build/code/game/movie/audiodec.o shows
audioDecIsPageFull at .text+0x310 with words 8c820050 28421000 03e00008
38420001, i.e. little-endian bytes 5000828C 00104228 0800E003 01004238,
identical to the original slice at file offset 0x13BE60. Full
`make split && make` then `cmp build/boot_elf.elf assets/boot_elf.elf`
passes; decomp_status target count 861 -> 860.
