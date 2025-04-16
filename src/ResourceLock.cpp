#include <Arduino.h>
#include "ResourceLock.h"
#include "Scheduler.h"

ResourceLock::ResourceLock(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
}

uint8_t ResourceLock::reserveResource(Task *pTask) {
    if (queue.isEmpty()) {
        // available
        queue.addTail(pTask->getIndex());
        return 0;
    } else {
        // not available, queue up the task
        queue.addTail(pTask->getIndex());

        if (pTask->isBlocking()) {
            reinterpret_cast<YieldingTask *>(pTask)->yieldSuspend();
            return 0;
        } else {
            pTask->suspend();
            return 1;
        }
    }
}

void ResourceLock::releaseResource() {
    if (queue.peekHead() != NULL_TASK) {
        // remove owner from head and give to next in line
        queue.removeHead();

        while (!queue.isEmpty()) {
            // give to this task
            Task *pNextTask = scheduler.getTask(queue.peekHead());

            if (pNextTask) {
                pNextTask->resume(0);
                return;
            } else {
                // discard, not a task id
                queue.removeHead();
            }
        }
    }
}

void ResourceLock::transferResource(Task *pTask) {
    queue.removeHead();
    queue.addHead(pTask->getIndex());

    // wake it up if necessary
    pTask->resume(0);
}

