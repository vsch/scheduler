#include "Arduino.h"
#include "Scheduler.h"

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
    if (index >= taskCount) return NULL;

    (void) tasks;
    return (Task *) (pgm_read_ptr(tasks + sizeof(Task *) * index));
}

void Scheduler::begin() {
    memset(delays, 0, sizeof(*delays) * taskCount);

    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = getTask(i);
        if (pTask->index != NULL_TASK) {
            debugSchedulerErrorsPrintf_P(PSTR("Task %S, is duplicated in scheduler at %d and %d"), pTask->id(),
                                         pTask->index, i);
        } else {
            pTask->index = i;
        }
    }

    clockTick = micros();
    for (uint8_t i = 0; i < taskCount; i++) {
        pTask = getTask(i);
        pTask->begin();
        pTask = NULL;
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
    debugSchedulerDelaysPrintf_P(PSTR("Tick %ld clockTick %ld diff %ld diffMs %d\n"), tick, clockTick, tick - clockTick,
                                 diffMs);

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
            pTask = getTask(id);

#ifdef SERIAL_DEBUG_SCHEDULER
            uint32_t start = micros();
            Task *pLastTask = pTask;
#endif

            if (pTask->isAsync()) {
                AsyncTask *pAsyncTask = reinterpret_cast<AsyncTask *>(pTask);
                pAsyncTask->activating();
                resumeContext(pAsyncTask->pContext);
                pAsyncTask->deactivated();
            } else {
                // just a Task
                pTask->loop();
            }
            pTask = NULL;

            unsigned long end = micros();

            if (timeSlice) {
                uint32_t timeSliceLimit = timeSlice * 1000L;

                if ((uint32_t) (end - tick) > timeSliceLimit) {
#ifdef SERIAL_DEBUG_SCHEDULER
                    debugSchedulerPrintf_P(PSTR("Scheduler[%u] time slice ended %lu limit %u last getTask %S[%d] took %lu\n")
                                           , iteration
                                           , (uint32_t)(end - tick)
                                           , timeSlice
                                           , pLastTask->id()
                                           , pLastTask->index
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

            debugSchedulerPrintf_P(PSTR("Scheduler[%d] getTask %S[%d] done in %lu\n"), iteration, pLastTask->id(),
                                   pLastTask->index, (uint32_t) (end - start));
        }
    }

    if (lastId != (uint8_t) -1) {
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

void Scheduler::resume(uint8_t taskId, uint16_t milliseconds) {
    if (taskId < taskCount) {
        delays[taskId] = milliseconds == INFINITE_DELAY ? INFINITE_DELAY - 1 : milliseconds;
    }
}

uint8_t Scheduler::isAsyncTask(uint8_t taskId) {
    Task *pTask = getTask(taskId);
    return pTask && pTask->isAsync();
}

bool Scheduler::isSuspended(Task *task) {
    return delays[task->index] == INFINITE_DELAY;
}

/**
 * Set current getTask's delay to infinite
 */
void Scheduler::suspend(uint8_t taskId) {
    delays[taskId] = INFINITE_DELAY;
}

uint8_t Scheduler::canLoop() const {
    return !inLoop;
}

Task *Scheduler::getTask() {
    return pTask;
}

uint8_t Scheduler::getTaskId() {
    return pTask ? pTask->index : NULL_TASK;
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

AsyncTask::AsyncTask(uint8_t *pStack, uint8_t stackMax) : Task() {
    // NOLINT(cppcoreguidelines-pro-type-member-init)
    pContext = (AsyncContext *) pStack;
    initContext(pStack, AsyncTask::yieldingLoop, this, stackMax);
}

/**
 * Suspend the task's execution and yield context
 * If successfully yielded, this function will return after the task is resumed.
 *
 * If the task is not the current context, it will not yield and return immediately.
 * Check return value for true if it did not yield and handle it, if needed.
 *
 * @return 0 if successfully yielded. 1 if no yield because was not the active task
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
 * @return 0 if successfully yielded. 1 if no yield because was not the active task
 */
uint8_t AsyncTask::yieldResume(uint16_t milliseconds) {
    resume(milliseconds);
    if (isInAsyncContext()) {
        yieldContext();
        return 0;
    }
    return 1;
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
    return pContext->stackMaxUsed;
}

void AsyncTask::yieldingLoop(void *arg) {
    ((AsyncTask *) arg)->loop();
}

#ifdef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
void Scheduler::dumpMaxStackInfo() {
    // TODO: implement
}

#endif

#ifdef CONSOLE_DEBUG

// print out queue for testing
void Scheduler::dump(char *buffer, uint32_t sizeofBuffer, uint8_t indent) {
    uint32_t len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    char indentStr[32];
    memset(indentStr, ' ', sizeof indentStr);
    indentStr[indent] = '\0';

    snprintf(buffer, sizeofBuffer, "%sScheduler { nTasks:%d\n", indentStr, taskCount);
    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
    snprintf(buffer, sizeofBuffer, "%s  canLoop() = %d\n", indentStr, canLoop());

    for (uint8_t i = 0; i < taskCount; i++) {
        len = strlen(buffer);
        buffer += len;
        sizeofBuffer -= len;
        Task *pTask = getTask(i);

        if (!(i % 16)) {
            if (i) {
                *buffer++ = '\n';
                sizeofBuffer -= 1;
            }

            *buffer++ = ' ';
            sizeofBuffer -= 1;
        }

        *buffer++ = ' ';
        sizeofBuffer--;

        snprintf(buffer, sizeofBuffer, "0x%2.2x", pTask->getIndex());

        len = strlen(buffer);
        buffer += len;
        sizeofBuffer -= len;
    }

    snprintf(buffer, sizeofBuffer, "\n%s}\n", indentStr);
    len = strlen(buffer);
    buffer += len;
    sizeofBuffer -= len;
}

#endif // CONSOLE_DEBUG
