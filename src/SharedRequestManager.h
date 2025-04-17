
#ifndef SCHEDULER_SHAREDREQUESTMANAGER_H
#define SCHEDULER_SHAREDREQUESTMANAGER_H

#include "common_defs.h"
#include "ByteQueue.h"
#include "RequestBase.h"
#include "SharedRequestPart.h"
#include "Request.h"
#include "WordQueue.h"

class SharedRequest;

class SharedRequestManager {
    ByteQueue *pByteQueue;              // common buffer queue
    WordQueue *pRequestQueue;           // pending requests queue, head is current if a request is being processed
    WordQueue *pFreeRequestQueue;       // available requests queue

    SharedRequest *pCurrentRequest;     // current request being processed

    // pRequestQueue contains indices of pending requests
    //       head is next request being processed
    // if pCurrentRequest is not NULL, then a request is being processed
    // pByteQueue is the common buffer queue. Single request data will wrap around, hence the reason for SharedRequestPart
    // so that C code can access contiguous pieces and not worry about wrapping around
    // as data is removed the request fields are updated to keep track of progress and to free up the data from the buffer
protected:
    // launch the processing of pending requests, if request becomes ready and no other pending requests
    virtual void startRequestProcessing() = 0;

public:
    // takes a single buffer and divides it as per request
    SharedRequestManager(ByteQueue *pByteQueue, WordQueue *pRequestQueue, WordQueue *pFreeRequestQueue);

    // give requests to manager to use as needed
    void addFreeRequest(SharedRequest *pRequest);

    /**
     * Request a common buffer request, must be release()d or it will be lost.
     *
     * @return request or NULL
     */
    SharedRequest *getRequest();

    /**
     * Out of common buffer request to send the accumulated data. The request will be queued for sending.
     * and initialized with the given data to be sent
     *
     * @param pRequest   request to be sent.
     * @return 0 if request queued, else request queue full, request ignored.
     */
    uint8_t processOwnBufferRequest(SharedRequest *pRequest, uint8_t *pData, uint16_t nSize);
    uint8_t processRequest(SharedRequest *pRequest);

    uint8_t havePendingRequests();
    uint8_t canAddRequest();
    SharedRequestPart_t *getNextPart();
    virtual void *getRequestExtra();

    // current request is now complete
    void completeRequest(uint8_t error);

    // optional callback to allow common buffer to release this many bytes taken from the current request
    // otherwise, all will be released when the next part is requested or completeRequest is invoked.
    void sentUpdate(uint16_t bytes);

    // C ISR callable functions
    static uint8_t havePendingRequests(pSharedRequestManager_t pMgr);
    static SharedRequestPart_t *getNextPart(pSharedRequestManager_t pMgr);
    static void *getRequestExtra(pSharedRequestManager_t pMgr);
    static void completeRequest(pSharedRequestManager_t pMgr, uint8_t error);
    void waitRequestDone(SharedRequest *pRequest);
    void addByte(SharedRequest *pRequest, uint8_t byte);
};

#ifdef __cplusplus
extern "C" {
#endif

// These functions must be callable from ISR processing the requests

// minimalist C interface to be able to get the next request for processing and getting characters from it
extern uint8_t req_mgr_havePendingRequests(pSharedRequestManager_t pMgr);

// get the next contiguous request chunk, a single request will have at most 2 parts because of buffer roll-around
// this applies to the current request, or the next available if no current, NULL otherwise
extern SharedRequestPart_t *req_mgr_getNextPart(pSharedRequestManager_t pMgr);

// return extra information about the request, applies to specific information needed for some request processing. May be null
extern void *req_mgr_getRequestExtra(pSharedRequestManager_t pMgr);

extern void req_mgr_completeRequest(pSharedRequestManager_t pMgr);

#ifdef __cplusplus
}
#endif

#endif //SCHEDULER_SHAREDREQUESTMANAGER_H
