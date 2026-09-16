extern int textureCursor;

void texResetCursor(void) {
    textureCursor = *(int*)0x15EE8C;
    *(int*)0x15EF20 = 0;
}
