# GetEffectTex__Fii (0x1F44B8, 328 B) — BLOCKED

## What the function does (semantics fully decoded, high confidence)
`u64 GetEffectTex(int index, int unused)` — 2nd param dead. Allocates a 16-byte
texture-record (tex0 descriptor) for `effectTexs[index]` on first use and registers a
GIF load slot.

```c
typedef struct { u64 tex0; u16 ppal; u16 ptex; u16 uLog; u16 vLog; } EffectTex; // 16B
typedef struct { u32 tex; u16 fmt; u16 ploc; u32 pal; u8 uLog; u8 vLog; u16 tloc; } GifLoad; // 16B
// effectTexs=0x18D440, textureCursor=0x15EE74, gifLoadCnt=0x15F458 (symbols.txt);
// gifLoads=0x18D040, effectTexBase=0x15F460 (still unnamed).

u64 GetEffectTex(int index, int unused) {
    if (effectTexs[index].tex0 == 0) {
        EffectTex* e = &effectTexs[index];
        int tbw  = (s16)e->uLog - 6;  tbw = tbw >= 0 ? tbw : 0;   // max(0, uLog-6)
        int ploc = textureCursor >> 8;
        int next = textureCursor + 0x400;
        u64 tex0 = (u32)(next >> 8);
        tex0 |= (u64)(1 << tbw) << 14;
        tex0 |= (u64)(s16)e->uLog << 26 | 0x1300000ULL;
        tex0 |= (u64)(s16)e->vLog << 30;
        tex0 |= (u64)ploc << 37 | 0x400000000ULL;
        tex0 |= 0x8000000000000000ULL;
        textureCursor = next + (1 << ((s16)e->uLog + (s16)e->vLog));
        e->tex0 = tex0;
        if (gifLoadCnt < 0x40) {
            GifLoad* s = &gifLoads[gifLoadCnt];
            s->ploc = ploc;  s->tex = effectTexBase + (u16)e->ptex*16;  s->fmt = 0;
            s->pal  = effectTexBase + (u16)e->ppal*16;  s->tloc = next >> 8;
            s->uLog = (u8)e->uLog;  s->vLog = (u8)e->vLog;
            gifLoadCnt++;
        }
    }
    return effectTexs[index].tex0;   // RELOAD: tail recomputes &effectTexs[index] and reloads
}
```
tex0 layout: bits0-12=(next>>8); (1<<tbw)<<14; (s16)uLog<<26 | 0x1300000 (bits20/22/24 + 26-41);
(s16)vLog<<30 (bits30-45); ploc<<37 | 0x400000000 (bit34); 1<<63.

The C above is SEMANTICALLY correct (verified against the disassembly field-by-field), but it
does not produce byte-identical assembly.

## Why it is blocked — three coupled EGC 2.95.2 codegen walls
Best candidate (probe5.cpp) = 324 B vs 328 B, 73 of 82 words differ. The 73 diffs collapse
to three coupled EGC-internal decisions that no tested source form moves:

1. **Register allocation shifted +1 from the prologue.** Original: index→t5, base_hi→t6,
   ptr→t2, 64-bit accumulator→v0 (t-registers t2-t6, 7 incl. t0). Candidate: index→t6,
   base→t7, ptr→t3, accumulator→t1/t2 (t-registers t2-t7, 8 incl. t0). The very first
   `move` assigns index to t5 (orig) vs t6 (cand) — the candidate holds one extra live
   t-register value. Pinning the accumulator to v0 (`register u64 tex0 asm("$2")`) does NOT
   shift index/base back to t5/t6, so the +1 shift is an allocator heuristic, not the accumulator.

2. **The dsll32+dsra sign-extend idiom for uLog/vLog.** Original descriptor terms are
   `lhu r,off; dsll32 r,r,16; dsra r,r,22` (uLog) / `dsra r,r,18` (vLog) = (s16)uLog<<26 /
   (s16)vLog<<30 (sign-extend a register value into the high word). Every field form EGC
   FOLDS to `lh + dsll`:
   - `(u64)(s16)e->uLog << 26` (u16 field)        -> lh + dsll
   - `(u64)e->uLog << 26` (unsigned field)        -> lhu + dsll (plain)
   - `signed int uLog : 16` bitfield              -> lh + dsll  (last-resort hypothesis, FAILED)
   - `u16 ul=e->uLog; (u64)(s16)ul << 26`         -> lh + dsll (EGC elides the local, reloads field)
   - `static inline buildTex0(u16 uLog,...)`      -> lh + dsll (elided through inlining)
   - `(u64)((s16)e->uLog << 26)` (32-bit shift)   -> sll (32-bit, wrong)
   The idiom reproduces ONLY when the value is a u16 FUNCTION PARAMETER (verified micro-probe:
   `u64 f(u16 x){u64 t=0;t|=(u64)(s16)x<<26|0x1300000ULL;return t;}` -> dsll32+dsra). GetEffectTex
   has no uLog/vLog parameter. The original loads uLog 3x (lh@1f44dc / lhu@1f44f4 / lbu@1f45d0)
   and vLog 3x — the lhu descriptor load is held in a register for 8 instructions before the
   dsll32, i.e. EGC scheduled the load early and sign-extended in the shift, which this build
   does not do for a field load.

3. **Scheduling** of the independent slt/sra/movz/addiu/sllv instructions differs (e.g. orig
   [slt,sra,movz] vs cand [sra,slt,...]).

Sibling contrast: LoadPifAsPSMT8H (0x1E9168) builds the same descriptor but loads uLog/vLog
with `lw` (int) and shifts with plain `dsll 26/30` — so int-source → plain dsll, u16-source →
idiom in the ORIGINAL. This build folds the u16 (s16) into `lh`, so the idiom never fires.

## What was tried (all with production flags -G8 -O2 -ffast-math -fno-exceptions -snas)
probe3 (s16, old struct) 73d; probe4 (u64, fixed struct) 73d; probe5 (s16, fixed struct) 73d;
probe6 (term order swap) 76d; probe7 (e outside if) 81d; probe8 (tex0 before if) 77d;
probe9 (32-bit shift) 76d; probe10 (static inline helper) 76d; probe11 (bitfield + $2 acc pin)
74d. Micro-probes micro2-10 confirmed the idiom is parameter-only. Flags
-fno-schedule-insns / -fno-schedule-insns2 / -mno-split-addresses not expected to create the
bitfield/idiom RTL or fix the +1 RA (consistent with sibling DoGifPaging blocker).

## Outcome
Retain INCLUDE_ASM. Full-ELF parity preserved. Semantics + struct layout are durably recorded
above; `gifLoads`/`effectTexBase` remain unnamed (only used by this function + SetupGifPaging).
