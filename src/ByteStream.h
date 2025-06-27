
#ifndef SCHEDULER_BYTESTREAM_H
#define SCHEDULER_BYTESTREAM_H

#include "Arduino.h"
#ifdef CONSOLE_DEBUG
#include "stdlib.h"
#include "new"
#else
#include <new.h>
#endif
#include "CByteStream.h"
#include "Queue.h"

struct ByteStream : protected Queue {
    friend class Queue;
    friend class Controller;
    uint8_t flags;
    /** Slave address byte (with read/write bit). in case of Twi */
    uint8_t addr; 
    uint8_t waitingTask; // if not NULL_TASK then index of task waiting for this request to complete

public:
    ByteStream(Queue *pByteQueue, uint8_t streamFlags);

    inline void setAddress(uint8_t addr) {
        this->addr = addr;
    }

    inline uint8_t getAddress() const {
        return addr;
    }
    
    void waitComplete();
    void triggerComplete();
    
    inline uint8_t getWaitingTask() const {
        return waitingTask;
    }
    
#ifdef SERIAL_DEBUG_TWI_DATA   
    void serialDebugDump(uint8_t id);
#else    
    inline void serialDebugDump() { }
#endif

    // set flags other than address and rd/wr permissions
    uint8_t setFlags(uint8_t flags, uint8_t mask);

    inline uint8_t getFlags() const { return flags; };

    inline uint8_t getFlags(uint8_t mask) const { return flags & mask; };
    inline uint8_t allFlags(uint8_t mask) const { return (flags & mask) == mask; };

    inline uint8_t canRead() const { return getFlags(STREAM_FLAGS_RD); }

    inline uint8_t canWrite() const { return getFlags(STREAM_FLAGS_WR); }

    inline uint8_t isPending() const { return getFlags(STREAM_FLAGS_PENDING); }
    
    inline uint8_t isProcessing() const { return getFlags(STREAM_FLAGS_PROCESSING); }
    
    inline uint8_t isUnbuffered() const { return getFlags(STREAM_FLAGS_UNBUFFERED); }

    inline uint8_t isUnbufferedPending() const { return allFlags( STREAM_FLAGS_PENDING | STREAM_FLAGS_UNBUFFERED); }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
    ByteStream *getStream(ByteStream *pOther, uint8_t flags);
#pragma clang diagnostic pop
    
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
    
    inline uint8_t count() const { return getCount(); }

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

    void pgmByteList(const uint8_t *bytes, uint16_t count);

#ifdef CONSOLE_DEBUG
    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);
#endif
};

#endif //SCHEDULER_BYTESTREAM_H
