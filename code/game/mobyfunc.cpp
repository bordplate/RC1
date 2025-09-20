#include "common.h"

#include "mobyfunc.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", SpawnMoby__Fii);
#ifdef SKIP_ASM
extern MobyInstance* MobyInstanceEnd;
extern MobyInstance* MobyInstancePermEnd;
extern s32 MobyVars;
extern s32 numSpawnableMobys;
extern s32 worldUpdateTime;

extern const char* s_tried_to_create_moby;

void InitMobyInstance(MobyInstance* mobyInstance, int oClass);
extern "C" void STUB_printf(const char* fmt, ...);

/**
 * Doesn't at all match the original assembly.
 * https://decomp.me/scratch/jF3YE
 */
MobyInstance* SpawnMoby(int oClass) {
    s8 sState;
    void* pVar;

    MobyInstance* end = MobyInstancePermEnd;
    MobyInstance* pMoby = MobyInstanceEnd;
    if (pMoby < end) {
        sState = pMoby->state;

        for (;;) {
            if ((0xfd < (u8)sState) && pMoby->unk1 <= (long)worldUpdateTime) {
                if (sState == -1) {
                    pMoby[1].state = -1;
                }

                InitMobyInstance(pMoby, oClass);

                pVar = (void*)(MobyVars + ((int)pMoby - (int)MobyInstanceEnd >> 8) * 0x80);
                pMoby->pVar = pVar;
                FastMemSet(pVar, 0, 0x80);

                if (numSpawnableMobys == 0) {
                    return pMoby;
                }

                numSpawnableMobys -= 1;
                return pMoby;
            }

            if (end <= pMoby + 1) break;
            sState = pMoby[1].state;
            pMoby += 1;
        }
    }

    STUB_printf(s_tried_to_create_moby, worldUpdateTime);

    return nullptr;
}
#endif

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", InitMobyInstance__FP12MobyInstanceib);
