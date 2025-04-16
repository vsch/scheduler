#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <avr/pgmspace.h>
#include "TinySwitcher.h"

#if defined(SERIAL_DEBUG_SCHEDULER) || defined(SERIAL_DEBUG_SCHEDULER_ERRORS) || defined(SERIAL_DEBUG_SCHEDULER_MAX_STACKS) || defined(CONSOLE_DEBUG)
#define SCHEDULER_TASK_IDS
#define defineSchedulerTaskId(str)  virtual PGM_P id() { return PSTR(str); }
#else
#define defineSchedulerTaskId(id)
#endif

#define INFINITE_DELAY  0xffff
#define NULL_TASK  0xff

class Scheduler;

// this must be declared in the main sketch
extern Scheduler scheduler;

class Task {
    friend class Scheduler;
    uint8_t index;         // getTask index in scheduler

protected:
    virtual void begin() = 0;            // begin getTask
    virtual void loop() = 0;             // loop getTask

public:
    virtual uint8_t isBlocking() {
        return false;
    }

#ifdef SCHEDULER_TASK_IDS
    virtual PGM_P id() = 0;        // printable id
#endif

    inline Task() {
        index = NULL_TASK;
    }

    /**
     * Suspend this getTask @see Scheduler::suspend()
     */
    void suspend();

    /**
     * Resume this getTask after given delay in milliseconds
     *
     * @param milliseconds delay in milliseconds to wait before resuming calls to loop()
     */
    void resume(uint16_t milliseconds);

    /**
     * return true if this getTask is currently suspended
     */
    bool isSuspended();

    /**
     * Return this task's index in Scheduler getTask table
     * @return
     */
    inline uint8_t getIndex() const {
        return index;
    }
};

// this task can call blocking wait functions of the scheduler
class YieldingTask : public Task {
    friend class Scheduler;

protected:
    Context context;

    virtual uint8_t isBlocking() {
        return true;
    }

public:
    /**
     * Constructor of a yielding task.
     *
     * Usually:
     *
     *
     * @param pStack
     * @param stackMax
     */
    inline YieldingTask(uint8_t *pStack, uint8_t stackMax) : Task() { // NOLINT(cppcoreguidelines-pro-type-member-init)
        initContext(&context, YieldingTask::yieldingLoop, this, pStack, stackMax);
    }

    /**
     * Suspend the task's execution and yield context
     * If successfully yielded, this function will return after the task is resumed.
     *
     * If the task is not the current context, it will not yield and return immediately.
     * Check return value for true if it did not yield and handle it, if needed.
     *
     * @return 0 if successfuly yielded. 1 if no yield because was not the active task
     */
    uint8_t yieldSuspend();

    /**
     * Set the resume milliseconds and yield the task's execution context. If successfully yielded, this
     * function will return after at least the given delay has passed.
     *
     * If the task is not the current context, it will not yield and return immediately.
     * Check return value for true if it did not yield and handle it, if needed.
     *
     * @param milliseconds delay in milliseconds to wait before resuming calls to loop()
     * @return 0 if successfuly yielded. 1 if no yield because was not the active task
     */
    uint8_t yieldResume(uint16_t milliseconds);

    /**
     * Yield cpu to other tasks. Returns to caller after the task was resumed.
     */
    void yield();

    /**
     * Test if the task has yielded or exited its loop function.
     *
     * @return 0 if has exited its loop function, otherwise it is the number of bytes in its saved context.
    */
    uint8_t hasYielded() const {
        return context.stackUsed;
    }

    /**
     * Get the maximum stack usage of the stack buffer for this task.
     *
     * @return maximum bytes of stack buffer used up to now during all previous resumptions.
     */
    uint8_t maxStackUsed() const {
        return context.stackMaxUsed;
    }

    /**
     * Test if the current task is the active context, i.e. it is currently running.
     *
     * @return 0 if not current context, else task context is the current one and the task can yield.
     */
    uint8_t isCurrentTask() const {
        return ::isCurrentContext(&context);
    };

private:
    static void yieldingLoop(void *arg) {
        ((YieldingTask *)arg)->loop();
    }
};

class Scheduler {
    friend class Task;
    friend class YieldingTask;

    uint8_t taskCount;              // getTask count
    uint8_t nextTask;               // id+1 of last getTask that was run when timeSlice ran out
    uint8_t inLoop;               // id+1 of last getTask that was run when timeSlice ran out

    uint16_t *delays;
    PGM_P tasks;
    uint32_t clockTick;             // clock tick for current delays (in micros)

#ifdef SERIAL_DEBUG_SCHEDULER
    uint16_t iteration;
#endif

#ifdef SERIAL_DEBUG_SCHEDULER_DELAYS
    void dumpDelays(PGM_P msg);
#endif

public:
    /**
     * Get task given by index
     *
     * @param index
     * @return
     */
    Task *getTask(uint8_t index);

    /**
     * Construct scheduler instance
     *
     * @param count         number of tasks in the table
     * @param taskTable     pointer to task table in PROGMEM
     * @param delayTable    pointer to task delay table in RAM
     */
    Scheduler(uint8_t count, PGM_P taskTable, uint16_t *delayTable);

    /**
     * Startup scheduler and call begin() of all tasks
     */
    void begin();

    /**
     * call loop() of ready tasks, return when all ready tasks have been called once or time slice in ms exceeded
     *
     * @param timeSlice     maximum time allotted to single loop() in ms, usually should be
     *                      set to minimum of all tasks' resume calls. 0 means no limit,
     *                      and will execute all ready tasks once. Otherwise, will execute as many tasks as complete within
     *                      the timeSlice and exit the loop. Next call to loop(), will continue with the next getTask, after
     *                      the last one that ran.
     */
    void loop(uint16_t timeSlice = 0);

#ifdef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
    void dumpMaxStackInfo();
#endif

    inline uint8_t canLoop() const {
        return !inLoop;
    }

private:
    bool reduceDelays(uint16_t milliseconds);

public:
    /**
     * Suspend task. The task's loop() will not be called until a resume() for the Task is called.
     *
     * @param task    pointer to getTask which to suspend
     *
     */
    void suspend(Task *task);           // suspend rescheduling

    /**
     * Resume getTask after given number of ms. The task's loop() will be called after given number,
     * or more, of ms has elapsed.
     *
     * @param task              pointer to getTask which to resume.
     * @param milliseconds      milliseconds to wait before resuming getTask
     *
     */
    void resume(Task *task, uint16_t milliseconds);

    /**
     * Return true if given getTask is currently suspended
     * @param task  getTask
     *
     * @return true if getTask is suspended
     */
    bool isSuspended(Task *task);
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

#ifdef SERIAL_DEBUG
#else
#undef SERIAL_DEBUG_SCHEDULER
#undef SERIAL_DEBUG_SCHEDULER_ERRORS
#undef SERIAL_DEBUG_SCHEDULER_DELAYS
#undef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
#endif

#ifdef SERIAL_DEBUG
#define serialDebugPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugPrintf_P(...) ((void)0)
#define serialDebugPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_SCHEDULER_DELAYS
#define serialDebugSchedulerDumpDelays(msg) dumpDelays(PSTR(msg))
#else
#define serialDebugSchedulerDumpDelays(msg) ((void)0)
#endif

#ifdef SERIAL_DEBUG_SCHEDULER
#define debugSchedulerPrintf_P(...) printf_P(__VA_ARGS__)
#define debugSchedulerPuts_P(...) puts_P(__VA_ARGS__)
#else
#define debugSchedulerPrintf_P(...) ((void)0)
#define debugSchedulerPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
#define debugSchedulerMaxStacksPrintf_P(...) printf_P(__VA_ARGS__)
#define debugSchedulerMaxStacksPuts_P(...) puts_P(__VA_ARGS__)
#else
#define debugSchedulerMaxStacksPrintf_P(...) ((void)0)
#define debugSchedulerMaxStacksPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_SCHEDULER_ERRORS
#define debugSchedulerErrorsPrintf_P(...) printf_P(__VA_ARGS__)
#define debugSchedulerErrorsPuts_P(...) puts_P(__VA_ARGS__)
#else
#define debugSchedulerErrorsPrintf_P(...) ((void)0)
#define debugSchedulerErrorsPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_SCHEDULER_DELAYS
#define debugSchedulerDelaysPrintf_P(...) printf_P(__VA_ARGS__)
#define debugSchedulerDelaysPuts_P(...) puts_P(__VA_ARGS__)
#else
#define debugSchedulerDelaysPrintf_P(...) ((void)0)
#define debugSchedulerDelaysPuts_P(...) ((void)0)
#endif

#endif //_SCHEDULER_H_
