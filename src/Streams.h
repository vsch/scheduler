
#ifndef SCHEDULER_STREAMS_H
#define SCHEDULER_STREAMS_H

#include "Stream.h"
#include "ByteQueue.h"
#include "new.h"

struct ByteStream : public ByteQueue {
    uint16_t flags;

public:
    ByteStream(ByteQueue *pByteQueue, uint16_t streamFlags) : ByteQueue(*pByteQueue) {
        flags = streamFlags;
        // should be copied by the copy constructor
        // nSize = pByteQueue->nSize;
        // nHead = pByteQueue->nHead;
        // nTail = pByteQueue->nTail;
        // pData = pByteQueue->pData;
    }

    inline uint8_t is_empty() const { return isEmpty(); }

    inline uint8_t is_full() const { return isFull(); }

    inline uint16_t capacity() const { return getCapacity(); }

    inline uint8_t get() { return removeHead(); }

    inline uint8_t peek() const { return peekHead(); }

    inline uint8_t put(uint8_t data) { return addTail(data); }

    inline uint8_t address() const { return flags & STREAM_FLAGS_ADDRESS; }

    inline uint8_t canRead() const { return flags & STREAM_FLAGS_RD; }
    inline uint8_t can_read() const { return flags & STREAM_FLAGS_RD; }

    inline uint8_t canWrite() const { return flags & STREAM_FLAGS_WR; }
    inline uint8_t can_write() const { return flags & STREAM_FLAGS_WR; }

    // block must be at least sizeof(ByteStream) in size
    static ByteStream *construct(void *pBlock, ByteQueue *pByteQueue, uint16_t flags) {
        return new(pBlock) ByteStream(pByteQueue, flags);
    }
};

#endif //SCHEDULER_STREAMS_H
