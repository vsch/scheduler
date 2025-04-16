#ifndef SCHEDULER_QUEUE_H
#define SCHEDULER_QUEUE_H

#include <stdint.h>

#define NULL_DATA  (0xff)

class Queue {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t *pData;

public:
    Queue(uint8_t *pData, uint8_t nSize);

    uint8_t getCount() const;

    uint8_t isEmpty() const {
        return nHead == nTail;
    }

    uint8_t isFull() const {
        return getCount() + 1 == nSize;
    }

    uint8_t peekHead() const;
    uint8_t peekTail() const;

    // enqueue/dequeue methods
    void addTail(uint8_t data);
    uint8_t removeTail();
    void addHead(uint8_t data);
    uint8_t removeHead();

    // enqueue/dequeue synonyms
    inline void enqueue(uint8_t data) { addTail(data); }

    inline uint8_t dequeue() { return removeHead(); }

    // push/pop synonyms
    inline void push(uint8_t data) { addTail(data); }

    inline uint8_t pop() { return removeTail(); }
};

#endif //SCHEDULER_QUEUE_H
