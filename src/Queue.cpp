#include "Queue.h"

Queue::Queue(uint8_t *pData, uint8_t nSize) {
    this->pData = pData;
    this->nSize = nSize;
    nHead = nTail = 0;
}

uint8_t Queue::getCount() const {
    return (nTail < nHead ? nTail + nSize : nTail) - nHead;
}

void Queue::addTail(uint8_t data) {
    if (isFull()) return;

    pData[nTail++] = data;
    if (nTail == nSize) nTail = 0;
}

uint8_t Queue::removeTail() {
    if (isEmpty()) return 0;

    if (!nTail) nTail = nSize;
    return pData[--nTail];
}

uint8_t Queue::removeHead() {
    if (isEmpty()) return 0;

    uint8_t data = pData[nHead++];
    if (nHead == nSize) nHead = 0;
    return data;
}

void Queue::addHead(uint8_t data) {
    if (isFull()) return;

    if (!nHead) nHead = nSize;
    pData[--nHead] = data;
}

uint8_t Queue::peekHead() const {
    return isEmpty() ? NULL_DATA : pData[nHead];
}

uint8_t Queue::peekTail() const {
    return isEmpty() ? NULL_DATA : pData[nTail ? nTail - 1 : nSize - 1];
}
