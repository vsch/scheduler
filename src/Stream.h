#ifndef SCHEDULER_STREAM_H
#define SCHEDULER_STREAM_H

#include "common_defs.h"

#define STREAM_FLAGS_RD         (0x01)      // marks the stream as read enabled, can get from it
#define STREAM_FLAGS_WR         (0x02)      // marks the stream as write enabled, can put to it
#define STREAM_FLAGS_RD_WR      (0x03)      // marks the stream as write enabled, can put to it
#define STREAM_FLAGS_PENDING    (0x04)      // marks the stream as pending

// Simple streaming both read and write for use in C interrupts and C code, provided from C/C++ code
// has the same layout as Stream
typedef struct ByteStream
{
    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t* pData;
    uint8_t flags;
    uint8_t addr;
} ByteStream_t;

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t stream_is_empty(const ByteStream_t* thizz); // test if any more data to read
extern uint8_t stream_is_full(const ByteStream_t* thizz); // test if room for more data to write
extern uint8_t stream_capacity(const ByteStream_t* thizz); // capacity to accept written bytes
extern uint8_t stream_get(ByteStream_t* thizz); // read byte
extern uint8_t stream_peek(const ByteStream_t* thizz); // get the next byte, but leave it in the stream
extern uint8_t stream_put(ByteStream_t* thizz, uint8_t data); // write byte
extern uint8_t stream_address(const ByteStream_t* thizz); // get address from flags
extern uint8_t stream_can_write(const ByteStream_t* thizz); // get permitted ops
extern uint8_t stream_can_read(const ByteStream_t* thizz); // get permitted ops

#ifdef __cplusplus
}

#endif

#endif //SCHEDULER_STREAM_H
