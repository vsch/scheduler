#ifdef SERIAL_DEBUG_TWI_TRACER

#include "Arduino.h"
#include "TraceBuffer.h"
#include "twiint.h"
#include "CTraceBuffer.h"

void twi_trace(CTwiTraceBuffer_t *thizz, uint8_t traceByte) {
    ((TraceBuffer *) thizz)->trace(traceByte);
}

void twi_trace_bytes(CTwiTraceBuffer_t *thizz, uint8_t traceByte, void *data, uint8_t count) {
    ((TraceBuffer *) thizz)->traceBytes(traceByte, data, count);
}

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
            } else {
                if (count > 1) {
                    serialDebugPrintf_P(PSTR("  %S(%d)"), pStr, count);
                } else {
                    serialDebugPrintf_P(PSTR("  %S"), pStr);
                }
            }
#else
            if (count > 1) {
                serialDebugPrintf_P(PSTR("  %S(%d)"), pStr, count);
            } else {
                serialDebugPrintf_P(PSTR("  %S"), pStr);
            }
#endif // DEBUG_MODE_TWI_TRACE_TIMEIT
        }

#endif //CONSOLE_DEBUG
#endif //SERIAL_DEBUG_TWI_RAW_TRACER
    }
#endif
    serialDebugPrintf_P(PSTR(" }\n"));
}

#endif //SERIAL_DEBUG_TWI_TRACER
