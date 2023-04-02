#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <avr/pgmspace.h>

//#define DEBUG_SCHEDULER_ERRORS
//#define DEBUG_SCHEDULER
//#define DEBUG_SCHEDULER_RUN
//#define DEBUG_SCHEDULER_DELAYS           // trace delays

#if defined(DEBUG_SCHEDULER) || defined(DEBUG_SCHEDULER_RUN) || defined(DEBUG_SCHEDULER_ERRORS) || defined(CONSOLE_DEBUG)
#define SCHEDULER_TASK_IDS
#endif

class Scheduler;
// this must be declared in the main sketch
extern Scheduler scheduler;

class Task {
    friend class Scheduler;
    uint8_t index;         // task index in scheduler

protected:
    virtual void begin() = 0;            // begin task
    virtual void loop() = 0;             // loop task
#ifdef SCHEDULER_TASK_IDS
    virtual const __FlashStringHelper *id() = 0;       // printable id
#endif

    Task();

public:
    /**
     * Suspend this task @see Scheduler::suspend()
     */
    void suspend();

    /**
     * Resume this task after given delay in milliseconds
     *
     * @param milliseconds delay in milliseconds to wait before resuming calls to loop()
     */
    void resume(uint16_t milliseconds);

    /**
     * return true if this task is currently suspended
     */
    bool isSuspended();

    /**
     * Return this task's index in Scheduler task table
     * @return
     */
    inline uint8_t getIndex() {
        return index;
    }
};

class Scheduler {
    friend class Task;

    uint8_t taskCount;         // task count
    uint16_t *delays;
    PGM_P tasks;
#ifdef DEBUG_SCHEDULER
    uint32_t iteration;
#endif

    unsigned long clockTick;    // clock tick for current delays (in micros)
    uint8_t nextTask;           // id of next task which was ready

#ifdef DEBUG_SCHEDULER_RUN
    void dumpDelays(const __FlashStringHelper *msg);
#endif

    Task *task(uint8_t id);

public:
    Scheduler(uint8_t count, PGM_P taskTable, uint16_t *delayTable);
    void begin();                       // start scheduler
    /**
     * call loop() of ready tasks, return when all ready tasks have been called once or time slice in ms exceeded
     *
     * @param timeSlice     maximum time allotted to single loop() in ms, usually should be
     *                      set to minimum of all tasks' resume calls. 0 means no limit,
     *                      execute all ready tasks once.
     *
     */
    void loop(uint16_t timeSlice = 0);

private:
    /**
     * Suspend task. The task's loop() will not be called until a resume() for the task is called.
     *
     * @param task    pointer to task which to suspend
     *
     */
    void suspend(Task *task);           // suspend rescheduling

    /**
     * Resume task after given number of ms. The task's loop() will be called after given number,
     * or more, of ms has elapsed.
     *
     * @param task              pointer to task which to resume.
     * @param milliseconds      milliseconds to wait before resuming task
     *
     */
    void resume(Task *task, uint16_t milliseconds);

    /**
     * Return true if given task is currently suspended
     * @param task  task
     *
     * @return true if task is suspended
     */
    bool isSuspended(Task *task);

    bool reduceDelays(uint16_t milliseconds);
};

inline void Task::resume(uint16_t milliseconds) {
    scheduler.resume(this, milliseconds);
}

inline void Task::suspend() {
    scheduler.suspend(this);
}

inline bool Task::isSuspended() {
    return scheduler.isSuspended(this);
}

#endif //_SCHEDULER_H_
