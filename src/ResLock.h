
#ifndef SCHEDULER_SEMAPHORE_H
#define SCHEDULER_SEMAPHORE_H

#include "Scheduler.h"
#include "ByteQueue.h"
#include "Mutex.h"

// sharable resource to be used in Task and AsyncTask calls

#define R2LOCK_TASK_QUEUE_SIZE(maxTasks)              (sizeOfQueue(maxTasks, uint8_t))
#define R2LOCK_RES_QUEUE_SIZE(maxTasks)               (sizeOfQueue(maxTasks, uint8_t))

#define R2LOCK_TASK_QUEUE_OFFS(maxTasks)              (0)
#define R2LOCK_RES_QUEUE_OFFS(maxTasks)               (R2LOCK_TASK_QUEUE_SIZE(maxTasks))

// Use this macro to allocate space for all the queues and buffers in the controller
#define sizeOfResLockBuffer(maxTasks) (0\
        + R2LOCK_TASK_QUEUE_SIZE(maxTasks) \
        + R2LOCK_RES_QUEUE_SIZE(maxTasks)  \
      )

class ResLock {
    friend class Controller;

    ByteQueue taskQueue;            // required resources of waiting tasks
    ByteQueue resQueue;             // required resources of waiting tasks

    const uint8_t nMaxAvailable;    // max resources available
    volatile uint8_t nAvailable;             // amount of available resources

public:
    inline ResLock(uint8_t *semaBuffer, uint8_t maxTasks, uint8_t available)
            : taskQueue(semaBuffer + R2LOCK_TASK_QUEUE_OFFS(maxTasks), R2LOCK_TASK_QUEUE_SIZE(maxTasks))
              , resQueue(semaBuffer + R2LOCK_RES_QUEUE_OFFS(maxTasks), R2LOCK_RES_QUEUE_SIZE(maxTasks))
              , nMaxAvailable(available) {
        nAvailable = nMaxAvailable;
    }

    inline void reset() {
        nAvailable = nMaxAvailable;
        taskQueue.reset();
        resQueue.reset();
    }

    inline uint8_t isAvailable(uint8_t count) const {
        return nAvailable >= count;
    }

    inline uint8_t isMaxAvailable(uint8_t count) const {
        return nMaxAvailable >= count;
    }

    inline uint8_t getAvailable() const {
        return nAvailable;
    }

    inline uint8_t getMaxAvailable() const {
        return nMaxAvailable;
    }

    inline void useAvailable(uint8_t count) {
        if (count > nAvailable) {
            nAvailable = 0;
        } else {
            nAvailable -= count;
        }
    }

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     * 
     * @param taskId        id of task
     * @param nCount        amount of desired resources
     * @return 
     */
    uint8_t reserve(uint8_t taskId, uint8_t nCount);

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserve(uint8_t nCount) {
        Task *pTask = scheduler.getTask();
        return pTask ? reserve(pTask->getIndex(), nCount) : NULL_TASK;
    }

    /**
     * Release resource from the current task and resume next task in line giving it the resource when the resource 
     * count becomes available
     *
     */
    void makeAvailable(uint8_t available);

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);

#endif
};

#endif //SCHEDULER_SEMAPHORE_H

