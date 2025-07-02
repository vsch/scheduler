
#ifndef ARDUINOPROJECTMODULE_DEBUG_TRACEBUFFER_H
#define ARDUINOPROJECTMODULE_DEBUG_TRACEBUFFER_H

#include "common_defs.h"
#include "CTraceBuffer.h"

struct TraceBuffer : public CTraceBuffer {

public:
    TraceBuffer() : CTraceBuffer() {
        reset();
    }

    NO_DISCARD inline uint8_t getCapacity() const {
        return nCapacity;
    }

    NO_DISCARD inline uint8_t getCount() const {
        return TWI_TRACE_SIZE - nCapacity;
    }

    NO_DISCARD inline uint8_t isEmpty() const {
        return nCapacity == TWI_TRACE_SIZE;
    }

    NO_DISCARD inline uint8_t isFull() const {
        return !nCapacity;
    }

    NO_DISCARD inline uint8_t isAllRead() const {
        return !nReadCapacity;
    }

    NO_DISCARD inline uint8_t getTraceByte() const {
        return traceByte;
    }

    NO_DISCARD inline uint8_t getTraceCount() const {
        return traceCount;
    }

    NO_DISCARD inline uint8_t getReadCapacity() const {
        return nReadCapacity;
    }

    void reset() {
        pPos = data;
        nCapacity = TWI_TRACE_SIZE;
        nReadCapacity = 0;
        traceByte = 0;
        traceCount = 0;
    }

    void trace(uint8_t byte) {
        if (traceByte == byte) {
            if (traceCount < 255) {
                traceCount++;
            }
        } else {
            storeTrace();
            traceByte = byte;
            traceCount = 1;
        }
    }

    void storeTrace() {
        if (traceByte) {
            if (traceCount) {
                if (nCapacity > 1) {
                    *pPos++ = traceByte | 0x80;
                    *pPos++ = traceCount;
                    nCapacity -= 2;
                }
            } else if (nCapacity) {
                *pPos++ = traceByte;
                nCapacity--;
            }
            traceByte = 0;
            traceCount = 0;
        }
    }

    void copyFrom(TraceBuffer *pOther) {
        memcpy(this, pOther, sizeof(TraceBuffer));
    }

    void startRead() {
        storeTrace();
        pPos = data;
        nReadCapacity = TWI_TRACE_SIZE - nCapacity;
    }

    void readEntry() {
        if (nReadCapacity) {
            traceByte = *pPos++;
            nReadCapacity--;
            if (traceByte & 0x80) {
                traceByte &= ~0x80;
                if (nReadCapacity) {
                    traceCount = *pPos++;
                    nReadCapacity--;
                } else {
                    traceCount = 1;
                }
            } else {
                traceCount = 1;
            }
        } else {
            traceByte = 0;
            traceCount = 0;
        }
    }
    
    void dump();
};

#endif //ARDUINOPROJECTMODULE_DEBUG_TRACEBUFFER_H
