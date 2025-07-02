
#ifndef SCHEDULER_TWICONTROLLER_H
#define SCHEDULER_TWICONTROLLER_H

#include "Controller.h"
#include "twiint.h"

#ifdef SERIAL_DEBUG_TWI_TRACER
extern ByteQueue twiTraceQ;
#endif

class TwiController : public Controller {

public:
    TwiController(uint8_t *pData, uint8_t maxStreams, uint8_t maxTasks, uint8_t writeBufferSize, uint8_t flags = CTR_FLAGS_REQ_AUTO_START)
            : Controller(pData, maxStreams, maxTasks, writeBufferSize, flags) {
    }

    defineSchedulerTaskId("TwiController");

    // IMPORTANT: called with interrupts disabled
    void requestCompleted(ByteStream *pStream) override {
#ifndef CONSOLE_DEBUG
#ifdef SERIAL_DEBUG_TWI_TRACER

        serialDebugPrintf_P(PSTR("TWI Completed: %d\n"), getReadStreamId(pStream));
        if (!twiTraceQ.isEmpty()) {
            serialDebugPrintf_P(PSTR("TWI Trc: %d {\n"), twiTraceQ.getCount());
            while (!twiTraceQ.isEmpty()) {
                uint8_t trc = twiTraceQ.removeHead();
                uint8_t count = 1;
                if (trc & 0x40) {
                    count = twiTraceQ.removeHead() & 0x7f;
                    trc &= 0x3f;
                }

                if (trc >= TRC_MAX) {
                    // out of bounds
                    if (count > 1) {
                        serialDebugPrintf_P(PSTR("  0x%2.2x(%d)"), trc, count);
                    } else {
                        serialDebugPrintf_P(PSTR("  0x%2.2x"), trc);
                    }
                } else {
                    const char *pStr = (const char *) pgm_read_word(trcStrings + trc);
                    if (count > 1) {
                        serialDebugPrintf_P(PSTR("  %S(%d)"), pStr, count);
                    } else {
                        serialDebugPrintf_P(PSTR("  %S"), pStr);
                    }
                }
            }
            serialDebugPrintf_P(PSTR(" }\n"));
        }
#endif
#endif
    }

    void startProcessingRequest(ByteStream *pStream) override {
        // output stream content and call endProcessingRequest
#ifndef CONSOLE_DEBUG

#ifdef SERIAL_DEBUG_TWI_DATA
        pStream->serialDebugDump(getReadStreamId(pStream));
#endif

#endif
        this->resume(0);

        twiint_start((CByteStream_t *) pStream);
    }
};

extern TwiController twiController;

#endif //SCHEDULER_TWICONTROLLER_H
