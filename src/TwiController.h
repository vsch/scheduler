
#ifndef SSD1306_TEST_TWICONTROLLER_H
#define SSD1306_TEST_TWICONTROLLER_H
#include "Controller.h"

class TwiController : public Controller {

public:
    TwiController(uint8_t *pData, uint8_t maxStreams, uint8_t maxTasks, uint8_t writeBufferSize, uint8_t flags = CTR_FLAGS_REQ_AUTO_START)
            : Controller(pData, maxStreams, maxTasks, writeBufferSize, flags) {

    }

    defineSchedulerTaskId("TwiController");

    void startProcessingRequest(ByteStream *pStream) override {
        // output stream content and call endProcessingRequest
        addActualOutput("processed TWI Request {");

        while (!pStream->is_empty()) {
            uint8_t byte = pStream->get();
            addActualOutput(" 0x%2.2x", byte);
        }

        addActualOutput(" }\n");

        endProcessingRequest(pStream);
    }

    void processNextRequest() {
        if (!pendingReadStreams.isEmpty()) {
            const uint8_t head = pendingReadStreams.peekHead();
            ByteStream *pStream = getReadStream(head);
            pStream->flags |= STREAM_FLAGS_PENDING;
            startProcessingRequest(pStream);
            handleCompletedRequests();
        }
    }

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact) override;

#endif
};

#ifdef CONSOLE_DEBUG

// print out queue for testing
void TwiController::dump(uint8_t indent, uint8_t compact) {
    //    Queue pendingReadStreams;           // requests waiting to be handleProcessedRequest
    //    Queue freeReadStreams;              // requests for processing available
    //    ByteStream *readStreams;            // pointer to first element in array of ByteSteam entries
    //    Queue writeBuffer;                  // shared write byte buffer
    //    ByteStream writeStream;             // write stream, must be requested and released in the same task invocation or pending data will not be handleProcessedRequest
    //    Mutex reservationLock;              // reservationLock for requests and buffer writes
    //    Queue requirementList;                 // byte queue of requirementList: max 8 reservationLock and 31*8 buffer, b7:b5+1 is reservationLock, B4:b0*8 = 248 bytes see Note below.

    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';


    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    addActualOutput("%sTwiController {\n%scompletedStreams\n", indentStr, indentStr);
    completedStreams.dump(indent + 2, compact);
    Controller::dump(indent, compact);
}

#endif // CONSOLE_DEBUG



#endif //SSD1306_TEST_TWICONTROLLER_H
