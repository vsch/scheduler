#ifndef SCHEDULER_STREAM_H
#define SCHEDULER_STREAM_H

#include "common_defs.h"
#include "CByteQueue.h"
#include "CByteBuffer.h"

#define STREAM_FLAGS_RD         (0x01)      // marks the stream as read enabled, can get from it
#define STREAM_FLAGS_WR         (0x02)      // marks the stream as write enabled, can put to it
#define STREAM_FLAGS_RD_WR      (0x03)      // marks the stream as write enabled, can put to it
#define STREAM_FLAGS_PENDING    (0x04)      // marks the stream as pending or processing
#define STREAM_FLAGS_PROCESSING (0x08)      // marks the stream as being processed
#define STREAM_FLAGS_UNBUFFERED (0x10)      // marks the stream is unbuffered
#define STREAM_FLAGS_APPEND     (0x20)      // used in getStream to disable resetting content to 0 for write streams

// Simple streaming both read and write for use in C interrupts and C code, provided from C/C++ code
// has the same layout as Stream
typedef struct CByteStream
{
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t* pData;
    uint8_t flags;
    uint8_t addr;
    CByteBuffer_t *pRcvBuffer;
} CByteStream_t;

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t stream_is_empty(const CByteStream_t* thizz); // test if any more data to read
extern uint8_t stream_is_full(const CByteStream_t* thizz); // test if room for more data to write
extern uint8_t stream_capacity(const CByteStream_t* thizz); // capacity to accept written bytes
extern uint8_t stream_count(const CByteStream_t* thizz); // capacity to accept written bytes
extern uint8_t stream_get(CByteStream_t* thizz); // read byte
extern uint8_t stream_peek(const CByteStream_t* thizz); // get the next byte, but leave it in the stream
extern uint8_t stream_put(CByteStream_t* thizz, uint8_t data); // write byte
extern uint8_t stream_try_put(CByteStream_t *thizz, uint8_t data);
extern uint8_t stream_can_write(const CByteStream_t* thizz); // get permitted ops
extern uint8_t stream_can_read(const CByteStream_t* thizz); // get permitted ops
extern uint8_t stream_is_unbuffered_pending(const CByteStream_t* thizz); // return true if the stream is unbuffered and not pending

extern uint8_t stream_is_pending(const CByteStream_t* thizz); // return true if the stream is unbuffered and not pending
extern uint8_t stream_is_processing(const CByteStream_t* thizz); // return true if the stream is unbuffered and not pending
extern uint8_t stream_is_unbuffered(const CByteStream_t* thizz); // return true if the stream is unbuffered and not pending

#ifdef __cplusplus
}

#endif

#endif //SCHEDULER_STREAM_H
