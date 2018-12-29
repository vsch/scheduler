#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <stdint-gcc.h>

#define INFINITE_DELAY  0xffffffff
#define NULL_TASK  0xff

#define DEBUG_SCHEDULER
//#define DEBUG_SCHEDULER_RUN
//#define DEBUG_SCHEDULER_DELAYS           // trace delays

class Task {
    friend class Scheduler;
    // main task broken into states at delay() boundaries
protected:
    virtual void run() = 0;             // run task
    virtual void init() = 0;            // init task
    virtual const char *id() = 0;       // printable id

    // these must be called from within run of current task
    void delay(uint16_t milliseconds); // run next state after delay, 0 yields and advances state
    void suspend();

    // this can be called outside of current task
    void resume(uint16_t milliseconds);
};

class PeriodicTask : public Task {
    friend class Scheduler;
    unsigned long expectedRun;
    unsigned long lastRun;
    int accumError;

public:
    PeriodicTask() {
        lastRun = 0;
        expectedRun = 0;
        accumError = 0;
    }

    // main task broken into states at delay() boundaries
protected:
    virtual void run() = 0;
    virtual void init() = 0;            // init task
    virtual const char *id() = 0;       // printable id

    void markRun();

    // these must be called from within run of current task
    void delay(uint16_t milliseconds); // run next state after delay, 0 yields and advances state
    void suspend();

    // this can be called outside of current task
    void resume(uint16_t milliseconds);
};

class Scheduler {
    friend class Task;
    friend class PeriodicTask;

    uint8_t taskCount;         // task count
    uint32_t *delays;
    Task **tasks;
#ifdef DEBUG_SCHEDULER
    uint32_t iteration;
#endif

    unsigned long clockTick;    // clock tick for current delays (in micros)
    uint8_t currentTask;        // id of current task
    uint8_t nextTask;           // id of next task which was ready

public:
    Scheduler(uint8_t taskCount, Task **tasks, uint32_t *delays);

    void start();      // start scheduler

    void run(uint16_t timeSlice);        // run tasks, return when all ready tasks have been run at least once or time slice in ms exceeded

    void dumpDelays(const char *msg);

protected:
    // task scheduling interface
    void delay(uint32_t microseconds);   // resched current task in milliseconds
    void suspend();                     // suspend rescheduling
    void resume(Task *task, uint32_t microseconds);
};

extern Scheduler scheduler;

#endif //_SCHEDULER_H_
