#include "Arduino.h"
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

#ifdef CONSOLE_DEBUG

// print out queue for testing
void ByteStream::dump(char *buffer, uint32_t sizeofBuffer) {
    uint32_t len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;

    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    snprintf(buffer, sizeofBuffer, "Stream { flags:%c%c nSize:%d, nHead:%d, nTail:%d\n", canRead() ? 'R':' ', canWrite() ? 'W':' ', nSize, nHead, nTail);
    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "  isEmpty() = %d isFull() = %d getCount() = %d getCapacity() = %d\n", isEmpty(), isFull(), getCount(), getCapacity());

    for (uint16_t i = 0; i < nSize; i++) {
        len = strlen(buffer);
        buffer += len;
        sizeofBuffer -= len;
        bool addSpace = false;
        bool hadHead = false;

        if (!(i % 16)) {
            if (i) {
                *buffer++ = '\n';
                sizeofBuffer -= 1;
            }

            *buffer++ = ' ';
            sizeofBuffer -= 1;
        }

        if (i == nHead) {
            *buffer++ = ' ';
            *buffer++ = '[';
            sizeofBuffer -= 2;
            hadHead = true;
        }

        if (i == nTail) {
            *buffer++ = ']';
            *buffer++ = ' ';
            sizeofBuffer -= 2;
        } else {
            addSpace = !hadHead;
        }

        if (addSpace) {
            *buffer++ = ' ';
            sizeofBuffer--;
        }

        snprintf(buffer, sizeofBuffer, "0x%2.2x", pData[i]);

        len = strlen(buffer);
        buffer += len;
        sizeofBuffer -= len;
    }

    *buffer++ = '\n';
    *buffer++ = '}';
    *buffer++ = '\n';
    sizeofBuffer -= 3;
    *buffer = '\0';
}

#endif
