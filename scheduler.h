#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

//#define DEBUG_SCHEDULER_ERRORS
//#define DEBUG_SCHEDULER
//#define DEBUG_SCHEDULER_RUN
//#define DEBUG_SCHEDULER_DELAYS           // trace delays

#if defined(DEBUG_SCHEDULER) || defined(DEBUG_SCHEDULER_RUN) || defined(DEBUG_SCHEDULER_ERRORS)
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
    void suspend();
    bool isSuspended();
    void resume(uint16_t milliseconds);
    uint8_t getIndex() { return index; }
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
    void loop(uint16_t timeSlice = 0);  // loop tasks, return when all ready tasks have been loop once or time slice in ms exceeded

private:
    // task scheduling interface intended to be used by Task
    void suspend(Task *task);           // suspend rescheduling
    void resume(Task *task, uint16_t milliseconds);
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
