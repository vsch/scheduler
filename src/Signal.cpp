#include "Signal.h"
#include "Scheduler.h"

Signal::Signal(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
}

uint8_t Signal::wait(Task *pTask) {
    if (!queue.isFull()) {
        queue.addTail(pTask->getIndex());

        if (pTask->isBlocking()) {
            reinterpret_cast<YieldingTask *>(pTask)->yieldSuspend();
            return 0;
        } else {
            pTask->suspend();
            return 1;
        }
    }
    return 1;
}

void Signal::trigger() {
    while (!queue.isEmpty()) {
        // give to this task
        Task *pNextTask = scheduler.getTask(queue.removeHead());

        if (pNextTask) {
            pNextTask->resume(0);
        }
    }
}

