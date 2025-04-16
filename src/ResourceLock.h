#ifndef SCHEDULER_RESOURCELOCK_H
#define SCHEDULER_RESOURCELOCK_H

#include "Scheduler.h"
#include "Queue.h"

// sharable resource to be used in Task and YieldingTask calls

class ResourceLock {
    Queue queue;

public:
    ResourceLock(uint8_t *queueBuffer, uint8_t queueSize);

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserveResource(Task *pTask);

    /**
     * Release resource from the current task and resume next task in line giving it the resource
     *
     */
    void releaseResource();

    /**
     * Transfer ownership of the resource to the given task. Resource should be owned by the task at the
     * head of the queue.
     *
     * NOTE: if the resource is currently not owned this will make the given task its new owner
     *
     * @param pTask new owner of resource
     */
    void transferResource(Task *pTask);
};

#endif //SCHEDULER_RESOURCELOCK_H
