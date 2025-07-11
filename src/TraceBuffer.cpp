#ifdef SERIAL_DEBUG_TWI_TRACER

#include "Arduino.h"
#include "TraceBuffer.h"
#include "twiint.h"

void twi_trace(CTwiTraceBuffer_t *thizz, uint8_t traceByte) {
    ((TraceBuffer *) thizz)->trace(traceByte);
}

void twi_trace_bytes(CTwiTraceBuffer_t *thizz, uint8_t traceByte, void *data, uint8_t count) {
    ((TraceBuffer *) thizz)->traceBytes(traceByte, data, count);
}

#ifdef CONSOLE_DEBUG
#include "tests/FileTestResults_AddResult.h"
#endif

TraceBuffer TraceBuffer::twiTraceBuffer;

void TraceBuffer::dumpTrace() {
    cli();
    dumpTrace(&twiTraceBuffer);
    sei();
}

void twi_dump_trace() {
    TraceBuffer::dumpTrace();
}

// IMPORTANT: called with interrupts disabled, so it should return with interrupts disabled
void TraceBuffer::dumpTrace(TraceBuffer *pBuffer) {
    if (!(twiint_flags & TWI_FLAGS_TRC_HAD_EMPTY) || !pBuffer->isEmpty()) {
        if (pBuffer->isEmpty()) {
            twiint_flags |= TWI_FLAGS_TRC_HAD_EMPTY;
        } else {
            twiint_flags &= ~TWI_FLAGS_TRC_HAD_EMPTY;
        }

        // set trace pending and wait for TWI to be idle so we don't mess up the twi interrupt timing
        twiint_flags |= TWI_FLAGS_TRC_PENDING;

#ifndef CONSOLE_DEBUG
        serialDebugPrintf_P(PSTR("Waiting for TWI TRACER. "));
        uint32_t start = micros();
        uint32_t timeoutMic = TWI_WAIT_TIMEOUT * 1000L;

        sei();
        while (twiint_busy()) {
            uint32_t diff = micros() - start;
            if (diff >= timeoutMic) {
                serialDebugPrintf_P(PSTR("timed out %dms. "), TWI_WAIT_TIMEOUT);
                break;
            }
        }
        cli();
        serialDebugPrintf_P(PSTR("done.\n"));
#endif

        TraceBuffer traceBuffer;

        // make a copy and clear the trace queue
        traceBuffer.copyFrom(pBuffer);
        pBuffer->reset();

        twiint_flags &= ~TWI_FLAGS_TRC_PENDING;

        // enable interrupts so twi processing can proceed
        sei();

        traceBuffer.dump();

        cli();
    }
}

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
