#include "Scheduler.h"
#include "Controller.h"

#ifdef CONSOLE_DEBUG

// print out queue for testing
void Controller::dump(char *buffer, uint32_t sizeofBuffer, uint8_t indent) {
    //    Queue pendingReadStreams;           // requests waiting to be handleProcessedRequest
    //    Queue freeReadStreams;              // requests for processing available
    //    ByteStream *readStreams;            // pointer to first element in array of ByteSteam entries
    //    Queue writeBuffer;                  // shared write byte buffer
    //    ByteStream writeStream;             // write stream, must be requested and released in the same task invocation or pending data will not be handleProcessedRequest
    //    Mutex reservationLock;              // reservationLock for requests and buffer writes
    //    Queue requirementList;                 // byte queue of requirementList: max 8 reservationLock and 31*8 buffer, b7:b5+1 is reservationLock, B4:b0*8 = 248 bytes see Note below.

    uint32_t len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';


    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    snprintf(buffer, sizeofBuffer, "%sController { maxStreams:%d, maxTasks:%d, writeBufferSize:%d\n", indentStr, maxStreams, maxTasks, writeBufferSize);
    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  requestCapacity() = %d byteCapacity() = %d\n", indentStr, requestCapacity(), byteCapacity());

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  pendingReadStreams ", indentStr);

    this->pendingReadStreams.dump(buffer, sizeofBuffer, indent + 2, 1);

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  freeReadStreams ", indentStr);
    this->freeReadStreams.dump(buffer, sizeofBuffer, indent + 2, 1);

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  readStreamTable \n%s", indentStr, indentStr);
    for (uint8_t i = 0; i < maxStreams; i++) {
        len = strlen(buffer);
        buffer += len;
        sizeofBuffer -= len;
        snprintf(buffer, sizeofBuffer, "%s  [%i]", indentStr, i);
        this->readStreamTable[i].dump(buffer, sizeofBuffer, indent + 2, 1);
    }

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  writeBuffer ", indentStr);
    this->writeBuffer.dump(buffer, sizeofBuffer, indent + 2, 1);

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  writeStream ", indentStr);
    this->writeStream.dump(buffer, sizeofBuffer, indent + 2, 1);

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  reservationLock ", indentStr);
    this->reservationLock.dump(buffer, sizeofBuffer, indent + 2, 1);

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  requirementList ", indentStr);
    this->requirementList.dump(buffer, sizeofBuffer, indent + 2, 1);

    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "\n%s}\n", indentStr);
}

#endif // CONSOLE_DEBUG
