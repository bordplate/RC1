# InitOnce__Fv (0x201650, 0x3CC = 243 words)

## Full call sequence (from objdump)
1. resetVif1Gif()
2. sceDmaReset(1)
3. sceCdInit(0)
4. sceCdDiskReady(0)
5. do{}while(sceSifRebootIop(iopRebootImagePath)==0)  // loop1, 8 words, beqz v0
6. do{}while(sceSifSyncIop()==0)                       // loop2, 6 words, beqz v0
7. STUB_printf(rebootedIopMessage)          // "Rebooted IOP\n" 0x15FA88
8. EnableCache(3)                            // 0x118A60 syscall 97
9. initializeSoundSystem(0)                  // 0x11AB20
10. DIntr()                                  // 0x11D660
11. sceSifInitIopHeap()                      // 0x11C840
12. EIntr()                                  // 0x11D6A8
13. sceCdInit(0)
14. sceCdDiskReady(0)
15. sceCdMmode(2)                            // 0x121388
16. sceFsReset()                             // 0x11BC48
17. wad_GetSectors_FiiPv(0x121, 1, buf)      // buf = local char[0x800] at sp
18. FlushCache(0)
19. isPAL = (buf[0x33] != 'N')
20. pauseActionListMode = isPAL              // 0x15EE90
21. memcard_GetName(buf)                     // return discarded
22. videoModePal = isPAL                     // 0x15ED80 (store in memcard_GetName delay slot)
23. sceGsResetGraph(0, 1, isPAL?3:2, 0)
24. InitDma()
25. SetPalMode()                             // a0 leftover 0
26. VU0_loadMicroProgram(vu0MicroProgram)    // 0x10E4C0
27. func_0012F2B8()                          // wad_GetHeaders, a0 leftover
28. IRX block:
    textBase = text_VRAM_END + 0x3FFF        // = 0x24135F (lui 0x24; addiu 0x135F)
    irxBufBase = (textBase & 0xFFFFC000) + 0x2C0000
    irxSrc = 0x1FF8000 - (irxCount << 11)
    irxSector = bootAssets.irxSector (0x12C0)
    irxCount  = bootAssets.irxCount  (0x12C4)
    wad_GetSectors_FiiPv(irxSector, irxCount, irxSrc)
    FlushCache(0)
    FastDecompress(irxSrc, irxBufBase)
    FlushCache(0)
    10x LoadIRXModule(irx[i].offset + irxBufBase, irx[i].size)
       irx = (IrxModule*)(irxBufBase + 0x70); struct{u32 offset;u32 size;}
       order: structs 0,1,2,3,4,5,5,7,8,6
29. STUB_printf(loadedIrxModulesMessage)      // 0x1E7AF8
30. sceCdDiskReady(0)
31. func_00217048()                           // stream_Init
32. sceGsSyncVCallback(vsync_callback)        // 0x122E68
33. InitMemSlots()   + levelDataBase=0x160000 in its delay slot  (0x160E0C)
34. sceCdDiskReady(0)
35. memcard_Init()
36. InitViewContext()
37. UpdateViewContext()
38. VU1_initChain()
39. sceCdDiskReady(0)
40. func_0022C8D0()                           // snd_Init
41. FastMemSet(loadImageBuffer, 0x80808080, 0x100)
42. sceGsSetDefLoadImage(buf, 0x3FFB, 1, 0, 0, 0, 8, 8)  // 8-arg: a0-a3,t0-t3
43. FlushCache(0)
44. sceGsExecLoadImage(buf, loadImageBuffer)
45. sceGsSyncPath(0, 0)
46. LoadDebugFont()
47. *(u32*)0x10000810 = 0x82; *(u32*)0x10000800 = 0;  // GS/VU0 regs, a0 keeps 0x82
48. stash_init()                               // 0x232CE0, a0 leftover 0x82
49. buildBitSwapLut()
50. lang = sceScfGetLanguage()
51. switch(lang): 1->gameLanguage=0, 2->2, 3->4, 4->3, 5->5,
    default->STUB_printf(unsupportedLangMessage, lang); gameLanguage=0
    (default falls through to shared gameLanguage=0 at 0x2019FC)

## Registers
frame 0x840; saved s0@0x800 s1@0x810 s2@0x820 ra@0x830
s0 = iopRebootImagePath hi (0x1E) hoisted in prologue, later textBase (0x24)
s1 = 0x1FF8000 (0x2000000 - 0x8000)
s2 = loadImageBuffer (0x1941C0)

## Symbols added to config/symbols.txt
DATA (keep): iopRebootImagePath 0x1E7AE0, rebootedIopMessage 0x15FA88,
  loadedIrxModulesMessage 0x1E7AF8, unsupportedLangMessage 0x1E7B10,
  vu0MicroProgram 0x10E4C0, levelDataBase 0x160E0C
SDK FUNC renames (took effect in sce/lib.s): sceCdInit 0x120EB0,
  sceCdDiskReady 0x121190, sceSifRebootIop 0x11CF48, sceSifSyncIop 0x11CF10,
  EnableCache 0x118A60, DIntr 0x11D660, EIntr 0x11D6A8, sceCdMmode 0x121388,
  sceFsReset 0x11BC48, sceGsSyncVCallback 0x122E68, sceScfGetLanguage 0x12D240

## LEARNING: function renames in symbols.txt
Renaming a FUNCTION in symbols.txt renames references in OTHER .s files but does
NOT rename the defining .s file's glabel/filename for game non-matching funcs
(func_XXXXXX.s kept old glabel) -> link error (undefined ref). It DID work for
SDK funcs in sce/lib.s. So: reverted wad_GetHeaders/stream_Init/snd_Init renames
(keep func_0012F2B8/func_00217048/func_0022C8D0); updated bmain.cpp
func_00122E68 -> sceGsSyncVCallback. DATA symbols in symbols.txt link fine.

## Open questions
- EGC reg alloc: need irxBufBase in s0 for the 10 LoadIRXModule lw loads.
- The 0x82 in a0 across two GS stores + stash_init call.
- sceGsSetDefLoadImage 8-arg reg map (a0-a3,t0-t3).
- textBase lui/addiu split scheduling.
- default-case fall-through to shared gameLanguage=0.

## CURRENT STATE (session 2 — probe-iterated, ndiff=91)
Probe loop: `./working/initonce_InitOnce__Fv/run_probe.sh` + defines.txt (now 10
symbols, added irxScratchTop=0x1ff8000). Result: orig=0x3cc cand=0x3d0 ndiff=91.

FIXES LANDED THIS SESSION:
- VU0_loadMicroProgram(long*) __FPl (was long -> __Fl); call casts (long*).
- IRX block: single base `u32* mod=(u32*)irxBufBase` + word offsets -> no extra s3,
  frame 0x840, prologue matches.
- isPal: `unsigned char buf[0x800]` (lbu); sceGsResetGraph 3rd arg uses the
  GLOBAL videoModePal (forces reload lw v1,-4736(v1)); `videoModePal=isPal`
  moved BEFORE memcard_GetName so isPal last-use precedes the call -> isPal in v0.
- memcard_GetName((char*)buf) cast (EGC treats unsigned->char signedness warn as FATAL).
- 0x1FF8000 -> symbol `irxScratchTop[]` (extern u32[]) so EGC emits the SIGNED
  split (lui 0x200; addiu -0x8000) like the original D_1FF8000, not unsigned.
- BUG FIX: the store after InitMemSlots is `currentVuChain = 0x160000`
  (0x160F0C), NOT `levelDataBase = 0x160000` (0x160E0C). Was misidentified.

REMAINING 4 DEEP CODEGEN ISSUES (ndiff=91):
1. isPal store address-mode SWAP (0x201750/58): orig = pauseActionListMode
   AT-based (lui at,0x16; sw v0,-4464(at)) before call + videoModePal GP-based
   (sw v0,-32384(gp)) in the jal delay slot. MINE = reversed (videoModePal at-based,
   pauseActionListMode gp-based). Both are plain `extern u32` in the gp window.
   Tried: source-order (no effect), `.data` section attr on pauseActionListMode
   (WORSE, 94). Unresolved — which global gets the at-based (2-instr) vs gp-based
   (1-instr) form is an EGC allocator/scheduler tie-break I cannot steer.
2. Dead `addiu sp,sp,48` tail at 0x201a20 (after `jr ra`/`addiu sp,sp,2112`).
   NO matching `addiu sp,sp,-48` allocation (only the 0x840 main frame). Pure dead
   48-byte dealloc -> makes cand 0x3d0 vs orig 0x3cc. Source unknown.
3. Language switch JUMP TABLE TYPE: orig uses a .rodata DATA jtbl (jtbl_001E7B30,
   5 words, indexed by lang-1) + `jr a0`; mine uses a LOCAL .text jtbl (0x201A30).
   Switch SEMANTICS match (jtbl[lang-1]: lang1->0x2019FC gl=0, lang2->0x2019C0 gl=2,
   lang3->0x2019D8 gl=4, lang4->0x2019CC gl=3, lang5->0x2019E4 gl=5, default
   0x2019F0 STUB_printf->gl=0). My source case order (2,3,4,5,default,1) matches
   the original block order, yet EGC emits a local jtbl. Possibly flag/version.
4. Boot-call + IRX LoadIRXModule SCHEDULING: orig hoists lui s0(textBase)/s1/s2
   early and splits the lui/addiu across nop delay slots; mine schedules the
   10 LoadIRXModule lw/jal/addu sequences in a different interleaving.

ESCALATION: normal iteration (source reorder, section attrs, symbol naming,
register/prototype analysis) exhausted for issues 1/2/3. Next: expert subagent
(one-shot), then last-resort-decompiler before any blocked.json entry.

## CURRENT STATE (session 3 — BLOCKED)
Consulted `expert` (GPT-6 Astra, one-shot) and then `last-resort-decompiler`
(GPT-5.6 Sol, required before blocking). last-resort returned 3 concrete fixes:
(1) unseeded VOLATILE store aliases for the isPal pair, (2) move
`currentVuChain = 0x160000` BEFORE `InitMemSlots()`, (3) source case order
`2,4,3,5` for the language switch.

VERIFIED CORRECT against the reference .s (would match if the function were
retried): (2) — original 0x2018BC-0x2018C4 is `lui $3,0x16 / jal InitMemSlots /
sw $3,-0x5CF4(gp)`, i.e. the store is in the call delay slot, so the assignment
belongs BEFORE the call; (3) — original physical blocks are gl=2,3,4,5 in
address order (0x2019C0/CC/D8/E4) with the jtbl mapping lang3->gl4, lang4->gl3,
so source order must be `case 2, case 4, case 3, case 5`. Both applied.

FIX (1) FAILED mechanically (the decompiler's own predicted failure mode):
with `extern volatile u32 pauseActionListModeStore/videoModePalStore` (+
linker_aliases.ld same-address aliases) the full build emits BOTH stores
AT-based (`lui at,0x16; sw v0,0xEE90(at)` AND `lui at,0x16; sw v0,0xED80(at)`)
BEFORE the `jal memcard_GetName`, and fills the jal delay slot with an
unrelated `addiu`. Volatile keeps the store ORDER but blocks EGC from placing
the 2nd store in the jal delay slot (the GPREL16 form). Result: InitOnce grows
0x3cc -> 0x3d4 (+8 B); that +8 shifts every later function in the .text TU and
corrupts the section table (build +48 B, 37307 diff runs, ~35500 in the text
section vram 0x200000-0x23FFFF). The isPal store address-mode selection is an
EGC 2.95.2 SN allocator/scheduler tie-break that neither plain externs (swaps
the pair) nor volatile aliases (blocks delay-slot fill) can steer. BLOCKED.

The "Cluster 2 scheduling" the expert/last-resort flagged as a ~89-word
mismatch was NOT a real scheduler wall: the probe's earlier 91 was inflated by
the seeded-alias phase shift; the default scheduler is correct and the boot/IRX
call layout matches. Only the isPal pair (issue 1) is the true blocker.

Reverted InitOnce to INCLUDE_ASM (committed form) and removed the store
aliases + irxScratchTop from linker_aliases.ld. Kept the symbols.txt data
symbols + SDK renames and the bmain.cpp/boot_level.h changes (they are
overlay-safe, same-address, and useful). Full build must re-verify to a clean
match before this is considered durable.

DEAD TAIL (2026-10-11): the 4-byte fragment at 0x201A20 (addiu sp,sp,0x30;
nop), previously its own INCLUDE_ASM target func_00201A20, is a ghost per the
dead-tail policy: it sits after the `jr ra`/`addiu sp,sp,2112` epilogue +
alignment nop, matches no live frame allocation (the only frame is 0x840),
and tools/deadness_scan.py reports 0 references (no Ghidra function there).
It is now emitted as byte-preserving inline asm in initonce.cpp (raw words
0x27bd0030 / 0x00000000 — note R5900 major-opcode reassignment: 0x27 is
ADDIU here, so Splat's comment field prints the reversed LE word as 3000BD27)
at the same source position, with the glabel retained. The parent keeps its
INCLUDE_ASM. Verified: objdump of the built ELF shows the identical words,
full build + cmp build/boot_elf.elf assets/boot_elf.elf byte-identical;
nonmatching count 535 -> 534.
