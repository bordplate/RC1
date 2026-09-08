extern "C" int memcard_Checksum(int* data, int len);

extern "C" int memcard_TestChecksum(int* data) {
    int c = data[1];
    int b = 0;
    int len = data[0];
    if (c != 0)
        b = memcard_Checksum(data + 2, len) == c;
    return b;
}
