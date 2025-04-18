#include "ByteQueue.h"
#include "Stream.h"
#include "Streams.h"

ByteQueue::ByteQueue(uint8_t *pData, uint8_t nSize) : pData(pData) {
    this->nSize = nSize;
    nHead = nTail = 0;
}

uint8_t ByteQueue::getCount() const {
    return (nTail < nHead ? nTail + nSize : nTail) - nHead;
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

uint8_t ByteQueue::peekHead() const {
    return isEmpty() ? NULL_BYTE : pData[nHead];
}

uint8_t ByteQueue::peekTail() const {
    return isEmpty() ? NULL_BYTE : pData[nTail ? nTail - 1 : nSize - 1];
}

uint8_t ByteQueue::updateStreamed(ByteQueue *pOther, uint16_t flags) {
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
