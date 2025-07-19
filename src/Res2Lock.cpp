#include <Arduino.h>
#include "Res2Lock.h"
#include "Scheduler.h"
#include "Controller.h"
#include "debug_config.h"

uint8_t Res2Lock::reserve(uint8_t taskId, uint8_t available1, uint8_t available2) {
    if (isMaxAvailable(available1, available2)) {
        Task *pTask = scheduler.getTask(taskId);
        if (pTask) {
            if (isFree()) {
                if (isAvailable(available1, available2)) {
                    serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: satisfied #%d: a1:%d <= nA1:%d && a2:%d <= nA2:%d\n")
                                                           , taskId
                                                           , available1, nAvailable1
                                                           , available2, nAvailable2);

                    // make it the owner of TWI resourceLock
                    owner = taskId;
                    return 0;
                }
            }

            // make it wait either for resources or its turn
            if (!isAvailable(available1, available2)) {
                serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: suspend #%d: a1:%d > nA1:%d || a2:%d > nA2:%d\n")
                                                       , taskId
                                                       , available1, nAvailable1
                                                       , available2, nAvailable2);
            } else {
                serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: suspend waiting task #%d: a1 %d - nA1 %d, a2:%d - nA2 %d\n")
                                                       , taskId
                                                       , available1, nAvailable1
                                                       , available2, nAvailable2);
            }

            // need to wait until they are available
            CLI();
            taskQueue.addTail(taskId);
            resQueue.addTail(available1);
            resQueue.addTail(available2);
            SEI();

            if (pTask->isAsync()) {
                reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                return 0;
            } else {
                scheduler.suspend(taskId);
                return 1;
            }
        } else {
            serialDebugPrintf_P(PSTR("Res2Lock:: invalid task id %d\n"), taskId);
        }
    } else {
        serialDebugPrintf_P(PSTR("Res2Lock:: never satisfied: a1:%d > maxA1:%d || a2:%d > maxA2:%d\n")
                            , available1, nMaxAvailable1
                            , available2, nMaxAvailable2);
    }
    return NULL_BYTE;
}

void Res2Lock::release() {
    if (owner != NULL_TASK) {
        owner = NULL_TASK;

        // just test for available resources
        makeAvailable(0,0);
    }
}

void Res2Lock::makeAvailable(uint8_t available1, uint8_t available2) {
    CLI();
    nAvailable1 += available1;
    if (nAvailable1 > nMaxAvailable1) nAvailable1 = nMaxAvailable1;
    nAvailable2 += available2;
    if (nAvailable2 > nMaxAvailable2) nAvailable2 = nMaxAvailable2;

    serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::make avail: a1 %d - nA1 %d, a2:%d - nA2:%d\n")
                                           , available1, nAvailable1
                                           , available2, nAvailable2);

    if (owner == NULL_TASK) {
        // remove all that will have resources based on requested maximum
        while (!taskQueue.isEmpty()) {
            uint8_t nResCount1 = resQueue.peekHead();
            uint8_t nResCount2 = resQueue.peekHead(1);

            if (nResCount1 == NULL_BYTE) {
                serialDebugPrintf_P(PSTR("Res2Lock:: res peek() %d\n"), nResCount1);
            }

            if (nResCount2 == NULL_BYTE) {
                serialDebugPrintf_P(PSTR("Res2Lock:: res peek(1) %d\n"), nResCount2);
            }

            if (nAvailable1 < nResCount1 || nAvailable2 < nResCount2) {
                serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::still waiting %d: a1 %d - nA1 %d, a2:%d - nA2 %d\n")
                                                       , taskQueue.getCount()
                                                       , nResCount1, nAvailable1
                                                       , nResCount2, nAvailable2);

                break;
            }

            // can release the task
            resQueue.removeHead();
            resQueue.removeHead();
            uint8_t taskId = taskQueue.removeHead();

            Task *pNextTask = scheduler.getTask(taskId);
            if (pNextTask) {
                serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::resuming #%d: a1 %d - nA1 %d, a2:%d - nA2 %d\n")
                                                       , taskId
                                                       , nResCount1, nAvailable1
                                                       , nResCount2, nAvailable2);

                // CAVEAT: these tasks are already suspended, there is no need to call Mutex::reserve for the newly enabled
                //  tasks because if they are not the first, then they will be suspended, but they are already suspended.
                //  suspended AsyncTasks should not call their yieldSuspend().
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
void Res2Lock::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    addActualOutput("%s", indentStr);

    addActualOutput("%sRes2Lock { owner: %d, max1:%d, avail1:%d max2:%d, avail2:%d\n", indentStr, owner, nMaxAvailable1, nAvailable1, nMaxAvailable2, nAvailable2);
    taskQueue.dump(indent + 2, compact);
    resQueue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
