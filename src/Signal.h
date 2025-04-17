#ifndef SCHEDULER_SIGNAL_H
#define SCHEDULER_SIGNAL_H

#include "Scheduler.h"
#include "ByteQueue.h"
#include "SignalBase.h"

// allows tasks to wait for signal from other tasks/locations
class Signal : protected SignalBase {
    ByteQueue *pQueue;

public:
    Signal(uint8_t *queueData, uint8_t queueDataSize);

    /**
     * Wait for signal to resumeTask
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not available and could not yield to addWaitingTask
     *           for it
     */
    uint8_t addWaitingTask(Task *pTask);

    /**
     * Resume all tasks waiting for signal
     *
     */
    void trigger();

    uint8_t isEmpty() const;
    uint8_t isFull() const;
};

#endif //SCHEDULER_SIGNAL_H
