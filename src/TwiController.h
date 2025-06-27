
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
        pStream->serialDebugDump(getReadStreamId(pStream));
#endif
        this->resume(0);
        
        twiint_start((CByteStream_t *) pStream);
    }
};

extern TwiController twiController;

#endif //SCHEDULER_TWICONTROLLER_H
