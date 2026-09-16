extern int textureCursor;
extern int textureMemoryBase;
extern int textureResetFlag;

void texResetCursor(void) {
    textureCursor = textureMemoryBase;
    textureResetFlag = 0;
}
