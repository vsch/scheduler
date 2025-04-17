
#ifndef SCHEDULER_BYTEQUEUE_H
#define SCHEDULER_BYTEQUEUE_H

#include "Arduino.h"
#include "common_defs.h"
#include "Queue.h"

typedef struct ByteQueue_Data {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t data[];
} ByteQueueData_t;

typedef struct ByteQueue_DataPtr {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t *data;
} ByteQueuePtr_t;

#define sizeOfByteQueue(s)      (sizeOfPlus(Queue_t, (s)+1, uint8_t))

#ifdef __cplusplus
extern "C" {
#endif

// access methods Data versions
extern uint8_t byteQueueData_getCount(const ByteQueueData_t *thizz);
extern uint8_t byteQueueData_isEmpty(const ByteQueueData_t *thizz);
extern uint8_t byteQueueData_isFull(const ByteQueueData_t *thizz);
extern uint8_t byteQueueData_peekHead(const ByteQueueData_t *thizz);
extern uint8_t byteQueueData_peekTail(const ByteQueueData_t *thizz);
extern uint8_t byteQueueData_addTail(ByteQueueData_t *thizz, uint8_t byte);
extern uint8_t byteQueueData_removeTail(ByteQueueData_t *thizz);
extern uint8_t byteQueueData_addHead(ByteQueueData_t *thizz, uint8_t byte);
extern uint8_t byteQueueData_removeHead(ByteQueueData_t *thizz);

// enqueue/dequeue synonyms, push/pop synonyms
#define byteQueueData_enqueue(thizz, byte) (byteQueueData_addTail((thizz), byte))
#define byteQueueData_dequeue(thizz) (byteQueueData_removeHead(thizz))
#define byteQueueData_push(thizz, byte) (byteQueueData_addTail((thizz), byte))
#define byteQueueData_pop(thizz) (byteQueueData_removeTail(thizz))

// Pointer versions
extern uint8_t byteQueuePtr_peekHead(const ByteQueuePtr_t *thizz);
extern uint8_t byteQueuePtr_peekTail(const ByteQueuePtr_t *thizz);
extern uint8_t byteQueuePtr_addTail(ByteQueuePtr_t *thizz, uint8_t byte);
extern uint8_t byteQueuePtr_removeTail(ByteQueuePtr_t *thizz);
extern uint8_t byteQueuePtr_addHead(ByteQueuePtr_t *thizz, uint8_t byte);
extern uint8_t byteQueuePtr_removeHead(ByteQueuePtr_t *thizz);

// enqueue/dequeue synonyms push/pop synonyms
#define byteQueuePtr_enqueue(thizz, byte) (byteQueuePtr_addTail((thizz), byte))
#define byteQueuePtr_dequeue(thizz) (byteQueuePtr_removeHead(thizz))
#define byteQueuePtr_push(thizz, byte) (byteQueuePtr_addTail((thizz), byte))
#define byteQueuePtr_pop(thizz) (byteQueuePtr_removeTail(thizz))

#ifdef __cplusplus
}

// C++ versions, just convert pointer to C struct to pointer to C++ version, they map with inlines
struct ByteQueue : public Queue_t {
    uint8_t data[];

    inline uint8_t getCount() const { return queue_getCount((const Queue_t *)this); }
    inline uint8_t isEmpty() const { return queue_isEmpty((const Queue_t *)this); }
    inline uint8_t isFull() const { return queue_isFull((const Queue_t *)this); }
    inline uint8_t peekHead() const { return byteQueueData_peekHead((const ByteQueueData_t *)this); }
    inline uint8_t peekTail() const { return byteQueueData_peekTail((const ByteQueueData_t *)this); }

    // enqueue/dequeue methods
    inline uint8_t addTail(uint8_t byte) { return byteQueueData_addTail((ByteQueueData_t *)this, byte); }
    inline uint8_t addHead(uint8_t byte) { return byteQueueData_addHead((ByteQueueData_t *)this, byte); }
    inline uint8_t removeTail() { return byteQueueData_removeTail((ByteQueueData_t *)this); }
    inline uint8_t removeHead() { return byteQueueData_removeHead((ByteQueueData_t *)this); }

    // enqueue/dequeue synonyms
    inline uint8_t enqueue(uint8_t data) { return addTail(data); }
    inline uint8_t dequeue() { return removeHead(); }

    // push/pop synonyms
    inline uint8_t push(uint8_t data) { return addTail(data); }
    inline uint8_t pop() { return removeTail(); }

    inline void setHead(uint8_t nHead) { this->nHead = nHead; }
    inline void setTail(uint8_t nTail) { this->nTail = nTail; }
};

struct ByteQueuePtr : public Queue_t {
    uint8_t *data;

    inline uint8_t getCount() const { return queue_getCount((const Queue_t *)this); }
    inline uint8_t isEmpty() const { return queue_isEmpty((const Queue_t *)this); }
    inline uint8_t isFull() const { return queue_isFull((const Queue_t *)this); }
    inline uint8_t peekHead() const { return byteQueuePtr_peekHead((const ByteQueuePtr_t *)this); }
    inline uint8_t peekTail() const { return byteQueuePtr_peekTail((const ByteQueuePtr_t *)this); }

    // enqueue/dequeue methods
    inline uint8_t addTail(uint8_t byte) { return byteQueuePtr_addTail((ByteQueuePtr_t *)this, byte); }
    inline uint8_t addHead(uint8_t byte) { return byteQueuePtr_addHead((ByteQueuePtr_t *)this, byte); }
    inline uint8_t removeTail() { return byteQueuePtr_removeTail((ByteQueuePtr_t *)this); }
    inline uint8_t removeHead() { return byteQueuePtr_removeHead((ByteQueuePtr_t *)this); }

    // enqueue/dequeue synonyms
    inline uint8_t enqueue(uint8_t data) { return addTail(data); }
    inline uint8_t dequeue() { return removeHead(); }

    // push/pop synonyms
    inline uint8_t push(uint8_t data) { return addTail(data); }
    inline uint8_t pop() { return removeTail(); }

    inline void setHead(uint8_t nHead) { this->nHead = nHead; }
    inline void setTail(uint8_t nTail) { this->nTail = nTail; }
};

#endif

#endif //SCHEDULER_BYTEQUEUE_H
