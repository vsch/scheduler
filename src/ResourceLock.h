#ifndef SCHEDULER_RESOURCELOCK_H
#define SCHEDULER_RESOURCELOCK_H

#include "Scheduler.h"
#include "Queue.h"
#include "SignalBase.h"

// sharable resource to be used in Task and AsyncTask calls

class ResourceLock : public SignalBase {
    uint8_t ownerReserveCount;  // counts how many times the owner reserved the lock, will release when this goes to 0
    Queue queue;

public:
    ResourceLock(uint8_t *queueBuffer, uint8_t queueSize);

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * NOTE: the current owner can call reserveResource again, and will obtain the resource. However,
     *     the owner must release the resource as many times as it has called reserve. Only the last call to
     *     release resource will actually release the resource.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserveResource(Task *pTask);
    uint8_t reserveResource(uint8_t taskId);

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
    void transferResource(uint8_t taskId);
};

#endif //SCHEDULER_RESOURCELOCK_H
