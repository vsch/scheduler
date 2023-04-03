#include "Arduino.h"
#include "Scheduler.h"

Scheduler::Scheduler(uint8_t count, PGM_P taskTable, uint16_t *delayTable) {
    taskCount = count;
    tasks = taskTable;
    delays = delayTable;
    clockTick = 0;
    nextTask = 0;
#ifdef DEBUG_SCHEDULER
    iteration = 0;
#endif
}

Task *Scheduler::task(uint8_t id) {
    return reinterpret_cast<Task *>(pgm_read_ptr(tasks + sizeof(Task *) * id));
}

void Scheduler::begin() {
    memset(delays, 0, sizeof(*delays) * taskCount);

    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = task(i);
        if (pTask->index != NULL_TASK) {
#ifdef DEBUG_SCHEDULER_ERRORS
            Serial.print(F("Task "));
            Serial.print(pTask->id());
            Serial.print(F(" is duplicated in scheduler at "));
            Serial.print(pTask->taskId);
            Serial.print(F(" and "));
            Serial.print(i);
#endif
        } else {
            pTask->index = i;
        }
    }

    clockTick = micros();
    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = task(i);
        pTask->begin();
    }

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays(F("Scheduler after start "));
#endif
}

#ifdef DEBUG_SCHEDULER_RUN
void Scheduler::dumpDelays(PGM_STR msg) {
    Serial.print(msg);

    for (uint8_t i = 0; i < taskCount; i++) {
        Serial.print(' ');
        Task *pTask = task(i);
        Serial.print(pTask->id());
        Serial.print('[');
        Serial.print(pTask->taskId);
        Serial.print(']');
        Serial.print('{');
        Serial.print(delays[i]);
        Serial.print('}');
        Serial.print(' ');
    }

    Serial.println();
}
#endif

bool Scheduler::reduceDelays(uint16_t milliseconds) {
    bool haveTasks = false;
    uint8_t i = taskCount;

    while (i-- > 0) {
        uint16_t delay = delays[i];
        if (delay) {
            if (milliseconds && delay != INFINITE_DELAY) {
                if (delay <= milliseconds) {
                    delay = 0;
                    haveTasks = true;
                } else {
                    delay -= milliseconds;
                }

                delays[i] = delay;
            }
        } else {
            haveTasks = true;
        }
    }
    return haveTasks;
}

void Scheduler::loop(uint16_t timeSlice) {
    unsigned long tick = micros();
    unsigned long diff = tick - clockTick;

    uint16_t diffMs = static_cast<uint16_t>(diff / 1000);
#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays(F("Scheduler loop before "));
    Serial.print(F("Tick "));
    Serial.print(tick);
    Serial.print(F(" clockTick "));
    Serial.print(clockTick);
    Serial.print(F(" diff "));
    Serial.print(tick - clockTick);
    Serial.print(F(" diffMs "));
    Serial.println(diffMs);
#endif

    bool haveTasks = reduceDelays(diffMs);

#ifdef DEBUG_SCHEDULER_DELAYS
    dumpDelays(F("Scheduler run after "));
#endif

    // adjust by ms used to reduce delay to eliminate error accumulation
    clockTick += (uint32_t) diffMs * 1000;

    if (!haveTasks) return;

#ifdef DEBUG_SCHEDULER
    iteration++;
#endif

#ifdef DEBUG_SCHEDULER_RUN
    Serial.print(F("Scheduler run "));
    Serial.print(tick);
    Serial.print(F(" slice "));
    Serial.println(timeSlice);
#endif

    uint32_t timeSliceLimit = timeSlice * 1000;

    // offset task index by nextTask so we can interrupt at a task
    // and continue with the same task next time slice
    for (uint8_t i = 0; i < taskCount; i++) {
        uint8_t id = i + nextTask;
        if (id >= taskCount) id -= taskCount;

        if (delays[id] == 0) {
#if defined(DEBUG_SCHEDULER) || defined(DEBUG_SCHEDULER_RUN)
            unsigned long start = micros();
#endif
            Task *pTask = task(id);
            pTask->loop();

#ifdef DEBUG_SCHEDULER_RUN
            unsigned long end = micros();
#endif

            if (timeSlice) {
#ifndef DEBUG_SCHEDULER_RUN
                unsigned long end = micros();
#endif
                if ((uint32_t) (end - tick) > timeSliceLimit) {
                    // ran out of time slice
#ifdef DEBUG_SCHEDULER
                    Serial.print(F("Scheduler["));
                    Serial.print(iteration);
                    Serial.print(F("] time slice ended "));
                    Serial.print(end - tick);
                    Serial.print(F(" "));
                    Serial.print(timeSliceLimit);
                    Serial.print(F(" last task "));
                    Serial.print(pTask->id());
                    Serial.print('[');
                    Serial.print(pTask->taskId);
                    Serial.print(']');
                    Serial.print(F(" took "));
                    Serial.println(end - start);
#endif
                    nextTask = id;
                    return;
                }
            }

#ifdef DEBUG_SCHEDULER_RUN
            Serial.print(F("Scheduler task "));
            Serial.print(pTask->id());
            Serial.print('[');
            Serial.print(pTask->taskId);
            Serial.print(']');
            Serial.print(F(" done in "));
            Serial.println(end - start);
#endif
        }
    }

    nextTask = 0;

#ifdef DEBUG_SCHEDULER_RUN
    unsigned long time = micros();
    Serial.print(F("Scheduler end run "));
    Serial.println(time - tick);
#endif
}

void Scheduler::resume(Task *task, uint16_t milliseconds) {
    delays[task->index] = milliseconds == INFINITE_DELAY ? INFINITE_DELAY - 1 : milliseconds;
}

bool Scheduler::isSuspended(Task *task) {
    return delays[task->index] == INFINITE_DELAY;
}

/**
 * Set current task's delay to infinite
 */
void Scheduler::suspend(Task *task) {
    delays[task->index] = INFINITE_DELAY;
}

