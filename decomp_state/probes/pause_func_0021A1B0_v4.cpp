typedef unsigned char u8;
typedef unsigned int u32;

typedef struct {
    u8 pad[0x34];
    u32 actionList;
} PauseActionMode;

extern "C" int D_001D4810[];
extern "C" int D_001D4840[];

extern "C" int SetPauseActionList(PauseActionMode* mode) {
    if (*(int *)0x15EE90 != 0)
        mode->actionList = (u32)D_001D4810;
    else
        mode->actionList = (u32)D_001D4840;
    return 0;
}
