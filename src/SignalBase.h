#ifndef SCHEDULER_SIGNALBASE_H
#define SCHEDULER_SIGNALBASE_H

#include "Scheduler.h"

class SignalBase {
protected:
    inline void triggerTask(Task *pTask) {
        if (pTask) {
            pTask->resume(0);
        }
    }

    uint8_t taskWait(Task *pTask);

    void triggerTask(uint8_t taskId);
    uint8_t taskWait(uint8_t taskId);
};

#endif //SCHEDULER_SIGNALBASE_H
