#ifndef SCHEDULER_SIGNALBASE_H
#define SCHEDULER_SIGNALBASE_H

#include "Scheduler.h"

class SignalBase {
protected:
    static void resumeTask(Task *pTask);
    static uint8_t suspendTask(Task *pTask);
    static void resumeTask(uint8_t taskId);
    static uint8_t suspendTask(uint8_t taskId);
};

#endif //SCHEDULER_SIGNALBASE_H
