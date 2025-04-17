
#include "ByteQueue.h"

void byteQueue_construct(uint8_t *pData, uint8_t nSize) {
    ByteQueue_t *this = (ByteQueue_t *)pData;
    this->nSize = nSize;
    this->nHead = this->nTail = 0;
}

uint8_t byteQueue_getCount(const ByteQueue_t *this) {
    return (this->nTail < this->nHead ? this->nTail + this->nSize : this->nTail) - this->nHead;
}

uint8_t byteQueue_peekHead(const ByteQueue_t *this) {
    return byteQueue_isEmpty(this) ? NULL_DATA : this->data[this->nHead];
}

uint8_t byteQueue_peekTail(const ByteQueue_t *this) {
    return byteQueue_isEmpty(this) ? NULL_DATA : this->data[this->nTail ? this->nTail - 1 : this->nSize - 1];
}

// enqueue/dequeue methods
void byteQueue_addTail(ByteQueue_t *this, uint8_t byte) {
    if (byteQueue_isFull(this)) return;

    this->data[this->nTail++] = byte;
    if (this->nTail == this->nSize) this->nTail = 0;
}

uint8_t byteQueue_removeTail(ByteQueue_t *this) {
    if (byteQueue_isEmpty(this)) return 0;

    if (!this->nTail) this->nTail = this->nSize;
    return this->data[--this->nTail];
}

void byteQueue_addHead(ByteQueue_t *this, uint8_t byte) {
    if (byteQueue_isFull(this)) return;

    if (!this->nHead) this->nHead = this->nSize;
    this->data[--this->nHead] = byte;
}

uint8_t byteQueue_removeHead(ByteQueue_t *this) {
    if (byteQueue_isEmpty(this)) return 0;

    uint8_t byte = this->data[this->nHead++];
    if (this->nHead == this->nSize) this->nHead = 0;
    return byte;
}
