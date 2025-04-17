#include <Arduino.h>
#include "ResourceLock.h"
#include "Scheduler.h"

ResourceLock::ResourceLock(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
    ownerReserveCount = 0;
}

uint8_t ResourceLock::reserveResource(Task *pTask) {
    if (queue.isEmpty()) {
        // available
        queue.addTail(pTask->getIndex());
        ownerReserveCount = 1;
        return 0;
    } else {
        if (queue.peekHead() == pTask->getIndex()) {
            // same owner, reserving again
            ownerReserveCount++;
            return 0;
        } else {
            // not available, queue up the task
            queue.addTail(pTask->getIndex());
            return taskWait(pTask);
        }
    }
}

uint8_t ResourceLock::reserveResource(uint8_t taskId) {
    return reserveResource(scheduler.getTask(taskId));
}

void ResourceLock::releaseResource() {
    if (queue.peekHead() != NULL_TASK) {
        if (!--ownerReserveCount) {
            // remove owner from head and give to next in line
            queue.removeHead();

            while (!queue.isEmpty()) {
                // give to this task
                Task *pNextTask = scheduler.getTask(queue.peekHead());

                if (pNextTask) {
                    triggerTask(pNextTask);
                    return;
                } else {
                    // discard, not a task id
                    queue.removeHead();
                }
            }
        }
    }
}

void ResourceLock::transferResource(Task *pTask) {
    queue.removeHead();
    queue.addHead(pTask->getIndex());
    ownerReserveCount = 1;

    // wake it up if necessary
    triggerTask(pTask);
}

void ResourceLock::transferResource(uint8_t taskId) {
    transferResource(scheduler.getTask(taskId));
}

