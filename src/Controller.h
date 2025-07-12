#ifndef SCHEDULER_CONTROLLER_H
#define SCHEDULER_CONTROLLER_H

#include "Arduino.h"
#include "ByteStream.h"
#include "ByteQueue.h"
#include "Mutex.h"

#ifdef INCLUDE_TWI_INT
#include "twiint.h"
#endif

#ifdef SERIAL_DEBUG_TWI_TRACER

#include "TraceBuffer.h"

extern TraceBuffer twiTraceBuffer;
#endif

#define DIV_ROUNDED_UP(v, d)            (((v)+(d)-1)/(d))
#define RESERVATIONS_PACK(r, b)         ((((r & 0x07) - 1) << 5) | (DIV_ROUNDED_UP(b,8) > 0x1f ? 0x1f : DIV_ROUNDED_UP(b,8)))
#define RESERVATIONS_BYTES(b)           ((DIV_ROUNDED_UP(b,8) > 0x1f ? 0x1f : DIV_ROUNDED_UP(b,8))*8)
#define RESERVATIONS_UNPACK_REQ(p)      ((((p) >> 5) & 0x07) + 1)
#define RESERVATIONS_UNPACK_BYTES(p)    (((p) & 0x1f)*8)

#define PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)    (sizeOfQueue(maxStreams, uint8_t))
#define COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)       (sizeOfQueue(maxStreams, uint8_t))
#define FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)       (sizeOfQueue(maxStreams, uint8_t))
#define WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize)            (sizeOfQueue(writeBufferSize, uint8_t))
#define RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize)        (sizeOfQueue(maxTasks, uint8_t))
#define REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize)        (sizeOfQueue(maxTasks, uint8_t))
#define WRITE_STREAM_SIZE(maxStreams, maxTasks, writeBufferSize)            (0)
#define READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize)       (sizeOfArray(maxStreams, ByteStream))

#define PENDING_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize)    (0)
#define COMPLETED_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize)       (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
#define FREE_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize)       (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
#define WRITE_BUFFER_OFFS(maxStreams, maxTasks, writeBufferSize)            (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
#define RESERVATION_LOCK_OFFS(maxStreams, maxTasks, writeBufferSize)        (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize))
#define REQUIREMENT_LIST_OFFS(maxStreams, maxTasks, writeBufferSize)        (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize) + RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize))
#define WRITE_STREAM_OFFS(maxStreams, maxTasks, writeBufferSize)            (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize) + RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize) + REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize))
#define READ_STREAM_TABLE_OFFS(maxStreams, maxTasks, writeBufferSize)       (PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize) + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize) + RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize) + REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize) + WRITE_STREAM_SIZE(maxStreams, maxTasks, writeBufferSize))

// Use this macro to allocate space for all the queues and buffers in the controller
#define sizeOfControllerBuffer(maxStreams, maxTasks, writeBufferSize) (0\
        + PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)  /* pendingReadStreams */ \
        + COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)     /* completedStreams */ \
        + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)     /* freeReadStreams */ \
        + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize)          /* writeBuffer */ \
        + RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize)      /* reservationLock */ \
        + REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize)      /* requirementList */      \
        + WRITE_STREAM_SIZE(maxStreams, maxTasks, writeBufferSize)          /* writeStream actual read streams added to queues */ \
        + READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize)     /* readStreamTable actual read streams added to queues */ \
      )

#define CTR_FLAGS_REQ_AUTO_START      (0x01)          // auto start requests when process request is called, default

class Controller : public Task {
protected:
    ByteQueue pendingReadStreams;   // requests waiting to be handleProcessedRequest
    ByteQueue completedStreams;     // requests already processed
    ByteQueue freeReadStreams;      // requests for processing available
    ByteQueue writeBuffer;          // shared write byte buffer
    Mutex reservationLock;          // reservationLock for requests and buffer write, first task will resume when resources it requested in willRequire() become available
    ByteQueue requirementList;      // byte queue of requirementList: max 8 reservationLock and 31*8 buffer, b7:b5+1 is reservationLock, B4:b0*8 = 248 bytes see Note below.
    ByteStream writeStream;         // write stream, must be requested and released in the same task invocation or pending data will not be handleProcessedRequest
    ByteStream *readStreamTable;    // pointer to first element in array of ByteSteam entries FIFO basis

    uint8_t maxStreams;
    uint8_t maxTasks;
    uint8_t writeBufferSize;
    uint8_t lastFreeHead;  // where next processing head position is  
    uint8_t flags;

public:
#ifdef RESOURCE_TRACE
    // allow tracing of resource requirements
    uint8_t startStreams;
    uint8_t startTasks;
    uint8_t startBufferSize;

    uint8_t usedStreams;
    uint8_t usedTasks;
    uint8_t usedBufferSize;
#endif

    /**
     * Construct a controller
     * @param pData             pointer to block of data for controller needs, one data block chopped up for all needs
     * @param maxStreams        maximum number of request streams needed, at least max of requests in willRequire() calls
     * @param maxTasks          maximum number of tasks making reservationLock and possibly being suspended
     * @param writeBufferSize   maximum buffer for write requests, at least max of bytes in willRequire() calls
     */
    /* @formatter:off */     
    Controller(uint8_t *pData, uint8_t maxStreams, uint8_t maxTasks, uint8_t writeBufferSize, uint8_t flags = CTR_FLAGS_REQ_AUTO_START)

            : pendingReadStreams(pData + PENDING_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize), PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
            , completedStreams(pData + COMPLETED_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize), COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
            , freeReadStreams(pData + FREE_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize), FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
            , writeBuffer(pData + WRITE_BUFFER_OFFS(maxStreams, maxTasks, writeBufferSize), WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize))
            , reservationLock(pData + RESERVATION_LOCK_OFFS(maxStreams, maxTasks, writeBufferSize), RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize))
            , requirementList(pData + REQUIREMENT_LIST_OFFS(maxStreams, maxTasks, writeBufferSize), REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize))
            , writeStream(&writeBuffer, 0)
			, readStreamTable(reinterpret_cast<ByteStream *>(pData + READ_STREAM_TABLE_OFFS(maxStreams, maxTasks, writeBufferSize)))
            , maxStreams(maxStreams)
            , maxTasks(maxTasks)
            , writeBufferSize(writeBufferSize), flags(flags) {
    /* @formatter:on */
        // now initialize all the read Streams

        //printf("pendingReadStreams %p 0x%4.4lx %p\n", pendingReadStreams.pData, PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize), pendingReadStreams.pData + PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("freeReadStreams %p 0x%4.4lx %p\n", freeReadStreams.pData, FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize), freeReadStreams.pData + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("writeBuffer %p 0x%4.4lx %p\n", writeBuffer.pData, WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize), writeBuffer.pData + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("reservationLock %p 0x%4.4lx %p\n", reservationLock.queue.pData, RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize), reservationLock.queue.pData + RESERVATION_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("requirementList %p 0x%4.4lx %p\n", requirementList.pData, REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize), requirementList.pData + REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("readStreamTable %p 0x%4.4lx %p\n", readStreamTable, READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize), (uint8_t *)(readStreamTable) + READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize));

        for (int i = 0; i < maxStreams; i++) {
            ByteStream *pStream = readStreamTable + i;
            ByteStream::construct(pStream, &writeBuffer, 0);
            freeReadStreams.addTail(i);
        }
        lastFreeHead = writeBuffer.nHead;

#ifdef RESOURCE_TRACE
        startStreams = 0;
        startTasks = 0;
        startBufferSize = 0;
        usedStreams = 0;
        usedTasks = 0;
        usedBufferSize = 0;
#endif
    }

    void reset() {
        cli();

        pendingReadStreams.reset();
        completedStreams.reset();
        sei();

        freeReadStreams.reset();

        writeBuffer.reset();
        reservationLock.reset();
        requirementList.reset();
        writeStream.reset();

        for (int i = 0; i < maxStreams; i++) {
            ByteStream *pStream = readStreamTable + i;
            ByteStream::construct(pStream, &writeBuffer, 0);
            freeReadStreams.addTail(i);
        }
        lastFreeHead = writeBuffer.nHead;
    }

#ifdef RESOURCE_TRACE

    //    void startResourceTrace();
    //    void updateResourceTrace();
    //    void updateResourcePreLockTrace();
    //    void updateResourceLockTrace();
    void startResourceTrace() {
        startStreams = freeReadStreams.getCount();
        startTasks = reservationLock.queue.getCapacity();
        startBufferSize = writeBuffer.getCapacity();
    }

    void dumpResourceTrace(PGM_P id);
    
    void releaseResourceLock();

private:    
    void updateResourceTrace() {
        uint8_t tmp = freeReadStreams.getCount();
        if (tmp < startStreams) {
            tmp = startStreams - tmp;
            if (usedStreams < tmp) { usedStreams = tmp; }
        }

        tmp = writeBuffer.getCapacity();
        if (tmp < startBufferSize) {
            tmp = startBufferSize - tmp;
            if (usedBufferSize < tmp) { usedBufferSize = tmp; }
        }

        updateResourceLockTrace();
    }

    void updateResourceLockTrace() {
        uint8_t tmp = reservationLock.queue.getCapacity();
        if (tmp < startTasks) {
            tmp = startTasks - tmp;
            if (usedTasks < tmp) { usedTasks = tmp; }
        }
    }

    void updateResourcePreLockTrace() {
        uint8_t tmp = reservationLock.queue.getCapacity();
        if (tmp < startTasks + 1) {
            tmp = startTasks - tmp + 1;
            if (usedTasks < tmp) { usedTasks = tmp; }
        }
    }
    
public:
#else

    inline void startResourceTrace() {}
    inline void dumpResourceTrace() {}

    // inline void updateResourceTrace() {}
    // inline void updateResourcePreLockTrace() {}
    // inline void updateResourceLockTrace() {}

#endif

    uint8_t getReadStreamId(ByteStream *pStream) {
        return pStream - readStreamTable;
    }

    NO_DISCARD uint8_t isRequestAutoStart() const {
        return flags & CTR_FLAGS_REQ_AUTO_START;
    }

    NO_DISCARD uint8_t isTracePending() const {
        return twiint_flags & TWI_FLAGS_TRC_PENDING;
    }

    void setRequestAutoStart() {
        flags |= CTR_FLAGS_REQ_AUTO_START;
    }

    void clrRequestAutoStart() {
        flags &= ~CTR_FLAGS_REQ_AUTO_START;
    }

    ByteStream *getReadStream(uint8_t id) {
        return readStreamTable + id;
    }

    NO_DISCARD uint8_t requestCapacity() const {
        return freeReadStreams.getCount();
    }

    NO_DISCARD uint8_t byteCapacity() const {
        return writeBuffer.getCapacity();
    }

    /**
     * Specify how many resources this task invocation will require for processing.
     *
     * Reserve how many requests and how many adjBytes may be needed for processing.
     * If not enough then suspend task until they are available.
     *
     * CAVEAT: bytes will be adjusted to be between 0 and 31*8, or 0..248 which is the max which can be encoded
     *    in the chosen reservationLock encoding scheme.
     *
     * NOTE: if the request bytes > adjusted bytes or > writeBuffer size then asking for more than is possible, and it
     *  will fail every call. Asking for less than what will actually be required will cause requests to be
     *  dropped and adding bytes to fail.
     *
     * @param requests  number of requests that will be generated, ie. separate process requests.
     * @param bytes     maximum total number of bytes generated in all requests for this call
     * @return          result 0 if reserved, 1 if need to suspend() waiting for resources, or NULL_BYTE if
     *                  requirements can never be satisfied because it exceeds allocated
     *                  resources
     */

    uint8_t willRequire(uint8_t requests, uint8_t bytes);

    ByteStream *getWriteStream() {
        return writeBuffer.getStream(&writeStream, STREAM_FLAGS_WR);
    }

    ByteStream *getStreamRequest() {
        uint8_t head = freeReadStreams.removeHead();
        updateResourceTrace();
        ByteStream *pStream = readStreamTable + head;
        return pStream;
    }

    /**
     * Send given buffered data as multiple self-buffered twi requests.
     * 
     * @param addr      twi address, including read flag
     * @param pData     pointer to byte buffer, needs to be 1 byte longer than used.
     * @param nSize     length of data buffer, sent data will be nSize-1
     * @param pRcvBuffer   extra bytes at end of buffer available for accumulating received data
     * @return          pointer to last request, can be used to wait for completion of the send
     */
    ByteStream *processRequest(uint8_t addr, uint8_t *pData, uint8_t nSize, CByteBuffer_t *pRcvBuffer = NULL) {

        if (nSize > QUEUE_MAX_SIZE) {
            nSize = QUEUE_MAX_SIZE;
        }

        uint8_t head = freeReadStreams.removeHead();
        updateResourceTrace();
        ByteStream *pStream = readStreamTable + head;
        pStream->set_address(addr);
        pStream->pData = pData;
        pStream->nSize = nSize;
        pStream->nHead = 0;
        pStream->nTail = nSize ? nSize - 1 : 0;
        pStream->pRcvBuffer = pRcvBuffer;

        // configure twi flags
        // serialDebugPrintf_P(PSTR("addr 0x%2.2x\n"), addr);
        pStream->flags = (addr & 0x01 ? STREAM_FLAGS_WR : STREAM_FLAGS_RD) | STREAM_FLAGS_PENDING | STREAM_FLAGS_UNBUFFERED;
        // pStream->flags = STREAM_FLAGS_RD | STREAM_FLAGS_PENDING | STREAM_FLAGS_UNBUFFERED;

        // NOTE: protect from mods in interrupts mid-way through this code
        cli();
        // queue it for processing
        pendingReadStreams.addTail(head);
        if (isRequestAutoStart() && pendingReadStreams.getCount() == 1) {
            startNextRequest();
        } else {
            // otherwise checking will be done in endProcessingRequest or in loop() for completed previous requests
            // and new request processing started if needed
        }
        sei();

        // make sure loop task is enabled start our loop task to monitor its completion
        this->resume(0);

        return pStream;
    }

    /**
     * Accept given byte stream for processing.
     *
     * NOTE: the new read stream created from this write stream will have its head set to previous read request's tail
     *     so that it will send only what the previous request will not send. This only applies to non-own buffer
     *     streams which provide a block of data to send, outside the shared writeBuffer.
     *
     * @param pWriteStream       pointer to stream to process, will be reset to new reality if needed
     * @return                  pointer to read stream or NULL if not handleProcessedRequest because of lack of readStreams
     *                          as made in the willRequire() call.
     */
    ByteStream *processStream(ByteStream *pWriteStream, CByteBuffer_t *pRcvBuffer = NULL) {
        uint8_t nextFreeHead;       // where next request head position should start

        if (freeReadStreams.isEmpty()) {
            return NULL;
        }

        uint8_t head = freeReadStreams.removeHead();
        ByteStream *pStream = readStreamTable + head;

        // incorporate tail into buffer if not own buffered stream
        uint8_t isSharedBuffer = pWriteStream->pData == writeBuffer.pData;
        uint8_t startProcessing = 0;

        // NOTE: protect from mods in interrupts mid-way through this code
        cli();

        if (isSharedBuffer) {
            writeBuffer.updateStreamed(pWriteStream);
            nextFreeHead = pWriteStream->nTail;
        }

        // copy the write stream info into read stream for processing
        pWriteStream->getStream(pStream, isSharedBuffer ? STREAM_FLAGS_RD : STREAM_FLAGS_RD | STREAM_FLAGS_UNBUFFERED);

        if (isSharedBuffer) {
            // new read stream starts where last read stream left off
            pStream->nHead = lastFreeHead;
            lastFreeHead = nextFreeHead;

            updateResourceTrace();
        }

        // queue it for processing
        pStream->flags |= STREAM_FLAGS_PENDING;
        pStream->pRcvBuffer = pRcvBuffer;
        pendingReadStreams.addTail(head);

        if (isRequestAutoStart() && pendingReadStreams.getCount() == 1) {
            // first one, then no-one to start it up but here
            serialDebugTwiDataPrintf_P(PSTR("AutoStart req %d\n"), head);
            startProcessing = 1;
        } else {
            // otherwise checking will be done in endProcessingRequest or in loop() for completed previous requests
            // and new request processing started if needed
        }

        // need to reset the write stream for stuff moved to read stream, ie prepare it for more requests
        pWriteStream->nHead = pWriteStream->nTail;

        sei();

        if (startProcessing) {
            pStream->flags |= STREAM_FLAGS_PROCESSING;
            startProcessingRequest(pStream);
        }

        // make sure loop task is enabled start our loop task to monitor its completion
        this->resume(0);
        return pStream;
    }

    /**
         * Start processing given request. This should start the interrupt calls for handling TWI data. 
         * Any request in the pending request queue will automatically start when this request is completed.
         * 
         * @param pStream 
         */
    virtual void startProcessingRequest(ByteStream *pStream) = 0;

    void startNextRequest() {
        if (!pendingReadStreams.isEmpty() && !isTracePending()) {
            uint8_t nexHead = pendingReadStreams.peekHead();
            ByteStream *pNextStream = getReadStream(nexHead);
            if (!(pNextStream->isProcessing())) {
                pNextStream->flags |= STREAM_FLAGS_PROCESSING;
                startProcessingRequest(pNextStream);
            }
        }
    }

    /**
     * mark end of request processing by the interrupt, this should be the 
     * first request in the pending streams. 
     * 
     * IMPORTANT: called from interrupt so no cli/sei needed
     * 
     * @param pStream   stream processed
     */
    void endProcessingRequest(ByteStream *pStream) {
        pStream->flags &= ~(STREAM_FLAGS_PENDING | STREAM_FLAGS_PROCESSING);

        // make sure it is a shared request stream
        uint8_t id = getReadStreamId(pStream);
        if (id < maxStreams) {
            if (pStream->pData == writeBuffer.pData) {
                // at this point the buffer used by this request is no longer needed, so the buffer head can be moved to processed request tail.
                writeBuffer.nHead = pStream->nTail;
            }

            uint8_t head = pendingReadStreams.removeHead();
            completedStreams.addTail(head);

            // don't start next request if trace processing is pending
            if (isRequestAutoStart()) {
                // start processing next request
                startNextRequest();
            }

            // restart loop
            resume(0);
        }
    }

    // IMPORTANT: called with interrupts disabled, but they can be enabled inside the function 
    //     to allow TWI processing to proceed
    virtual void requestCompleted(ByteStream *pStream) {
    };

    // IMPORTANT: called with interrupts disabled
    void handleCompletedRequests() {
        while (!completedStreams.isEmpty()) {
            const uint8_t head = completedStreams.removeHead();
            ByteStream *completedStream = getReadStream(head);
            // serialDebugTwiDataPrintf_P(PSTR("Completing Request: %d \n"), head);

            requestCompleted(completedStream);

            // put its handled info back to writeBuffer and it back in the free queue
            // unless it is an own buffer request
            completedStream->triggerComplete();
            completedStream->flags = 0;
            completedStream->pRcvBuffer = NULL;
            freeReadStreams.addTail(getReadStreamId(completedStream));
        }
    }

    void begin() override;

    void loop() override;

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    virtual void dump(uint8_t indent, uint8_t compact);

#endif
};


#ifdef RESOURCE_TRACE
#define resourceTracePrintf_P(...) printf_P(__VA_ARGS__)
#define resourceTracePuts_P(...) puts_P(__VA_ARGS__)
#else
#define resourceTracePrintf_P(...) ((void)0)
#define resourceTracePuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_RESOURCE_TRACE
#define serialDebugResourceTracePrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugResourceTracePuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugResourceTracePrintf_P(...) ((void)0)
#define serialDebugResourceTracePuts_P(...) ((void)0)
#endif

#endif //SCHEDULER_CONTROLLER_H
