#ifndef SCHEDULER_MUTEX_H
#define SCHEDULER_MUTEX_H

#include "Scheduler.h"
#include "Queue.h"

// sharable resource to be used in Task and AsyncTask calls

class Mutex
{
    friend class Controller;

    Queue queue;

public:
    Mutex(uint8_t* queueBuffer, uint8_t queueSize);

    uint8_t isFree() const;

    uint8_t reserve(uint8_t taskId);
    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserve()
    {
        Task* pTask = scheduler.getTask();
        return pTask ? reserve(pTask->getIndex()) : NULL_TASK;
    }

    /**
     * Release resource from the current task and resume next task in line giving it the resource
     *
     */
    uint8_t release();

    /**
     * Transfer ownership of the resource to the given task if the mutex is owned by the task at the
     * head of the queue, ie. currently active task.
     *
     * @param fromTaskId
     * @param toTaskId new owner of resource
     * @return 0 if done, NULL_TASK if current task is not the owner
     */
    uint8_t transfer(uint8_t fromTaskId, uint8_t toTaskId);
    bool isOwner(uint8_t taskId);
    inline bool isOwner(Task* pTask) { return isOwner(pTask->getIndex()); }

#ifdef CONSOLE_DEBUG
    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);
#endif

    void reset();
};

#endif //SCHEDULER_MUTEX_H
