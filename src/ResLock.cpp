#include <Arduino.h>
#include "ResLock.h"
#include "Scheduler.h"

uint8_t ResLock::reserve(uint8_t taskId, uint8_t available) {
    if (isMaxAvailable(available)) {
        if (scheduler.isValidId(taskId)) {
            // No one waiting, can check and release right away.
            // Otherwise, have to first satisfy waiting tasks
            Task *pTask = scheduler.getTask(taskId);

            if (pTask) {
                if (!Mutex::isFree()) {
                    serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: suspend waiting mutex: a1 %d - nA1 %d\n")
                                                           , available, nAvailable);
                    return Mutex::reserve(taskId);
                } else if (taskQueue.isEmpty()) {
                    if (isAvailable(available)) {
                        serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: satisfied %d: a1:%d <= nA1:%d\n")
                                                               , taskQueue.getCount()
                                                               , available, nAvailable);

                        return Mutex::reserve(taskId);
                    } else {
                        serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: suspend: a1:%d > nA1:%d\n")
                                                               , available, nAvailable);
                    }
                } else {
                    serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: suspend waiting task: a1 %d - nA1 %d\n")
                                                           , available, nAvailable);
                }


                // need to wait until they are available
                taskQueue.addTail(taskId);
                resQueue.addTail(available);

                if (pTask->isAsync()) {
                    reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                    return 0;
                } else {
                    scheduler.suspend(taskId);
                    return 1;
                }
            }
        } else {
            serialDebugPrintf_P(PSTR("ResLock:: invalid task id %d\n"), taskId);
        }
    } else {
        serialDebugPrintf_P(PSTR("ResLock:: never satisfied: a1:%d > maxA1:%d\n")
                            , available, nMaxAvailable);
    }
    return NULL_BYTE;
}

// called from interrupt code
void ResLock::makeAvailable(uint8_t available) {
    nAvailable += available;
    if (nAvailable > nMaxAvailable) nAvailable = nMaxAvailable;

    serialDebugResourceDetailTracePrintf_P(PSTR("ResLock::make avail: a1 %d - nA1 %d\n")
                                           , available, nAvailable);

    available = nAvailable;

    // remove all that will have resources based on requested maximum
    while (!taskQueue.isEmpty()) {
        uint8_t nResCount = resQueue.peekHead();

        if (nResCount == NULL_BYTE) {
            serialDebugPrintf_P(PSTR("ResLock:: res peek() %d\n"), nResCount);
        }

        if (available < nResCount) {
            serialDebugResourceDetailTracePrintf_P(PSTR("ResLock::still waiting %d: a1 %d - nA1 %d\n")
                                                   , taskQueue.getCount()
                                                   , nResCount, available);

            break;
        }

        // can release the task
        resQueue.removeHead();
        resQueue.removeHead();
        uint8_t taskId = taskQueue.removeHead();

        Task *pNextTask = scheduler.getTask(taskId);
        if (pNextTask) {
            available -= nResCount;

            serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::resuming #%d: a1 %d - nA1 %d\n")
                                                   , pNextTask->getIndex()
                                                   , nResCount, available);

            // resume the task, in case it is first 
            scheduler.resume(taskId, 0);
            Mutex::reserve(taskId);
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

    addActualOutput("%sRes2Lock { max:%d, avail:%d\n", indentStr, nMaxAvailable, nAvailable);
    taskQueue.dump(indent + 2, compact);
    resQueue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
