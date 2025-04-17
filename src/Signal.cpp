#include "Signal.h"
#include "Scheduler.h"

Signal::Signal(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
}

uint8_t Signal::wait(Task *pTask) {
    if (!queue.isFull()) {
        queue.addTail(pTask->getIndex());
        return taskWait(pTask);
    }
    return 1;
}

void Signal::trigger() {
    while (!queue.isEmpty()) {
        // give to the next task
        triggerTask(queue.removeHead());
    }
}

