#include "Scheduler.h"
#include "SignalBase.h"

uint8_t SignalBase::suspendTask(Task *pTask) {
    if (pTask) {
        if (pTask->isAsync()) {
            reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
            return 0;
        } else {
            pTask->suspend();
            return 1;
        }
    }
    return 1;
}

uint8_t SignalBase::suspendTask(uint8_t taskId) {
    Task *pTask = scheduler.getTask(taskId);
    return suspendTask(pTask);
}

void SignalBase::resumeTask(uint8_t taskId) {
    Task *pTask = scheduler.getTask(taskId);
    resumeTask(pTask);
}

void SignalBase::resumeTask(Task *pTask) {
    if (pTask) {
        pTask->resume(0);
    }
}
