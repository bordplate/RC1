extern "C" int menu_pointIsClockwise(int a, int b, int c, int d, int e, int f) {
    int x = a - c;
    int y = b - d;
    int r = (e - c) * y - (f - d) * x;
    if (r < 0)
        return 1;
    return 0;
}
