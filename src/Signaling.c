#include <stdint-gcc.h>
#include "Scheduler_C.h"
#include "Signaling.h"

uint8_t signal_suspend_task(pTask_t pTask) {
    if (pTask) {
        if (scheduler_isAsyncTask(pTask)) {
            scheduler_asyncYieldSuspend(pTask);
            return 0;
        } else {
            scheduler_suspendTask(pTask);
            return 1;
        }
    }
    return NULL_TASK;
}

uint8_t signal_resume_task_id(uint8_t taskId) {
    pTask_t *pTask = scheduler_getTask(taskId);
    return signal_resume_task(pTask);
}

uint8_t signal_resume_task(pTask_t pTask) {
    if (pTask) {
        scheduler_resumeTask(pTask, 0);
        return 0;
    }
    return NULL_TASK;
}

uint8_t signal_suspend_task_id(uint8_t taskId) {
    pTask_t *pTask = scheduler_getTask(taskId);
    return signal_suspend_task(pTask);
}

