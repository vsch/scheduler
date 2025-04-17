#ifndef SCHEDULER_REQUESTBASE_H
#define SCHEDULER_REQUESTBASE_H

#include "Scheduler.h"
#include "SignalBase.h"
#include "Queue.h"

// allows tasks to wait for request completion and then get status and other information
#define REQ_RESULT_NONE     (0)    // request not started
#define REQ_RESULT_BUSY     (1)    // request in progress
#define REQ_RESULT_DONE     (2)    // request done
#define REQ_RESULT_ERROR    (3)    // request done with errors

#define REQ_RESULT_MASK     (0x03)

class RequestBase : public SignalBase {
protected:
    uint8_t taskId;     // id of task making request
    uint8_t result;     // result of executing the request

    /**
     * start request for given task, suspends task until request is complete
     *
     * @return      0 if successfully yielded and request started. 1 if could not start or could not yield
     *              to wait for completion because not an AsyncTask
     */
    inline uint8_t startForTask() {
        taskWait(taskId);
    }

    /**
     * Request is complete, triggers and resumes task
     *
     * @param result        - result to store with the request
     */
    inline void triggerDone(uint8_t result) {
        this->result = result;
        triggerTask(taskId);
    }

    RequestBase() {
        taskId = 0;
        result = REQ_RESULT_NONE;
    }

public:
    inline uint8_t getStatus() const { return getResult(REQ_RESULT_MASK); }
    inline uint8_t getResult() const { return result; }

    inline uint8_t getResult(uint8_t mask) const {
        return result & mask;
    }

};

#endif //SCHEDULER_REQUESTBASE_H
