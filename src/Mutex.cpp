#include <Arduino.h>
#include "Mutex.h"
#include "Scheduler.h"

Mutex::Mutex(uint8_t *queueBuffer, uint8_t queueSize)
        : queue(queueBuffer, queueSize) {
}

void Mutex::reset() {
    queue.reset();
}

uint8_t Mutex::isFree() const {
    return queue.isEmpty();
}

uint8_t Mutex::reserve(uint8_t taskId) {
    if (scheduler.isValidId(taskId)) {
        if (queue.isEmpty()) {
            // available
            queue.addTail(taskId);
            return 0;
        } else {
            // not available, queue up the task
            queue.addTail(taskId);

            Task *pTask = scheduler.getTask(taskId);

            if (pTask && pTask->isAsync()) {
                reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                return 0;
            } else {
                scheduler.suspend(taskId);
                return 1;
            }
        }
    }
    return NULL_TASK;
}

uint8_t Mutex::release(uint8_t taskId) {
    return queue.removeHead();
}

void Mutex::release() {
    // remove owner from head and give to next in line
    while (!queue.isEmpty()) {
        queue.removeHead();

        // give to this task
        Task *pNextTask = scheduler.getTask(queue.peekHead());

        if (pNextTask) {
            pNextTask->resume(0);
            return;
        }
    }
}

uint8_t Mutex::transfer(uint8_t fromTaskId, uint8_t toTaskId) {
    if (isOwner(fromTaskId)) {
        queue.removeHead();
        queue.addHead(toTaskId);

        // wake it up if necessary
        scheduler.resume(toTaskId, 0);
        return 0;
    }
    return NULL_TASK;
}

bool Mutex::isOwner(uint8_t taskId) {
    return queue.peekHead() == taskId;
}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void Mutex::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    uint8_t head = queue.peekHead();
    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    addActualOutput("%s", indentStr);

    addActualOutput("%sMutex { Owner:%d\n", indentStr, head);
    queue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
