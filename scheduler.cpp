#include "Arduino.h"
#include "scheduler.h"

Scheduler::Scheduler(uint8_t taskCount, Task **tasks, uint32_t *delays) {
    this->taskCount = taskCount;
    this->tasks = tasks;
    this->delays = delays;
    this->clockTick = 0;
    this->currentTask = NULL_TASK;
    this->nextTask = 0;
#ifdef DEBUG_SCHEDULER
    iteration = 0;
#endif
}

void Scheduler::start() {
    clockTick = micros();

    nextTask = NULL_TASK;

    for (uint8_t i = 0; i < taskCount; i++) {
        currentTask = i;
        delays[i] = 0;
        tasks[i]->init();
    }

    nextTask = 0;
    currentTask = NULL_TASK;

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays("Scheduler after start ");
#endif
}

void Scheduler::dumpDelays(const char *msg){
    Serial.print(msg);

    for (uint8_t i = 0; i < taskCount; i++) {
        Serial.print(" ");
        Serial.print(tasks[i]->id());
        Serial.print("[");
        Serial.print(delays[i]);
        Serial.print("] ");
    }

    Serial.println();
}

void Scheduler::run(uint16_t timeSlice) {
    unsigned long tick = micros();
    long diff = tick - clockTick;
    if (diff < 1000) return;

    bool haveTasks = false;

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays("Scheduler run before ");
#endif

    for (uint8_t i = 0; i < taskCount; i++) {
        uint32_t i1 = delays[i];
        if (i1) {
            if (i1 != INFINITE_DELAY) {
                if (i1 <= diff) {
                    delays[i] = 0;
                    haveTasks = true;
                } else {
                    delays[i] -= diff;
                }
            }
        } else {
            haveTasks = true;
        }
    }

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays("Scheduler run after ");
#endif

    if (!haveTasks) return;

#ifdef DEBUG_SCHEDULER
    iteration++;
#endif

#ifdef DEBUG_SCHEDULER_RUN
    Serial.print("Scheduler run ");
    Serial.print(tick);
    Serial.print(" slice ");
    Serial.println(timeSlice);
#endif

    clockTick = tick;

    uint32_t timeSliceLimit = timeSlice * 1000;
    uint32_t lastRunTime = 0;

    for (uint8_t i = 0; i < taskCount; i++) {
        uint8_t id = (i + nextTask) % taskCount;

        if (delays[id] == 0) {
            if (timeSlice && id != nextTask) {
                unsigned long time = micros();
//                Serial.print("Scheduler tick ");
//                Serial.print(tick);
//                Serial.print(" time ");
//                Serial.println(time);

                if ((uint32_t) (time - tick) > timeSliceLimit) {
                    // ran out of time
                    nextTask = id;
#ifdef DEBUG_SCHEDULER
                    Serial.print("Scheduler[");
                    Serial.print(iteration);
                    Serial.print("] time slice ended ");
                    Serial.print(time - tick);
                    Serial.print(" ");
                    Serial.print(timeSliceLimit);
                    Serial.print(" last task ");
                    Serial.print(tasks[currentTask]->id());
                    Serial.print(" took ");
                    Serial.println(lastRunTime);
#endif
                    return;
                }
            }

            unsigned long start = micros();

            Task *pTask = tasks[id];
            currentTask = id;
            pTask->run();
            unsigned long end = micros();
            lastRunTime = end - start;

#ifdef DEBUG_SCHEDULER_RUN
            Serial.print("Scheduler task ");
            Serial.print(tasks[id]->id());
            Serial.print(" done in ");
            Serial.println(lastRunTime);
#endif
        }
    }

    currentTask = NULL_TASK;
    nextTask = 0;

#ifdef DEBUG_SCHEDULER_RUN
    unsigned long time = micros();
    Serial.print("Scheduler end run ");
    Serial.println(time - tick);
#endif
}

void Scheduler::resume(Task *task, uint32_t microseconds) {
    for (uint8_t i = 0; i < taskCount; i++) {
        if (tasks[i] == task) {
            delays[i] = microseconds;
            break;
        }
    }
}

void Scheduler::suspend() {
    // current task to delay by milliseconds
    if (currentTask == NULL_TASK) {
        Serial.println("Error: suspend() called with NULL_TASK");
    }
    delays[currentTask] = INFINITE_DELAY;
}

void Scheduler::delay(uint32_t milliseconds) {
    // current task to delay by milliseconds
    if (currentTask == NULL_TASK) {
        Serial.println("Error: delay() called with NULL_TASK");
    }
    delays[currentTask] = milliseconds;
}

void Task::delay(uint16_t milliseconds) {
    scheduler.delay((uint32_t) milliseconds * 1000);
}

void Task::resume(uint16_t milliseconds) {
    scheduler.resume(this, (uint32_t) milliseconds * 1000);
}

void Task::suspend() {
    scheduler.suspend();
}

void PeriodicTask::delay(uint16_t milliseconds) {
    if (scheduler.nextTask == NULL_TASK) {
        // from initialization, just regular delay
        scheduler.delay((uint32_t) milliseconds * 1000);
    } else {
        int delay = accumError + (int) milliseconds * 1000;

        if (delay < 0) {
            delay = 0;
        }

        expectedRun = lastRun + delay;

        Serial.print("Periodic accum error ");
        Serial.print(accumError);
        Serial.print(" ");
        Serial.println(delay);

        scheduler.delay((uint32_t) delay);
    }
}

void PeriodicTask::resume(uint16_t milliseconds) {
    scheduler.resume(this, (uint32_t) milliseconds * 1000);
}

void PeriodicTask::suspend() {
    scheduler.suspend();
}

void PeriodicTask::markRun() {
    lastRun = micros();
    long lastRunError = expectedRun - lastRun;
    accumError += lastRunError;

    Serial.print("Periodic run error ");
    Serial.print(lastRunError);
    Serial.print(" ");
    Serial.println(accumError);
}

