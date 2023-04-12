#include <Arduino.h>
#include "Arduino.h"
#include "Scheduler.h"

#ifdef SERIAL_DEBUG_SCHEDULER_DELAYS
#define serialDebugSchedulerDumpDelays(msg) dumpDelays(PSTR(msg))
#else
#define serialDebugSchedulerDumpDelays(msg) ((void)0)
#endif

Scheduler::Scheduler(uint8_t count, PGM_P taskTable, uint16_t *delayTable) {
    taskCount = count;
    tasks = taskTable;
    delays = delayTable;
    clockTick = 0;
    nextTask = 0;
#ifdef SERIAL_DEBUG_SCHEDULER
    iteration = 0;
#endif
}

Task *Scheduler::getTask(uint8_t index) {
    return reinterpret_cast<Task *>(pgm_read_ptr(tasks + sizeof(Task *) * index));
}

void Scheduler::begin() {
    memset(delays, 0, sizeof(*delays) * taskCount);

    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = getTask(i);
        if (pTask->index != NULL_TASK) {
            debugSchedulerErrorsPrintf_P(PSTR("Task %S, is duplicated in scheduler at %d and %d")
                                         , pTask->id()
                                         , pTask->index
                                         , i
            );
        } else {
            pTask->index = i;
        }
    }

    clockTick = micros();
    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = getTask(i);
        pTask->begin();
    }

    serialDebugSchedulerDumpDelays("Scheduler after start ");
}

#ifdef SERIAL_DEBUG_SCHEDULER_DELAYS

void Scheduler::dumpDelays(PGM_P msg) {
    serialDebugPuts_P(msg);

    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = getTask(i);
        serialDebugPrintf_P(PSTR("%S [%d] { %d }\n")
                            , pTask->id()
                            , pTask->index
                            , delays[i]
        );
    }
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
    uint32_t tick = micros();
    uint32_t diff = tick - clockTick;

    uint16_t diffMs = (uint16_t) (diff / 1000);
    serialDebugSchedulerDumpDelays("Scheduler loop before ");
    debugSchedulerDelaysPrintf_P(PSTR("Tick %ld clockTick %ld diff %ld diffMs %d\n")
                                 , tick
                                 , clockTick
                                 , tick - clockTick
                                 , diffMs
    );

    bool haveTasks = reduceDelays(diffMs);

    serialDebugSchedulerDumpDelays("Scheduler loop after ");

    // adjust by ms used to reduce delay and to eliminate error accumulation
    clockTick += (uint32_t) diffMs * 1000;

    if (!haveTasks) return;

#ifdef SERIAL_DEBUG_SCHEDULER
    iteration++;
#endif

    // offset task index by nextTask so we can interrupt at a getTask
    // and continue with the same getTask next time slice
    uint8_t lastId;

    for (uint8_t i = 0; i < taskCount; i++) {
        uint8_t id = i + nextTask;
        if (id >= taskCount) id -= taskCount;
        lastId = id;

        if (delays[id] == 0) {
#ifdef SERIAL_DEBUG_SCHEDULER
            uint32_t start = micros();
#endif
            Task *pTask = getTask(id);
            pTask->loop();
            unsigned long end = micros();

            if (timeSlice) {
                uint32_t timeSliceLimit = timeSlice * 1000L;

                if ((uint32_t) (end - tick) > timeSliceLimit) {
#ifdef SERIAL_DEBUG_SCHEDULER
                    debugSchedulerPrintf_P(PSTR("Scheduler[%u] time slice ended %lu limit %u last getTask %S[%d] took %lu\n")
                                           , iteration
                                           , (uint32_t)(end - tick)
                                           , timeSlice
                                           , pTask->id()
                                           , pTask->index
                                           , (uint32_t) (end - start)
                    );
#endif
                    // next time continue checking after the current getTask
                    nextTask = id + 1;
                    if (nextTask >= taskCount) nextTask = 0;
                    lastId = -1;
                    break;
                }
            }

            debugSchedulerPrintf_P(PSTR("Scheduler[%d] getTask %S[%d] done in %lu\n")
                                   , iteration
                                   , pTask->id()
                                   , pTask->index
                                   , (uint32_t) (end - start)
            );
        }
    }

    if (lastId != -1) {
        // all ran, next time start with the first
        nextTask = 0;
    }

#ifdef SERIAL_DEBUG_SCHEDULER
    uint32_t time = micros();
    debugSchedulerPrintf_P(PSTR("Scheduler end run %ld\n")
                           , time - tick
    );
#endif
}

void Scheduler::resume(Task *task, uint16_t milliseconds) {
    delays[task->index] = milliseconds == INFINITE_DELAY ? INFINITE_DELAY - 1 : milliseconds;
}

bool Scheduler::isSuspended(Task *task) {
    return delays[task->index] == INFINITE_DELAY;
}

/**
 * Set current getTask's delay to infinite
 */
void Scheduler::suspend(Task *task) {
    delays[task->index] = INFINITE_DELAY;
}

