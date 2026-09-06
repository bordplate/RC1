extern "C" void scTag2(unsigned long* tag, unsigned int address,
                       unsigned int id, unsigned int count) {
    *tag = ((unsigned long)address << 32) |
           (((unsigned long)id << 32) >> 4) | (unsigned long)count;
}
