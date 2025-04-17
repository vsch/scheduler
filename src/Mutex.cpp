#include <Arduino.h>
#include "Mutex.h"
#include "Scheduler.h"

Mutex::Mutex(uint8_t *queueData, uint8_t queueDataSize) {
    queue_construct(queueData, queueDataSize);
    this->pQueue = (ByteQueue *)queueData;
    ownerReserveCount = 0;
}

uint8_t Mutex::reserveResource(Task *pTask) {
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
            if (pQueue->isFull()) {
                return NULL_TASK;
            }
            pQueue->addTail(pTask->getIndex());
            return suspendTask(pTask);
        }
    }
}

uint8_t Mutex::reserveResource(uint8_t taskId) {
    return reserveResource(scheduler.getTask(taskId));
}

void Mutex::releaseResource() {
    if (pQueue->peekHead() != NULL_TASK) {
        if (!--ownerReserveCount) {
            // remove owner from head and give to next in line
            pQueue->removeHead();

            while (!pQueue->isEmpty()) {
                // give to this task
                Task *pNextTask = scheduler.getTask(pQueue->peekHead());

                if (pNextTask) {
                    resumeTask(pNextTask);
                    return;
                } else {
                    // discard, not a task id
                    pQueue->removeHead();
                }
            }
        }
    }
}

void Mutex::transferResource(Task *pTask) {
    pQueue->removeHead();
    pQueue->addHead(pTask->getIndex());
    ownerReserveCount = 1;

    // wake it up if necessary
    resumeTask(pTask);
}

void Mutex::transferResource(uint8_t taskId) {
    transferResource(scheduler.getTask(taskId));
}

uint8_t Mutex::reserveResource() {
    Task *pTask = scheduler.getTask();
    if (pTask) {
        return reserveResource(pTask);
    }
    return NULL_TASK;
}

