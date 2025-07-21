#include <Arduino.h>
#include "ResLock.h"
#include "Scheduler.h"
#include "debug_config.h"

uint8_t ResLock::reserve(uint8_t taskId, uint8_t available1) {
    if (isMaxAvailable(available1)) {
        Task *pTask = scheduler.getTask(taskId);
        if (pTask) {
            if (isFree()) {
                if (isAvailable(available1)) {
                    serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: satisfied %d: a1:%d <= nA1:%d\n")
                                                           , taskQueue.getCount()
                                                           , available1, nAvailable1);

                    // make it the owner of TWI resourceLock
                    owner = taskId;
                    return 0;
                }
            }

            // make it wait either for resources or its turn
            if (!isAvailable(available1)) {
                serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: suspend: a1:%d > nA1:%d\n")
                                                       , available1, nAvailable1);
            } else {
                serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: suspend waiting task: a1 %d - nA1 %d\n")
                                                       , available1, nAvailable1);
            }


            // need to wait until they are available
            taskQueue.addTail(taskId);
            resQueue.addTail(available1);

            if (pTask->isAsync()) {
                reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                return 0;
            } else {
                scheduler.suspend(taskId);
                return 1;
            }
        } else {
            resourceTracePrintf_P(PSTR("ResLock:: invalid task id %d\n"), taskId);
        }
    } else {
        resourceTracePrintf_P(PSTR("ResLock:: never satisfied: a1:%d > maxA1:%d\n")
                            , available1, nMaxAvailable1);
    }
    return NULL_BYTE;
}

void ResLock::release() {
    if (owner != NULL_TASK) {
        owner = NULL_TASK;

        // just test for available resources
        makeAvailable(0);
    }
}

// IMPORTANT: called from interrupt code, so interrupts should be disabled.
void ResLock::makeAvailable(uint8_t available1) {
    CLI();
    nAvailable1 += available1;
    if (nAvailable1 > nMaxAvailable1) nAvailable1 = nMaxAvailable1;

    serialDebugResourceDetailTracePrintf_P(PSTR("ResLock::make avail: a1 %d - nA1 %d\n")
                                           , available1, nAvailable1);

    if (owner == NULL_TASK) {
        // remove all that will have resources based on requested maximum
        while (!taskQueue.isEmpty()) {
            uint8_t nResCount1 = resQueue.peekHead();

            if (nResCount1 == NULL_BYTE) {
                serialDebugResourceDetailTracePrintf_P(PSTR("ResLock:: res peek() %d\n"), nResCount1);
            }

            if (nAvailable1 < nResCount1) {
                serialDebugResourceDetailTracePrintf_P(PSTR("ResLock::still waiting %d: a1 %d - nA1 %d\n")
                                                       , taskQueue.getCount()
                                                       , nResCount1, nAvailable1);

                break;
            }

            // can release the task
            resQueue.removeHead();
            uint8_t taskId = taskQueue.removeHead();

            Task *pNextTask = scheduler.getTask(taskId);
            if (pNextTask) {
                serialDebugResourceDetailTracePrintf_P(PSTR("ResLock::resuming #%d: a1 %d - nA1 %d\n")
                                                       , taskId
                                                       , nResCount1, nAvailable1);

                // CAVEAT: these tasks are already suspended, there is no need to call Mutex::reserve for the newly enabled
                //  tasks because if they are not the first, then they will be suspended, but they are already suspended.
                //  suspened AsyncTasks should not call their yieldSuspend().
                owner = taskId;
                scheduler.resume(taskId, 0);
                break;
            }
        }
    }
    SEI();
}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void ResLock::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    addActualOutput("%s", indentStr);

    addActualOutput("%sResLock { owner: %d, max:%d, avail:%d\n", indentStr, owner,  nMaxAvailable1, nAvailable1);
    taskQueue.dump(indent + 2, compact);
    resQueue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
