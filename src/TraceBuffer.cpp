#ifdef SERIAL_DEBUG_TWI_TRACER

#include "Arduino.h"
#include "TraceBuffer.h"
#include "twiint.h"
#include "CTraceBuffer.h"

#ifdef SERIAL_DEBUG_TWI_RAW_TRACER_WORD

void twi_trace(CTwiTraceBuffer_t *thizz, uint16_t data) {
    ((TraceBuffer *) thizz)->trace(data);
}

#else

void twi_trace(CTwiTraceBuffer_t *thizz, uint8_t traceByte) {
    ((TraceBuffer *) thizz)->trace(traceByte);
}

void twi_trace_bytes(CTwiTraceBuffer_t *thizz, uint8_t traceByte, void *data, uint8_t count) {
    ((TraceBuffer *) thizz)->traceBytes(traceByte, data, count);
}

#endif  //SERIAL_DEBUG_TWI_RAW_TRACER_WORD

#ifdef CONSOLE_DEBUG
#include "tests/FileTestResults_AddResult.h"
#endif

void TraceBuffer::dump() {
    startRead();

    serialDebugPrintf_P(PSTR("TWI Trc: %d {"), getReadCapacity());
#if 0
    while (nReadCapacity--) {
        serialDebugPrintf_P(PSTR("  0x%2.2x"), *pPos++);
    }
#else
    while (!isAllRead()) {
#ifdef SERIAL_DEBUG_TWI_RAW_TRACER_WORD
        uint16_t entry = readEntry();
        if (entry & ~TW_STATUS_MASK) {
            // have other bits
            serialDebugPrintf_P(PSTR("  0x%4.4x(0x%4.4x)"), entry & TW_STATUS_MASK, entry);
        } else {
            serialDebugPrintf_P(PSTR("  0x%4.4x"), entry & TW_STATUS_MASK);
        }
#else
        readEntry();

        uint8_t trc = getTraceByte();
        uint8_t count = getTraceCount();

#ifdef SERIAL_DEBUG_TWI_RAW_TRACER
        if (count > 1) {
            serialDebugPrintf_P(PSTR(" 0x%2.2x(%d)"), trc, count);
        } else {
            serialDebugPrintf_P(PSTR("  0x%2.2x"), trc);
        }
#else
        if (trc >= TRC_MAX) {
            // out of bounds
            if (count > 1) {
                serialDebugPrintf_P(PSTR("  0x%2.2x(%d)"), trc, count);
            } else {
                serialDebugPrintf_P(PSTR("  0x%2.2x"), trc);
            }
        } else {
#ifdef CONSOLE_DEBUG
            const char *pStr = (const char *)(trcStrings[trc]);
            if (count > 1) {
                serialDebugPrintf_P(PSTR("  %s(%d)"), pStr, count);
            } else {
                serialDebugPrintf_P(PSTR("  %s"), pStr);
            }
        }
#else
            const char *pStr = (const char *) pgm_read_ptr(trcStrings + trc);

#ifdef DEBUG_MODE_TWI_TRACE_TIMEIT
            if (trc == TRC_STOP) {
                uint16_t elapsedMicros;
                readBytes(&elapsedMicros, sizeof(elapsedMicros));
                serialDebugPrintf_P(PSTR("  %S(%dus)"), pStr, elapsedMicros);
            } else 
#endif

            if (count > 1) {
                serialDebugPrintf_P(PSTR("  %S(%d)"), pStr, count);
            } else {
                serialDebugPrintf_P(PSTR("  %S"), pStr);
            }
        }
#endif
#endif
#endif
    }
#endif
    serialDebugPrintf_P(PSTR(" }\n"));
}

#endif //SERIAL_DEBUG_TWI_TRACER
