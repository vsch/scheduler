
#ifndef SCHEDULER_SCHEDULER_C_H
#define SCHEDULER_SCHEDULER_C_H
#include "common_defs.h"

// scheduler C interface
#ifdef __cplusplus
extern "C" {
#endif

extern pTask_t scheduler_getTask(TaskId_t taskId);
extern uint8_t scheduler_isAsyncTask(pTask_t pTask);
extern uint8_t scheduler_isAsyncTaskId(TaskId_t taskId);
extern void scheduler_asyncYieldSuspend(pTask_t pTask);
extern void scheduler_suspendTask(pTask_t pTask);
extern void scheduler_suspendTaskId(TaskId_t taskId);
extern void scheduler_resumeTask(pTask_t pTask, uint16_t millis);
extern void scheduler_resumeTaskId(TaskId_t taskId, uint16_t millis);

#ifdef __cplusplus
};
#endif

#endif //SCHEDULER_SCHEDULER_C_H
