#include "Queue.h"
#include "Stream.h"
#include "Streams.h"

Queue::Queue(uint8_t *pData, uint8_t nSize) : pData(pData) {
    this->nSize = nSize;
    nHead = nTail = 0;
}

uint8_t Queue::getCount() const {
    return (nTail < nHead ? nTail + nSize : nTail) - nHead;
}

uint8_t Queue::addTail(uint8_t data) {
    if (isFull()) return NULL_BYTE;

    pData[nTail++] = data;
    if (nTail == nSize) nTail = 0;
    return data;
}

uint8_t Queue::removeTail() {
    if (isEmpty()) return 0;

    if (!nTail) nTail = nSize;
    return pData[--nTail];
}

uint8_t Queue::removeHead() {
    if (isEmpty()) return NULL_BYTE;

    uint8_t data = pData[nHead++];
    if (nHead == nSize) nHead = 0;
    return data;
}

uint8_t Queue::addHead(uint8_t data) {
    if (isFull()) return NULL_BYTE;

    if (!nHead) nHead = nSize;
    pData[--nHead] = data;
    return data;
}

uint8_t Queue::peekHead(uint8_t offset) const {
    return isEmpty(offset) ? NULL_BYTE : pData[nHead + offset < nSize ? nHead + offset : nHead + offset - nSize];
}

uint8_t Queue::peekTail(uint8_t offset) const {
    return isEmpty(offset) ? NULL_BYTE : pData[nTail >= offset ? nTail - offset : nSize - offset];
}

uint8_t Queue::updateQueued(Queue *pOther, uint8_t flags) {
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

uint8_t Queue::updateStreamed(ByteStream *pOther) {
    return updateQueued(pOther, pOther->flags);
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

uint16_t Queue::addTailW(uint16_t data) {
    if (!isFull(sizeof(uint16_t))) {
        addTail(data & 0xff);
        addTail(data >> 8);
        return data;
    }
    return NULL_WORD;
}

uint16_t Queue::addHeadW(uint16_t data) {
    if (!isFull(sizeof(uint16_t))) {
        addHead(data >> 8);
        addHead(data & 0xff);
        return data;
    }
    return NULL_WORD;
}

uint16_t Queue::removeTailW() {
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

uint16_t Queue::removeHeadW() {
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

uint16_t Queue::peekTailW() const {
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

uint16_t Queue::peekHeadW() const {
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

ByteStream *Queue::getStream(ByteStream *pOther, uint8_t flags) {
    pOther->nSize = nSize;
    pOther->nHead = nHead;
    pOther->nTail = nTail;
    pOther->pData = pData;
    pOther->flags = flags;
    return pOther;
}

