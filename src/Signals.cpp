#include "Signals.h"
#include "Scheduler.h"

Signals::Signals(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
}

uint8_t Signals::wait(Task *pTask) {
    if (!queue.isFull()) {
        queue.addTail(pTask->getIndex());

        if (pTask->isAsync()) {
            reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
            return 0;
        } else {
            pTask->suspend();
            return 1;
        }
    }
    return 1;
}

void Signals::trigger() {
    while (!queue.isEmpty()) {
        // give to this task
        Task *pNextTask = scheduler.getTask(queue.removeHead());

        if (pNextTask) {
            pNextTask->resume(0);
        }
    }
}

