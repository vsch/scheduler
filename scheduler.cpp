#include "Arduino.h"
#include "scheduler.h"

#define INFINITE_DELAY  0xffff
#define NULL_TASK  0xff

Scheduler::Scheduler(uint8_t taskCount, PGM_P tasks, uint16_t *delays) {
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

Task *Scheduler::task(uint8_t id) {
    return reinterpret_cast<Task *>(pgm_read_dword(tasks + sizeof(Task *) * id));
}

void Scheduler::begin() {
    memset(delays, 0, sizeof(*delays) * taskCount);

    clockTick = millis();
    nextTask = NULL_TASK;

    for (uint8_t i = 0; i < taskCount; i++) {
        currentTask = i;
        task(i)->begin();
    }

    nextTask = 0;
    currentTask = NULL_TASK;

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays(F("Scheduler after start "));
#endif
}

#ifdef DEBUG_SCHEDULER_RUN
void Scheduler::dumpDelays(const __FlashStringHelper *msg) {
    Serial.print(msg);

    for (uint8_t i = 0; i < taskCount; i++) {
        Serial.print(" ");
        Serial.print(task(i)->id());
        Serial.print("[");
        Serial.print(delays[i]);
        Serial.print("] ");
    }

    Serial.println();
}
#endif

void Scheduler::loop(uint16_t timeSlice) {
    unsigned long tick = micros();
    if (tick - clockTick < 1000) return;

    uint16_t diffMs = static_cast<uint16_t>((tick - clockTick) / 1000);

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays(F("Scheduler run before "));
    Serial.print(F("Tick "));
    Serial.print(tick);
    Serial.print(F(" clockTick "));
    Serial.print(clockTick);
    Serial.print(F(" diff "));
    Serial.print(tick - clockTick);
    Serial.print(F(" diffMs "));
    Serial.println(diffMs);
#endif

    bool haveTasks = false;
    for (uint8_t i = 0; i < taskCount; i++) {
        uint16_t delay = delays[i];
        if (delay) {
            if (delay != INFINITE_DELAY) {
                if (delay <= diffMs) {
                    delay = 0;
                    haveTasks = true;
                } else {
                    delay -= diffMs;
                }

                delays[i] = delay;
            }
        } else {
            haveTasks = true;
        }
    }

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays("Scheduler run after ");
#endif

    clockTick += (uint32_t) diffMs * 1000;

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

    uint32_t timeSliceLimit = timeSlice * 1000;
#if defined(DEBUG_SCHEDULER) || defined(DEBUG_SCHEDULER_RUN)
    uint32_t lastRunTime = 0;
#endif

    for (uint8_t i = 0; i < taskCount; i++) {
        uint8_t id = (i + nextTask) % taskCount;

        if (delays[id] == 0) {
            if (timeSlice && id != nextTask) {
                unsigned long time = micros();
//                Serial.print(F("Scheduler tick "));
//                Serial.print(tick);
//                Serial.print(F(" time "));
//                Serial.println(time);

                if ((uint32_t) (time - tick) > timeSliceLimit) {
                    // ran out of time
                    nextTask = id;
#ifdef DEBUG_SCHEDULER
                    Serial.print(F("Scheduler["));
                    Serial.print(iteration);
                    Serial.print(F("] time slice ended "));
                    Serial.print(time - tick);
                    Serial.print(F(" "));
                    Serial.print(timeSliceLimit);
                    Serial.print(F(" last task "));
                    Serial.print(task(currentTask)->id());
                    Serial.print(F(" took "));
                    Serial.println(lastRunTime);
#endif
                    return;
                }
            }

#if defined(DEBUG_SCHEDULER) || defined(DEBUG_SCHEDULER_RUN)
            unsigned long start = micros();
#endif
            currentTask = id;
            Task *pTask = task(id);
            pTask->loop();

#if defined(DEBUG_SCHEDULER) || defined(DEBUG_SCHEDULER_RUN)
            lastRunTime = micros() - start;
#endif

#ifdef DEBUG_SCHEDULER_RUN
            Serial.print("Scheduler task ");
            Serial.print(pTask->id());
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

uint8_t Scheduler::taskIndex(Task *pTask) {
    for (uint8_t i = 0; i < taskCount; i++) {
        if (task(i) == pTask) {
            return i;
        }
    }
    return NULL_TASK;
}

void Scheduler::setDelay(uint8_t index, uint16_t milliseconds) {
    if (index != NULL_TASK) {
        delays[index] = milliseconds == INFINITE_DELAY ? milliseconds - 1 : milliseconds;
    }
}

void Scheduler::resume(Task *task, uint16_t milliseconds) {
    setDelay(taskIndex(task), milliseconds);
}

bool Scheduler::isSuspended(Task *task) {
    uint8_t index = taskIndex(task);
    return index != NULL_TASK && delays[index] == INFINITE_DELAY;
}

void Scheduler::suspend() {
    // current task to delay by milliseconds
    if (currentTask == NULL_TASK) {
#ifdef DEBUG_SCHEDULER_ERRORS
        Serial.println(F("Error: suspend() called with NULL_TASK"));
#endif
    } else {
        delays[currentTask] = INFINITE_DELAY;
    }
}

void Scheduler::delay(uint16_t milliseconds) {
    // current task to delay by milliseconds
#ifdef DEBUG_SCHEDULER_ERRORS
    if (currentTask == NULL_TASK) {
        Serial.println(F("Error: suspend() called with NULL_TASK"));
    }
#endif
    setDelay(currentTask, milliseconds);
}

void Task::delay(uint16_t milliseconds) {
    scheduler.delay(milliseconds);
}

void Task::resume(uint16_t milliseconds) {
    scheduler.resume(this, milliseconds);
}

void Task::suspend() {
    scheduler.suspend();
}

bool Task::isSuspended() {
    return scheduler.isSuspended(this);
}
