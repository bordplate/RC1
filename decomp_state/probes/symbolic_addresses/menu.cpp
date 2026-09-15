// Isolated experiment: intentionally ordinary small-data scalar declarations.
extern int menuPostFlags;
extern int menuPostCallbackIndex;

void menu_post_openInventory() {
    if (!(menuPostFlags & 0x40)) {
        menuPostCallbackIndex = 3;
    }
}
