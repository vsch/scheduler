#ifndef SCHEDULER_SIGNALS_H
#define SCHEDULER_SIGNALS_H

#include "Scheduler.h"
#include "ByteQueue.h"

// allows tasks to wait for signal from other tasks/locations

class Signal {
    ByteQueue queue;

public:
    Signal(uint8_t *queueBuffer, uint8_t queueSize)
            : queue(queueBuffer, queueSize) {
    }


    /**
     * Wait for signal to trigger
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t wait(Task *pTask);

    inline uint8_t wait() {
        return wait(scheduler.getCurrentTask());
    }

    /**
     * Resume all tasks waiting for signal
     *
     */
    void trigger();

#ifdef CONSOLE_DEBUG
    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);
#endif
};

#endif //SCHEDULER_SIGNALS_H
