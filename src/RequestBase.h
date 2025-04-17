
#ifndef SCHEDULER_REQUESTBASE_H
#define SCHEDULER_REQUESTBASE_H

#include "Scheduler_C.h"

#define REQ_RESULT_NONE     (0)    // request not started
#define REQ_RESULT_BUSY     (1)    // request pending or in progress
#define REQ_RESULT_DONE     (2)    // request done
#define REQ_RESULT_ERROR    (3)    // request done with errors
#define REQ_RESULT_MASK     (0x03)


#define REQ_OUT_OF_BUFFER   (0x04) // request is from a dedicated buffer
#define REQ_WAITING_DONE    (0x08) // wait done was requested

#define REQ_ERROR_MAX      (0x7)
#define REQ_ERROR_MASK     (0x70)
#define REQ_ERROR_TO_RESULT(e)     (0xf0 & ((e) << 4))
#define REQ_RESULT_TO_ERROR(e)     ((0xf0 & (e)) >> 4)
#define REQ_FRAME_OPEN     (0x80) // wait done was requested

typedef struct RequestBase {
    uint8_t taskId;     // id of task making request
    uint8_t status;     // status of executing the request
} SharedRequest_t;

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t req_getResultMasked(const SharedRequest_t *thizz, uint8_t mask);
extern pTask_t *req_getTask(const SharedRequest_t *thizz);
extern uint8_t req_getResult(const SharedRequest_t *thizz);
extern uint8_t req_getStatus(const SharedRequest_t *thizz);
extern void req_setResult(SharedRequest_t *thizz, uint8_t status);
extern void req_setResultMasked(SharedRequest_t *thizz, uint8_t status, uint8_t mask);

#ifdef __cplusplus
}
#endif


#endif //SCHEDULER_REQUESTBASE_H
