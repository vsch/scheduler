
#ifndef ARDUINOPROJECTMODULE_DEBUG_RESOURCEUSE_H
#define ARDUINOPROJECTMODULE_DEBUG_RESOURCEUSE_H

#include <Arduino.h>
#include <stdint.h>

#ifndef RESOURCE_TRACE_INTERVAL_MS
RESOURCE_TRACE_INTERVAL_MS  (2000L)
#endif

struct ResourceUse {
    const char *id;
    uint8_t minUsedStreams;
    uint8_t minUsedBufferSize;
    uint8_t maxUsedStreams;
    uint8_t maxUsedBufferSize;

    explicit ResourceUse(PGM_P name);

    inline void reset() {
        minUsedStreams = 0xff;
        minUsedBufferSize = 0xff;
        maxUsedStreams = 0;
        maxUsedBufferSize = 0;
    }

    void addValue(uint8_t usedStreams, uint8_t usedBufferSize);

    uint8_t canDump(uint32_t *pLastDump, uint16_t delayMs);
};

#endif //ARDUINOPROJECTMODULE_DEBUG_RESOURCEUSE_H
