
#ifndef SCHEDULER_RES2LOCK_H
#define SCHEDULER_RES2LOCK_H

#include "Scheduler.h"
#include "ByteQueue.h"
#include "debug_config.h"

// sharable resource to be used in Task and AsyncTask calls

#define RLOCK_TASK_QUEUE_SIZE(maxTasks)             (sizeOfQueue((maxTasks), uint8_t))
#define RLOCK_RES_QUEUE_SIZE(maxTasks)              (sizeOfQueue((maxTasks)*2, uint8_t))

#define RLOCK_TASK_QUEUE_OFFS(maxTasks)             (0)
#define RLOCK_RES_QUEUE_OFFS(maxTasks)              (RLOCK_TASK_QUEUE_OFFS(maxTasks) + RLOCK_TASK_QUEUE_SIZE(maxTasks))
#define RLOCK_NEXT_MEMBER_OFFS(maxTasks)            (RLOCK_RES_QUEUE_OFFS(maxTasks) + RLOCK_RES_QUEUE_SIZE(maxTasks))

// Use this macro to allocate space for Res2Lock queues
#define sizeOfRes2LockBuffer(maxTasks)               (RLOCK_NEXT_MEMBER_OFFS(maxTasks))

class Res2Lock {
    friend class Controller;
    
    uint8_t owner;                  // task that owns the resource or NULL_TASK
    ByteQueue taskQueue;            // list of tasks waiting for resources.
    ByteQueue resQueue;             // required resources of waiting tasks

    const uint8_t nMaxAvailable1;    // max resources available
    const uint8_t nMaxAvailable2;    // max resources available
    uint8_t nAvailable1;    // amount of available resources
    uint8_t nAvailable2;    // amount of available resources

public:
    inline Res2Lock(uint8_t *semaBuffer, uint8_t maxTasks, uint8_t available1, uint8_t available2)
              : taskQueue(semaBuffer + RLOCK_TASK_QUEUE_OFFS(maxTasks), RLOCK_TASK_QUEUE_SIZE(maxTasks))
              , resQueue(semaBuffer + RLOCK_RES_QUEUE_OFFS(maxTasks), RLOCK_RES_QUEUE_SIZE(maxTasks))
              , nMaxAvailable1(available1)
              , nMaxAvailable2(available2) {
        nAvailable1 = nMaxAvailable1;
        nAvailable2 = nMaxAvailable2;
        owner = NULL_TASK;
    }

    // IMPORTANT: must be called with interrupts disabled 
    inline void reset() {
        nAvailable1 = nMaxAvailable1;
        nAvailable2 = nMaxAvailable2;
        owner = NULL_TASK;
        taskQueue.reset();
        resQueue.reset();
    }

    NO_DISCARD inline uint8_t isFree() const {
        return owner == NULL_TASK && taskQueue.isEmpty();
    }

    NO_DISCARD inline uint8_t isAvailable(uint8_t available1, uint8_t available2) const {
        return nAvailable1 >= available1 && nAvailable2 >= available2;
    }

    NO_DISCARD inline uint8_t isMaxAvailable(uint8_t available1, uint8_t available2) const {
        return nMaxAvailable1 >= available1 && nMaxAvailable2 >= available2;
    }

    NO_DISCARD inline uint8_t getAvailable1() const {
        return nAvailable1;
    }

    NO_DISCARD inline uint8_t getAvailable2() const {
        return nAvailable2;
    }

    NO_DISCARD inline uint8_t getMaxAvailable1() const {
        return nMaxAvailable1;
    }

    NO_DISCARD inline uint8_t getMaxAvailable2() const {
        return nMaxAvailable2;
    }

    inline void useAvailable1(uint8_t available1) {
        if (available1 > nAvailable1) {
            nAvailable1 = 0;
        } else {
            nAvailable1 -= available1;
        }
    }

    inline void useAvailable2(uint8_t available2) {
        if (available2 > nAvailable2) {
            nAvailable2 = 0;
        } else {
            nAvailable2 -= available2;
        }
    }

    inline void useAvailable(uint8_t available1, uint8_t available2) {
        useAvailable1(available1);
        useAvailable2(available2);
    }

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     * 
     * @param taskId        id of task
     * @param available1    amount of desired resource 1
     * @param available2    amount of desired resource 2
     * @return              0 if available, 1 if need to suspend, NULL_BYTE if can never be satisfied
     */
    uint8_t reserve(uint8_t taskId, uint8_t available1, uint8_t available2);

    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @param available1    amount of desired resource 1
     * @param available2    amount of desired resource 2
     * @return              0 if available, 1 if need to suspend, NULL_BYTE if can never be satisfied
     */
    uint8_t reserve(uint8_t available1, uint8_t available2) {
        Task *pTask = scheduler.getCurrentTask();
        return pTask ? reserve(pTask->getTaskId(), available1, available2) : NULL_TASK;
    }
    
    void release();

    /**
     * Release resource from the current task and resume next task in line giving it the resource when the resource 
     * count becomes available
     *
     * @param available1    amount of desired resource 1
     * @param available2    amount of desired resource 2
     */
    void makeAvailable(uint8_t available1, uint8_t available2);

#ifdef CONSOLE_DEBUG

    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);

#endif
};

#endif //SCHEDULER_SEMAPHORE_H

