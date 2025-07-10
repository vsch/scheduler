
#ifndef ARDUINOPROJECTMODULE_DEBUG_CBYTEBUFFER_H
#define ARDUINOPROJECTMODULE_DEBUG_CBYTEBUFFER_H

#include <stdint.h>     //uint8_t type
#include "common_defs.h"

#define BUFFER_PUT_REVERSE (0x01) 

// Simple buffering for received data with reversal storage
typedef struct CByteBuffer {
    uint8_t flags;
    uint8_t nPos;
    uint8_t nSize;
    uint8_t *pData;
} CByteBuffer_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void buffer_init(CByteBuffer_t *thizz, uint8_t flags, void *pData, uint8_t nSize);
extern void buffer_copy(CByteBuffer_t *thizz, CByteBuffer_t *other);
extern uint8_t buffer_is_empty(const CByteBuffer_t *thizz); // test if room for more data to write
extern uint8_t buffer_is_full(const CByteBuffer_t *thizz); // test if room for more data to write
extern uint8_t buffer_count(const CByteBuffer_t *thizz); // capacity to accept written bytes
extern uint8_t buffer_capacity(const CByteBuffer_t *thizz); // capacity to accept written bytes
extern void buffer_put(CByteBuffer_t *thizz, uint8_t data); // write byte

#ifdef __cplusplus
}

#endif

#endif //ARDUINOPROJECTMODULE_DEBUG_CBYTEBUFFER_H
