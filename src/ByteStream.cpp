#include "Arduino.h"
#include "ByteStream.h"
#include "Scheduler.h"
#include "CByteStream.h"

ByteStream::ByteStream(ByteQueue *pByteQueue, uint8_t streamFlags) : ByteQueue(*pByteQueue) {
    flags = streamFlags;
    addr = 0;
    pCallbackParam = NULL;
    fCallback = NULL;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    startTime = 0;
#endif

    nRdSize = 0;
    pRdData = NULL;
}

void ByteStream::reset() {
    ByteQueue::reset();

    flags = 0;
    addr = 0;
    pCallbackParam = NULL;
    fCallback = NULL;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    startTime = 0;
#endif

    nRdSize = 0;
    pRdData = NULL;
}

uint8_t ByteStream::setFlags(uint8_t flags, uint8_t mask) {
    mask &= ~(STREAM_FLAGS_RD_WR);
    this->flags &= ~mask;
    this->flags |= flags & mask;
    return this->flags;
}

void ByteStream::setOwnBuffer(uint8_t *pData, uint8_t nSize) {
    this->nHead = 0;
    this->nTail = nSize ? nSize - 1 : 0;
    this->nSize = nSize;
    this->pData = pData;
    this->flags |= STREAM_FLAGS_UNBUFFERED;
}

void ByteStream::setRdBuffer(uint8_t rdReverse, uint8_t *pRdData, uint8_t nRdSize) {
    this->nRdSize = nRdSize;
    this->pRdData = pRdData;
    if (rdReverse) {
        this->flags |= STREAM_FLAGS_BUFF_REVERSE;
    } else {
        this->flags &= ~STREAM_FLAGS_BUFF_REVERSE;
    }
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

// if stream is writeable, unbufferred and has room for more data to write, save the byte and turn off readable flag
// so reading will not read the bytes written
uint8_t stream_try_put(CByteStream_t *thizz, uint8_t data) {
    return ((ByteStream *) thizz)->try_put(data);
}

uint8_t stream_can_write(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->can_write();
}

uint8_t stream_can_read(const CByteStream_t *thizz) {
    return ((ByteStream *) thizz)->can_read();
}

// NOTE: the following can take a NULL for byte stream pointer
// return true if the stream is unbuffered and not pending
uint8_t stream_is_unbuffered_pending(const CByteStream_t *thizz) {
    return thizz && ((ByteStream *) thizz)->isUnbufferedPending();
}

uint8_t stream_is_pending(const CByteStream_t *thizz) {
    return thizz && ((ByteStream *) thizz)->isPending();
}

uint8_t stream_is_processing(const CByteStream_t *thizz) {
    return thizz && ((ByteStream *) thizz)->isProcessing();
}

uint8_t stream_is_unbuffered(const CByteStream_t *thizz) {
    return thizz && ((ByteStream *) thizz)->isUnbuffered();
}

void stream_set_own_buffer(const CByteStream_t *thizz, uint8_t *pData, uint8_t nSize) {
    ((ByteStream *) thizz)->setOwnBuffer(pData, nSize);
}

void stream_set_rd_buffer(const CByteStream_t *thizz, uint8_t rdReverse, uint8_t *pRdData, uint8_t nRdSize) {
    ((ByteStream *) thizz)->setRdBuffer(rdReverse, pRdData, nRdSize);
}

void ByteStream::pgmByteList(const uint8_t *bytes, uint16_t count) {
    while (count-- > 0) {
        put(pgm_read_byte(bytes++));
    }
}

#ifndef SERIAL_DEBUG_DUMP

void stream_serialDebugDump(const CByteStream_t *thizz, uint8_t id) {
}

#else

void stream_serial_debug_dump(const CByteStream_t *thizz, uint8_t id) {
    ((ByteStream *) thizz)->serialDebugDump(id);
}

void ByteStream::serialDebugDump(uint8_t id) {
    uint8_t iMax = getCount();
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    serialDebugDumpPrintf_P(PSTR("Stream: @0x%2.2x %8.8lx %c #%d {"), addr >> 1, startTime, addr & 0x01 ? 'R' : 'W', id);
#else
    serialDebugDumpPrintf_P(PSTR("Stream: @0x%2.2x %c #%d {"), addr >> 1, addr & 0x01 ? 'R' : 'W', id);
#endif

    if (nRdSize && pRdData) {
        serialDebugDumpPrintf_P(PSTR("  rcvBuffer: @0x%2.2X { flags: 0x%1.1X nSize: %d } "), pRdData, flags & STREAM_FLAGS_BUFF_REVERSE, nSize);
    }
    for (uint8_t i = 0; i < iMax; i++) {
        uint8_t byte = peekHead(i);
        serialDebugDumpPrintf_P(PSTR(" %2.2x"), byte);
    }
    serialDebugDumpPrintf_P(PSTR(" }\n"));
}

#endif

void ByteStream::getStream(ByteStream *pOther, uint8_t rdWrFlags) {
    pOther->flags = flags;
    pOther->addr = addr;
    pOther->pCallbackParam = pCallbackParam;
    pOther->fCallback = fCallback;
    pOther->nRdSize = nRdSize;
    pOther->pRdData = pRdData;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    pOther->startTime = startTime;
#endif

    pCallbackParam = NULL;
    fCallback = NULL;
    nRdSize = 0;
    pRdData = NULL;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    startTime = 0;
#endif

    ByteQueue::getStream(pOther, rdWrFlags);
}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void ByteStream::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    char flagStr[9];
    char *pFlag = flagStr;
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    memset(flagStr, 0, sizeof flagStr);
    flagStr[0] = '0';
    flagStr[1] = '\0';

    if (!isUnprocessed()) *pFlag++ = '!';
    if (isProcessing()) *pFlag++ = '*';
    if (canRead()) *pFlag++ = 'R';
    if (canWrite()) *pFlag++ = 'W';
    if (isAppend()) *pFlag++ = 'A';
    if (isPending()) *pFlag++ = 'P';
    if (isUnbuffered()) *pFlag++ = 'U';

    // Output: ByteQueue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    addActualOutput("%sStream { start:%8.8lx flags:%s nSize:%d, nHead:%d, nTail:%d\n", indentStr, startTime, flagStr, nSize, nHead, nTail);
#else
    addActualOutput("%sStream { flags:%s nSize:%d, nHead:%d, nTail:%d\n", indentStr, flagStr, nSize, nHead, nTail);
#endif
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
                    if (compact) {
                        addActualOutput("]");
                    } else {
                        addActualOutput("] ");
                    }
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
