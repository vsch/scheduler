#include "Arduino.h"
#include "Scheduler.h"

Scheduler::Scheduler(uint8_t count, const char *taskTable, time_t *delayTable) {
    taskCount = count;
    tasks = taskTable;
    taskTimes = delayTable;
    flags = 0;
    clockTick = 0;
    nextTask = 0;
    pTask = NULL;
#ifdef SERIAL_DEBUG_SCHEDULER
    iteration = 0;
#endif
}

Task *Scheduler::getTask(uint8_t taskId) {
    if (taskId >= taskCount) return NULL;

    (void) tasks;
    return (Task *) (pgm_read_ptr(tasks + sizeof(Task *) * taskId));
}

void Scheduler::begin() {
    // start off with all suspended
    memset(taskTimes, 0, sizeof(*taskTimes) * taskCount);

#if defined(DEBUG_MODE_SCHEDULER_VALIDATE) && defined(SERIAL_DEBUG_SCHEDULER_ERRORS)
    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = getTask(i);
        if (pTask->taskId != NULL_TASK) {
            debugSchedulerErrorsPrintf_P(PSTR("Task %S, is duplicated in scheduler at %d and %d"), pTask->id(), pTask->taskId, i);
        } else {
            pTask->taskId = i;
        }
    }
#endif

    clockTick = micros();
    for (uint8_t i = 0; i < taskCount; i++) {
        pTask = getTask(i);
        pTask->taskId = i;
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
        serialDebugPrintf_P(PSTR("%S [%d] { %d }\n"), pTask->id(), pTask->taskId, taskTimes[i]);
    }
}

#endif

void Scheduler::loop(uint16_t timeSlice) {
    time_t tick = micros();

    if (!isElapsed(tick, clockTick + SCHED_MIN_LOOP_DELAY_MICROS)) {
        // this is to avoid needlessly scanning the delay table too frequently
        return;
    }

    flags |= SCHED_FLAGS_IN_LOOP;
    clockTick = tick;

    serialDebugSchedulerDumpDelays("Scheduler loop ");

#ifdef SERIAL_DEBUG_SCHEDULER
    iteration++;
#endif

    // offset task index by nextTask so we can interrupt at a getTask
    // and continue with the same getTask next time slice
    uint8_t lastId;
    time_t timeSliceLimit = timeSlice * 1000UL + tick;

    for (uint8_t i = 0; i < taskCount; i++) {
        uint8_t id = i + nextTask;
        if (id >= taskCount) id -= taskCount;
        lastId = id;

        time_t start = micros();

        if (!isElapsed(start, taskTimes[id])) continue;

        // the task is ready
        executeTask(id);

#ifdef SERIAL_DEBUG_SCHEDULER
        Task *pLastTask = pTask;
#endif
        pTask = NULL;

        time_t end = micros();
        
        if (timeSlice) {
            if (isElapsed(tick, timeSliceLimit)) {
#ifdef SERIAL_DEBUG_SCHEDULER
                debugSchedulerPrintf_P(PSTR("Scheduler[%u] time slice ended %lu limit %u last getTask %S[%d] took %lu\n"),
                                       iteration, (uint32_t) (end - tick), timeSlice, pLastTask->id(), pLastTask->taskId, (uint32_t) (end - start)
                );
#endif
                // next time continue checking after the current getTask
                nextTask = id + 1;
                if (nextTask >= taskCount) nextTask = 0;
                lastId = -1;
                break;
            }
        }

        debugSchedulerPrintf_P(PSTR("Scheduler[%d] getCurrentTask %S[%d] done in %lu\n"),
                               iteration, pLastTask->id(), pLastTask->taskId, (uint32_t) (end - start));

    }

    if (lastId != (uint8_t) -1) {
        // all ran, next time start with the first
        nextTask = 0;
    }

#ifdef SERIAL_DEBUG_SCHEDULER
    uint32_t time = micros();
    debugSchedulerPrintf_P(PSTR("Scheduler end run %ld\n"), time - tick
    );
#endif

    flags &= SCHED_FLAGS_IN_LOOP;
}

void Scheduler::executeTask(uint8_t taskId) {
    pTask = getTask(taskId);

    if (pTask->isAsync()) {
        AsyncTask *pAsyncTask = reinterpret_cast<AsyncTask *>(pTask);
        resumeContext(pAsyncTask->pContext);
    } else {
        // just a Task
        pTask->loop();
    }
}

uint8_t Scheduler::isAsyncTask(uint8_t taskId) {
    Task *pTask = getTask(taskId);
    return pTask && pTask->isAsync();
}

bool Scheduler::isSuspended(Task *task) {
    return taskTimes[task->taskId] == TASK_DELAY_SUSPENDED;
}

/**
 * Set current getCurrentTask's delay to infinite
 */
void Scheduler::suspend(uint8_t taskId) {
    taskTimes[taskId] = TASK_DELAY_SUSPENDED;
}

void Scheduler::resumeMicros(uint8_t taskId, time_t microseconds) {
    if (taskId < taskCount) {
        if (microseconds >= TASK_DELAY_MAX) {
            microseconds = TASK_DELAY_MAX - 1;
        }

        time_t endTime = (time_t) (microseconds + micros());
        if (endTime == TASK_DELAY_SUSPENDED) endTime++;
        taskTimes[taskId] = endTime;
    }
}

uint8_t Scheduler::getCurrentTaskId() {
    return pTask ? pTask->taskId : NULL_TASK;
}

uint8_t Scheduler::isElapsed(time_t now, time_t endTime) {
    if (endTime != TASK_DELAY_SUSPENDED) {
        if (endTime <= now) {
            time_t diff = now - endTime;
            if (diff < TASK_DELAY_MAX) {
                // timed out
                return 1;
            }
            // not timed out, or wait for wrap around
        }
    }
    return 0;
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
void AsyncTask::yieldSuspend() {
    suspend();
    yieldContext();
}

/**
 * Set the resume milliseconds and yield the task's execution context. If successfully yielded, this
 * function will return after at least the given delay has passed.
 *
 * If there is no async context, it will not yield and return immediately.
 *
 * @param microseconds delay in microseconds before resuming task
 */
void AsyncTask::yieldResumeMicros(time_t microseconds) {
    resumeMicros(microseconds);
    yieldContext();
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
void AsyncTask::yieldResume(uint16_t milliseconds) {
    resume(milliseconds);
    yieldContext();
}

void AsyncTask::yield() {
    resume(0);
    yieldContext();
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

        snprintf(buffer, sizeofBuffer, "0x%2.2x", pTask->getTaskId());

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
