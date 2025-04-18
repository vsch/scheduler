#ifndef SCHEDULER_SIGNAL_H
#define SCHEDULER_SIGNAL_H

#include "Scheduler.h"
#include "ByteQueue.h"

// allows tasks to wait for signal from other tasks/locations

class Signal {
    ByteQueue queue;

public:
    Signal(uint8_t *queueBuffer, uint8_t queueSize);

    /**
     * Wait for signal to trigger
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t wait(Task *pTask);

    /**
     * Resume all tasks waiting for signal
     *
     */
    void trigger();
};

#endif //SCHEDULER_SIGNAL_H
