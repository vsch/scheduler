#include "Scheduler.h"
#include "Controller.h"
#include "twiint.h"
#include "debug_config.h"

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"
#include "tests/FileTestResults.h"
#include "common_defs.h"
// #else
// #define addActualOutput(fmt, ...)  ((void)0)

// print out queue for testing
void Controller::dump(uint8_t indent, uint8_t compact) {
    //    Queue pendingReadStreams;           // requests waiting to be handleProcessedRequest
    //    Queue freeReadStreams;              // requests for processing available
    //    ByteStream *readStreams;            // pointer to first element in array of ByteSteam entries
    //    Queue writeBuffer;                  // shared write byte buffer
    //    ByteStream writeStream;             // write stream, must be requested and released in the same task invocation or pending data will not be handleProcessedRequest
    //    Mutex resourceLock;              // resourceLock for requests and buffer writes
    //    Queue requirementList;                 // byte queue of requirementList: max 8 resourceLock and 31*8 buffer, b7:b5+1 is resourceLock, B4:b0*8 = 248 bytes see Note below.

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

    addActualOutput("%s  resourceLock ", indentStr);
    this->resourceLock.dump(indent + 2, compact);

    addActualOutput("\n%s}\n", indentStr);
}

#endif // CONSOLE_DEBUG

#ifdef RESOURCE_TRACE

void Controller::dumpResourceTrace(ResourceUse *resourceUse, uint32_t *pLastDump, uint16_t dumpDelay) {
    resourceUse->addValue(usedStreams, usedBufferSize);

    if (resourceUse->canDump(pLastDump, dumpDelay)) {
#ifdef CONSOLE_DEBUG
        resourceTracePrintf_P(PSTR("%s::resources act(locked) streams:%d(%d), bytes:%d(%d)\n")
                              , resourceUse->id ? resourceUse->id : PSTR("Ctrl")
                              , resourceUse->maxUsedStreams
                              , lockedStreams
                              , resourceUse->maxUsedBufferSize
                              , lockedBufferSize);
#else
        resourceTracePrintf_P(PSTR("%S::reso act(locked) str:%d(%d), b:%d(%d)\n")
                              , resourceUse->id ? resourceUse->id : PSTR("Ctrl")
                              , resourceUse->maxUsedStreams
                              , lockedStreams
                              , resourceUse->maxUsedBufferSize
                              , lockedBufferSize);
#endif
    }

    lockedStreams = 0;
    lockedBufferSize = 0;
}

#endif // RESOURCE_TRACE

uint8_t Controller::reserveResources(uint8_t requests, uint8_t bytes) {
    CLI();
    if (freeReadStreams.getCount() != resourceLock.getAvailable1() || writeBuffer.getCapacity() != resourceLock.getAvailable2()) {
        resourceTracePrintf_P(PSTR("Ctrl:: mismatch %d, %d lock: %d %d \n"), freeReadStreams.getCount(), writeBuffer.getCapacity(), resourceLock.getAvailable1(), resourceLock.getAvailable2());
    }

    uint8_t reserved = resourceLock.reserve(requests, bytes);
    SEI();

#ifdef RESOURCE_TRACE
    lockedStreams = requests;
    lockedBufferSize = bytes;
#endif

    if (reserved == NULL_BYTE) {
        // cannot ever satisfy these requirements
        serialDebugResourceDetailTracePrintf_P(PSTR("Ctrl:: never: R %d > maxR %d || B %d > maxB %d\n")
                                               , requests, resourceLock.getMaxAvailable1()
                                               , bytes, resourceLock.getMaxAvailable2());
    } else if (reserved) {
        serialDebugResourceDetailTracePrintf_P(PSTR("Ctrl:: suspend lock %d, %d avail %d %d \n"), requests, bytes, resourceLock.getAvailable1(), resourceLock.getAvailable2());
    }

    return reserved;
}

#ifdef SERIAL_DEBUG_RESOURCE_DETAIL_TRACE

void Controller::dumpReservationLockData() {
    serialDebugResourceDetailTracePrintf_P(PSTR("Ctrl:: res2Lock %d %d, Max: %d %d\n")
                                           , resourceLock.getAvailable1(), resourceLock.getAvailable2()
                                           , resourceLock.getMaxAvailable1(), resourceLock.getMaxAvailable2());
}

#endif

void Controller::begin() {
}

void Controller::loop() {
    handleCompletedRequests();

#ifdef SERIAL_DEBUG_TWI_TRACER
    TraceBuffer::dumpTrace();
#endif

    startNextRequest();

    resume(1);
}

// IMPORTANT: called from interrupt code
void Controller::endProcessingRequest(ByteStream *pStream) {
    CLI();
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    if (!pStream->startTime) {
        pStream->startTime = twiint_request_start_time;
        if (!pStream->startTime) pStream->startTime = 1;
    }
#endif

    pStream->flags &= ~(STREAM_FLAGS_PENDING | STREAM_FLAGS_PROCESSING);
    pStream->triggerCallback();

    // make sure it is a shared request stream
    uint8_t id = getReadStreamId(pStream);
    if (id < maxStreams) {
        uint8_t availBytes = 0;

        if (pStream->pData == writeBuffer.pData) {
            // at this point the buffer used by this request is no longer needed, so the buffer head can be moved to processed request tail.
            availBytes = writeBuffer.getCapacity();
            writeBuffer.nHead = pStream->nTail;
            availBytes = writeBuffer.getCapacity() - availBytes;
        }

        uint8_t head = pendingReadStreams.removeHead();
        completedStreams.addTail(head);
        resourceLock.makeAvailable(0, availBytes);

        // don't start next request if trace processing is pending
        if (isRequestAutoStart()) {
            // start processing next request
            startNextRequest();
        }

        // restart loop
        resume(0);
    }
    SEI();
}

void Controller::handleCompletedRequests() {
    CLI();
    for (;;) {
        cli();
        if (completedStreams.isEmpty()) break;

        const uint8_t id = completedStreams.removeHead();
        SEI();

        ByteStream *completedStream = getReadStream(id);

        // put its handled info back to writeBuffer and it back in the free queue
        // unless it is an own buffer request
        // completedStream->reset();
        completedStream->flags = 0;
        completedStream->nRdSize = 0;
        completedStream->pRdData = NULL;
        completedStream->fCallback = NULL;
        completedStream->pCallbackParam = NULL;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
        completedStream->startTime = 0;
#endif

        freeReadStreams.addTail(id);
        resourceLock.makeAvailable(1, 0);
    }
    SEI();
}

ByteStream *Controller::getWriteStream() {
    // don't do anything until process is called on the write stream.
    // this allows pre-configuring some data before calling functions to fill it with actual request
    if (!(writeStream.flags & STREAM_FLAGS_UNPROCESSED)) {
        writeStream.flags = STREAM_FLAGS_UNPROCESSED;
        writeBuffer.getStream(&writeStream, STREAM_FLAGS_WR);
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
        writeStream.startTime = 0;
#endif
        writeStream.addr = 0;
        writeStream.pCallbackParam = NULL;
        writeStream.fCallback = NULL;
    }
    return &writeStream;
}

ByteStream *Controller::processStream(ByteStream *pWriteStream) {
    uint8_t nextFreeHead;       // where next request head position should start

    if (freeReadStreams.isEmpty()) {
        serialDebugPuts_P(PSTR("Ctrl:: no req free"));
        return NULL;
    }

    // mark as processed, need to make new request before writing
    pWriteStream->flags &= ~STREAM_FLAGS_UNPROCESSED;

    uint8_t head = freeReadStreams.removeHead();
#ifdef RESOURCE_TRACE
    usedStreams++;
#endif

    ByteStream *pStream = readStreamTable + head;

    // incorporate tail into buffer if not own buffered stream
    uint8_t isSharedBuffer = pWriteStream->pData == writeBuffer.pData;

    // NOTE: protect from mods in interrupts mid-way through this code
    CLI();
    resourceLock.useAvailable1(1);

    if (isSharedBuffer) {
#ifdef RESOURCE_TRACE
        usedBufferSize += pWriteStream->count();
#endif
        resourceLock.useAvailable2(pWriteStream->count());

        writeBuffer.updateStreamed(pWriteStream);
        nextFreeHead = pWriteStream->nTail;
    }

    // copy the write stream info into read stream for processing
    pWriteStream->getStream(pStream, STREAM_FLAGS_RD);

    if (isSharedBuffer) {
        // new read stream starts where last read stream left off
        pStream->nHead = lastFreeHead;
        lastFreeHead = nextFreeHead;
    }

    // queue it for processing
    pStream->flags |= STREAM_FLAGS_PENDING;
    pendingReadStreams.addTail(head);
    uint8_t count = pendingReadStreams.getCount();

    // need to reset the write stream for stuff moved to read stream, ie prepare it for more requests
    pWriteStream->nHead = pWriteStream->nTail;
    SEI();

    if (isRequestAutoStart() && count == 1) {
        // first one, then no-one to start it up but here
        startProcessingRequest(pStream);
        serialDebugTwiDataPrintf_P(PSTR("AutoStart req %d\n"), head);
    } else {
        // otherwise checking will be done in endProcessingRequest or in loop() for completed previous requests
        // and new request processing started if needed
    }


    // make sure loop task is enabled start our loop task to monitor its completion
    this->resume(1);
    return pStream;
}

