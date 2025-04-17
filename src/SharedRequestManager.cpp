#include "Arduino.h"
#include "SharedRequestManager.h"
#include "SharedRequest.h"
#include "common_defs.h"

SharedRequestManager::SharedRequestManager(ByteQueue *pByteQueue, WordQueue *pRequestQueue, WordQueue *pFreeRequestQueue) {
    this->pByteQueue = pByteQueue;              // common buffer queue
    this->pRequestQueue = pRequestQueue;           // pending requests queue, head is current if a request is being processed
    this->pFreeRequestQueue = pFreeRequestQueue;       // available requests queue
    pCurrentRequest = NULL;
}

void SharedRequestManager::addFreeRequest(SharedRequest *pRequest) {
    pRequest->setManager(this);
    pFreeRequestQueue->addTail((uint16_t) pRequest);
}

SharedRequest *SharedRequestManager::getRequest() {
    return (SharedRequest *) pFreeRequestQueue->removeHead();
}

uint8_t SharedRequestManager::havePendingRequests() {
    return !pRequestQueue->isEmpty();
}

uint8_t SharedRequestManager::canAddRequest() {
    return !pRequestQueue->isFull();
}

SharedRequestPart_t *SharedRequestManager::getNextPart() {
    // TODO: get the next part of the current request or the first part of the next request
    return NULL;
}

void *SharedRequestManager::getRequestExtra() {
    // TODO: let base class handle this
    return NULL;
}

void SharedRequestManager::sentUpdate(uint16_t bytes) {
    if (pCurrentRequest) {
        Queue_t *pQ = pCurrentRequest->getFields();
        if (pCurrentRequest->getResult(REQ_OUT_OF_BUFFER)) {
            // just ignore until done
        } else {
            if (queue_getCount(pQ) <= bytes) {
                // remove the request part's bytes from buffer
                pByteQueue->nHead = pQ->nTail;

                // all done, move head to tail
                pQ->nHead = pQ->nTail;
            } else {
                // remove the request part's bytes from buffer
                pByteQueue->nHead += bytes;
                if (pByteQueue->nHead >= pByteQueue->nSize) {
                    pByteQueue->nHead -= pByteQueue->nSize;
                }

                // all done, move head to tail
                pQ->nHead += bytes;
                if (pQ->nHead >= pQ->nSize) {
                    pQ->nHead -= pQ->nSize;
                }
            }
        }
    }
}

void SharedRequestManager::waitRequestDone(SharedRequest *pRequest) {
    if (pRequest->getResult() == REQ_RESULT_BUSY) {
        // only if it is pending or being processed
        pRequest->setResult(REQ_WAITING_DONE, REQ_WAITING_DONE);
        pRequest->suspendTask(pRequest->taskId);
    }
}

void SharedRequestManager::completeRequest(uint8_t error) {
    if (error > REQ_ERROR_MAX) {
        error = REQ_ERROR_MAX;
    }

    if (error) {
        pCurrentRequest->setResult(REQ_RESULT_ERROR | REQ_ERROR_TO_RESULT(error), REQ_RESULT_MASK | REQ_ERROR_MASK);
    } else {
        pCurrentRequest->setResult(REQ_RESULT_DONE, REQ_RESULT_MASK | REQ_ERROR_MASK);
    }

    if (pCurrentRequest->getResult(REQ_WAITING_DONE)) {
        pCurrentRequest->triggerDone();
    }
}

void SharedRequestManager::addByte(SharedRequest *pRequest, uint8_t byte) {
    if (!pByteQueue->isFull()) {
        if (!pRequest->getResult(REQ_FRAME_OPEN)) {
            // copy for temporary use
            pRequest->setResult(REQ_FRAME_OPEN);
            queue_copyFields(pRequest, pByteQueue);
            // here we point to common byte buffer
            pRequest->data = pByteQueue->data;
        }
        pByteQueue->addTail(byte);
        pRequest->nTail = pByteQueue->nTail;

        queue_addHead(pRequest); // just move the head, do nothing else
    }
}

uint8_t SharedRequestManager::processRequest(SharedRequest *pRequest) {
    if (pRequest->getCount() && !pRequestQueue->isFull()) {
        pRequest->setResult(0, REQ_FRAME_OPEN);
        // the queue values contain the request's data
        pRequestQueue->addTail((uint16_t) pRequest);
        pRequest->setResult(REQ_RESULT_BUSY, REQ_RESULT_MASK);
        startRequestProcessing();
        return 0;
    }
    return 1;
}

uint8_t SharedRequestManager::processOwnBufferRequest(SharedRequest *pRequest, uint8_t *pData, uint16_t nSize) {
    pRequest->nSize = nSize;
    pRequest->data = pData;
    pRequest->nHead = 0;
    pRequest->nTail = nSize;
    pRequest->setManager(this);
    pRequest->setResult(0); // nothing done yet
    if (!pRequestQueue->isFull()) {
        pRequestQueue->addTail((uint16_t) pRequest);
        pRequest->setResult(REQ_RESULT_BUSY, REQ_RESULT_MASK);
        startRequestProcessing();
        return 0;
    }
    return 1;
}

uint8_t SharedRequestManager::havePendingRequests(pSharedRequestManager_t pMgr) {
    return pMgr->havePendingRequests();
}

SharedRequestPart_t *SharedRequestManager::getNextPart(pSharedRequestManager_t pMgr) {
    return pMgr->getNextPart();
}

void *SharedRequestManager::getRequestExtra(pSharedRequestManager_t pMgr) {
    return pMgr->getRequestExtra();
}

void SharedRequestManager::completeRequest(pSharedRequestManager_t pMgr, uint8_t error) {
    pMgr->completeRequest(error);
}

uint8_t req_mgr_havePendingRequests(pSharedRequestManager_t pMgr) {
    SharedRequestManager::havePendingRequests(pMgr);
}

// the next contiguous request chunk, a single request will have at most 2 parts because of buffer roll-aroun {

// this applies to the current request, or the next available if no current, NULL otherwise
SharedRequestPart_t *req_mgr_getNextPart(pSharedRequestManager_t pMgr) {
    SharedRequestManager::getNextPart(pMgr);
}

// return extra information about the request, applies to specific information needed for some request processing. May be null
void *req_mgr_getRequestExtra(pSharedRequestManager_t pMgr) {
    SharedRequestManager::getRequestExtra(pMgr);
}

void req_mgr_completeRequest(pSharedRequestManager_t pMgr) {
    SharedRequestManager::completeRequest(pMgr, 0);
}
