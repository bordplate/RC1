extern void snd_PrepareReturnBuffer(int*, int);
extern void FlushCache(int);
extern int D_00153D20 __attribute__((section(".data")));
extern int D_0015EBC0 __attribute__((section(".data")));
extern int* snd_batchCommandBuffers[2];
extern int snd_batchFreeBytes[2];
extern int* snd_batchReturnBuffers[2];
extern int snd_batchIndex;
extern int sceSifCheckStatRpc(void*);
extern void func_00116078(void*);
#ifndef RPC_RETURN
#define RPC_RETURN int
#endif
extern RPC_RETURN sceSifCallRpc(void*, int, int, void*, int, void*, int, void (*)(void*), void*);

void snd_SendCurrentBatch(void) {
    int next;
    snd_PrepareReturnBuffer(snd_batchReturnBuffers[snd_batchIndex],
                           *snd_batchCommandBuffers[snd_batchIndex]);
    while (sceSifCheckStatRpc(&D_0015EBC0)) {
        func_00116078(&D_00153D20);
        FlushCache(0);
    }
    sceSifCallRpc(&D_0015EBC0, 0x4D, 1, snd_batchCommandBuffers[snd_batchIndex],
                  0x1000 - snd_batchFreeBytes[snd_batchIndex],
                  snd_batchReturnBuffers[snd_batchIndex],
                  *snd_batchCommandBuffers[snd_batchIndex] * 4 + 8, 0, 0);
    next = snd_batchIndex != 1;
    snd_batchIndex = next;
    *snd_batchCommandBuffers[next] = 0;
    snd_batchFreeBytes[next] = 0xFFC;
}
