#include "types.h"

extern int drawOcclusionEnabled;

void enableOcclusion(void) {
    drawOcclusionEnabled = 1;
}

void disableOcclusion(void) {
    drawOcclusionEnabled = 0;
}
