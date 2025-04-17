
#ifndef SCHEDULER_QUEUE_H
#define SCHEDULER_QUEUE_H
#include "Arduino.h"
#include "common_defs.h"

typedef struct QueueFields {
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
} Queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @param pData    pointer to where the queue buffer is allocated (use sizeOfByteQueue() to allocate uint8_t buffer for structure and queue
 * @param nSize    size of memory pointed to by pData
 */
extern void queue_construct(uint8_t *pData, uint8_t nSize);
extern void queue_copyFields(Queue_t *pFields, Queue_t *thizz);
extern uint8_t queue_getCount(const Queue_t *thizz);

#define queue_isEmpty(thizz) ((thizz)->nHead == (thizz)->nTail)
#define queue_isFull(thizz) (queue_getCount(thizz) + 1 == (thizz)->nSize)

// methods common returning index
extern uint8_t queue_peekHead(const Queue_t *thizz);
extern uint8_t queue_peekTail(const Queue_t *thizz);
extern uint8_t queue_addTail(Queue_t *thizz);
extern uint8_t queue_removeTail(Queue_t *thizz);
extern uint8_t queue_addHead(Queue_t *thizz);
extern uint8_t queue_removeHead(Queue_t *thizz);

#ifdef __cplusplus
}
#endif

#endif //SCHEDULER_QUEUE_H
