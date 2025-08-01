#ifndef SCHEDULER_CONTROLLER_H
#define SCHEDULER_CONTROLLER_H

#include "Arduino.h"
#include "ByteStream.h"
#include "ByteQueue.h"
#include "Res2Lock.h"

#include "twiint.h"
#include "ResourceUse.h"

#ifdef SERIAL_DEBUG_TWI_TRACER

#include "TraceBuffer.h"

extern TraceBuffer twiTraceBuffer;
#endif

// @formatter:off
#define CTRL_PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)   (sizeOfQueue(maxStreams, uint8_t))
#define CTRL_COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)      (sizeOfQueue(maxStreams, uint8_t))
#define CTRL_FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize)      (sizeOfQueue(maxStreams, uint8_t))
#define CTRL_RESOURCE_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize)          (sizeOfRes2LockBuffer(maxTasks))
#define CTRL_WRITE_STREAM_SIZE(maxStreams, maxTasks, writeBufferSize)           (0)
#define CTRL_READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize)      (sizeOfArray(maxStreams, ByteStream))
#define CTRL_WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize)           (sizeOfQueue(writeBufferSize, uint8_t))

#define CTRL_PENDING_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize)   (0)
#define CTRL_COMPLETED_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize)      (CTRL_PENDING_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
#define CTRL_FREE_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize)      (CTRL_COMPLETED_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
#define CTRL_RESOURCE_LOCK_OFFS(maxStreams, maxTasks, writeBufferSize)          (CTRL_FREE_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
#define CTRL_WRITE_STREAM_OFFS(maxStreams, maxTasks, writeBufferSize)           (CTRL_RESOURCE_LOCK_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_RESOURCE_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize))
#define CTRL_READ_STREAM_TABLE_OFFS(maxStreams, maxTasks, writeBufferSize)      (CTRL_WRITE_STREAM_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_WRITE_STREAM_SIZE(maxStreams, maxTasks, writeBufferSize))
#define CTRL_WRITE_BUFFER_OFFS(maxStreams, maxTasks, writeBufferSize)           (CTRL_READ_STREAM_TABLE_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize))
#define CTRL_NEXT_MEMBER_OFFS(maxStreams, maxTasks, writeBufferSize)            (CTRL_WRITE_BUFFER_OFFS(maxStreams, maxTasks, writeBufferSize) + CTRL_WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize))
// @formatter:on

// Use this macro to allocate space for all the queues and buffers in the controller.
#define sizeOfControllerBuffer(maxStreams, maxTasks, writeBufferSize) (CTRL_NEXT_MEMBER_OFFS(maxStreams, maxTasks, writeBufferSize))

#define CTR_FLAGS_REQ_AUTO_START      (0x01)          // auto start requests when process request is called, default

class Controller : public Task {
protected:
    ByteQueue pendingReadStreams;   // requests waiting to be handleProcessedRequest
    ByteQueue completedStreams;     // requests already processed
    ByteQueue freeReadStreams;      // requests for processing available
    Res2Lock resourceLock;          // resourceLock for requests and buffer write, first task will resume when resources it requested in willRequire() become available
    ByteStream writeStream;         // write stream, must be requested and released in the same task invocation or pending data will not be handleProcessedRequest
    ByteStream *readStreamTable;    // pointer to first element in array of ByteSteam entries FIFO basis
    ByteQueue writeBuffer;          // shared write byte buffer

    uint8_t maxStreams;
    uint8_t maxTasks;
    uint8_t writeBufferSize;
    uint8_t lastFreeHead;  // where next processing head position is
    uint8_t flags;

public:
#ifdef RESOURCE_TRACE
    // allow tracing of resource requirements
    uint8_t usedStreams;
    uint8_t usedBufferSize;
    uint8_t lockedStreams;
    uint8_t lockedBufferSize;
#endif

    /**
     * Construct a controller
     * @param pData             pointer to block of data for controller needs, one data block chopped up for all needs
     * @param maxStreams        maximum number of request streams needed, at least max of requests in reserveResources() calls
     * @param maxTasks          maximum number of tasks making reservationLock and possibly being suspended
     * @param writeBufferSize   maximum buffer for write requests, at least max of bytes in reserveResources() calls
     */
    /* @formatter:off */
    Controller(uint8_t *pData, uint8_t maxStreams, uint8_t maxTasks, uint8_t writeBufferSize, uint8_t flags = CTR_FLAGS_REQ_AUTO_START)
            : pendingReadStreams(pData + CTRL_PENDING_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize), CTRL_PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
            , completedStreams(pData + CTRL_COMPLETED_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize), CTRL_COMPLETED_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
            , freeReadStreams(pData + CTRL_FREE_READ_STREAMS_OFFS(maxStreams, maxTasks, writeBufferSize), CTRL_FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize))
            , resourceLock(pData + CTRL_RESOURCE_LOCK_OFFS(maxStreams, maxTasks, writeBufferSize), maxTasks, maxStreams, writeBufferSize)
            , writeStream(&writeBuffer, 0)
            , readStreamTable(reinterpret_cast<ByteStream *>(pData + CTRL_READ_STREAM_TABLE_OFFS(maxStreams, maxTasks, writeBufferSize)))
            , writeBuffer(pData + CTRL_WRITE_BUFFER_OFFS(maxStreams, maxTasks, writeBufferSize), CTRL_WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize))
            , maxStreams(maxStreams)
            , maxTasks(maxTasks)
            , writeBufferSize(writeBufferSize)
            , flags(flags) {
    /* @formatter:on */
        // now initialize all the read Streams

        //printf("pendingReadStreams %p 0x%4.4lx %p\n", pendingReadStreams.pData, PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize), pendingReadStreams.pData + PENDING_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("freeReadStreams %p 0x%4.4lx %p\n", freeReadStreams.pData, FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize), freeReadStreams.pData + FREE_READ_STREAMS_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("writeBuffer %p 0x%4.4lx %p\n", writeBuffer.pData, WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize), writeBuffer.pData + WRITE_BUFFER_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("reservationLock %p 0x%4.4lx %p\n", reservationLock.queue.pData, RESOURCE_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize), reservationLock.queue.pData + RESOURCE_LOCK_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("requirementList %p 0x%4.4lx %p\n", requirementList.pData, REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize), requirementList.pData + REQUIREMENT_LIST_SIZE(maxStreams, maxTasks, writeBufferSize));
        //printf("readStreamTable %p 0x%4.4lx %p\n", readStreamTable, READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize), (uint8_t *)(readStreamTable) + READ_STREAM_TABLE_SIZE(maxStreams, maxTasks, writeBufferSize));

        for (int i = 0; i < maxStreams; i++) {
            ByteStream *pStream = readStreamTable + i;
            ByteStream::construct(pStream, &writeBuffer, 0);
            freeReadStreams.addTail(i);
        }
        lastFreeHead = writeBuffer.nHead;

#ifdef RESOURCE_TRACE
        usedStreams = 0;
        usedBufferSize = 0;
        lockedStreams = 0;
        lockedBufferSize = 0;
#endif
    }

    void reset() {
        CLI();

        pendingReadStreams.reset();
        completedStreams.reset();
        resourceLock.reset();
        freeReadStreams.reset();
        writeBuffer.reset();
        writeStream.reset();

        for (int i = 0; i < maxStreams; i++) {
            ByteStream *pStream = readStreamTable + i;
            ByteStream::construct(pStream, &writeBuffer, 0);
            freeReadStreams.addTail(i);
        }

        lastFreeHead = writeBuffer.nHead;

#ifdef RESOURCE_TRACE
        usedStreams = 0;
        usedBufferSize = 0;
        lockedStreams = 0;
        lockedBufferSize = 0;
#endif
        SEI();
    }

#ifdef RESOURCE_TRACE

    void startResourceTrace() {
        usedStreams = 0;
        usedBufferSize = 0;
    }

    void dumpResourceTrace(ResourceUse *resourceUse, uint32_t *pLastDump = NULL, uint16_t dumpDelay = 0);

#else

    inline void startResourceTrace() {}

    inline void dumpResourceTrace(ResourceUse *resourceUse, uint32_t *pLastDump = NULL, uint16_t dumpDelay = 0) {}

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

    uint8_t reserveResources(uint8_t requests, uint8_t bytes);

    inline void releaseResources() {
        resourceLock.release();
    }

    ByteStream *getWriteStream();

    ByteStream *getStreamRequest() {
        uint8_t head = freeReadStreams.removeHead();
#ifdef RESOURCE_TRACE
        usedStreams++;
#endif
        CLI();
        resourceLock.useAvailable1(1);
        SEI();
        ByteStream *pStream = readStreamTable + head;
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
     *                          as made in the reserveResources() call.
     */
    ByteStream *processStream(ByteStream *pWriteStream);

    /**
         * Start processing given request. This should start the interrupt calls for handling TWI data.
         * Any request in the pending request queue will automatically start when this request is completed.
         *
         * @param pStream
         */
    virtual void startProcessingRequest(ByteStream *pStream) = 0;

    // IMPORTANT: called from interrupt so no cli/sei needed
    void startNextRequest() {
        CLI();
        if (!pendingReadStreams.isEmpty() && !isTracePending()) {
            uint8_t nextHead = pendingReadStreams.peekHead();
            ByteStream *pNextStream = getReadStream(nextHead);
            if (!(pNextStream->isProcessing())) {
                startProcessingRequest(pNextStream);
            }
        }
        SEI();
    }

    /**
     * mark end of request processing by the interrupt, this should be the
     * first request in the pending streams.
     *
     * IMPORTANT: called from interrupt so no cli/sei needed
     *
     * @param pStream   stream processed
     */
    void endProcessingRequest(ByteStream *pStream);

    void handleCompletedRequests();

    void begin() override;

    void loop() override;

#ifdef SERIAL_DEBUG_RESOURCE_DETAIL_TRACE
    void dumpReservationLockData();
#else

    inline void dumpReservationLockData() {}

#endif // SERIAL_DEBUG_RESOURCE_DETAIL_TRACE

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    virtual void dump(uint8_t indent, uint8_t compact);

#endif
};

#endif //SCHEDULER_CONTROLLER_H
