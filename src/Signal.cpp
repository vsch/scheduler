#include "Signal.h"
#include "Scheduler.h"

uint8_t Signal::addWaitingTask(Task *pTask) {
    if (!pQueue->isFull()) {
        pQueue->addTail(pTask->getIndex());
        return suspendTask(pTask);
    }
    return NULL_TASK;
}

void Signal::trigger() {
    while (!pQueue->isEmpty()) {
        // give to the next task
        resumeTask(pQueue->removeHead());
    }
}

uint8_t Signal::isFull() const {
    return pQueue->isFull();
}

uint8_t Signal::isEmpty() const {
    return pQueue->isEmpty();
}
