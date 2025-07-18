
#ifndef SCHEDULER_RESLOCK_H
#define SCHEDULER_RESLOCK_H

#include "Scheduler.h"
#include "ByteQueue.h"
#include "debug_config.h"

// sharable resource to be used in Task and AsyncTask calls

#define RLOCK_TASK_QUEUE_SIZE(maxTasks)             (sizeOfQueue((maxTasks), uint8_t))
#define RLOCK_RES_QUEUE_SIZE(maxTasks)              (sizeOfQueue((maxTasks), uint8_t))

#define RLOCK_TASK_QUEUE_OFFS(maxTasks)             (0)
#define RLOCK_RES_QUEUE_OFFS(maxTasks)              (RLOCK_TASK_QUEUE_OFFS(maxTasks) + RLOCK_TASK_QUEUE_SIZE(maxTasks))
#define RLOCK_NEXT_MEMBER_OFFS(maxTasks)            (RLOCK_RES_QUEUE_OFFS(maxTasks) + RLOCK_RES_QUEUE_SIZE(maxTasks))

// Use this macro to allocate space for Res2Lock queues
#define sizeOfResLockBuffer(maxTasks)               (RLOCK_NEXT_MEMBER_OFFS(maxTasks))

class ResLock {
    friend class Controller;
    
    uint8_t owner;                  // task that owns the resource or NULL_TASK
    ByteQueue taskQueue;            // list of tasks waiting for resources.
    ByteQueue resQueue;             // required resources of waiting tasks

    const uint8_t nMaxAvailable1;    // max resources available
    uint8_t nAvailable1;    // amount of available resources

public:
    inline ResLock(uint8_t *semaBuffer, uint8_t maxTasks, uint8_t available1)
            : taskQueue(semaBuffer + RLOCK_TASK_QUEUE_OFFS(maxTasks), RLOCK_TASK_QUEUE_SIZE(maxTasks))
              , resQueue(semaBuffer + RLOCK_RES_QUEUE_OFFS(maxTasks), RLOCK_RES_QUEUE_SIZE(maxTasks))
              , nMaxAvailable1(available1) {
        nAvailable1 = nMaxAvailable1;
        owner = NULL_TASK;
    }

    inline void reset() {
        nAvailable1 = nMaxAvailable1;
        taskQueue.reset();
        resQueue.reset();
        owner = NULL_TASK;
    }

    inline uint8_t isFree() const {
        return owner == NULL_TASK && taskQueue.isEmpty();
    }

    inline uint8_t isAvailable(uint8_t available1) const {
        return nAvailable1 >= available1;
    }

    inline uint8_t isMaxAvailable(uint8_t available1) const {
        return nMaxAvailable1 >= available1;
    }

    inline uint8_t getAvailable() const {
        return nAvailable1;
    }

    inline uint8_t getMaxAvailable() const {
        return nMaxAvailable1;
    }

    inline void useAvailable(uint8_t available1) {
        if (available1 > nAvailable1) {
            nAvailable1 = 0;
        } else {
            nAvailable1 -= available1;
        }
    }

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     * 
     * @param taskId        id of task
     * @param available1        amount of desired resources
     * @return 
     */
    uint8_t reserve(uint8_t taskId, uint8_t available1);

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not avaialble and could not yield to wait
     *           for it
     */
    uint8_t reserve(uint8_t available1) {
        Task *pTask = scheduler.getCurrentTask();
        return pTask ? reserve(pTask->getTaskId(), available1) : NULL_TASK;
    }
    
    void release();

    /**
         * Release resource from the current task and resume next task in line giving it the resource when the resource 
         * count becomes available
         *
         */
    void makeAvailable(uint8_t available1);

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);

#endif
};

#endif //SCHEDULER_SEMAPHORE_H

