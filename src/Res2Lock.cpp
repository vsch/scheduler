#include <Arduino.h>
#include "Res2Lock.h"
#include "Scheduler.h"
#include "Controller.h"

uint8_t Res2Lock::reserve(uint8_t taskId, uint8_t available1, uint8_t available2) {
    if (isMaxAvailable(available1, available2) && scheduler.isValidId(taskId)) {
        // No one waiting, can check and release right away.
        // Otherwise, have to first satisfy waiting tasks
        Task *pTask = scheduler.getTask(taskId);

        if (pTask) {
            if (taskQueue.isEmpty()) {
                if (isAvailable(available1, available2)) {
                    serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: satisfied %d: a1:%d <= nA1:%d && a2:%d <= nA2:%d\n")
                                                           , taskQueue.getCount()
                                                           , available1, nAvailable1
                                                           , available2, nAvailable2);
                    return 0;
                } else {
                    serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: suspend: a1:%d > nA1:%d || a2:%d > nA2:%d\n")
                                                           , available1, nAvailable1
                                                           , available2, nAvailable2);
                }
            } else {
                serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: suspend waiting task: a1 %d - nA1 %d, a2:%d - nA2 %d\n")
                                                       , available1, nAvailable1
                                                       , available2, nAvailable2);
            }


            // need to wait until they are available
            taskQueue.addTail(taskId);
            resQueue.addTail(available1);
            resQueue.addTail(available2);

            if (pTask->isAsync()) {
                reinterpret_cast<AsyncTask *>(pTask)->yieldSuspend();
                return 0;
            } else {
                scheduler.suspend(taskId);
                return 1;
            }
        }
    } else {
        serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock:: never satisfied: a1:%d > maxA1:%d || a2:%d > maxA2:%d\n")
                                               , available1, nMaxAvailable1
                                               , available2, nMaxAvailable2);
    }
    return NULL_BYTE;
}

// called from interrupt code
void Res2Lock::makeAvailable(uint8_t available1, uint8_t available2) {
    nAvailable1 += available1;
    if (nAvailable1 > nMaxAvailable1) nAvailable1 = nMaxAvailable1;
    nAvailable2 += available2;
    if (nAvailable2 > nMaxAvailable2) nAvailable2 = nMaxAvailable2;

    // serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::make avail: a1 %d - nA1 %d, a2:%d - nA2:%d\n")
    //                                  , available1, nAvailable1
    //                                  , available2, nAvailable2);

    available1 = nAvailable1;
    available2 = nAvailable2;

    // remove all that will have resources based on requested maximum
    while (!taskQueue.isEmpty()) {
        uint8_t nResCount1 = resQueue.peekHead();
        uint8_t nResCount2 = resQueue.peekHead(1);

        if (nResCount1 == NULL_BYTE) {
            serialDebugPrintf_P(PSTR("Res2Lock:: res peek() %d\n"), nResCount1);
            resQueue.serialDebugDump(PSTR("resQueue"));
        }

        if (nResCount2 == NULL_BYTE) {
            serialDebugPrintf_P(PSTR("Res2Lock:: res peek(1) %d\n"), nResCount2);
            resQueue.serialDebugDump(PSTR("resQueue"));
        }

        if (available1 < nResCount1 || available2 < nResCount2) {
            serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::still waiting %d: a1 %d - nA1 %d, a2:%d - nA2 %d\n")
                                                   , taskQueue.getCount()
                                                   , nResCount1, available1
                                                   , nResCount2, available2);

            break;
        }

        // can release the task
        resQueue.removeHead();
        resQueue.removeHead();
        uint8_t taskId = taskQueue.removeHead();

        Task *pNextTask = scheduler.getTask(taskId);
        if (pNextTask) {
            available1 -= nResCount1;
            available2 -= nResCount2;

            serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::resuming #%d: a1 %d - nA1 %d, a2:%d - nA2 %d\n")
                                                   , pNextTask->getIndex()
                                                   , nResCount1, available1
                                                   , nResCount2, available2);

            pNextTask->resume(0);
        }
    }
}

void Res2Lock::useAvailable2(uint8_t available2) {
    uint8_t wasAvalable2 = nAvailable2;

    if (available2 > nAvailable2) {
        nAvailable2 = 0;
    } else {
        nAvailable2 -= available2;
    }

    // serialDebugResourceDetailTracePrintf_P(PSTR("Res2Lock::use2 a2:%d - nA2:%d -> %d\n")
    //                                  , available2
    //                                  , wasAvalable2
    //                                  , nAvailable2);

}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void Res2Lock::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    addActualOutput("%s", indentStr);

    addActualOutput("%sRes2Lock { max1:%d, avail1:%d max2:%d, avail2:%d\n", indentStr, nMaxAvailable1, nAvailable1, nMaxAvailable2, nAvailable2);
    taskQueue.dump(indent + 2, compact);
    resQueue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
