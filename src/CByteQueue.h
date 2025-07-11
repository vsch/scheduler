#ifndef SCHEDULER_CBYTEQUEUE_H
#define SCHEDULER_CBYTEQUEUE_H

#include "common_defs.h"

#define QUEUE_MAX_SIZE      (254)

#define sizeOfQueue(s, t)            (sizeOfArray((s)+1, t))
#define sizeOfByteQueue(s)          (sizeOfQueue((s), uint8_t))

// Simple queueing both read and write for use in C interrupts and C code, provided from C/C++ code
// has the same layout as Queue
typedef struct CByteQueue
{
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t* pData;
} CByteQueue_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void queue_init(CByteQueue_t* thizz, uint8_t *pData, uint8_t nSize);

extern uint8_t queue_is_empty(const CByteQueue_t* thizz); // test if any more data to read
extern uint8_t queue_is_full(const CByteQueue_t* thizz); // test if room for more data to write
extern uint8_t queue_capacity(const CByteQueue_t* thizz); // capacity to accept written bytes
extern uint8_t queue_count(const CByteQueue_t* thizz); // capacity to accept written bytes
extern uint8_t queue_get(CByteQueue_t* thizz); // read byte
extern uint8_t queue_peek(const CByteQueue_t* thizz); // get the next byte, but leave it in the queue
extern uint8_t queue_put(CByteQueue_t* thizz, uint8_t data); // write byte

#ifdef __cplusplus
}

#endif

#endif //SCHEDULER_CBYTEQUEUE_H
