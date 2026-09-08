extern void snd_FlushSoundCommands(void);
extern int* snd_batchCommandBuffers[2];
extern int snd_batchIndex;

void snd_PostMessage(void) {
    int* p = snd_batchCommandBuffers[snd_batchIndex];
    int v = *p + 1;
    snd_FlushSoundCommands();
    *p = v;
}
