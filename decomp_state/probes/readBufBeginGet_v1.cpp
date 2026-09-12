typedef struct ReadBuf {
    unsigned char data[0x50000];
    int putPos;
    int count;
    int capacity;
} ReadBuf;

int readBufBeginGet(ReadBuf* buf, unsigned char** out) {
    if (buf->count) {
        *out = buf->data + ((buf->putPos - buf->count) + buf->capacity) % buf->capacity;
    }
    return buf->count;
}
