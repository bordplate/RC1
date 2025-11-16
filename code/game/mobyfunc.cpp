#include "common.h"

#include "mobyfunc.h"

#if !defined(SKIP_ASM) && !defined(ALLOW_NONMATCHING)
INCLUDE_ASM("code/_generated/matchings/game/mobyfunc", CreateMoby__Fi);
#endif

#if defined(SKIP_ASM) || defined(ALLOW_NONMATCHING)
extern MobyInstance* MobyInstanceEnd __attribute__((section(".bss")));
extern MobyInstance* MobyInstancePermEnd __attribute__((section(".bss")));

//extern MobyInstance* MobyInstancePermEnd;
extern s32 MobyVars __attribute__((section(".bss")));
extern s32 numSpawnableMobys __attribute__((section(".bss")));
extern s32 worldUpdateTime __attribute__((section(".bss")));  // worldUpdateTime

extern const char s_tried_to_create_moby __attribute__((section(".rdata")));

void InitMobyInstance(MobyInstance* mobyInstance, int oClass);
extern "C" void STUB_printf(const char* fmt, ...);

/**
 * Doesn't at all match the original assembly.
 * https://decomp.me/scratch/jF3YE
 */
MobyInstance* CreateMoby(s32 oClass) {
    u8 var_7;
    MobyInstance* var_16;
    u8 var_4;
    void* temp_3;

    var_16 = MobyInstanceEnd;
    if ((u32)var_16 < (u32)MobyInstancePermEnd) {
        var_7 = 0xFF;
        var_4 = var_16->state;
        loop_2:
                if (var_4 < 0xFEU) {
                    var_16 += 0x100;
                    goto block_13;
                }
        if ((u32) worldUpdateTime < (u64) var_16->unk1) {
            var_16 += 0x100;
            block_13:
                        if ((u32)var_16 < (u32) MobyInstancePermEnd) {
                            var_4 = var_16->state;
                            goto loop_2;
                        }
            goto block_16;
        }
        if (var_4 == 0xFF) {
            var_16[1].state = var_4;
        }
        InitMobyInstance(var_16, oClass);
        temp_3 = (void*)(MobyVars + (((s32) (var_16 - (u32)MobyInstanceEnd) >> 8) << 7));
        var_16->pVar = temp_3;
        FastMemSet(temp_3, 0, 0x80);
        if (numSpawnableMobys != 0) {
            numSpawnableMobys -= 1;
        }
        return var_16;
    }
    block_16:
        //STUB_printf(&s_tried_to_create_moby, worldUpdateTime, oClass);

#ifndef SKIP_ASM
    asm("nop");
    asm("nop");
    asm("nop");
#endif


    return nullptr;
}

#endif

INCLUDE_ASM("code/_generated/nonmatchings/game/mobyfunc", InitMobyInstance__FP12MobyInstancei);
