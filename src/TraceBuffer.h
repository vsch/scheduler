#ifdef SERIAL_DEBUG_TWI_TRACER

#ifndef ARDUINOPROJECTMODULE_DEBUG_TRACEBUFFER_H
#define ARDUINOPROJECTMODULE_DEBUG_TRACEBUFFER_H

#include "common_defs.h"
#include "CTraceBuffer.h"
#include "twiint.h"

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
        haveByte = 0;
        nextAfterStop = 0;
    }

    void trace(uint8_t traceByte) {
        if (haveByte) {
            if (this->traceByte == traceByte) {
                if (traceCount < 255) {
                    traceCount++;
                }
                return;
            } else {
                storeTrace();
            }
        }

        this->traceByte = traceByte;
        traceCount = 1;
        haveByte = 1;
    }

#ifdef DEBUG_MODE_TWI_TRACE_TIMEIT

    void traceBytes(uint8_t traceByte, void *data, uint8_t count) {
        storeTrace();

        if (nCapacity >= count + 1) {
            uint8_t *bytes = (uint8_t *) data;

            nCapacity -= count + 1;

            *pPos++ = traceByte;

            while (count--) {
                *pPos++ = *bytes++;
            }
            
            if (traceByte == TRC_STOP) {
                nextAfterStop = pPos - this->data;
            }
        } else {
            // no room, terminate current trace frame
            nCapacity = 0;
            nextAfterStop = pPos - this->data;
        }
    }

#endif // DEBUG_MODE_TWI_TRACE_TIMEIT

    void storeTrace() {
        if (haveByte) {
            if (traceCount > 1) {
                if (nCapacity > 1) {
                    *pPos++ = traceByte | TRACER_BYTE_COUNT_MARKER;
                    *pPos++ = traceCount;
                    nCapacity -= 2;
                } else {
                    nCapacity = 0;
                }
            } else if (nCapacity) {
                *pPos++ = traceByte;
                nCapacity--;
            }
            
            if (traceByte == TRC_STOP || !nCapacity) {
                nextAfterStop = pPos - this->data;
            }

            traceByte = 0;
            traceCount = 0;
            haveByte = 0;
        }
    }

    void copyFrom(TraceBuffer *pOther) {
        // TODO: copy only up to the nextAfterStop so we trace complete frames, unless the trace buffer is full
        memcpy(this, pOther, sizeof(CTwiTraceBuffer_t));

        // this is not copied correctly
        pPos = data + (pOther->pPos - pOther->data);
    }

    void startRead() {
        storeTrace();
        pPos = data;
        nReadCapacity = TWI_TRACE_SIZE - nCapacity;
    }

    void readEntry() {
        if (nReadCapacity) {
            traceByte = *pPos++;
            haveByte = 1;
            nReadCapacity--;
            if (traceByte & TRACER_BYTE_COUNT_MARKER) {
                traceByte &= ~TRACER_BYTE_COUNT_MARKER;
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
            haveByte = 0;
        }
    }

    void readBytes(void *data, uint8_t count) {
        if (nReadCapacity >= count) {
            nReadCapacity -= count;
            uint8_t *bytes = (uint8_t *) data;

            while (count--) {
                *bytes++ = *pPos++;
            }
        }
    }

    void dump();
    
    static TraceBuffer twiTraceBuffer;
    static void dumpTrace();
    
private:    
    static void dumpTrace(TraceBuffer *pBuffer);
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SERIAL_DEBUG_TWI_TRACER

extern void twi_dump_trace();

#endif



#ifdef __cplusplus
}
#endif

#endif //ARDUINOPROJECTMODULE_DEBUG_TRACEBUFFER_H

#endif //SERIAL_DEBUG_TWI_TRACER
