extern int textureCursor;
extern int textureMemoryBase;
extern int textureAllocCounter;

void texResetCursor(void) {
    textureCursor = textureMemoryBase;
    textureAllocCounter = 0;
}
