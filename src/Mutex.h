#ifndef SCHEDULER_MUTEX_H
#define SCHEDULER_MUTEX_H

#include "Scheduler.h"
#include "ByteQueue.h"

// sharable resource to be used in Task and AsyncTask calls

class Mutex
{
    friend class Controller;

protected:
    ByteQueue queue;

public:
    inline Mutex(uint8_t *queueBuffer, uint8_t queueSize)
            : queue(queueBuffer, queueSize) {
    }

    inline void reset() {
        queue.reset();
    }

    inline uint8_t isFree() const {
        return queue.isEmpty();
    }

    inline uint8_t getCount() const {
        return queue.getCount();
    }

    inline uint8_t getSize() const {
        return queue.getSize();
    }

    uint8_t reserve(uint8_t taskId);
    /**
     * Get resource if available or suspend calling task until it is available.
     * If the resource is not available, suspend the task and if possible yield.
     *
     * @return 0 if successfully yielded and resource reserved. 1 if not available and could not yield to wait
     *           for it, in case of non-async tasks.
     */
    uint8_t reserve()
    {
        Task* pTask = scheduler.getCurrentTask();
        return pTask ? reserve(pTask->getTaskId()) : NULL_TASK;
    }

    /**
     * Release resource from the current task and resume next task in line giving it the resource
     *
     */
    uint8_t release();

    inline bool isOwner(uint8_t taskId) {
        return queue.peekHead() == taskId;
    }

    inline bool isOwner(Task* pTask) { return isOwner(pTask->getTaskId()); }

    inline uint8_t getOwner() {
        return queue.peekHead();
    }

#ifdef CONSOLE_DEBUG
    // print out queue for testing
    void dump(uint8_t indent, uint8_t compact);
#endif

};

#endif //SCHEDULER_MUTEX_H
