#include <Arduino.h>
#include "Mutex.h"
#include "Scheduler.h"

Mutex::Mutex(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
}

uint8_t Mutex::isFree() const {
    return queue.isEmpty();
}

uint8_t Mutex::reserve() {
    Task *pTask = scheduler.getTask();
    if (pTask) {
        uint8_t taskId = scheduler.getTaskId();
        if (queue.isEmpty()) {
            // available
            queue.addTail(taskId);
            return 0;
        } else {
            // not available, queue up the task
            queue.addTail(taskId);

            if (pTask->isAsync()) {
                reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                return 0;
            } else {
                pTask->suspend();
                return 1;
            }
        }
    }
    return NULL_TASK;
}

void Mutex::release() {
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

uint8_t Mutex::transfer(Task *pTask) {
    if (queue.peekHead() == scheduler.getTaskId()) {
        queue.removeHead();
        queue.addHead(pTask->getIndex());

        // wake it up if necessary
        pTask->resume(0);
        return 0;
    }
    return NULL_TASK;
}

bool Mutex::isOwner(uint8_t taskId) {
    return queue.peekHead() == taskId;
}

