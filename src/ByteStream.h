
#ifndef SCHEDULER_BYTESTREAM_H
#define SCHEDULER_BYTESTREAM_H

#include "Arduino.h"

#ifdef CONSOLE_DEBUG
#include <time.h>
#include "stdlib.h"
#include "new"
#else

#include <new.h>

#endif

#include "CByteStream.h"
#include "CByteBuffer.h"
#include "ByteQueue.h"

struct ByteStream : protected ByteQueue {
    friend class ByteQueue;
    friend class Controller;
    friend class TwiController2;

    volatile uint8_t flags;
    uint8_t addr;               // Slave address byte (with read/write bit). in case of Twi
    void *pCallbackParam;                   // an arbitrary parameter to be used by callback function.
    CTwiCallback_t fCallback;
#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    time_t startTime;                       // if it is 0, then twiint will set it to micros() on start of processing, else it will be left as is
#endif

    uint8_t nRdSize;
    uint8_t *pRdData;
    // IMPORTANT: above fields must be the same as in CByteStream

public:
    ByteStream(ByteQueue *pByteQueue, uint8_t streamFlags);

    void reset();

    inline void setAddress(uint8_t addr) {
        this->addr = addr;
    }

    inline uint8_t getAddress() const {
        return addr;
    }

    inline void setCallback(CTwiCallback_t fCallback, void *pCallbackParam = NULL) {
        this->fCallback = fCallback;
        this->pCallbackParam = pCallbackParam;
    }

    inline void *getCallbackParam() {
        return pCallbackParam;
    }

    inline void triggerCallback() const {
        if (fCallback) fCallback((const CByteStream_t *)this);
    }

#ifdef SERIAL_DEBUG_DUMP
    void serialDebugDump(uint8_t id);
#else
    inline void serialDebugDump(uint8_t id) { }
#endif

    // set flags other than address and rd/wr permissions
    uint8_t setFlags(uint8_t flags, uint8_t mask);

    NO_DISCARD inline uint8_t getFlags() const { return flags; };

    NO_DISCARD inline uint8_t getFlags(uint8_t mask) const { return flags & mask; };
    NO_DISCARD inline uint8_t allFlags(uint8_t mask) const { return (flags & mask) == mask; };

    NO_DISCARD inline uint8_t canRead() const { return getFlags(STREAM_FLAGS_RD); }

    NO_DISCARD inline uint8_t canWrite() const { return getFlags(STREAM_FLAGS_WR); }

    NO_DISCARD inline uint8_t isPending() const { return getFlags(STREAM_FLAGS_PENDING); }

    NO_DISCARD inline uint8_t isProcessing() const { return getFlags(STREAM_FLAGS_PROCESSING); }

    NO_DISCARD inline uint8_t isUnbuffered() const { return getFlags(STREAM_FLAGS_UNBUFFERED); }

    NO_DISCARD inline uint8_t isAppend() const { return getFlags(STREAM_FLAGS_APPEND); }

    NO_DISCARD inline uint8_t isUnbufferedPending() const { return allFlags(STREAM_FLAGS_PENDING | STREAM_FLAGS_UNBUFFERED); }

    NO_DISCARD inline uint8_t isUnprocessed() const { return getFlags(STREAM_FLAGS_UNPROCESSED); }

    void getStream(ByteStream *pOther, uint8_t rdWrFlags);

    /**
     * Setup write stream for own buffer.
     *
     * @param pData     pointer to data
     * @param nSize     amount of data in buffer
     * @return 0 if all done, NULL_BYTE if not a write buffer
     */
    void setOwnBuffer(uint8_t *pData, uint8_t nSize);
    void setRdBuffer(uint8_t rdReverse, uint8_t *pRdData, uint8_t nRdSize);

    NO_DISCARD inline uint8_t is_empty() const { return isEmpty(); }

    NO_DISCARD inline uint8_t is_full() const { return isFull(); }

    NO_DISCARD inline uint8_t capacity() const { return getCapacity(); }

    NO_DISCARD inline uint8_t count() const { return getCount(); }

    NO_DISCARD inline uint8_t get() { return can_read() ? removeHead() : NULL_BYTE; }

    NO_DISCARD inline uint8_t peek() const { return can_read() ? peekHead() : NULL_BYTE; }

    inline uint8_t put(uint8_t data) { return can_write() ? addTail(data) : NULL_BYTE; }

    inline uint8_t try_put(uint8_t data) {
        if (canWrite() && isUnbuffered() && !isFull()) {
            addTail(data);
            return 1;
        } else {
            return 0;
        }
    }

#ifdef QUEUE_WORD_FUNCS

    // Word versions
    inline uint16_t getW() { return can_read() ? removeHeadW() : NULL_WORD; }

    NO_DISCARD inline uint16_t peekW() const { return can_read() ? peekHeadW() : NULL_WORD; }

    inline uint16_t putW(uint16_t data) { return can_write() ? addTailW(data) : NULL_WORD; }

#endif // QUEUE_WORD_FUNCS

    // status and maintenance functions
    NO_DISCARD inline uint8_t address() const { return getAddress(); }

    inline void set_address(uint8_t addr) { setAddress(addr); }

    inline uint8_t set_flags(uint8_t flags, uint8_t mask) { return setFlags(flags, mask); }

    inline uint8_t get_flags(uint8_t mask) { return getFlags(mask); }

    NO_DISCARD inline uint8_t can_read() const { return canRead(); }

    NO_DISCARD inline uint8_t can_write() const { return canWrite(); }

    // block must be at least sizeof(ByteStream) in size
    static ByteStream *construct(void *pBlock, ByteQueue *pByteQueue, uint8_t flags) {
        return new(pBlock) ByteStream(pByteQueue, flags);
    }

    void pgmByteList(const uint8_t *bytes, uint16_t count);

#ifdef CONSOLE_DEBUG
    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);
#endif
};

#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
#define serialDebugTwiReqTimingPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugTwiReqTimingPuts_P(...) puts_P(__VA_ARGS__)
#else
#ifdef CONSOLE_DEBUG
#define serialDebugPrintf_P(...) addActualOutput(__VA_ARGS__)
#endif
#endif

#endif //SCHEDULER_BYTESTREAM_H
