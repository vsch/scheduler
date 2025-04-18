
#ifndef SCHEDULER_STREAMS_H
#define SCHEDULER_STREAMS_H

#include "Arduino.h"
#include "Stream.h"
#include "Queue.h"
#include "new.h"

struct ByteStream : protected Queue {
    friend class Queue;
    friend class Controller;
    uint8_t flags;
    uint8_t addr;

public:
    ByteStream(Queue *pByteQueue, uint8_t streamFlags) : Queue(*pByteQueue) {
        flags = streamFlags;
        addr = 0;
    }

    inline void setAddress(uint8_t addr) {
        this->addr = addr;
    }

    inline uint8_t getAddress() const {
        return addr;
    }

    // set flags other than address and rd/wr permissions
    uint8_t setFlags(uint8_t flags, uint8_t mask);

    inline uint8_t getFlags() const { return flags; };

    inline uint8_t getFlags(uint8_t mask) const { return flags & mask; };

    inline uint8_t canRead() const { return getFlags(STREAM_FLAGS_RD); }

    inline uint8_t canWrite() const { return getFlags(STREAM_FLAGS_WR); }

    inline uint8_t isPending() const { return getFlags(STREAM_FLAGS_PENDING); }

    /**
     * Setup write stream for own buffer.
     *
     * @param pData     pointer to data
     * @param nSize     amount of data in buffer
     * @return 0 if all done, NULL_BYTE if not a write buffer
     */
    uint8_t setOwnBuffer(uint8_t *pData, uint8_t nSize);

    inline uint8_t is_empty() const { return isEmpty(); }

    inline uint8_t is_full() const { return isFull(); }

    inline uint8_t capacity() const { return getCapacity(); }

    inline uint8_t get() { return can_read() ? removeHead() : NULL_BYTE; }

    inline uint8_t peek() const { return can_read() ? peekHead() : NULL_BYTE; }

    inline uint8_t put(uint8_t data) { return can_write() ? addTail(data) : NULL_BYTE; }

#ifdef QUEUE_WORD_FUNCS
    // Word versions
    inline uint16_t getW() { return can_read() ? removeHeadW() : NULL_WORD; }

    inline uint16_t peekW() const { return can_read() ? peekHeadW() : NULL_WORD; }

    inline uint16_t putW(uint16_t data) { return can_write() ? addTailW(data) : NULL_WORD; }
#endif // QUEUE_WORD_FUNCS

    // status and maintenance functions
    inline uint8_t address() const { return getAddress(); }

    inline void set_address(uint8_t addr) { setAddress(addr); }

    inline uint8_t set_flags(uint8_t flags, uint8_t mask) { return setFlags(flags, mask); }

    inline uint8_t get_flags(uint8_t mask) { return getFlags(mask); }

    inline uint8_t can_read() const { return canRead(); }

    inline uint8_t can_write() const { return canWrite(); }

    // block must be at least sizeof(ByteStream) in size
    static ByteStream *construct(void *pBlock, Queue *pByteQueue, uint8_t flags) {
        return new(pBlock) ByteStream(pByteQueue, flags);
    }
};

#endif //SCHEDULER_STREAMS_H
