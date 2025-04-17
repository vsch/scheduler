#ifndef SCHEDULER_SIGNALBASE_H
#define SCHEDULER_SIGNALBASE_H

#include "Signaling.h"

class SignalBase {
public:
    inline static uint8_t resumeTask(Task *pTask) { return signal_resume_task(pTask); }

    inline static uint8_t resumeTask(uint8_t taskId) { return signal_resume_task_id(taskId); }

    inline static uint8_t suspendTask(Task *pTask) { return signal_suspend_task(pTask); }

    inline static uint8_t suspendTask(uint8_t taskId) { return signal_suspend_task_id(taskId); }
};

#endif //SCHEDULER_SIGNALBASE_H
