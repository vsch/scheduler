
#ifndef SCHEDULER_WORDQUEUE_H
#define SCHEDULER_WORDQUEUE_H

#include "Arduino.h"
#include "common_defs.h"
#include "Queue.h"

typedef struct WordQueue_Data {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint16_t data[];
} WordQueueData_t;

typedef struct WordQueue_DataPtr {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint16_t *data;
} WordQueuePtr_t;

#define sizeOfWordQueue(s)      (sizeOfPlus(Queue_t, (s)+1, uint16_t))

#ifdef __cplusplus
extern "C" {
#endif

// access methods Data versions
extern uint16_t wordQueueData_peekHead(const WordQueueData_t *thizz);
extern uint16_t wordQueueData_peekTail(const WordQueueData_t *thizz);
extern uint16_t wordQueueData_addTail(WordQueueData_t *thizz, uint16_t word);
extern uint16_t wordQueueData_removeTail(WordQueueData_t *thizz);
extern uint16_t wordQueueData_addHead(WordQueueData_t *thizz, uint16_t word);
extern uint16_t wordQueueData_removeHead(WordQueueData_t *thizz);

// enqueue/dequeue synonyms, push/pop synonyms
#define wordQueueData_enqueue(thizz, word) (wordQueueData_addTail((thizz), word))
#define wordQueueData_dequeue(thizz) (wordQueueData_removeHead(thizz))
#define wordQueueData_push(thizz, word) (wordQueueData_addTail((thizz), word))
#define wordQueueData_pop(thizz) (wordQueueData_removeTail(thizz))

// Pointer versions
extern uint16_t wordQueuePtr_peekHead(const WordQueuePtr_t *thizz);
extern uint16_t wordQueuePtr_peekTail(const WordQueuePtr_t *thizz);
extern uint16_t wordQueuePtr_addTail(WordQueuePtr_t *thizz, uint16_t word);
extern uint16_t wordQueuePtr_removeTail(WordQueuePtr_t *thizz);
extern uint16_t wordQueuePtr_addHead(WordQueuePtr_t *thizz, uint16_t word);
extern uint16_t wordQueuePtr_removeHead(WordQueuePtr_t *thizz);

// enqueue/dequeue synonyms push/pop synonyms
#define wordQueuePtr_enqueue(thizz, word) (wordQueuePtr_addTail((thizz), word))
#define wordQueuePtr_dequeue(thizz) (wordQueuePtr_removeHead(thizz))
#define wordQueuePtr_push(thizz, word) (wordQueuePtr_addTail((thizz), word))
#define wordQueuePtr_pop(thizz) (wordQueuePtr_removeTail(thizz))

#ifdef __cplusplus
}

// C++ versions, just convert pointer to C struct to pointer to C++ version, they map with inlines
struct WordQueue : public Queue_t {
    uint16_t data[];

    inline uint8_t getCount() const { return queue_getCount((const Queue_t *)this); }
    inline uint8_t isEmpty() const { return queue_isEmpty((const Queue_t *)this); }
    inline uint8_t isFull() const { return queue_isFull((const Queue_t *)this); }
    inline uint16_t peekHead() const { return wordQueueData_peekHead((const WordQueueData_t *)this); }
    inline uint16_t peekTail() const { return wordQueueData_peekTail((const WordQueueData_t *)this); }

    // enqueue/dequeue methods
    inline uint16_t addTail(uint16_t word) { return wordQueueData_addTail((WordQueueData_t *)this, word); }
    inline uint16_t addHead(uint16_t word) { return wordQueueData_addHead((WordQueueData_t *)this, word); }
    inline uint16_t removeTail() { return wordQueueData_removeTail((WordQueueData_t *)this); }
    inline uint16_t removeHead() { return wordQueueData_removeHead((WordQueueData_t *)this); }

    // enqueue/dequeue synonyms
    inline uint16_t enqueue(uint16_t data) { return addTail(data); }
    inline uint16_t dequeue() { return removeHead(); }

    // push/pop synonyms
    inline uint16_t push(uint16_t data) { return addTail(data); }
    inline uint16_t pop() { return removeTail(); }

    inline void setHead(uint8_t nHead) { this->nHead = nHead; }
    inline void setTail(uint8_t nTail) { this->nTail = nTail; }
};

struct WordQueuePtr : public WordQueuePtr_t {
    uint16_t *data;

    inline uint8_t getCount() const { return queue_getCount((const Queue_t *)this); }
    inline uint8_t isEmpty() const { return queue_isEmpty((const Queue_t *)this); }
    inline uint8_t isFull() const { return queue_isFull((const Queue_t *)this); }
    inline uint16_t peekHead() const { return wordQueuePtr_peekHead((const WordQueuePtr_t *)this); }
    inline uint16_t peekTail() const { return wordQueuePtr_peekTail((const WordQueuePtr_t *)this); }

    // enqueue/dequeue methods
    inline uint16_t addTail(uint16_t word) { return wordQueuePtr_addTail((WordQueuePtr_t *)this, word); }
    inline uint16_t addHead(uint16_t word) { return wordQueuePtr_addHead((WordQueuePtr_t *)this, word); }
    inline uint16_t removeTail() { return wordQueuePtr_removeTail((WordQueuePtr_t *)this); }
    inline uint16_t removeHead() { return wordQueuePtr_removeHead((WordQueuePtr_t *)this); }

    // enqueue/dequeue synonyms
    inline uint16_t enqueue(uint16_t data) { return addTail(data); }
    inline uint16_t dequeue() { return removeHead(); }

    // push/pop synonyms
    inline uint16_t push(uint16_t data) { return addTail(data); }
    inline uint16_t pop() { return removeTail(); }

    inline void setHead(uint8_t nHead) { this->nHead = nHead; }
    inline void setTail(uint8_t nTail) { this->nTail = nTail; }
};

#endif

#endif //SCHEDULER_WORDQUEUE_H
