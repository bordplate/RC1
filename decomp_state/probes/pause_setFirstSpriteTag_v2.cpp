typedef struct {
    unsigned char pad[0x34];
    unsigned int spriteList;
} ProbeItem;

int probe_func(ProbeItem* item) {
    unsigned short* p = (unsigned short*)(item->spriteList + 2);
    *p = (*(unsigned char*)0x1413F4 == 1) ? 0 : 3;
    return 0;
}
