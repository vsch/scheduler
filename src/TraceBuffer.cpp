#include "Arduino.h"
#include "TraceBuffer.h"
#include "twiint.h"

void twi_trace(CTwiTraceBuffer_t *thizz, uint8_t data) {
    ((TraceBuffer *) thizz)->trace(data);
}

const char sSTR_NONE[] PROGMEM = "";
const char sSTR_START[] PROGMEM = STR_TRC_START;
const char sSTR_REP_START[] PROGMEM = STR_TRC_REP_START;
const char sSTR_MT_SLA_ACK[] PROGMEM = STR_TRC_MT_SLA_ACK;
const char sSTR_MT_DATA_ACK[] PROGMEM = STR_TRC_MT_DATA_ACK;
const char sSTR_MR_DATA_ACK[] PROGMEM = STR_TRC_MR_DATA_ACK;
const char sSTR_MR_SLA_ACK[] PROGMEM = STR_TRC_MR_SLA_ACK;
const char sSTR_MR_DATA_NACK[] PROGMEM = STR_TRC_MR_DATA_NACK;
const char sSTR_MT_ARB_LOST[] PROGMEM = STR_TRC_MT_ARB_LOST;
const char sSTR_MT_SLA_NACK[] PROGMEM = STR_TRC_MT_SLA_NACK;
const char sSTR_MT_DATA_NACK[] PROGMEM = STR_TRC_MT_DATA_NACK;
const char sSTR_MR_SLA_NACK[] PROGMEM = STR_TRC_MR_SLA_NACK;

PGM_P const trcStrings[] PROGMEM = {
        sSTR_START,
        sSTR_START,
        sSTR_REP_START,
        sSTR_MT_SLA_ACK,
        sSTR_MT_DATA_ACK,
        sSTR_MR_DATA_ACK,
        sSTR_MR_SLA_ACK,
        sSTR_MR_DATA_NACK,
        sSTR_MT_ARB_LOST,
        sSTR_MT_SLA_NACK,
        sSTR_MT_DATA_NACK,
        sSTR_MR_SLA_NACK,
};

#ifdef CONSOLE_DEBUG
#include "tests/FileTestResults_AddResult.h"
#endif

void TraceBuffer::dump() {
    startRead();
    
    serialDebugPrintf_P(PSTR("TWI Trc: %d {"), getCount());
    while (!isAllRead()) {
        // make a copy of the queue
        readEntry();

        uint8_t trc = getTraceByte();
        uint8_t count = getTraceCount();

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
            const char *pStr = (const char *)pgm_read_ptr(trcStrings + trc);
            if (count > 1) {
                serialDebugPrintf_P(PSTR("  %S(%d)"), pStr, count);
            } else {
                serialDebugPrintf_P(PSTR("  %S"), pStr);
            }
        }
#endif        
    }
    serialDebugPrintf_P(PSTR(" }\n"));

}
