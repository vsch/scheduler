#ifndef SCHEDULER_STREAM_H
#define SCHEDULER_STREAM_H

#ifdef CONSOLE_DEBUG
#include <time.h>
#endif

#include "common_defs.h"
#include "CByteQueue.h"
#include "CByteBuffer.h"

#define STREAM_FLAGS_BUFF_REVERSE   (0x01)      // when writing to rdbuffer, do it in reverse, same as BUFF_PUT_REVERSE
#define STREAM_FLAGS_RD             (0x02)      // marks the stream as read enabled, can get from it
#define STREAM_FLAGS_WR             (0x04)      // marks the stream as write enabled, can put to it
#define STREAM_FLAGS_PENDING        (0x08)      // marks the stream as pending or processing
#define STREAM_FLAGS_PROCESSING     (0x10)      // marks the stream as being processed
#define STREAM_FLAGS_UNBUFFERED     (0x20)      // marks the stream is unbuffered
#define STREAM_FLAGS_APPEND         (0x40)      // used in getStream to disable resetting content to 0 for write streams.
#define STREAM_FLAGS_UNPROCESSED    (0x80)      // cleared on write stream when processStream was called on it

#define STREAM_FLAGS_RD_WR          (STREAM_FLAGS_RD |  STREAM_FLAGS_WR)      // marks the stream as write enabled, can put to it
#define STREAM_FLAGS_RD_WR_APPEND   (STREAM_FLAGS_RD_WR |  STREAM_FLAGS_APPEND)      // marks the stream as write enabled, can put to it


#if STREAM_FLAGS_BUFF_REVERSE != BUFFER_PUT_REVERSE
#error STREAM_FLAGS_BUFF_REVERSE != BUFFER_PUT_REVERSE
#endif

// Simple streaming both read and write for use in C interrupts and C code, provided from C/C++ code
// has the same layout as ByteStream.
struct CByteStream;
typedef void (*CTwiCallback_t)(const struct CByteStream *pStream);

typedef struct CByteStream {
    CByteQueue_t byteQueue;
    uint8_t flags;
    uint8_t addr;
    void *pCallbackParam;                   // an arbitrary parameter to be used by callback function.
    CTwiCallback_t fCallback;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    time_t startTime;                       // if it is 0, then twiint will set it to micros() on start of processing, else it will be left as is
#endif

    uint8_t nRdSize;
    uint8_t *pRdData;
} CByteStream_t;


#ifdef __cplusplus
extern "C" {
#endif

// get the callback index for given callback and cache it (*pCache == 0 if cache not initialized, NULL_BYTE means not found, 0 will be returned)
extern uint8_t stream_get_callback_id(CTwiCallback_t callback, uint8_t *pCache);

extern uint8_t stream_is_empty(const CByteStream_t *thizz); // test if any more data to read
extern uint8_t stream_is_full(const CByteStream_t *thizz); // test if room for more data to write
extern uint8_t stream_capacity(const CByteStream_t *thizz); // capacity to accept written bytes
extern uint8_t stream_count(const CByteStream_t *thizz); // capacity to accept written bytes
extern uint8_t stream_get(CByteStream_t *thizz); // read byte
extern uint8_t stream_peek(const CByteStream_t *thizz); // get the next byte, but leave it in the stream
extern uint8_t stream_put(CByteStream_t *thizz, uint8_t data); // write byte
extern uint8_t stream_try_put(CByteStream_t *thizz, uint8_t data);
extern uint8_t stream_can_write(const CByteStream_t *thizz); // get permitted ops
extern uint8_t stream_can_read(const CByteStream_t *thizz); // get permitted ops
extern uint8_t stream_is_unbuffered_pending(const CByteStream_t *thizz); // return true if the stream is unbuffered and not pending

extern uint8_t stream_is_pending(const CByteStream_t *thizz); // return true if the stream is unbuffered and not pending
extern uint8_t stream_is_processing(const CByteStream_t *thizz); // return true if the stream is unbuffered and not pending
extern uint8_t stream_is_unbuffered(const CByteStream_t *thizz); // return true if the stream is unbuffered and not pending

extern void stream_set_own_buffer(const CByteStream_t *thizz, uint8_t *pData, uint8_t nSize); // return true if the stream is unbuffered and not pending
extern void stream_set_rd_buffer(const CByteStream_t *thizz, uint8_t rdReverse, uint8_t *pRdData, uint8_t nRdSize); // return true if the stream is unbuffered and not pending

#ifdef SERIAL_DEBUG
extern void stream_serial_debug_dump(const CByteStream_t *thizz, uint8_t id); // return true if the stream is unbuffered and not pending
#endif

#ifdef __cplusplus
}

#endif

#endif //SCHEDULER_STREAM_H
