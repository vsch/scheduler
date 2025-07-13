#include "Scheduler.h"
#include "Controller.h"
#include "twiint.h"

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"
#include "tests/FileTestResults.h"
// #else
// #define addActualOutput(fmt, ...)  ((void)0)

// print out queue for testing
void Controller::dump(uint8_t indent, uint8_t compact) {
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
    addActualOutput("%sController { maxStreams:%d, maxTasks:%d, writeBufferSize:%d\n", indentStr, maxStreams, maxTasks, writeBufferSize);
    addActualOutput("%s  lastFreeHead = %d, requestCapacity() = %d byteCapacity() = %d\n", indentStr, lastFreeHead, requestCapacity(), byteCapacity());
    addActualOutput("%s  pendingReadStreams ", indentStr);

    this->pendingReadStreams.dump(indent + 2, 1);

    if (!compact || compact == 2) {
        addActualOutput("%s  freeReadStreams ", indentStr);
        this->freeReadStreams.dump(indent + 2, 1);

    }

    if (!this->pendingReadStreams.isEmpty()) {
        // output individual pending streams
        int iMax = pendingReadStreams.getCount();
        for (int i = 0; i < iMax; i++) {
            const uint8_t head = pendingReadStreams.peekHead(i);
            addActualOutput("%s  readStream[%d]:", indentStr, head);
            ByteStream *pStream = getReadStream(head);
            pStream->dump(indent + 2, 1);
        }
    }

    if (!compact) {
        addActualOutput("%s  readStreamTable \n%s", indentStr, indentStr);
        for (uint8_t i = 0; i < maxStreams; i++) {
            addActualOutput("%s  [%i]", indentStr, i);
            this->readStreamTable[i].dump(indent + 2, 1);
        }
    }

    addActualOutput("%s  writeBuffer ", indentStr);
    this->writeBuffer.dump(indent + 2, compact);

    addActualOutput("%s  writeStream ", indentStr);
    this->writeStream.dump(indent + 2, compact);

    addActualOutput("%s  reservationLock ", indentStr);
    this->reservationLock.dump(indent + 2, compact);

    addActualOutput("%s  requirementList ", indentStr);
    this->requirementList.dump(indent + 2, compact);

    addActualOutput("\n%s}\n", indentStr);
}

#endif // CONSOLE_DEBUG

#ifdef RESOURCE_TRACE

void Controller::dumpResourceTrace(PGM_P id) {
    // TODO: add trace code
    resourceTracePrintf_P(PSTR("%S::resources streams:%d, bytes:%d\n"), id ? id : PSTR("Ctrl"), usedStreams, usedBufferSize);
}

#endif // RESOURCE_TRACE

uint8_t Controller::willRequire(uint8_t requests, uint8_t bytes) {
    // serialDebugResourceTracePrintf_P(PSTR("Ctrl::require summary: a1:%d - r1:%d, a2:%d - r2:%d\n")
    //                                  , reservationLock.getAvailable1()
    //                                  , freeReadStreams.getCount()
    //                                  , reservationLock.getAvailable2()
    //                                  , writeBuffer.getCapacity());
    
    cli();
    uint8_t reserved = reservationLock.reserve(requests, bytes);
    sei();

    if (reserved == NULL_BYTE) {
        // cannot ever satisfy these requirements
        serialDebugResourceTracePrintf_P(PSTR("Ctrl:: never satisfied: R %d > maxR %d || B > maxB %d\n")
                                         , requests, reservationLock.getMaxAvailable1()
                                         , bytes, reservationLock.getMaxAvailable2());
    } else if (reserved) {
        serialDebugResourceTracePrintf_P(PSTR("Ctrl:: suspend lock %d, %d avail %d %d \n"), requests, bytes, reservationLock.getAvailable1(), reservationLock.getAvailable2());
    } else {
        // serialDebugResourceTracePrintf_P(PSTR("Ctrl:: reserved lock %d, %d\n"), requests, bytes);
    }

    return reserved;
}

void Controller::begin() {
    
}

void Controller::loop() {
    cli();
    handleCompletedRequests();

#ifdef SERIAL_DEBUG_TWI_TRACER
    TraceBuffer::dumpTrace();
#endif

    startNextRequest();
    sei();

    resume(1);
}

// IMPORTANT: called from interrupt code
void Controller::endProcessingRequest(ByteStream *pStream) {
    pStream->flags &= ~(STREAM_FLAGS_PENDING | STREAM_FLAGS_PROCESSING);

    // make sure it is a shared request stream
    uint8_t id = getReadStreamId(pStream);
    if (id < maxStreams) {
        uint8_t availBytes = 0;

        if (pStream->pData == writeBuffer.pData) {
            // at this point the buffer used by this request is no longer needed, so the buffer head can be moved to processed request tail.
            availBytes = writeBuffer.getCapacity();
            writeBuffer.nHead = pStream->nTail;
            
            // serialDebugResourceTracePrintf_P(PSTR("Ctrl::end proc %d: before:%d, cap after:%d, diff:%d\n")
            //                                  , id
            //                                  , availBytes
            //                                  , writeBuffer.getCapacity()
            //                                  , writeBuffer.getCapacity() - availBytes);
            
            availBytes = writeBuffer.getCapacity() - availBytes;
        }

        uint8_t head = pendingReadStreams.removeHead();
        completedStreams.addTail(head);
        reservationLock.makeAvailable(0, availBytes);
        
        // serialDebugResourceTracePrintf_P(PSTR("Ctrl::end summary %d: a1:%d - r1:%d, a2:%d - r2:%d\n")
        //                                  , id
        //                                  , reservationLock.getAvailable1()
        //                                  , freeReadStreams.getCount()
        //                                  , reservationLock.getAvailable2()
        //                                  , writeBuffer.getCapacity());


        // don't start next request if trace processing is pending
        if (isRequestAutoStart()) {
            // start processing next request
            startNextRequest();
        }

        // restart loop
        resume(0);
    }
}

// IMPORTANT: interrupts disabled when called
void Controller::handleCompletedRequests() {
    while (!completedStreams.isEmpty()) {
        const uint8_t id = completedStreams.removeHead();
        ByteStream *completedStream = getReadStream(id);
        // serialDebugTwiDataPrintf_P(PSTR("Completing Request: %d \n"), head);

        // put its handled info back to writeBuffer and it back in the free queue
        // unless it is an own buffer request
        completedStream->triggerComplete();
        completedStream->flags = 0;
        completedStream->pRcvBuffer = NULL;
        freeReadStreams.addTail(id);

        reservationLock.makeAvailable(1, 0);

        // serialDebugResourceTracePrintf_P(PSTR("Ctrl::handle summary %d: a1:%d - r1:%d, a2:%d - r2:%d\n")
        //                                  , id
        //                                  , reservationLock.getAvailable1()
        //                                  , freeReadStreams.getCount()
        //                                  , reservationLock.getAvailable2()
        //                                  , writeBuffer.getCapacity());
    }
}

ByteStream *Controller::processStream(ByteStream *pWriteStream, CByteBuffer_t *pRcvBuffer) {
    uint8_t nextFreeHead;       // where next request head position should start

    if (freeReadStreams.isEmpty()) {
        return NULL;
    }

    uint8_t head = freeReadStreams.removeHead();
#ifdef RESOURCE_TRACE
    usedStreams++;
#endif
    
    ByteStream *pStream = readStreamTable + head;

    // incorporate tail into buffer if not own buffered stream
    uint8_t isSharedBuffer = pWriteStream->pData == writeBuffer.pData;
    uint8_t startProcessing = 0;

    // NOTE: protect from mods in interrupts mid-way through this code
    cli();
    reservationLock.useAvailable1(1);

    if (isSharedBuffer) {
#ifdef RESOURCE_TRACE
        usedBufferSize += pWriteStream->count();
#endif
        reservationLock.useAvailable2(pWriteStream->count());

        writeBuffer.updateStreamed(pWriteStream);
        nextFreeHead = pWriteStream->nTail;

        // serialDebugResourceTracePrintf_P(PSTR("Ctrl::proc summary: a1:%d - r1:%d, a2:%d - r2:%d\n")
        //                                  , reservationLock.getAvailable1()
        //                                  , freeReadStreams.getCount()
        //                                  , reservationLock.getAvailable2()
        //                                  , writeBuffer.getCapacity());

    }

    // copy the write stream info into read stream for processing
    pWriteStream->getStream(pStream, isSharedBuffer ? STREAM_FLAGS_RD : STREAM_FLAGS_RD | STREAM_FLAGS_UNBUFFERED);

    if (isSharedBuffer) {
        // new read stream starts where last read stream left off
        pStream->nHead = lastFreeHead;
        lastFreeHead = nextFreeHead;
    }

    // queue it for processing
    pStream->flags |= STREAM_FLAGS_PENDING;
    pStream->pRcvBuffer = pRcvBuffer;
    pendingReadStreams.addTail(head);

    if (isRequestAutoStart() && pendingReadStreams.getCount() == 1) {
        // first one, then no-one to start it up but here
        serialDebugTwiDataPrintf_P(PSTR("AutoStart req %d\n"), head);
        startProcessing = 1;
    } else {
        // otherwise checking will be done in endProcessingRequest or in loop() for completed previous requests
        // and new request processing started if needed
    }

    // need to reset the write stream for stuff moved to read stream, ie prepare it for more requests
    pWriteStream->nHead = pWriteStream->nTail;

    sei();

    if (startProcessing) {
        pStream->flags |= STREAM_FLAGS_PROCESSING;
        startProcessingRequest(pStream);
    }

    // make sure loop task is enabled start our loop task to monitor its completion
    this->resume(0);
    return pStream;
}

