#include "Arduino.h"
#include "SharedRequestManager.h"
#include "SharedRequest.h"

SharedRequest::SharedRequest() : Request(), ByteQueuePtr() {
}

void SharedRequest::endFrame() {
    pManager->processRequest(this);
}

void SharedRequest::addByte(uint8_t byte) {
    pManager->addByte(this, byte);
}

void SharedRequest::addWord(uint16_t word) {
    pManager->addByte(this, word &  0xff);
    pManager->addByte(this, word >> 8);
}

void SharedRequest::waitDone() {
    pManager->waitRequestDone(this);
}

void SharedRequest::release() {
    pManager->addFreeRequest(this);
}

void active_req_endFrame() {
    SharedRequest *pRequest = SharedRequest::pActiveRequest;
    if (!pRequest) return;
    pRequest->endFrame();
}

/**
 * Add byte to the frame, frame must be started
 */
void active_req_addByte(uint8_t byte) {
    SharedRequest *pRequest = SharedRequest::pActiveRequest;
    if (!pRequest) return;
    pRequest->addByte(byte);
}

void active_req_addWord(uint16_t word) {
    SharedRequest *pRequest = SharedRequest::pActiveRequest;
    if (!pRequest) return;
    pRequest->addWord(word);
}

/**
 * Optionally endFrame and Wait for request to be sent/processed
 */
void active_req_waitDone() {
    SharedRequest *pRequest = SharedRequest::pActiveRequest;
    if (!pRequest) return;
    pRequest->waitDone();
}

/**
 * Return the request to the manager without sending. Abandon request.
 */
void active_req_release() {
    SharedRequest *pRequest = SharedRequest::pActiveRequest;
    if (!pRequest) return;
    pRequest->release();
}
