#ifndef SCHEDULER_REQUEST_H
#define SCHEDULER_REQUEST_H

// allows tasks to wait for request completion and then get status and other information
#include "Arduino.h"
#include "Signaling.h"
#include "RequestBase.h"
#include "SignalBase.h"

class Request : protected RequestBase, protected SignalBase {
protected:

    inline Request() {
        taskId = 0;
        status = REQ_RESULT_NONE;
    }

    /**
     * start request for given task, suspends task until request is complete
     *
     * @return      0 if successfully yielded and request started. 1 if could not start or could not yield
     *              to addWaitingTask for completion because not an AsyncTask
     */
    inline uint8_t startForTask() {
        suspendTask(taskId);
    }

    /**
     * Request is complete, triggers and resumes task
     */
    inline void triggerDone() {
        resumeTask(taskId);
    }

public:
    inline void setTaskId(uint8_t taskId) { this->taskId = taskId; }

    inline uint8_t getTaskId() const { return taskId; }

    inline Task *getTask() const { return scheduler.getTask(taskId); }

    inline uint8_t getResult() const { return getResult(REQ_RESULT_MASK); }

    inline uint8_t getStatus() const { return status; }

    inline uint8_t getResult(uint8_t mask) const {
        return status & mask;
    }

    inline void setResult(uint8_t status) { this->status = status; }

    inline void setResult(uint8_t status, uint8_t mask) {
        this->status &= ~mask;
        this->status |= status & mask;
    }
};

#endif //SCHEDULER_REQUEST_H
