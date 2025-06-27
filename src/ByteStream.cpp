#include "Arduino.h"
#include "ByteStream.h"
#include "Scheduler.h"

ByteStream::ByteStream(Queue *pByteQueue, uint8_t streamFlags) : Queue(*pByteQueue) {
    flags = streamFlags;
    addr = 0;
    waitingTask = NULL_TASK;
}

uint8_t ByteStream::setFlags(uint8_t flags, uint8_t mask) {
    mask &= ~(STREAM_FLAGS_RD_WR);
    this->flags &= ~mask;
    this->flags |= flags & mask;
    return this->flags;
}

uint8_t ByteStream::setOwnBuffer(uint8_t *pData, uint8_t nSize) {
    if (can_write()) {
        this->nTail = this->nSize = nSize;
        this->nHead = 0;
        this->pData = pData;
        return 0;
    }
    return NULL_BYTE;
}

// test if any more data to read
uint8_t stream_is_empty(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->is_empty();
}

// test if room for more data to write
uint8_t stream_is_full(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->is_full();
}

// capacity to accept written bytes
uint8_t stream_capacity(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->capacity();
}

uint8_t stream_count(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->count();
}

// read byte
uint8_t stream_get(CByteStream_t *thizz) {
    if (((ByteStream *) thizz)->can_read()) {
        return ((ByteStream *) thizz)->get();
    }
    return NULL_BYTE;
}

// get the next byte, but leave it in the stream
uint8_t stream_peek(const CByteStream_t *thizz) {
    if (((ByteStream *) thizz)->can_read()) {
        return ((ByteStream *) thizz)->peek();
    }
    return NULL_BYTE;
}

// write byte
uint8_t stream_put(CByteStream_t *thizz, uint8_t data) {
    if (((ByteStream *) thizz)->can_write()) {
        return ((ByteStream *) thizz)->put(data);
    }
    return NULL_BYTE;
}

uint8_t stream_can_write(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->can_write();
}

uint8_t stream_can_read(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->can_read();
}

// return true if the stream is unbuffered and not pending
uint8_t stream_is_unbuffered_pending(const CByteStream_t* thizz) {
    return thizz ? ((ByteStream *) thizz)->isUnbufferedPending() : 0;
} 

// get address from flags
uint8_t stream_address(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->address();
}

void ByteStream::pgmByteList(const uint8_t *bytes, uint16_t count) {
    while (count-- > 0) {
        put(pgm_read_byte(bytes++));
    }
}

void ByteStream::waitComplete() {
    Task *pTask = scheduler.getTask();
    if (pTask) {
        scheduler.suspend(pTask);
        waitingTask = pTask->getIndex();
    }
}

void ByteStream::triggerComplete() {
    if (waitingTask != NULL_TASK) {
        Task *pTask = scheduler.getTask(waitingTask);
        waitingTask = NULL_TASK;
        scheduler.resume(pTask, 0);
    }
}

#ifdef SERIAL_DEBUG_TWI_DATA

void ByteStream::serialDebugDump(uint8_t id) {
    uint8_t iMax = getCount();
    serialDebugTwiDataPrintf_P(PSTR("TWI: 0x%2.2x %c #%d {"), addr >> 1, addr & 0x01 ? 'R' : 'W', id);
    for (uint8_t i = 0; i < iMax; i++) {
        uint8_t byte = peekHead(i);
        serialDebugTwiDataPrintf_P(PSTR("  %2.2x"), byte);
    }
    serialDebugTwiDataPrintf_P(PSTR(" }\n"));
}

#endif

ByteStream *ByteStream::getStream(ByteStream *pOther, uint8_t flags) {
    pOther->addr = addr;
    pOther->waitingTask = waitingTask;
    waitingTask = NULL_TASK;
    return Queue::getStream(pOther, flags);
}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void ByteStream::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';


    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    addActualOutput("%sStream { flags:%s%s%s%s%s%s nSize:%d, nHead:%d, nTail:%d\n", indentStr, !flags ? "0" : "", isProcessing() ? "*" : "", canRead() ? "R" : "", canWrite() ? "W" : "", isPending() ? "P" : "", isUnbuffered() ? "U" : "", nSize, nHead, nTail);
    addActualOutput("%s  isEmpty() = %d isFull() = %d getCount() = %d getCapacity() = %d\n%s", indentStr, isEmpty(), isFull(), getCount(), getCapacity(), indentStr);
    uint16_t cnt = 0, last_cnt = -1;

    if (!compact || !isEmpty()) {
        for (uint16_t i = 0; i < nSize; i++) {
            bool addSpace = false;
            bool hadHead = false;

            if (last_cnt != cnt && !(cnt % 16)) {
                last_cnt = cnt;

                if (cnt) {
                    addActualOutput("\n%s  ", indentStr);
                }
            }

            if (!compact || (nHead <= nTail && i >= nHead && i <= nTail) ||
                (nHead > nTail && i <= nTail && i >= nHead)) {

                if (i == nHead) {
                    addActualOutput(" [");
                    hadHead = true;
                }

                if (i == nTail) {
                    addActualOutput("] ");
                } else {
                    addSpace = !hadHead;
                }

                if (addSpace) {
                    addActualOutput(" ");
                }

                if (!compact || (nHead <= nTail && i >= nHead && i < nTail) ||
                    (nHead > nTail && i < nTail && i >= nHead)) {
                    cnt++;
                    addActualOutput("0x%2.2x", pData[i]);
                }
            }
        }
        addActualOutput("\n");

        if (!compact) {
            int iMax = getCount();
            addActualOutput("%s  peekHead {", indentStr);
            for (int i = 0; i <= iMax; ++i) {
                addActualOutput(" 0x%2.2x", peekHead(i));
            }
            addActualOutput("%s }\n", indentStr);
            addActualOutput("%s  peekTail {", indentStr);
            for (int i = 0; i <= iMax; ++i) {
                addActualOutput(" 0x%2.2x", peekTail(i));
            }
            addActualOutput("%s }\n", indentStr);
        }
        
        addActualOutput("%s}\n", indentStr);
    } else {
        addActualOutput("}\n");
    }
}

#endif // CONSOLE_DEBUG
