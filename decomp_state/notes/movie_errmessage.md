# ErrMessage (vram 0x23AB78, file 0x13BAF8, 36 bytes) — MATCHED

`extern "C" void ErrMessage(const char* msg)` prints the `.lit` string at
D_001611F8 (`"[ Error ] %s\n"`) through the printf import thunk STUB_printf
(vram 0x1E93B0, kept as INCLUDE_ASM in code/_generated/game/stub.s).

Matched 2026-09-04 with a one-line body; standalone EGC -G8 -O2 test compile
reproduced all 9 words and the register allocation on the first attempt. See
decomp_state/matched.json for verification details.

## Sibling candidates in code/game/movie/movie.cpp (investigated same session)

- switchThread (0x23A770, 0x1C): `jal func_001188C0` with a0=1 set in the
  delay slot (`func_001188C0(1);`, return discarded; sp -0x10/ra save is the
  standard call frame). func_001188C0 needs identification first. (Still in
  movie.cpp after the 2026-09-12 range split.)
- isAudioOK (0x23A790, 0x2C): MATCHED 2026-09-12, now in movie_mid.cpp — see
  decomp_state/notes/movie_isAudioOK.md. Original investigation: loads
  `*D_0016120C` (global, used all over the movie code as a base pointer —
  e.g. DAT_0016120c + 0xd9xxx offsets in mpegError/readMpeg), adds constant
  0xD9100 to it, and calls audioDecIsPageFull (matched) with the sum; returns
  its result.

## Data layout note

D_0016120C sits at vram 0x16120C inside the `.lit` section (0x15EF00-0x161228),
not in .data; it holds a pointer that movie code combines with +0xD9xxx
offsets. The strings passed to ErrMessage (0x1E8B08 "pts buffer overflow\n",
0x1E8B38 "decode thread: aborted\n", 0x1E8B50 "sceMpegGetPicture() decode
error") live just before lvl.vtbl at 0x1E8B80, inside the `.text` section.
