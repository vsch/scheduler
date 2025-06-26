#include "Signals.h"
#include "Scheduler.h"

#ifdef CONSOLE_DEBUG

#include "../tests/FileTestResults_AddResult.h"

#endif // CONSOLE_DEBUG

uint8_t Signal::wait(Task *pTask) {
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

void Signal::trigger() {
    while (!queue.isEmpty()) {
        // give to this task
        uint8_t head = queue.removeHead();
        Task *pNextTask = scheduler.getTask(head);

        if (pNextTask) {
#ifdef CONSOLE_DEBUG
            addActualOutput("Resuming task %d\n", pNextTask->getIndex());
#endif // CONSOLE_DEBUG
            pNextTask->resume(0);
        }
    }
}

#ifdef CONSOLE_DEBUG

#include "tests/FileTestResults_AddResult.h"

// print out queue for testing
void Signal::dump(uint8_t indent, uint8_t compact) {
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    // Output: Queue { nSize:%d, nHead:%d, nTail:%d
    // 0xdd ... [ 0xdd ... 0xdd ] ... 0xdd
    // }
    addActualOutput("%s", indentStr);

    addActualOutput("%sSignal {\n", indentStr);
    queue.dump(indent + 2, compact);
    addActualOutput("%s}\n", indentStr);
}

#endif
