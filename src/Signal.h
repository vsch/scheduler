#ifndef SCHEDULER_SIGNAL_H
#define SCHEDULER_SIGNAL_H

#include "Scheduler.h"
#include "SignalBase.h"
#include "Queue.h"

// allows tasks to wait for signal from other tasks/locations

class Signal : public SignalBase {
    Queue queue;

public:
    Signal(uint8_t *queueBuffer, uint8_t queueSize);

    /**
     * Wait for signal to triggerTask
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
