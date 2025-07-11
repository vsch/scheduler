
#ifndef SCHEDULER_TWICONTROLLER_H
#define SCHEDULER_TWICONTROLLER_H

#include "Controller.h"
#include "twiint.h"

class TwiController : public Controller {

public:
    TwiController(uint8_t *pData, uint8_t maxStreams, uint8_t maxTasks, uint8_t writeBufferSize, uint8_t flags = CTR_FLAGS_REQ_AUTO_START)
            : Controller(pData, maxStreams, maxTasks, writeBufferSize, flags) {
    }

    defineSchedulerTaskId("TwiController");

    void startProcessingRequest(ByteStream *pStream) override {
        // output stream content and call endProcessingRequest
#ifndef CONSOLE_DEBUG

#ifdef SERIAL_DEBUG_TWI_DATA
        pStream->serialDebugDump(getReadStreamId(pStream));
#endif

#endif
        this->resume(0);
        
#ifndef CONSOLE_DEBUG
        twiint_start((CByteStream_t *) pStream);
#endif        
        
    }
};

extern TwiController twiController;

#endif //SCHEDULER_TWICONTROLLER_H
