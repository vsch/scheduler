#ifndef SCHEDULER_MUTEX_H
#define SCHEDULER_MUTEX_H

#include "Scheduler.h"
#include "Queue.h"

// sharable resource to be used in Task and AsyncTask calls

class Mutex {
    Queue queue;

public:
    Mutex(uint8_t *queueBuffer, uint8_t queueSize);

    uint8_t isFree() const;

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserve();

    /**
     * Release resource from the current task and resume next task in line giving it the resource
     *
     */
    void release();

    /**
     * Transfer ownership of the resource to the given task if the mutex is owned by the task at the
     * head of the queue, ie. currently active task.
     *
     * @param pTask new owner of resource
     * @return 0 if done, NULL_TASK if current task is not the owner
     */
    uint8_t transfer(Task *pTask);
    bool isOwner(uint8_t taskId);
    inline bool isOwner(Task *pTask) { return isOwner(pTask->getIndex()); }
};

#endif //SCHEDULER_MUTEX_H
