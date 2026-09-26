#include "types.h"

typedef struct {
    float posX;
    float posY;
    float posZ;
    float pad_0C;
    float scale;
    int data;
    int texId;
    float angle;
    int state;
    s16 field_24;
    s16 count;
    float angleStep;
    int type;
} EffectSprite;

u64 GetEffectTex1(int texId) asm("GetEffectTex__Fii");
float FastSin(float angle);
extern "C" float FastCos(float angle);
float FastAddRots(float angle, float step);

extern "C" void func_001F5AB0(
    float x,
    float y,
    float width,
    float height,
    float angle,
    float z1,
    float z2,
    int c0,
    int c1,
    u64 texture,
    int colour,
    int data,
    int flag0,
    int flag1);

void func_001EE008(float x, float y, EffectSprite* spr)
{
    float scaleConstant = 40.0f;
    u64 texture = GetEffectTex1(spr->texId);
    int type = spr->type;
    float angle = spr->angle;

    switch (type) {
    case 0:
        if (spr->count > 0) {
            int i = 0;
            do {
                func_001F5AB0(
                    x, y,
                    scaleConstant * spr->scale,
                    scaleConstant * spr->scale,
                    angle, 0.0f, 0.0f,
                    0x3f, 0x3f, texture, 0xfffff3,
                    spr->data, 0, 0);

                i++;
                angle = FastAddRots(angle, spr->angleStep);
            } while (i < spr->count);
        }
        break;

    case 1:
        {
            float offsetA[4];
            float offsetB[4];

            offsetA[0] =
                FastSin(angle) * scaleConstant * spr->scale;
            offsetA[1] =
                FastCos(angle) * scaleConstant * spr->scale;
            offsetB[0] =
                FastCos(angle) * scaleConstant * spr->scale;
            offsetB[1] =
                FastSin(angle) * -scaleConstant * spr->scale;

            func_001F5AB0(
                x, y,
                scaleConstant * spr->scale,
                scaleConstant * spr->scale,
                angle, 0.0f, 0.0f,
                0x3f, 0x3f, texture, 0xfffff3,
                spr->data, 0, 0);

            func_001F5AB0(
                x + offsetB[0], y + offsetB[1],
                scaleConstant * spr->scale,
                scaleConstant * spr->scale,
                angle, 0.0f, 0.0f,
                0x3f, 0x3f, texture, 0xfffff3,
                spr->data, 1, 0);

            func_001F5AB0(
                x - offsetA[0], y - offsetA[1],
                scaleConstant * spr->scale,
                scaleConstant * spr->scale,
                angle, 0.0f, 0.0f,
                0x3f, 0x3f, texture, 0xfffff3,
                spr->data, 0, 1);

            func_001F5AB0(
                x + offsetB[0] - offsetA[0],
                y + offsetB[1] - offsetA[1],
                scaleConstant * spr->scale,
                scaleConstant * spr->scale,
                angle, 0.0f, 0.0f,
                0x3f, 0x3f, texture, 0xfffff3,
                spr->data, 1, 1);
        }
        break;

    case 2:
        func_001F5AB0(
            x, y,
            scaleConstant * spr->scale,
            scaleConstant * spr->scale,
            angle, 0.5f, 0.5f,
            0x3f, 0x3f, texture, 0xfffff3,
            spr->data, 0, 0);
        break;
    }
}
