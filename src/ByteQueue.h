
#ifndef SCHEDULER_BYTEQUEUE_H
#define SCHEDULER_BYTEQUEUE_H

#include "Arduino.h"
#include "common_defs.h"

#define NULL_DATA  (0xff)

typedef struct ByteQueue_data {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t data[];
} ByteQueue_t;

#define sizeOfByteQueue(s)      (sizeOfPlus(ByteQueue_t, (s)+1))

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @param pData    pointer to where the queue buffer is allocated (use sizeOfByteQueue() to allocate uint8_t buffer for structure and queue
 * @param nSize    size of memory pointed to by pData
 */
extern void byteQueue_construct(uint8_t *pData, uint8_t nSize);
extern uint8_t byteQueue_getCount(const ByteQueue_t *thizz);

#define byteQueue_isEmpty(thizz) ((thizz)->nHead == (thizz)->nTail)
#define byteQueue_isFull(thizz) (byteQueue_getCount(thizz) + 1 == (thizz)->nSize)

extern uint8_t byteQueue_peekHead(const ByteQueue_t *thizz);
extern uint8_t byteQueue_peekTail(const ByteQueue_t *thizz);

// enqueue/dequeue methods
extern void byteQueue_addTail(ByteQueue_t *thizz, uint8_t byte);
extern uint8_t byteQueue_removeTail(ByteQueue_t *thizz);
extern void byteQueue_addHead(ByteQueue_t *thizz, uint8_t byte);
extern uint8_t byteQueue_removeHead(ByteQueue_t *thizz);

// enqueue/dequeue synonyms
#define byteQueue_enqueue(thizz, byte) (byteQueue_addTail((thizz), byte))
#define byteQueue_dequeue(thizz) (byteQueue_removeHead(thizz))

// push/pop synonyms
#define byteQueue_push(thizz, byte) (byteQueue_addTail((thizz), byte))
#define byteQueue_pop(thizz) (byteQueue_removeTail(thizz))

#ifdef __cplusplus
}

struct ByteQueue {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t data[];

    inline uint8_t getCount() const { return byteQueue_getCount((const ByteQueue_t *)this); }
    inline uint8_t isEmpty() const { return byteQueue_isEmpty((const ByteQueue_t *)this); }
    inline uint8_t isFull() const { return byteQueue_isFull((const ByteQueue_t *)this); }
    inline uint8_t peekHead() const { return byteQueue_peekHead((const ByteQueue_t *)this); }
    inline uint8_t peekTail() const { return byteQueue_peekTail((const ByteQueue_t *)this); }

    // enqueue/dequeue methods
    inline void addTail(uint8_t byte) { byteQueue_addTail((ByteQueue_t *)this, byte); }
    inline void addHead(uint8_t byte) { byteQueue_addHead((ByteQueue_t *)this, byte); }
    inline uint8_t removeTail() { return byteQueue_removeTail((ByteQueue_t *)this); }
    inline uint8_t removeHead() { return byteQueue_removeHead((ByteQueue_t *)this); }

    // enqueue/dequeue synonyms
    inline void enqueue(uint8_t data) { addTail(data); }
    inline uint8_t dequeue() { return removeHead(); }

    // push/pop synonyms
    inline void push(uint8_t data) { addTail(data); }
    inline uint8_t pop() { return removeTail(); }
};

#endif

#endif //SCHEDULER_BYTEQUEUE_H
