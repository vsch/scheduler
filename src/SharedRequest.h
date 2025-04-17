
#ifndef SCHEDULER_SHAREDREQUEST_H
#define SCHEDULER_SHAREDREQUEST_H

#include "Request.h"
#include "SharedRequestManager.h"

class SharedRequest : public ByteQueuePtr, public Request {
    friend class SharedRequestManager;

    // ByteQueue_fields
    // uint8_t nSize;           // size of the circular buffer or out of buffer data, nHead==nTail is empty, otherwise just like ByteQueue
    // uint8_t nHead;           // only valid if the frame was started for the request, head in the byte queue, or current position in out of buffer data
    // uint8_t nTail;           // only valid if the frame was started for the request, until frame ends, this is the current tail in the byte queue, or end position in the out of buffer data
    // uint8_t *pData;          // pointer to data, if status has REQ_OUT_OF_BUFFER set then data is coming from dedicated buffer
    SharedRequestManager *pManager;

    void setManager(SharedRequestManager *pManager) {
        this->pManager = pManager;
    }

    inline QueueFields *getFields() {
        return this;
    }

    inline Request *getRequest() {
        return this;
    }

public:
    SharedRequest();

    // these can only be called after the request is added to the common request manager

    /**
     * End current send frame. Request start/end must occur without task switching. No async filling
     *
     * If attempt is made to put more bytes than available in the queue, then this request frame will be closed,
     * data sent, task suspended and the request will be re-started before the task resumes. So request will be split
     * across send frames.
     *
     * Frame is automatically started with the first byte placed in it
     */
    void endFrame();

    /**
     * Add byte to the frame, frame must be started
     */
    void addByte(uint8_t byte);
    void addWord(uint16_t word);

    /**
     * Optionally endFrame and Wait for request to be sent/processed
     */
    void waitDone();

    /**
     * Return the request to the manager without sending. Abandon request.
     */
    void release();

    static SharedRequest *pActiveRequest;
    static void setActiveRequest(SharedRequest *pRequest) {
        pActiveRequest = pRequest;
    }

};


#ifdef __cplusplus
extern "C" {
#endif

static void active_req_endFrame();

/**
 * Add byte to the frame, frame must be started
 */
static void active_req_addByte(uint8_t byte);
static void active_req_addWord(uint16_t word);

/**
 * Optionally endFrame and Wait for request to be sent/processed
 */
static void active_req_waitDone();

/**
 * Return the request to the manager without sending. Abandon request.
 */
static void active_req_release();

#ifdef __cplusplus
}
#endif

#endif //SCHEDULER_SHAREDREQUEST_H
