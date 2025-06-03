#include "Arduino.h"
#include "Streams.h"

uint8_t Stream::setFlags(uint8_t flags, uint8_t mask) {
    mask &= ~(STREAM_FLAGS_RD_WR);
    this->flags &= ~mask;
    this->flags |= flags & mask;
    return this->flags;
}

uint8_t Stream::setOwnBuffer(uint8_t *pData, uint8_t nSize) {
    if (can_write()) {
        this->nTail = this->nSize = nSize;
        this->nHead = 0;
        this->pData = pData;
        return 0;
    }
    return NULL_BYTE;
}

// test if any more data to read
uint8_t stream_is_empty(const ByteStream_t *thizz) {
    return ((Stream *) thizz)->is_empty();
}

// test if room for more data to write
uint8_t stream_is_full(const ByteStream_t *thizz) {
    return ((Stream *) thizz)->is_full();
}

// capacity to accept written bytes
uint8_t stream_capacity(const ByteStream_t *thizz) {
    return ((Stream *) thizz)->capacity();
}

// read byte
uint8_t stream_get(ByteStream_t *thizz) {
    if (((Stream *) thizz)->can_read()) {
        return ((Stream *) thizz)->get();
    }
    return NULL_BYTE;
}

// get the next byte, but leave it in the stream
uint8_t stream_peek(const ByteStream_t *thizz) {
    if (((Stream *) thizz)->can_read()) {
        return ((Stream *) thizz)->peek();
    }
    return NULL_BYTE;
}

// write byte
uint8_t stream_put(ByteStream_t *thizz, uint8_t data) {
    if (((Stream *) thizz)->can_write()) {
        return ((Stream *) thizz)->put(data);
    }
    return NULL_BYTE;
}

uint8_t stream_can_write(const ByteStream_t *thizz) {
    return ((Stream *) thizz)->can_write();
}

uint8_t stream_can_read(const ByteStream_t *thizz) {
    return ((Stream *) thizz)->can_read();
}

// get address from flags
uint8_t stream_address(const ByteStream_t *thizz) {
    return ((Stream *) thizz)->address();
}

#ifdef CONSOLE_DEBUG
#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void Stream::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';


    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    addActualOutput("%sStream { flags:%c%c nSize:%d, nHead:%d, nTail:%d\n", indentStr, canRead() ? 'R' : ' ',
               canWrite() ? 'W' : ' ', nSize, nHead, nTail);
    addActualOutput("%s  isEmpty() = %d isFull() = %d getCount() = %d getCapacity() = %d\n", indentStr, isEmpty(), isFull(),
               getCount(), getCapacity());
    uint16_t cnt = 0, last_cnt = -1;

    if (!compact || !isEmpty()) {
        // first line, add indent
        addActualOutput("%s", indentStr);
        
        for (uint16_t i = 0; i < nSize; i++) {
            bool addSpace = false;
            bool hadHead = false;

            if (last_cnt != cnt && !(cnt % 16)) {
                last_cnt = cnt;

                if (cnt) {
                    addActualOutput("\n");
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

                if (!compact || (nHead <= nTail && i >= nHead && i < nTail) ||
                    (nHead > nTail && i < nTail && i >= nHead)) {
                    cnt++;

                    if (addSpace) {
                        addActualOutput(" ");
                    }

                    addActualOutput("0x%2.2x", pData[i]);
                }
            }
        }

        addActualOutput("\n%s}\n", indentStr);
    } else {
        addActualOutput("%s}\n", indentStr);
    }
}

#endif
