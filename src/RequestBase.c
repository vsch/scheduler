#include "Arduino.h"
#include "Scheduler_C.h"
#include "RequestBase.h"

uint8_t req_getResultMasked(const SharedRequest_t *thizz, uint8_t mask) {
    return thizz->status & mask;
}

pTask_t *req_getTask(const SharedRequest_t *thizz) {
    return scheduler_getTask(thizz->taskId);
}

uint8_t req_getResult(const SharedRequest_t *thizz) {
    return req_getResultMasked(thizz, REQ_RESULT_MASK);
}

uint8_t req_getStatus(const SharedRequest_t *thizz) {
    return thizz->status;
}

void req_setResult(SharedRequest_t *thizz, uint8_t status) {
    thizz->status = status;
}

void req_setResultMasked(SharedRequest_t *thizz, uint8_t status, uint8_t mask) {
    thizz->status &= ~mask;
    thizz->status |= status & mask;
}
