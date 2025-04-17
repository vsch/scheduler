#include "Signal.h"
#include "Scheduler.h"

uint8_t Signal::wait(Task *pTask) {
    if (!pQueue->isFull()) {
        pQueue->addTail(pTask->getIndex());
        return taskWait(pTask);
    }
    return 1;
}

void Signal::trigger() {
    while (!pQueue->isEmpty()) {
        // give to the next task
        triggerTask(pQueue->removeHead());
    }
}

