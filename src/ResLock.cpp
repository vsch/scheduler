#include <Arduino.h>
#include "ResLock.h"
#include "Scheduler.h"

uint8_t ResLock::reserve(uint8_t taskId, uint8_t nCount) {
    if (isMaxAvailable(nCount) && scheduler.isValidId(taskId)) {
        // No one waiting, can check and release right away.
        // Otherwise, have to first satisfy waiting tasks
        if (taskQueue.isEmpty()) {
            if (isAvailable(nCount)) {
                return 0;
            }
        }

        Task *pTask = scheduler.getTask(taskId);

        if (pTask) {
            // need to wait until they are available
            taskQueue.addTail(taskId);
            resQueue.addTail(nCount);

            if (pTask->isAsync()) {
                reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                return 0;
            } else {
                scheduler.suspend(taskId);
                return 1;
            }
        }
    }
    return NULL_BYTE;
}

void ResLock::makeAvailable(uint8_t available) {
    nAvailable += available;
    if (nAvailable > nMaxAvailable) nAvailable = nMaxAvailable;

    available = nAvailable;
    
    // remove all that will have resources based on requested maximum
    while (!taskQueue.isEmpty()) {
        uint8_t nResCount = resQueue.peekHead();
        if (available < nResCount) break;
        
        // can release the task
        resQueue.removeHead();
        available -= nResCount;
        
        Task *pNextTask = scheduler.getTask(taskQueue.removeHead());

        if (pNextTask) {
            pNextTask->resume(0);
        }
    }
}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void ResLock::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    addActualOutput("%s", indentStr);

    addActualOutput("%sResLock { max:%d, avail:%d\n", indentStr, nMaxAvailable, nAvailable);
    taskQueue.dump(indent + 2, compact);
    resQueue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
