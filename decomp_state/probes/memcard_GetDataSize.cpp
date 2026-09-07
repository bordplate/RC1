extern "C" int memcard_GetDataSize(int* data) {
    int size = 8;
    while (data[0] != 0) {
        size += 8;
        size += data[1];
        size = (size + 3) & -4;
        data += 4;
    }
    return size + 8;
}
