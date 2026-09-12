typedef struct {
    unsigned char pad[0x34];
    unsigned int spriteList;
} ProbeItem;

extern "C" unsigned char probe_tagByte __attribute__((section(".data")));

int probe_func(ProbeItem* item) {
    *(unsigned short*)(item->spriteList + 2) = (probe_tagByte == 1) ? 0 : 3;
    return 0;
}
