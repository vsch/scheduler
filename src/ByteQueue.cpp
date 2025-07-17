#include "Arduino.h"
#include "ByteQueue.h"
#include "CByteQueue.h"
#include "ByteStream.h"

uint8_t ByteQueue::updateQueued(ByteQueue *pOther, uint8_t flags) {
    // take updated values if based on our data
    if (pOther->pData == pData) {
        if (flags & STREAM_FLAGS_RD) {
            nHead = pOther->nHead;
        }
        if (flags & STREAM_FLAGS_WR) {
            nTail = pOther->nTail;
        }
        return 0;
    }
    return NULL_BYTE;
}

uint8_t ByteQueue::updateStreamed(ByteStream *pOther) {
    return updateQueued(pOther, pOther->flags);
}

uint8_t ByteQueue::getCount() const {
    return (nTail < nHead ? nTail + nSize : nTail) - nHead;
}

uint8_t ByteQueue::peekHead(uint8_t offset) const {
    return getCount() <= offset ? NULL_BYTE : pData[nHead + offset < nSize ? nHead + offset : nHead + offset - nSize];
}

uint8_t ByteQueue::peekTail(uint8_t offset) const {
    return getCount() <= offset ? NULL_BYTE : pData[nTail <= offset ? nSize - (offset - nTail) - 1 : nTail - offset - 1];
}

#ifdef QUEUE_BLOCK_FUNCS

void *Queue::addTail(void *pVoid, uint8_t count) {
    // natural byte order
    if (!isFull(count)) {
        uint8_t *pData = (uint8_t *) pVoid;
        while (count--) {
            addTail(*pData++);
        }
    }
    return pVoid;
}

void *Queue::removeHead(void *pVoid, uint8_t count) {
    // natural byte order
    uint8_t avail = getCount();
    if (count && avail) {
        uint8_t *pData = (uint8_t *) pVoid;
        while (count--) {
            if (avail) {
                avail--;
                *pData++ = removeHead();
            } else {
                *pData++ = 0;
            }
        }
    }
    return pVoid;
}

void *Queue::peekHead(void *pVoid, uint8_t count) const {
    // natural byte order
    uint8_t avail = getCount();
    if (count && avail) {
        uint8_t *pData = (uint8_t *) pVoid;
        uint8_t offset = 0;
        while (count--) {
            if (avail) {
                avail--;
                *pData++ = peekHead(offset);
            } else {
                *pData++ = 0;
            }
            offset++;
        }
    }
    return pVoid;
}

void *Queue::addHead(void *pVoid, uint8_t count) {
    // reversed byte order
    if (!isFull(count)) {
        uint8_t *pData = (uint8_t *) pVoid;
        pData += count;
        while (count--) {
            addHead(*--pData);
        }
    }
    return pVoid;
}

void *Queue::removeTail(void *pVoid, uint8_t count) {
    // reversed byte order
    uint8_t avail = getCount();
    if (count && avail) {
        uint8_t *pData = (uint8_t *) pVoid;
        pData += count;
        while (count--) {
            if (avail) {
                avail--;
                *--pData = removeTail();
            } else {
                *--pData = 0;
            }
        }
    }
    return pVoid;
}

void *Queue::peekTail(void *pVoid, uint8_t count) const {
    // reversed byte order
    uint8_t avail = getCount();
    if (count && avail) {
        uint8_t *pData = (uint8_t *) pVoid;
        uint8_t offset = 0;
        pData += count;
        while (count--) {
            if (avail) {
                avail--;
                *--pData = peekTail(offset);
            } else {
                *--pData = 0;
            }
            offset++;
        }
    }
    return pVoid;
}

#endif

#ifdef QUEUE_DEDICATED_WORD_FUNCS

uint16_t ByteQueue::addTailW(uint16_t data) {
    if (!isFull(sizeof(uint16_t))) {
        addTail(data & 0xff);
        addTail(data >> 8);
        return data;
    }
    return NULL_WORD;
}

uint16_t ByteQueue::addHeadW(uint16_t data) {
    if (!isFull(sizeof(uint16_t))) {
        addHead(data >> 8);
        addHead(data & 0xff);
        return data;
    }
    return NULL_WORD;
}

uint16_t ByteQueue::removeTailW() {
    uint8_t count = getCount();
    if (count) {
        if (count >= sizeof(uint16_t)) {
            return removeTail() << 8 | removeTail();
        } else {
            return removeTail() << 8;
        }
    }
    return NULL_WORD;
}

uint16_t ByteQueue::removeHeadW() {
    uint8_t count = getCount();
    if (count) {
        if (count >= sizeof(uint16_t)) {
            return removeHead() | removeHead() << 8;
        } else {
            return removeHead();
        }
    }
    return NULL_WORD;
}

uint16_t ByteQueue::peekTailW() const {
    uint8_t count = getCount();
    if (count) {
        if (count >= sizeof(uint16_t)) {
            return peekTail() << 8 | peekTail(1);
        } else {
            return peekTail() << 8;
        }
    }
    return NULL_WORD;
}

uint16_t ByteQueue::peekHeadW() const {
    uint8_t count = getCount();
    if (count) {
        if (count >= sizeof(uint16_t)) {
            return peekHead() | peekHead(1) << 8;
        } else {
            return peekHead();
        }
    }
    return NULL_WORD;
}

#endif

void ByteQueue::getStream(ByteStream *pOther, uint8_t flags) {
    pOther->nSize = nSize;
    pOther->nHead = nHead;
    pOther->nTail = nTail;
    pOther->pData = pData;
    
    // only change the rd/wr flags
    pOther->flags &= ~STREAM_FLAGS_RD_WR_APPEND;
    pOther->flags |= flags & (STREAM_FLAGS_RD_WR_APPEND);
    
    if ((flags & STREAM_FLAGS_RD_WR_APPEND) == STREAM_FLAGS_WR) {
        // reset to empty at tail if it is a write only stream    
        pOther->nHead = pOther->nTail;
    }
}

uint8_t ByteQueue::addTail(uint8_t data) {
    if (isFull()) return NULL_BYTE;

    pData[nTail++] = data;
    if (nTail == nSize) nTail = 0;
    return data;
}

uint8_t ByteQueue::removeTail() {
    if (isEmpty()) return 0;

    if (!nTail) nTail = nSize;
    return pData[--nTail];
}

uint8_t ByteQueue::removeHead() {
    if (isEmpty()) return NULL_BYTE;

    uint8_t data = pData[nHead++];
    if (nHead == nSize) nHead = 0;
    return data;
}

uint8_t ByteQueue::addHead(uint8_t data) {
    if (isFull()) return NULL_BYTE;

    if (!nHead) nHead = nSize;
    pData[--nHead] = data;
    return data;
}

void queue_init(CByteQueue_t *thizz, uint8_t *pData, uint8_t nSize) {
    thizz->nSize = nSize;
    thizz->nHead = 0;
    thizz->nTail = 0;
    thizz->pData = pData;
#ifdef DEBUG_MODE_QUEUE_MEMORY
    memset(pData, DEBUG_MODE_QUEUE_MEMORY, nSize);
#endif
}

// test if any more data to read
uint8_t queue_is_empty(const CByteQueue_t *thizz) {
    return ((ByteQueue *) thizz)->isEmpty();
}

// test if room for more data to write
uint8_t queue_is_full(const CByteQueue_t *thizz) {
    return ((ByteQueue *) thizz)->isFull();
}

// capacity to accept written bytes
uint8_t queue_capacity(const CByteQueue_t *thizz) {
    return ((ByteQueue *) thizz)->getCapacity();
}

uint8_t queue_count(const CByteQueue_t *thizz) {
    return ((ByteQueue *) thizz)->getCount();
}

uint8_t queue_get(CByteQueue_t *thizz) {
    return ((ByteQueue *) thizz)->removeHead();
}

// get the next byte, but leave it in the queue
uint8_t queue_peek(const CByteQueue_t *thizz) {
    return ((ByteQueue *) thizz)->peekHead();
}

// write byte
uint8_t queue_put(CByteQueue_t *thizz, uint8_t data) {
    return ((ByteQueue *) thizz)->addTail(data);
}

#ifdef SERIAL_DEBUG

const char strQueue[] PROGMEM = "Queue";

void ByteQueue::serialDebugDump(PGM_P name) {
    uint8_t iMax = getSize();
    static const char strPrefix[] PROGMEM = " ["; 
    static const char strEmpty[] PROGMEM = " []"; 
    static const char strSuffix[] PROGMEM = " ]"; 
    static const char strNull[] PROGMEM = ""; 
    serialDebugPrintf_P(PSTR("%S: @0x%2.2X {"), name ? name : strQueue, pData);
    for (uint8_t i = 0; i < iMax; i++) {
        uint8_t byte = pData[i];
        PGM_P prefix = strNull;
        
        if (i == nHead && i == nTail) {
            prefix = strEmpty;
        } else if (i == nHead) {
            prefix = strPrefix;
        } else if (i == nTail) {
            prefix = strSuffix;
        }
        serialDebugPrintf_P(PSTR("%S %2.2x"), prefix, byte);
    }
    serialDebugPrintf_P(PSTR(" }\n"));
}

#endif

///**
// * Copy data from another queue
// * 
// * CAUTION: if the source queue is longer, head and tail are not adjusted for destination size, intended to copy 
// *          queues of same data size
// * 
// * @param pQueue 
// */
//void ByteQueue::copyFrom(const ByteQueue *pQueue) {
//    uint8_t len = nSize;
//    if (len > pQueue->nSize) {
//        len = pQueue->nSize;
//    }
//
//    if (len) {
//        len--;
//    }
//
//    memcpy(pData, pQueue->pData, len);
//    nHead = pQueue->nHead;
//    nTail = pQueue->nTail;
//}


#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void ByteQueue::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';


    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
//    addActualOutput("%s%sQueue { nSize:%d, nHead:%d, nTail:%d\n", *indentStr ? "\n" : "", indentStr, nSize, nHead, nTail);
    addActualOutput("%sQueue { nSize:%d, nHead:%d, nTail:%d\n", indentStr, nSize, nHead, nTail);
    addActualOutput("%s  isEmpty() = %d isFull() = %d getCount() = %d getCapacity() = %d\n%s", indentStr, isEmpty(), isFull(), getCount(), getCapacity(), indentStr);
    // addActualOutput("%s  peekHead() = %d peekTail() = %d peekHead(0) = %d peekTail(0) = %d\n%s", indentStr, peekHead(), peekTail(), peekHead(0), peekTail(0), indentStr);
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
