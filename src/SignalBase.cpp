#include "Scheduler.h"
#include "SignalBase.h"

uint8_t SignalBase::taskWait(Task *pTask) {
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

void SignalBase::triggerTask(uint8_t taskId) {
    Task *pTask = scheduler.getTask(taskId);
    triggerTask(pTask);
}

uint8_t SignalBase::taskWait(uint8_t taskId) {
    Task *pTask = scheduler.getTask(taskId);
    return taskWait(pTask);
}
