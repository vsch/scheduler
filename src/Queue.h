#ifndef SCHEDULER_QUEUE_H
#define SCHEDULER_QUEUE_H

#include <stdint.h>
#include "common_defs.h"

#define sizeOfQueue(s, t)            (sizeOfArray((s)+1, t))
#define sizeOfByteQueue(s)          (sizeOfQueue((s), uint8_t))

// #define QUEUE_BLOCK_FUNCS
#define QUEUE_WORD_FUNCS
#define QUEUE_DEDICATED_WORD_FUNCS

#ifndef QUEUE_BLOCK_FUNCS
#ifdef QUEUE_WORD_FUNCS
#define QUEUE_DEDICATED_WORD_FUNCS
#endif
#endif

class Stream;
class Controller;

/**
 * Queuing class which stores data in byte format but has word access methods for making it a word based queue.
 */

class Queue {
    friend class Stream;
    friend class Controller;

    uint8_t nSize;
    uint8_t nHead;
    uint8_t nTail;
    uint8_t *pData;

public:
    Queue(uint8_t *pData, uint8_t nSize);

    inline void empty() {
        nHead = nTail = 0;
    }

    uint8_t getCount() const;
    uint8_t peekHead(uint8_t offset) const;
    uint8_t peekTail(uint8_t offset) const;

    uint8_t peekHead() const { return peekHead(0); }

    uint8_t peekTail() const { return peekTail(0); }

    // enqueue/dequeue methods
    uint8_t addTail(uint8_t data);
    uint8_t addHead(uint8_t data);
    uint8_t removeTail();
    uint8_t removeHead();

    inline uint8_t getSize() const { return nSize - 1; }

    inline uint8_t getCapacity() const { return nSize - getCount() - 1; }

    inline uint8_t isEmpty() const { return nHead == nTail; }

    inline uint8_t isFull() const { return getCount() + 1 == nSize; }

    // version which allows testing if there is enough room for given number of bytes
    inline uint8_t isFull(uint8_t toAdd) const { return getCount() + toAdd == nSize; }

    inline uint8_t isEmpty(uint8_t toRemove) const { return toRemove ? getCount() < toRemove : isEmpty(); }

    inline uint8_t enqueue(uint8_t data) { return addTail(data); }

    inline uint8_t dequeue() { return removeHead(); }

    inline uint8_t push(uint8_t data) { return addTail(data); }

    inline uint8_t pop() { return removeTail(); }

    /*
     * multi-byte versions of methods, the data will be stored in the queue in such a way as to
     * preserve it on removal with a corresponding multi-byte get and compatible with little-endian memory storage. However
     * AT328p push/pop results in big-endian format for data stored on the stack. Low byte pushed first. So queue's push/pop are not
     * compatible in their memory image with regular push/pop instructions.
     *
     * The data will be written in little-endian format in memory.
     *
     * This means data  when storing with post increment (tail add) and getting with
     * post-increment (head remove), and reverse with storing with pre-decrement (head-add)
     * and get with pre-decrement (tail remove). The endianness of the data in memory
     * is preserved and all remove/add combinations will retrieve the same word.
     *
     * CAVEAT: multi-byte block versions will reverse order for addHead, removeTail and peekTail.
     *   Trying to add more bytes than room in the queue, or removing from an emtpy queue will do nothing and return.
     *   Trying to read more bytes than are in the queue, will fill in with the shortfall using 0.
     *   |
     *   All add versions will return the pointer passed in, even if nothing was done.
     *   All peek and remove versions will do nothing if the queue is empty.
     */

#ifdef QUEUE_BLOCK_FUNCS
    void *addTail(void *pVoid, uint8_t count);
    void *addHead(void *pVoid, uint8_t count);
    void *peekTail(void *pVoid, uint8_t count) const;
    void *removeTail(void *pVoid, uint8_t count);
    void *peekHead(void *pVoid, uint8_t count) const;
    void *removeHead(void *pVoid, uint8_t count);
#endif

#ifdef QUEUE_WORD_FUNCS
#ifdef QUEUE_DEDICATED_WORD_FUNCS
    uint16_t addTailW(uint16_t data);
    uint16_t addHeadW(uint16_t data);
    uint16_t removeTailW();
    uint16_t removeHeadW();
    uint16_t peekTailW() const;
    uint16_t peekHeadW() const;
#else
#ifdef QUEUE_BLOCK_FUNCS
    inline uint16_t addTailW(uint16_t data) { return *(uint16_t *) addTail(&data, 2); }

    inline uint16_t addHeadW(uint16_t data) { return *(uint16_t *) addHead(&data, 2); }

    inline uint16_t peekTailW() const {
        uint16_t data = 0;
        return *(uint16_t *) peekTail(&data, 2);
    }

    inline uint16_t removeTailW() {
        uint16_t data = 0;
        return *(uint16_t *) removeTail(&data, 2);
    }

    inline uint16_t peekHeadW() const {
        uint16_t data = 0;
        return *(uint16_t *) peekHead(&data, 2);
    }

    inline uint16_t removeHeadW() {
        uint16_t data = 0;
        return *(uint16_t *) removeHead(&data, 2);
    }
#endif

#endif // QUEUE_DEDICATED_WORD_FUNCS
#endif // QUEUE_WORD_FUNCS

#ifdef QUEUE_WORD_FUNCS

    inline uint16_t enqueueW(uint16_t data) { return addTailW(data); }

    inline uint16_t dequeueW() { return removeHeadW(); }

    inline uint16_t pushW(uint16_t data) { return addTailW(data); }

    inline uint16_t popW() { return removeTailW(); }

#endif // QUEUE_WORD_FUNCS

    uint8_t updateQueued(Queue *pOther, uint8_t flags);
    uint8_t updateStreamed(Stream *pOther);

    Stream *getStream(Stream *pOther, uint8_t flags);

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    void dump(char *buffer, uint32_t sizeofBuffer, uint8_t indent, uint8_t compact);
#endif
};

#endif //SCHEDULER_QUEUE_H
