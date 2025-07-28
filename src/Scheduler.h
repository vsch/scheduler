#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "Arduino.h"

#ifndef CONSOLE_DEBUG

#include <avr/pgmspace.h>

#else

#include "type_defs.h"

#endif

#include "TinySwitcher.h"
#include "common_defs.h"

#if defined(SERIAL_DEBUG_SCHEDULER) || defined(SERIAL_DEBUG_SCHEDULER_ERRORS) || defined(SERIAL_DEBUG_SCHEDULER_DELAYS) || defined(SERIAL_DEBUG_SCHEDULER_MAX_STACKS) || defined(CONSOLE_DEBUG)
#define SCHEDULER_TASK_IDS
#define defineSchedulerTaskId(str)  virtual PGM_P id() override { return PSTR(str); }
#else
#define defineSchedulerTaskId(id)
#endif

class Scheduler;

// this must be declared in the main sketch
extern Scheduler scheduler;

class Task {
    friend class Scheduler;
    uint8_t taskId;         // getTask index in scheduler

protected:
#ifdef SCHED_TASK_ACTIVE
    time_t activeTaskMicros;     // active micros, up to the last task switch, does not include time from scheduler.clockTick to micros()
#endif

    virtual void begin() = 0;            // begin getTask
    virtual void loop() = 0;             // loop getTask

public:
    virtual uint8_t isAsync() {
        return false;
    }

#ifdef SCHEDULER_TASK_IDS
    virtual PGM_P id() = 0;        // printable id
#endif

    inline Task() {
        taskId = NULL_TASK;
#ifdef SCHED_TASK_ACTIVE
        activeTaskMicros = 0;
#endif
    }

    /**
    * Return this task's index in Scheduler getCurrentTask table
    * @return
    */
    inline uint8_t getTaskId() const {
        return taskId;
    }

    NO_DISCARD inline time_t getActiveMicros() const {
#ifdef SCHED_TASK_ACTIVE
        return activeTaskMicros;
#else
        return micros();
#endif
    }

    NO_DISCARD time_t getCurrentActiveMicros() const;

    /**
    * Suspend this getCurrentTask @see Scheduler::suspend()
    */
    void suspend();

    /**
     * Resume this getCurrentTask after given delay in microseconds
     *
     * @param microseconds delay in microseconds to wait before resuming calls to loop()
     */
    void resumeMicros(time_t microseconds);

    /**
     * Resume this getCurrentTask after given delay in milliseconds
     *
     * @param milliseconds delay in milliseconds to wait before resuming calls to loop()
     */
    void resume(uint16_t milliseconds);

    /**
     * return true if this getCurrentTask is currently suspended
     */
    bool isSuspended();

    time_t getResumeMicros();

};

// this task can call blocking wait functions of the scheduler
class AsyncTask : public Task {
    friend class Scheduler;

protected:
    AsyncContext *pContext;

    uint8_t isAsync() override {
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
    AsyncTask(uint8_t *pStack, uint8_t stackMax);

    /**
     * Suspend the task's execution and yield context
     * If successfully yielded, this function will return after the task is resumed.
     *
     * If the task is not the current context, it will not yield and return immediately.
     * Check return value for true if it did not yield and handle it, if needed.
     */
    void yieldSuspend();

    /**
     * Set the resume milliseconds and yield the task's execution context. If successfully yielded, this
     * function will return after at least the given delay has passed.
     *
     * If there is no async context, it will not yield and return immediately.
     *
     * @param microseconds delay in microseconds before resuming task
     */
    void yieldResumeMicros(time_t microseconds);

    /**
     * Set the resume milliseconds and yield the task's execution context. If successfully yielded, this
     * function will return after at least the given delay has passed.
     *
     * If the task is not the current context, it will not yield and return immediately.
     *
     * @param milliseconds delay in milliseconds before resuming task
     */
    void yieldResume(uint16_t milliseconds);

    /**
     * Yield cpu to other tasks. Returns to caller after the task was resumed.
     */
    void yield();

    /**
     * Test if the task has yielded or exited its loop function. Applies outside the task
    */
    uint8_t hasYielded() const;

    /**
     * Get the maximum stack usage of the stack buffer for this task.
     *
     * @return maximum bytes of stack buffer used up to now during all previous resumptions.
     */
    uint8_t maxStackUsed() const;

private:
    static void yieldingLoop(void *arg);
};

#define TASK_DELAY_SUSPENDED    (0UL)
#define TASK_DELAY_MAX          (0x8000000UL)    // any delay >= this would be caused by wrap around at 0xffffffff

#define SCHED_FLAGS_IN_LOOP     (0x01)           // scheduler is currently in loop() execution

#ifndef SCHED_MIN_LOOP_TIMESLICE_MICROS
#define SCHED_MIN_LOOP_TIMESLICE_MICROS (250UL)      // least delay between loop() executions, ie. max resolution of task delay is this.
#endif

extern "C" uint8_t is_elapsed(time_t now, time_t endTime);
extern "C" int32_t elapsed_micros(time_t startTime, time_t endTime);

// inline time_t to_micros(uint16_t millis) {
//     return millis * 1000UL;
// }

class Scheduler {
    friend class Task;
    friend class AsyncTask;

    uint8_t flags;
    uint8_t taskCount;              // getTask count
    time_t *taskTimes;              // task ready timestamp
    PGM_P tasks;                    // pointer to task table

    // loop() invocation state variables
    uint8_t nextTask;               // id+1 of last getTask that was run when timeSlice ran out
    Task *pTask;                    // currently executing task or NULL if not in loop
    time_t startLoopMicros;               // clock tick for last scheduler.loop() invocation
    time_t startTaskMicros;             // micros for last task invocation


#ifdef SERIAL_DEBUG_SCHEDULER
    uint16_t iteration;
#endif

#ifdef SERIAL_DEBUG_SCHEDULER_DELAYS
    void dumpDelays(PGM_P msg);
#endif

public:

    inline uint8_t isInLoop() const {
        return flags & SCHED_FLAGS_IN_LOOP;
    }

    inline uint8_t canLoop() const {
        return !isInLoop();
    }

    inline time_t getStartLoopMicros() const {
        return startLoopMicros;
    }

    inline time_t getStartTaskMicros() const {
        return startTaskMicros;
    }

    /**
     * Get micros task is currently active, (now - startTaskMicros)
     *
     * @param now micros representing call to micros(), made after startTaskMicros
     * @return    micros task was active till now
     */
    time_t getCurrentActiveMicros(time_t now) const {
        return now - startTaskMicros;
    }

    /**
         * Test if the task is suspended
         * @param taskId of task to test
         * @return true if task is suspended
         */
    uint8_t isTaskSuspended(uint8_t taskId) {
        return !taskTimes[taskId];
    }

    /**
     * Test if the given task delay has expired
     * @param now       microseconds
     * @param endTime     task timestamp when it is ready to run.
     *
     * CAVEAT: maximum delay to resumeMicros() is 30 bits, because it is
     *  assumed that anything greater is caused by wrap around, which means
     *  waiting for micros to roll around. This means that there is a
     *  maximum of 1,073.741824 seconds delay.
     *
     * @return true if task is ready to run
     */
    inline static uint8_t isElapsed(time_t now, time_t endTime) {
        return is_elapsed(now, endTime);
    }

    /**
     * Get task given by index
     *
     * @param taskId
     * @return
     */
    Task *getTask(uint8_t taskId);

    /**
     * Get the currently running task or NULL if none.
     *
     * @return  currently running task or NULL if none.
     */
    inline Task *getCurrentTask() {
        return pTask;
    }

    /**
     * Get the currently running task id or NULL_TASK if none.
     *
     * @return  currently running task or NULL_TASK if none.
     */
    uint8_t getCurrentTaskId();

    /**
     * Construct scheduler instance
     *
     * @param count         number of tasks in the table
     * @param taskTable     pointer to task table in PROGMEM
     * @param delayTable    pointer to task delay table in RAM
     */
    Scheduler(uint8_t count, PGM_P taskTable, time_t *delayTable);

    /**
     * Startup scheduler and call begin() of all tasks
     */
    void begin();

    /**
     * call loop() of ready tasks, return when all ready tasks have been called once or time slice in ms exceeded
     *
     * @param timeSlice     maximum time allotted to single loop() in microseconds, usually should be
     *                      set to minimum of all tasks' resume calls. 0 means no limit,
     *                      and will execute all ready tasks once. Otherwise, will execute as many tasks as complete within
     *                      the timeSlice and exit the loop. Next call to loop(), will continue with the next getCurrentTask, after
     *                      the last one that ran.
     */
    void loopMicros(time_t timeSlice = 0);

    inline void loop(uint16_t timeSlice = 0) {
        loopMicros(timeSlice * 1000UL);
    }

    /**
     * Execute the given task as if in a scheduler loop. Used mainly for testing
     *
     * IMPORTANT: pTask is set to the task given by taskId but it is not cleared to NULL.
     *   Need to call clearCurrentTask() to clear it.
     *
     * @param taskId    task id to execute
     * @return          pointer to executed task or NULL
     */
    void executeTask(uint8_t taskId);

    inline void clearCurrentTask() {
        pTask = NULL;
    }

#ifdef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
    void dumpMaxStackInfo();
#endif

public:
    /**
     * Suspend task. The task's loop() will not be called until a resume() for the Task is called.
     *
     * @param task    pointer to getCurrentTask which to suspend
     *
     */
    void suspend(uint8_t taskId);           // suspend rescheduling

    inline void suspend(Task *task) {
        suspend(task->getTaskId());
    }

    /**
     * Resume getCurrentTask after given number of ms. The task's loop() will be called after given number,
     * or more, of ms has elapsed.
     *
     * @param taskId            task index
     * @param microseconds      microseconds to wait before resuming getCurrentTask, from micros()
     *
     */
    void resumeMicros(uint8_t taskId, time_t microseconds);
    time_t getResumeMicros(uint8_t taskId);

    inline void resumeMicros(Task *task, time_t microseconds) {
        resumeMicros(task->taskId, microseconds);
    }

    /**
     * Resume getCurrentTask after given number of ms. The task's loop() will be called after given number,
     * or more, of ms has elapsed.
     *
     * @param taskId            task index
     * @param milliseconds      milliseconds to wait before resuming getCurrentTask
     *
     */
    void resume(uint8_t taskId, uint16_t milliseconds) {
        resumeMicros(taskId, milliseconds * 1000UL);
    }

    void resume(Task *task, uint16_t milliseconds) {
        resume(task->taskId, milliseconds);
    }

    inline uint8_t isValidTaskId(uint8_t taskId) {
        return taskId < taskCount;
    }

    uint8_t isAsyncTask(uint8_t taskId);

    /**
     * Return true if given getCurrentTask is currently suspended
     * @param task  getCurrentTask
     *
     * @return true if getCurrentTask is suspended
     */
    bool isSuspended(Task *task);

#ifdef CONSOLE_DEBUG
    // print out queue for testing
    void dump(char *buffer, uint32_t sizeofBuffer, uint8_t indent = 0);
#endif
};

inline void Task::resumeMicros(time_t microseconds) {
    scheduler.resumeMicros(this, microseconds);
}

inline void Task::resume(uint16_t milliseconds) {
    scheduler.resume(this, milliseconds);
}

inline void Task::suspend() {
    scheduler.suspend(this);
}

inline bool Task::isSuspended() {
    return scheduler.isSuspended(this);
}

inline time_t Task::getResumeMicros() {
    return scheduler.getResumeMicros(taskId);
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
#ifdef CONSOLE_DEBUG
#define serialDebugPrintf_P(...) addActualOutput(__VA_ARGS__)
#else
#define serialDebugPrintf_P(...) ((void)0)
#define serialDebugPuts_P(...) ((void)0)
#endif
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
