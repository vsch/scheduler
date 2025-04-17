#include "Queue.h"

void queue_construct(uint8_t *pData, uint8_t nSize) {
    Queue_t *this = (Queue_t *) pData;
    this->nSize = nSize;
    this->nHead = this->nTail = 0;
}

void queue_copyFields(Queue_t *pFields, Queue_t *thizz) {
    pFields->nSize = thizz->nSize;
    pFields->nHead = thizz->nHead;
    pFields->nTail = thizz->nTail;
}

uint8_t queue_getCount(const Queue_t *this) {
    return (this->nTail < this->nHead ? this->nTail + this->nSize : this->nTail) - this->nHead;
}

uint8_t queue_peekHead(const Queue_t *thizz) {
    return queue_isEmpty(thizz) ? NULL_BYTE : thizz->nHead;
}

uint8_t queue_peekTail(const Queue_t *thizz) {
    return queue_isEmpty(thizz) ? NULL_BYTE : thizz->nTail ? thizz->nTail - 1 : thizz->nSize - 1;
}

// enqueue/dequeue methods
uint8_t queue_addTail(Queue_t *thizz) {
    if (queue_isFull(thizz)) return NULL_BYTE;

    uint8_t index = thizz->nTail++;
    if (thizz->nTail == thizz->nSize) thizz->nTail = 0;
    return index;
}

uint8_t queue_removeTail(Queue_t *thizz) {
    if (queue_isEmpty(thizz)) return NULL_BYTE;

    if (!thizz->nTail) thizz->nTail = thizz->nSize;
    return --thizz->nTail;
}

uint8_t queue_addHead(Queue_t *thizz) {
    if (queue_isFull(thizz)) return NULL_BYTE;

    if (!thizz->nHead) thizz->nHead = thizz->nSize;
    return --thizz->nHead;
}

uint8_t queue_removeHead(Queue_t *thizz) {
    if (queue_isEmpty(thizz)) return 0;

    uint8_t index = thizz->nHead++;
    if (thizz->nHead == thizz->nSize) thizz->nHead = 0;
    return index;
}

