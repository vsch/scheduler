#ifndef SCHEDULER_SIGNAL_H
#define SCHEDULER_SIGNAL_H

#include "Scheduler.h"
#include "SignalBase.h"
#include "ByteQueue.h"

// allows tasks to wait for signal from other tasks/locations
class Signal : public SignalBase {
    ByteQueue *pQueue;

public:
    inline Signal(uint8_t *queueData, uint8_t queueDataSize) {
        byteQueue_construct(queueData, queueDataSize);
        this->pQueue = (ByteQueue *)queueData;
    }

    /**
     * Wait for signal to triggerTask
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not available and could not yield to wait
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
