
#ifndef SCHEDULER_CONTROLLER_H
#define SCHEDULER_CONTROLLER_H

#include "Streams.h"
#include "Queue.h"
#include "Mutex.h"

#define DIV_ROUNDED_UP(v, d)            (((v)+(d)-1)/(d))
#define RESERVATIONS_PACK(r, b)         ((((r & 0x07) - 1) << 5) | (DIV_ROUNDED_UP(b,8) > 0x1f ? 0x1f : DIV_ROUNDED_UP(b,8)))
#define RESERVATIONS_BYTES(b)           ((DIV_ROUNDED_UP(b,8) > 0x1f ? 0x1f : DIV_ROUNDED_UP(b,8))*8)
#define RESERVATIONS_UNPACK_REQ(p)      ((((p) >> 5) & 0x07) + 1)
#define RESERVATIONS_UNPACK_BYTES(p)    (((p) & 0x1f)*8)

class Controller : public Task {
    Queue pendingReadStreams;           // requests waiting to be handleProcessedRequest
    Queue freeReadStreams;              // requests for processing available
    ByteStream *readStreams;            // pointer to first element in array of ByteSteam entries
    Queue writeBuffer;                  // shared write byte buffer
    ByteStream writeStream;             // write stream, must be requested and released in the same task invocation or pending data will not be handleProcessedRequest
    Mutex reservationLock;              // reservationLock for requests and buffer writes
    Queue requirementList;                 // byte queue of requirementList: max 8 reservationLock and 31*8 buffer, b7:b5+1 is reservationLock, B4:b0*8 = 248 bytes see Note below.
    // FIFO basis, first task will resume when resources needed become available

public:
    /**
     * Construct a controller
     * @param pData             pointer to block of data for controller needs, one data block chopped up for all needs
     * @param maxStreams        maximum number of request streams needed, at least max of requests in willRequire() calls
     * @param maxTasks          maximum number of tasks making reservationLock and possibly being suspended
     * @param writeBufferSize   maximum buffer for write requests, at least max of bytes in willRequire() calls
     */
    Controller(uint8_t *pData, uint8_t maxStreams, uint8_t maxTasks, uint8_t writeBufferSize)

#define sizeOfController(maxStreams, maxTasks, writeBufferSize) (0\
      + sizeOfQueue(maxStreams, uint8_t)  /* pendingReadStreams */ \
      + sizeOfQueue(maxStreams, uint8_t) /* freeReadStreams */ \
      + sizeOfQueue(writeBufferSize, uint8_t) /* writeBuffer */ \
      + sizeOfQueue(maxTasks, uint8_t) /* reservationLock */ \
      + sizeOfQueue(maxTasks, uint8_t) /* requirementList */      \
      + sizeOfArray(maxStreams, ByteStream) /* actual read streams added to queues */ \

            : pendingReadStreams(pData, sizeOfQueue(maxStreams, uint8_t))
              , freeReadStreams(pData + sizeOfQueue(maxStreams, uint8_t), sizeOfQueue(maxStreams, uint8_t))
              , writeBuffer(pData + sizeOfQueue(maxStreams, uint8_t) + sizeOfQueue(maxStreams, uint8_t),
                            sizeOfQueue(writeBufferSize, uint8_t))
              , reservationLock(pData + sizeOfQueue(maxStreams, uint8_t) + sizeOfQueue(maxStreams, uint8_t) +
                                sizeOfQueue(writeBufferSize, uint8_t), sizeOfQueue(maxTasks, uint8_t))
              , requirementList(pData + sizeOfQueue(maxStreams, uint8_t) + sizeOfQueue(maxStreams, uint8_t) +
                                sizeOfQueue(writeBufferSize, uint8_t) + sizeOfQueue(maxTasks, uint8_t),
                                sizeOfQueue(maxTasks, uint8_t))
              , writeStream(&writeBuffer, 0) {

        // now initialize all the read Streams
        readStreams = (ByteStream *) (pData
                                      + sizeOfQueue(maxStreams, uint8_t)   /* pendingReadStreams */
                                      + sizeOfQueue(maxStreams, uint8_t)   /* freeReadStreams */
                                      + sizeOfQueue(writeBufferSize, uint8_t)   /* writeBuffer */
                                      + sizeOfQueue(maxTasks, uint8_t)   /* reservationLock */
                                      + sizeOfQueue(maxTasks, uint8_t));   /* requirementList */

        ByteStream *pStream = readStreams;

        for (int i = 0; i < maxStreams; i++) {
            ByteStream::construct(pStream, &writeBuffer, 0);
            freeReadStreams.addTail(i);
            pStream++;
        }
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

    uint8_t willRequire(uint8_t requests, uint8_t bytes) {
        uint8_t adjBytes = RESERVATIONS_BYTES(bytes);
        if (bytes > adjBytes || bytes > writeBuffer.getSize() || requests > 8) {
            // cannot ever satisfy these requirements
            return NULL_BYTE;
        }

        if (reservationLock.isFree()) {
            // NOTE: need special processing because there are no waiters. Either, we have the required resources
            //  and can obtain the reservationLock for the calling task. Otherwise, the reservationLock really needs to
            //  be taken by the controller so that it can release the lock and have the next task in line get the
            //  resource. However, this call is from the task's context, so we take the resource for it and transfer to
            //  the controller task then the call for this task's context will suspend the task and resume it when the
            //  controller task releases the lock because there are sufficient resources for the next task's processing.
            if (requests <= freeReadStreams.getCount() && adjBytes <= writeBuffer.getCapacity()) {
                // can proceed now, overwrite whatever was uncommitted in the write stream
                writeBuffer.getStream(&writeStream, STREAM_FLAGS_WR);
                reservationLock.reserve();
                return 0;
            }

            reservationLock.reserve();
            reservationLock.transfer(this);
        }

        // add to the queue of waiters
        requirementList.addTail(RESERVATIONS_PACK(requests, adjBytes));
        return reservationLock.reserve();
    }

    ByteStream *getWriteStream() {
        return writeBuffer.getStream(&writeStream, STREAM_FLAGS_WR);
    }

    void begin() override {
    }

    // to be implemented by subclasses, will be called from the default loop()
    virtual void checkForCompletedRequests() = 0;

    // start processing the given request
    virtual void startProcessingRequest(ByteStream *pStream) = 0;

    void handleProcessedRequest(ByteStream *pStream) {
        // put its handled info back to writeBuffer and it back in the free queue
        // unless it is an own buffer request
        if (pStream->pData == writeBuffer.pData) {
            writeBuffer.updateStreamed(pStream);
        }
        pStream->flags = 0;

        // this should be the one completed
        pendingReadStreams.removeHead();
        freeReadStreams.addTail(pStream - readStreams);
    }

    void loop() override {
        if (pendingReadStreams.getCount()) {
            checkForCompletedRequests();

            if (!pendingReadStreams.isEmpty()) {
                ByteStream *pStream = readStreams + pendingReadStreams.peekHead();
                if (!(pStream->isPending())) {
                    // its ready for processing and not already being processed
                    pStream->flags |= STREAM_FLAGS_PENDING;
                    startProcessingRequest(pStream);
                }
            }
        }

        if (!requirementList.isEmpty()) {
            // have some waiting
            uint8_t packed = requirementList.peekHead();
            uint8_t requests = RESERVATIONS_UNPACK_REQ(packed);
            uint8_t bytes = RESERVATIONS_UNPACK_BYTES(packed);

            if (requests <= freeReadStreams.getCount() && bytes <= writeBuffer.getCapacity()) {
                // let'er rip there are enough free resources
                requirementList.removeHead();
                reservationLock.release();
            }
        }

        if (!pendingReadStreams.getCount() && requirementList.isEmpty()) {
            // can suspend until there is something to check for.
            suspend();
        } else {
            resume(0);
        }
    }

    /**
     * Accept given byte stream for processing.
     *
     * NOTE: the new read stream created from this write stream will have its head set to previous read request's tail
     *     so that it will send only what the previous request will not send. This only applies to non-own buffer
     *     streams which provide a block of data to send, outside the shared writeBuffer.
     *
     * @param writeStream       pointer to stream to process, will be reset to new reality if needed
     * @return                  pointer to writeStream or NULL if not handleProcessedRequest because of lack of readStreams
     *                          as made in the willRequire() call.
     */
    ByteStream *processStream(ByteStream *writeStream) {
        if (freeReadStreams.isEmpty()) {
            return NULL;
        }

        ByteStream *pStream = readStreams + freeReadStreams.removeHead();

        // incorporate tail into buffer if not own buffered stream
        if (writeStream->pData == writeBuffer.pData) {
            writeBuffer.updateStreamed(writeStream);
        }

        // copy the write stream info into read stream for processing
        writeStream->getStream(pStream, STREAM_FLAGS_RD);

        if (!pendingReadStreams.isEmpty() && writeStream->pData == writeBuffer.pData) {
            // copy last request's tail to this one's head to have this one not include
            // previous request's data. If this is the first pending request or own-buffer stream then there is no such
            // issue to address.
            ByteStream *pPrevRequest = readStreams + pendingReadStreams.peekTail();
            pStream->nHead = pPrevRequest->nTail;
        }

        // queue it for processing
        pendingReadStreams.addTail(readStreams - pStream);
        if (pendingReadStreams.getCount() == 1) {
            // first one, then no-one to start it up but here
            pStream->flags |= STREAM_FLAGS_PENDING;
            startProcessingRequest(pStream);
        } else {
            // otherwise checking will be done in loop() for completed previous requests
            // and new request processing started if needed
        }

        // make sure loop task is enabled start our loop task to monitor its completion
        this->resume(0);

        // need to reset the write stream for stuff moved to read stream, ie prepare it for more requests
        writeBuffer.getStream(writeStream, STREAM_FLAGS_WR);
        return writeStream;
    }
};

#endif //SCHEDULER_CONTROLLER_H
