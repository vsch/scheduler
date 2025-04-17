#include <Arduino.h>
#include "ResourceLock.h"
#include "Scheduler.h"

uint8_t ResourceLock::reserveResource(Task *pTask) {
    if (pQueue->isEmpty()) {
        // available
        pQueue->addTail(pTask->getIndex());
        ownerReserveCount = 1;
        return 0;
    } else {
        if (pQueue->peekHead() == pTask->getIndex()) {
            // same owner, reserving again
            ownerReserveCount++;
            return 0;
        } else {
            // not available, queue up the task
            pQueue->addTail(pTask->getIndex());
            return taskWait(pTask);
        }
    }
}

uint8_t ResourceLock::reserveResource(uint8_t taskId) {
    return reserveResource(scheduler.getTask(taskId));
}

void ResourceLock::releaseResource() {
    if (pQueue->peekHead() != NULL_TASK) {
        if (!--ownerReserveCount) {
            // remove owner from head and give to next in line
            pQueue->removeHead();

            while (!pQueue->isEmpty()) {
                // give to this task
                Task *pNextTask = scheduler.getTask(pQueue->peekHead());

                if (pNextTask) {
                    triggerTask(pNextTask);
                    return;
                } else {
                    // discard, not a task id
                    pQueue->removeHead();
                }
            }
        }
    }
}

void ResourceLock::transferResource(Task *pTask) {
    pQueue->removeHead();
    pQueue->addHead(pTask->getIndex());
    ownerReserveCount = 1;

    // wake it up if necessary
    triggerTask(pTask);
}

void ResourceLock::transferResource(uint8_t taskId) {
    transferResource(scheduler.getTask(taskId));
}

