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

void Controller::dumpResourceTrace() {
    updateResourceTrace();

#ifdef CONSOLE_DEBUG
    ADD_RESULT("Controller::resources usedStreams:%d, usedTasks:%d, usedBufferSize:%d\n", usedStreams, usedTasks, usedBufferSize);
#elif defined(SERIAL_DEBUG)
#include "Serial.h"
    // TODO: add trace code
    Serial.print();
#endif // CONSOLE_DEBUG
}

#endif // RESOURCE_TRACE

// IMPORTANT: called with interrupts disabled
#ifdef SERIAL_DEBUG_TWI_TRACER

void Controller::dumpTrace(uint8_t noWait) {
    if (!twiTraceBuffer.isEmpty()) {

        // set trace pending and wait for TWI to be idle so we don't mess up the twi interrupt timing
        flags |= CTR_FLAGS_TRC_PENDING;
        
#ifndef CONSOLE_DEBUG       
        if (!noWait) {
            sei();
            while (twiint_busy());
            cli();
        }
#endif        

        TraceBuffer traceBuffer;

        // make a copy and clear the trace queue
        traceBuffer.copyFrom(&twiTraceBuffer);
        twiTraceBuffer.reset();

        flags &= ~CTR_FLAGS_TRC_PENDING;

        // enable interrupts so twi processing can proceed
        sei();

        traceBuffer.dump();

        cli();
    }
}

#endif
