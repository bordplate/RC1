#include "common.h"

INCLUDE_ASM("code/_generated/nonmatchings/game/boot", ParseBin);

typedef void (*StartLevelPtr)(void);

extern "C" void FlushCache(int arg1);
extern "C" void startLevel();
extern "C" StartLevelPtr ParseBin();

int main(int argc, char **argv) {
    StartLevelPtr pcVar1 = startLevel;  // assign first function

    while (true) {
        pcVar1();             // call current function
        pcVar1 = ParseBin();  // assign next function
        FlushCache(0);
        FlushCache(2);
    }
}
