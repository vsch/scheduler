#include "Scheduler.h"
#include "Controller.h"
#include "twiint.h"
#include "debug_config.h"

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
        resourceTracePrintf_P(PSTR("%S::resources act(locked) streams:%d(%d), bytes:%d(%d)\n")
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
    cli();
    uint8_t reserved = resourceLock.reserve(requests, bytes);
    sei();

#ifdef RESOURCE_TRACE
    lockedStreams = requests;
    lockedBufferSize = bytes;
#endif

    if (reserved == NULL_BYTE) {
        // cannot ever satisfy these requirements
        serialDebugResourceDetailTracePrintf_P(PSTR("Ctrl:: never satisfied: R %d > maxR %d || B > maxB %d\n")
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
    cli();
    cliHandleCompletedRequests();

#ifdef SERIAL_DEBUG_TWI_TRACER
    TraceBuffer::cliDumpTrace();
#endif

    cliStartNextRequest();
    sei();

    resume(1);
}

// IMPORTANT: called from interrupt code
void Controller::cliEndProcessingRequest(ByteStream *pStream) {
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
            cliStartNextRequest();
        }

        // restart loop
        resume(0);
    }
}

// IMPORTANT: interrupts disabled when called
void Controller::cliHandleCompletedRequests() {
    while (!completedStreams.isEmpty()) {
        const uint8_t id = completedStreams.removeHead();
        ByteStream *completedStream = getReadStream(id);

        // put its handled info back to writeBuffer and it back in the free queue
        // unless it is an own buffer request
        completedStream->flags = 0;
        completedStream->nRdSize = 0;
        completedStream->pRdData = NULL;
        freeReadStreams.addTail(id);

        resourceLock.makeAvailable(1, 0);
    }
}

/*
ByteStream *Controller::processRequest(uint8_t addr, uint8_t *pData, uint8_t nSize, CByteBuffer_t *pRcvBuffer) {

    if (nSize > QUEUE_MAX_SIZE) {
        nSize = QUEUE_MAX_SIZE;
    }

    if (freeReadStreams.isEmpty()) {
        return NULL;
    }

    uint8_t head = freeReadStreams.removeHead();
#ifdef RESOURCE_TRACE
    usedStreams++;
#endif

    ByteStream *pStream = readStreamTable + head;
    pStream->set_address(addr);
    pStream->pData = pData;
    pStream->nSize = nSize;
    pStream->nHead = 0;
    pStream->nTail = nSize ? nSize - 1 : 0;
    pStream->nRdSize = pRcvBuffer->nSize;
    pStream->pRdData = pRcvBuffer->pData;

    // configure twi flags
    // serialDebugPrintf_P(PSTR("addr 0x%2.2x\n"), addr);
    pStream->flags = (addr & 0x01 ? STREAM_FLAGS_WR : STREAM_FLAGS_RD) | STREAM_FLAGS_UNBUFFERED;
    if (pRcvBuffer->flags & BUFFER_PUT_REVERSE) {
        pStream->flags |= STREAM_FLAGS_RD_REVERSE;
    }
    // pStream->flags = STREAM_FLAGS_RD | STREAM_FLAGS_PENDING | STREAM_FLAGS_UNBUFFERED;

    // NOTE: protect from mods in interrupts mid-way through this code
    cli();
    resourceLock.useAvailable1(1);
    // queue it for processing
    pStream->flags |= STREAM_FLAGS_PENDING;
    pendingReadStreams.addTail(head);
    if (isRequestAutoStart() && pendingReadStreams.getCount() == 1) {
        cliStartNextRequest();
    } else {
        // otherwise checking will be done in endProcessingRequest or in loop() for completed previous requests
        // and new request processing started if needed
    }
    sei();

    // make sure loop task is enabled start our loop task to monitor its completion
    this->resume(0);

    return pStream;
}
*/

ByteStream *Controller::processStream(ByteStream *pWriteStream) {
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
    pWriteStream->getStream(pStream, isSharedBuffer ? STREAM_FLAGS_RD : STREAM_FLAGS_RD | STREAM_FLAGS_UNBUFFERED);

    if (isSharedBuffer) {
        // new read stream starts where last read stream left off
        pStream->nHead = lastFreeHead;
        lastFreeHead = nextFreeHead;
    }

    // queue it for processing
    pStream->flags |= STREAM_FLAGS_PENDING;
    pendingReadStreams.addTail(head);

    if (isRequestAutoStart() && pendingReadStreams.getCount() == 1) {
        // first one, then no-one to start it up but here
        serialDebugTwiDataPrintf_P(PSTR("AutoStart req %d\n"), head);
        cliStartProcessingRequest(pStream);
    } else {
        // otherwise checking will be done in endProcessingRequest or in loop() for completed previous requests
        // and new request processing started if needed
    }
    sei();


    // need to reset the write stream for stuff moved to read stream, ie prepare it for more requests
    pWriteStream->nHead = pWriteStream->nTail;

    // make sure loop task is enabled start our loop task to monitor its completion
    this->resume(0);
    return pStream;
}

