#include <stddef.h>
#include "CByteBuffer.h"

void buffer_init(CByteBuffer_t *thizz, uint8_t flags, void *pData, uint8_t nSize) {
    thizz->flags = flags;
    thizz->nPos = flags & BUFFER_PUT_REVERSE ? nSize : 0;
    thizz->nSize = nSize;
    thizz->pData = pData;
}

void buffer_copy(CByteBuffer_t *thizz, CByteBuffer_t *other) {
    if (other) {
        thizz->flags = other->flags;
        thizz->nPos = other->nPos;
        thizz->nSize = other->nSize;
        thizz->pData = other->pData;
    } else {
        thizz->flags = 0;
        thizz->nPos = 0;
        thizz->nSize = 0;
        thizz->pData = NULL;
    }
}

// test if any more data to read
uint8_t buffer_is_empty(const CByteBuffer_t *thizz) {
    return thizz->flags & BUFFER_PUT_REVERSE ? thizz->nPos == thizz->nSize : !thizz->nPos;
}

// test if room for more data to write
uint8_t buffer_is_full(const CByteBuffer_t *thizz) {
    return thizz->flags & BUFFER_PUT_REVERSE ? !thizz->nPos : thizz->nPos == thizz->nSize;
}

// capacity to accept written bytes
uint8_t buffer_capacity(const CByteBuffer_t *thizz) {
    return thizz->flags & BUFFER_PUT_REVERSE ? thizz->nPos : thizz->nSize - thizz->nPos;
}

uint8_t buffer_count(const CByteBuffer_t *thizz) {
    return thizz->flags & BUFFER_PUT_REVERSE ? thizz->nSize - thizz->nPos : thizz->nPos;
}

// write byte
void buffer_put(CByteBuffer_t *thizz, uint8_t data) {
    if (!buffer_is_full(thizz)) {
        if (thizz->flags & BUFFER_PUT_REVERSE) {
            thizz->pData[--thizz->nPos] = data;
        } else {
            thizz->pData[thizz->nPos++] = data;
        }
    }
}

