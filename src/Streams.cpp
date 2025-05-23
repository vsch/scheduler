#include "Streams.h"

uint8_t ByteStream::setFlags(uint8_t flags, uint8_t mask) {
    mask &= ~(STREAM_FLAGS_RD_WR);
    this->flags &= ~mask;
    this->flags |= flags & mask;
    return this->flags;
}

uint8_t ByteStream::setOwnBuffer(uint8_t *pData, uint8_t nSize) {
    if (can_write()) {
        this->nTail = this->nSize = nSize;
        this->nHead = 0;
        this->pData = pData;
        return 0;
    }
    return NULL_BYTE;
}

// test if any more data to read
uint8_t stream_is_empty(const ByteStream_t *thizz) {
    return thizz->is_empty();
}

// test if room for more data to write
uint8_t stream_is_full(const ByteStream_t *thizz) {
    return thizz->is_full();
}

// capacity to accept written bytes
uint8_t stream_capacity(const ByteStream_t *thizz) {
    return thizz->capacity();
}

// read byte
uint8_t stream_get(ByteStream_t *thizz) {
    if (thizz->can_read()) {
        return thizz->get();
    }
    return NULL_BYTE;
}

// get the next byte, but leave it in the stream
uint8_t stream_peek(const ByteStream_t *thizz) {
    if (thizz->can_read()) {
        return thizz->peek();
    }
    return NULL_BYTE;
}

// write byte
uint8_t stream_put(ByteStream_t *thizz, uint8_t data) {
    if (thizz->can_write()) {
        return thizz->put(data);
    }
    return NULL_BYTE;
}

uint8_t stream_can_write(const ByteStream_t *thizz) {
    return thizz->can_write();
}

uint8_t stream_can_read(const ByteStream_t *thizz) {
    return thizz->can_read();
}

// get address from flags
uint8_t stream_address(const ByteStream_t *thizz) {
    return thizz->address();
}
