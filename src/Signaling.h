#ifndef SCHEDULER_SIGNALING_H
#define SCHEDULER_SIGNALING_H

#include "common_defs.h"

#ifdef __cplusplus
#include "Scheduler.h"
extern "C" {
#endif

extern uint8_t signal_resume_task(pTask_t pTask);
extern uint8_t signal_suspend_task(pTask_t pTask);
extern uint8_t signal_resume_task_id(uint8_t taskId);
extern uint8_t signal_suspend_task_id(uint8_t taskId);

#ifdef __cplusplus
};
#endif
#endif //SCHEDULER_SIGNALING_H
