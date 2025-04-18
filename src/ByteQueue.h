#ifndef SCHEDULER_BYTEQUEUE_H
#define SCHEDULER_BYTEQUEUE_H

#include <stdint.h>
#include "common_defs.h"

#define sizeOfByteQueue(s)          (sizeOfArray((s)+1, uint8_t))

class ByteQueue {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t *const pData;

public:
    ByteQueue(uint8_t *pData, uint8_t nSize);

    uint8_t getCount() const;

    uint8_t peekHead() const;
    uint8_t peekTail() const;

    // enqueue/dequeue methods
    uint8_t addTail(uint8_t data);
    uint8_t removeTail();
    uint8_t addHead(uint8_t data);
    uint8_t removeHead();

    inline uint8_t getSize() const { return nSize - 1; }

    inline uint8_t getCapacity() const { return nSize - getCount() - 1; }

    inline uint8_t isEmpty() const { return nHead == nTail; }

    inline uint8_t isFull() const { return getCount() + 1 == nSize; }

    inline uint8_t enqueue(uint8_t data) { return addTail(data); }

    inline uint8_t dequeue() { return removeHead(); }

    inline uint8_t push(uint8_t data) { return addTail(data); }

    inline uint8_t pop() { return removeTail(); }

    uint8_t updateStreamed(ByteQueue *pOther, uint16_t flags);
};

#endif //SCHEDULER_BYTEQUEUE_H
