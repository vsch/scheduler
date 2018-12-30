#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

//#define DEBUG_SCHEDULER_ERRORS
//#define DEBUG_SCHEDULER
//#define DEBUG_SCHEDULER_RUN
//#define DEBUG_SCHEDULER_DELAYS           // trace delays

class Task {
    friend class Scheduler;
    // main task broken into states at delay() boundaries

protected:
    virtual void loop() = 0;             // loop task
    virtual void begin() = 0;            // begin task
    virtual const __FlashStringHelper *id() = 0;       // printable id

    // these must be called from within loop of this task
    void delay(uint16_t milliseconds); // loop next state after delay, 0 yields and advances state
    void suspend();

    // this can be called from anywhere
    boolean isSuspended();
    void resume(uint16_t milliseconds);
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
    uint8_t currentTask;        // id of current task
    uint8_t nextTask;           // id of next task which was ready

#ifdef DEBUG_SCHEDULER_RUN
    void dumpDelays(const __FlashStringHelper *msg);
#endif

    uint8_t taskIndex(Task *task);     // task index or NULL_TASK
    void setDelay(uint8_t index, uint16_t milliseconds);
    Task *task(uint8_t id);

public:
    Scheduler(uint8_t taskCount, PGM_P tasks, uint16_t *delays);
    void begin();                       // start scheduler
    void loop(uint16_t timeSlice = 0);  // loop tasks, return when all ready tasks have been loop once or time slice in ms exceeded

private:
    // task scheduling interface
    void delay(uint16_t milliseconds);  // reschedule current task in milliseconds
    void suspend();                     // suspend rescheduling
    void resume(Task *task, uint16_t milliseconds);
    bool isSuspended(Task *task);
};

// this must be declared in the main sketch
extern Scheduler scheduler;

#endif //_SCHEDULER_H_
