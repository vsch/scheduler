#include "Arduino.h"
#include "Scheduler.h"
#include "ResourceLock.h"
#include "Signal.h"

Scheduler::Scheduler(uint8_t count, PGM_P taskTable, uint16_t *delayTable) {
    taskCount = count;
    tasks = taskTable;
    delays = delayTable;
    clockTick = 0;
    nextTask = 0;
    inLoop = 0;
#ifdef SERIAL_DEBUG_SCHEDULER
    iteration = 0;
#endif
}

Task *Scheduler::getTask(uint8_t index) {
    (void) tasks;
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

    inLoop = true;

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

    if (!haveTasks) {
        inLoop = 0;
        return;
    }

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
            Task *pCurrentTask = getTask(id);

            if (pCurrentTask->isAsync()) {
                AsyncTask *pBlockingTask = reinterpret_cast<AsyncTask *>(pCurrentTask);
                resumeContext(pBlockingTask->pContext);
            } else {
                // just a Task
                pCurrentTask->loop();
            }

            unsigned long end = micros();

            if (timeSlice) {
                uint32_t timeSliceLimit = timeSlice * 1000L;

                if ((uint32_t) (end - tick) > timeSliceLimit) {
#ifdef SERIAL_DEBUG_SCHEDULER
                    debugSchedulerPrintf_P(PSTR("Scheduler[%u] time slice ended %lu limit %u last getTask %S[%d] took %lu\n")
                                           , iteration
                                           , (uint32_t)(end - tick)
                                           , timeSlice
                                           , pCurrentTask->id()
                                           , pCurrentTask->index
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
                                   , pCurrentTask->id()
                                   , pCurrentTask->index
                                   , (uint32_t) (end - start)
            );

            pCurrentTask = NULL;
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

    inLoop = 0;
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

uint8_t Scheduler::canLoop() const {
    return !inLoop;
}

uint8_t Task::reserveResource(ResourceLock *pResource) {
    return pResource->reserveResource(this);
}

void Task::releaseResource(ResourceLock *pResource) {
    pResource->releaseResource();
}

uint8_t Task::waitOnSignal(Signal *pSignal) {
    return pSignal->wait(this);
}

void Task::resume(uint16_t milliseconds) {
    scheduler.resume(this, milliseconds);
}

void Task::suspend() {
    scheduler.suspend(this);
}

bool Task::isSuspended() {
    return scheduler.isSuspended(this);
}

uint8_t Task::isAsync() {
    return false;
}

uint8_t AsyncTask::isAsync() {
    return true;
}

AsyncTask::AsyncTask(uint8_t *pStack, uint8_t stackMax) : Task() { // NOLINT(cppcoreguidelines-pro-type-member-init)
    pContext = (AsyncContext *)pStack;
    initContext(pStack, AsyncTask::yieldingLoop, this, stackMax);
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
uint8_t AsyncTask::yieldSuspend() {
    suspend();
    if (isInAsyncContext()) {
        yieldContext();
        return false;
    }
    return true;
}

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
uint8_t AsyncTask::yieldResume(uint16_t milliseconds) {
    resume(milliseconds);
    if (isInAsyncContext()) {
        yieldContext();
        return false;
    }
    return true;
}

void AsyncTask::yield() {
    resume(0);
    if (isInAsyncContext()) {
        yieldContext();
    }
}

uint8_t AsyncTask::hasYielded() const {
    return pContext->stackUsed;
}

uint8_t AsyncTask::maxStackUsed() const {
    pContext->stackMaxUsed;
}

void AsyncTask::yieldingLoop(void *arg) {
    ((AsyncTask *)arg)->loop();
}

#ifdef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
void Scheduler::dumpMaxStackInfo() {
    // TODO: implement
}

#endif

