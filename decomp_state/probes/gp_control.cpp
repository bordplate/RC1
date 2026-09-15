extern "C" unsigned int menuSelectionCount;
extern "C" int gp_control(void) {
    return menuSelectionCount < 1;
}
