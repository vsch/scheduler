#include "ResourceUse.h"

ResourceUse::ResourceUse(const char *name) {
    id = name;
    reset();
}

uint8_t ResourceUse::canDump(uint32_t *pLastDump, uint16_t delayMs) {
    if (!pLastDump || !delayMs) return 1;

    uint32_t mic = micros();
    if (*pLastDump + delayMs * 1000L <= mic) {
        *pLastDump = mic;
        return 1;
    }
    return 0;
}

void ResourceUse::addValue(uint8_t usedStreams, uint8_t usedBufferSize) {
    if (usedStreams) {
        if (minUsedStreams > usedStreams) {
            minUsedStreams = usedStreams;
        }
        if (maxUsedStreams < usedStreams) {
            maxUsedStreams = usedStreams;
        }
    }

    if (usedBufferSize) {
        if (minUsedBufferSize > usedBufferSize) {
            minUsedBufferSize = usedBufferSize;
        }
        if (maxUsedBufferSize < usedBufferSize) {
            maxUsedBufferSize = usedBufferSize;
        }
    }
}

