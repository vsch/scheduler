
#ifndef SCHEDULER_SEMAPHORE_H
#define SCHEDULER_SEMAPHORE_H

#include "Scheduler.h"
#include "ByteQueue.h"
#include "Mutex.h"
#include "debug_config.h"

// sharable resource to be used in Task and AsyncTask calls

#define RLOCK_MUTEX_SIZE(maxTasks)                 (sizeOfQueue((maxTasks), uint8_t))
#define RLOCK_TASK_QUEUE_SIZE(maxTasks)             (sizeOfQueue((maxTasks), uint8_t))
#define RLOCK_RES_QUEUE_SIZE(maxTasks)              (sizeOfQueue((maxTasks), uint8_t))

#define RLOCK_MUTEX_OFFS(maxTasks)                  (0)
#define RLOCK_TASK_QUEUE_OFFS(maxTasks)             (RLOCK_MUTEX_OFFS(maxTasks) + RLOCK_MUTEX_SIZE(maxTasks))
#define RLOCK_RES_QUEUE_OFFS(maxTasks)              (RLOCK_TASK_QUEUE_OFFS(maxTasks) + RLOCK_TASK_QUEUE_SIZE(maxTasks))
#define RLOCK_NEXT_MEMBER_OFFS(maxTasks)            (RLOCK_RES_QUEUE_OFFS(maxTasks) + RLOCK_RES_QUEUE_SIZE(maxTasks))

// Use this macro to allocate space for Res2Lock queues
#define sizeOfRes2LockBuffer(maxTasks)               (RLOCK_NEXT_MEMBER_OFFS(maxTasks))

class ResLock : private Mutex {
    friend class Controller;

    ByteQueue taskQueue;            // required resources of waiting tasks
    ByteQueue resQueue;             // required resources of waiting tasks

    const uint8_t nMaxAvailable;    // max resources available
    volatile uint8_t nAvailable;             // amount of available resources

public:
    inline ResLock(uint8_t *semaBuffer, uint8_t maxTasks, uint8_t available1)
            : Mutex(semaBuffer + RLOCK_MUTEX_OFFS(maxTasks), RLOCK_MUTEX_SIZE(maxTasks))
              , taskQueue(semaBuffer + RLOCK_TASK_QUEUE_OFFS(maxTasks), RLOCK_TASK_QUEUE_SIZE(maxTasks))
              , resQueue(semaBuffer + RLOCK_RES_QUEUE_OFFS(maxTasks), RLOCK_RES_QUEUE_SIZE(maxTasks))
              , nMaxAvailable(available1) {
        nAvailable = nMaxAvailable;
    }

    inline void reset() {
        Mutex::reset();
        nAvailable = nMaxAvailable;
        taskQueue.reset();
        resQueue.reset();
    }

    inline uint8_t isEmpty() const {
        return Mutex::isFree() && taskQueue.isEmpty();
    }

    inline uint8_t isAvailable(uint8_t available) const {
        return nAvailable >= available;
    }

    inline uint8_t isMaxAvailable(uint8_t available) const {
        return nMaxAvailable >= available;
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
     * @param available        amount of desired resources
     * @return 
     */
    uint8_t reserve(uint8_t taskId, uint8_t available);

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserve(uint8_t available) {
        Task *pTask = scheduler.getTask();
        return pTask ? reserve(pTask->getIndex(), available) : NULL_TASK;
    }

    inline void release() {
        Mutex::release();
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

