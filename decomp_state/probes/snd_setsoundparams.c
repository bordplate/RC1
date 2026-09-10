extern void snd_SendIOPCommandNoWait(int cmd, int count, void* data, int x, int y);

void snd_SetSoundParams_CB(int a, int b, int c, int d, int e, int f, int g, int h) {
    int buf[6];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    snd_SendIOPCommandNoWait(0x21, 0x18, buf, g, h);
}
