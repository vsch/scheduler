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
    updateResourceTrace();

#ifdef CONSOLE_DEBUG
    ADD_RESULT("%s::resources usedStreams:%d, usedTasks:%d, usedBufferSize:%d\n", id ? id : "Ctrl", usedStreams, usedTasks, usedBufferSize);
#elif defined(SERIAL_DEBUG)
#include <stdio.h>
    // TODO: add trace code
    resourceTracePrintf_P(PSTR("%S::resources usedStreams:%d, usedTasks:%d, usedBufferSize:%d\n"), id ? id : PSTR("Ctrl"), usedStreams, usedTasks, usedBufferSize);
#endif // CONSOLE_DEBUG
}

// TODO: need to release the lock when the task completes
uint8_t Controller::willRequire(uint8_t requests, uint8_t bytes) {
    uint8_t adjBytes = RESERVATIONS_BYTES(bytes);
    if (bytes > adjBytes || bytes > writeBuffer.getSize() || requests > 8) {
        // cannot ever satisfy these requirements
        serialDebugResourceTracePrintf_P(PSTR("Ctrl:: never satisfied: b %d > ab %d || b > wbs %d || r %d > 8\n"), bytes, adjBytes, writeBuffer.getSize(), requests);
        return NULL_BYTE;
    }

    if (reservationLock.isFree()) {
        // NOTE: need special processing because there are no waiters. Either, we have the required resources
        //  and can obtain the reservationLock for the calling task. Otherwise, the reservationLock really needs to
        //  be taken by the controller so that it can release the lock and have the next task in line get the
        //  resource. However, this call is from the task's context, so we take the resource for it and transfer to
        //  the controller task then the call for this task's context will suspend the task and resume it when the
        //  controller task releases the lock because there are sufficient resources for the next task's processing.
        if (requests <= freeReadStreams.getCount() && adjBytes <= writeBuffer.getCapacity()) {
            // can proceed now, overwrite whatever was uncommitted in the write stream
            writeBuffer.getStream(&writeStream, STREAM_FLAGS_WR);
            updateResourcePreLockTrace();
            reservationLock.reserve();
            serialDebugResourceTracePrintf_P(PSTR("Ctrl:: reserved lock\n"));
            return 0;
        }

        serialDebugResourceTracePrintf_P(PSTR("Ctrl:: transferring lock to ctrl task\n"));
        updateResourceLockTrace();
        reservationLock.reserve();
        reservationLock.transfer(scheduler.getTaskId(), this->getIndex());
    }

    // add to the queue of waiters
    serialDebugResourceTracePrintf_P(PSTR("Ctrl:: adding task to lock queue %d/%d, r: %d f %d, b %d ab %d hv %d\n"), reservationLock.getCount(), reservationLock.getSize(), requests, freeReadStreams.getCount(), bytes, adjBytes, 
                                     writeBuffer.getCapacity());
    requirementList.addTail(RESERVATIONS_PACK(requests, adjBytes));
    updateResourcePreLockTrace();
    return reservationLock.reserve();
}

void Controller::begin() {
}

void Controller::loop() {
    uint8_t pendingCount = 0;
    uint8_t writeCapacity = 0;
    // serialDebugTwiDataPrintf_P(PSTR("Ctr::Loop start\n"));

    cli();
    handleCompletedRequests();

#ifdef SERIAL_DEBUG_TWI_TRACER
    TraceBuffer::dumpTrace();
#endif

    startNextRequest();

    pendingCount = pendingReadStreams.getCount();
    writeCapacity = writeBuffer.getCapacity();
    sei();

    if (!requirementList.isEmpty()) {
        // have some waiting
        
        uint8_t packed = requirementList.peekHead();
        uint8_t requests = RESERVATIONS_UNPACK_REQ(packed);
        uint8_t bytes = RESERVATIONS_UNPACK_BYTES(packed);
        resourceTracePrintf_P(PSTR("Ctrl::loop have waiting %d, req: %d, bytes: %d\n"), reservationLock.getOwner(), requests, bytes);

        if (requests <= freeReadStreams.getCount() && bytes <= writeCapacity) {
            // let'er rip there are enough free resources
            resourceTracePrintf_P(PSTR("Ctrl::Loop release %d\n"), reservationLock.getOwner());
            requirementList.removeHead();
            reservationLock.release();
        }
    }

    if (pendingCount > 1 && !requirementList.isEmpty()) {
        // can suspend until there is something to check for.
        // serialDebugTwiDataPrintf_P(PSTR("Ctr::Loop resume(1)\n"));
        resume(1);
    } else if (pendingCount <= 1) {
        // serialDebugTwiDataPrintf_P(PSTR("Ctr::Loop suspend()\n"));
        //suspend();
        resume(100);
    } else {
        // resume just in case have completed requests
#ifdef SERIAL_DEBUG_TWI_DATA
        if (isSuspended()) {
                // serialDebugTwiDataPrintf_P(PSTR("Ctr::Loop resume(1)\n"));
                resume(1);
            }
#else
        resume(1);
#endif
    }

    //        serialDebugPrintf_P(PSTR("Ctr::Loop end\n"));
}

void Controller::releaseResourceLock() {
    if (reservationLock.isOwner(scheduler.getTask())) {
        serialDebugResourceTracePrintf_P(PSTR("Ctrl:: releasing resources.\n"));
        reservationLock.release();
    }else {
        serialDebugResourceTracePrintf_P(PSTR("Ctrl:: release called not owner task.\n"));
    }
}

#endif // RESOURCE_TRACE

