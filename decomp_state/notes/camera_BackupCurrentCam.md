# BackupCurrentCam (0x1EBC90, 0x5C bytes) — matched 2026-09-08

Backs up the active camera: copies the 0xA0-byte camera struct from `*curCam`
(0x1870C0) into `backupCam` (0x189210), copies the camera's 0x280-byte data
buffer (`backupCamData - 0x500` = 0x1893D0) into `backupCamData` (0x1898D0),
then patches the backup struct's data-pointer field (offset 0x70, verified via
FUN_001ebf10's `puVar8[0x1c]`/`DAT_001870c4 + 0xe`) to the backup buffer.
Sits between `CamPostUpdRoutines` (0x1892B0) and the 0x1893D0 buffer; the
three 0x280-byte buffers are 0x1893D0 (active), 0x189650 (second cam),
0x1898D0 (backup).

Final source (code/game/camera.cpp):

```cpp
extern u8 backupCam[];
extern u8 backupCamData[];
extern u32 curCam __attribute__((section(".data")));

extern "C" void BackupCurrentCam(void) {
    u8 *dst = backupCam;
    FastMemCopy(dst, (void*)curCam, 0xA0);
    u8 *p = backupCamData;
    FastMemCopy(p, p - 0x500, 0x280);
    *(void**)(dst + 0x70) = p;
}
```

Symbols added to config/symbols.txt: `curCam = 0x1870c0`,
`backupCam = 0x189210`, `backupCamData = 0x1898d0` (all .data, out of the gp
window → absolute lui/addiu, no GPREL risk).

## Codegen findings (the two non-obvious requirements)

1. The 0x1870C0 load must be a GLOBAL LOAD with a section attribute:
   `extern u32 curCam __attribute__((section(".data")));` + `(void*)curCam`.
   EGC then emits `lui v0,%hi; lw a1,%lo(v0)` with a SEPARATE base register
   (v0) and hoists the lui to the second prologue slot (between `addiu sp`
   and `sq s1`), interleaving `lw a1` between `lui s1`/`addiu s1` — the
   original's exact prologue. Alternatives fail:
   - constant cast `*(void**)0x1870C0`: base register = destination (a1),
     lui+lw forced adjacent at the end of the prologue (7-word prologue diff);
   - `extern u32 *curCam;` without section: R_MIPS_GPREL16 link error
     (small-data assumption for the undefined scalar).
2. The buffers must be LOCAL POINTERS, not inline array symbols:
   `u8 *dst = backupCam; ... u8 *p = backupCamData;`. EGC keeps dst in s1 and
   p in s0 across both calls (0x30 frame, saves s1/ra/s0). Passing the
   arrays directly swaps the allocation (dst→s0, p→s1), and
   `backupCamData - 0x500` as an array expression is folded by EGC into an
   independent `lui/addiu` of 0x1893D0 — the original instead reuses the
   base: `addiu a1, s0, -0x500` in the second jal's delay slot. Pointer
   arithmetic on the local (`p - 0x500`) is what preserves the dependency.

Both call arguments a0/a2 materialize as expected (`move a0,s1`/`move a0,s0`
in delay slots, `li a2` reloaded for the second call); store is
`sw s0, 112(s1)`.

Verified: probe byte-match 92/92 (decomp_state/probes/camera_backupcam_v5.cpp),
clean `make split && make` + `cmp build/boot_elf.elf assets/boot_elf.elf` OK,
independent verifier (objdump diff of both ELFs + clean rebuild) passed.
743 nonmatching targets remain.
