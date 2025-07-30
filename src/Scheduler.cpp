#include "Arduino.h"
#include "Scheduler.h"

Scheduler::Scheduler(uint8_t count, const char *taskTable, time_t *delayTable) {
    taskCount = count;
    tasks = taskTable;
    taskTimes = delayTable;
    flags = 0;
    startLoopMicros = 0;
    nextTask = 0;
    pTask = NULL;
#ifdef SERIAL_DEBUG_SCHEDULER
    iteration = 0;
#endif
}

Task *Scheduler::getTask(uint8_t taskId) {
#ifdef SERIAL_DEBUG_SCHEDULER_VALIDATE
    if (taskId >= taskCount) return NULL;
#endif
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

    startLoopMicros = micros();
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
    uint32_t now = micros();
    serialDebugPrintf_P(PSTR("%S %ld\n"), msg, now);

    for (uint8_t i = 0; i < taskCount; i++) {
        Task *pTask = getTask(i);
        serialDebugPrintf_P(PSTR("%S [%d] @ %ld in %ld\n"), pTask->id(), pTask->taskId, taskTimes[i], elapsed_micros(now, taskTimes[i]));
    }
}

#endif

void Scheduler::loopMicros(time_t timeSlice) {
    const time_t tick = micros();

#if defined(SCHED_MIN_LOOP_TIMESLICE_MICROS) && SCHED_MIN_LOOP_TIMESLICE_MICROS
    if (!isElapsed(tick, startLoopMicros + SCHED_MIN_LOOP_TIMESLICE_MICROS)) {
        // this is to avoid needlessly scanning the delay table too frequently
        return;
    }
#endif

    flags |= SCHED_FLAGS_IN_LOOP;
    startLoopMicros = tick;

    serialDebugSchedulerDumpDelays("Scheduler loop ");

#ifdef SERIAL_DEBUG_SCHEDULER
    iteration++;
#endif

    // offset task index by nextTask so we can interrupt at a getTask
    // and continue with the same getTask next time slice
    uint8_t lastId = -1;
    time_t timeSliceLimit = timeSlice + tick;
    uint8_t hadTask = 0;

    for (uint8_t i = 0; i < taskCount; i++) {
        uint8_t id = i + nextTask;
        if (id >= taskCount) id -= taskCount;
        lastId = id;

        startTaskMicros = micros();
        time_t start = startTaskMicros;

        if (taskTimes[id] == TASK_DELAY_SUSPENDED || !isElapsed(startTaskMicros, taskTimes[id])) continue;

        // the task is ready
        pTask = getTask(id);

        if (!(pTask->getFlags() & TASK_DBG_FLAGS_NO_SCHED)) {
            hadTask |= 1;
        }

#ifdef SERIAL_DEBUG_SCHEDULER_CLI
        uint8_t oldSREG = SREG;
#endif
        executeTask();
#ifdef SERIAL_DEBUG_SCHEDULER_CLI
        uint8_t newSREG = SREG;
#endif

        Task *pLastTask = pTask;
        pTask = NULL;

        time_t end = micros();

#ifdef SCHED_TASK_ACTIVE
        pLastTask->activeTaskMicros += end - startTaskMicros;
        startTaskMicros = 0;
#endif

        if (timeSlice && isElapsed(tick, timeSliceLimit)) {
#ifdef SERIAL_DEBUG_SCHEDULER
            if (!(pTask->getFlags() & TASK_DBG_FLAGS_NO_SCHED)) {
                debugSchedulerPrintf_P(PSTR("Scheduler[%u] time slice ended %lu limit %u last getTask %S[%d] took %lu\n"), iteration, (uint32_t) (end - tick), timeSlice, pLastTask->id(), pLastTask->taskId, (uint32_t) (end - start));
            }
#endif

            // next time continue checking after the current getTask
            nextTask = id + 1;
            if (nextTask >= taskCount) nextTask = 0;
            lastId = -1;
            break;
        }

        if (hadTask) {
            debugSchedulerPrintf_P(PSTR("Scheduler[%d] %S[%d] done in %lu\n"), iteration, pLastTask->id(), pLastTask->taskId, elapsed_micros(start, end));

#ifdef SERIAL_DEBUG_SCHEDULER_CLI
            if ((oldSREG & 0x80) && !(newSREG & 0x80)) {
                serialDebugSchedulerCliPrintf_P(PSTR("Sched: task %S, interrupts disabled, last %S:%d:%d\n"), pLastTask->id(), pCliFile, nCliLine, nSeiLine);
            }
#endif
        }

#ifdef SERIAL_DEBUG_SCHEDULER_CLI
        // KLUDGE: if interrupts are disabled in a task, enable them here
        sei();
#endif
    }

    if (lastId != (uint8_t) -1) {
        // all ran, next time start with the first
        nextTask = 0;
    }

#ifdef SERIAL_DEBUG_SCHEDULER
    if (hadTask) {
        uint32_t time = micros();
        debugSchedulerPrintf_P(PSTR("Scheduler end run %ld\n"), elapsed_micros(tick, time));
    }
#endif

    flags &= SCHED_FLAGS_IN_LOOP;
}

void Scheduler::executeTask() {
    if (pTask->isAsync()) {
        AsyncTask *pAsyncTask = reinterpret_cast<AsyncTask *>(pTask);
        pAsyncTask->clearFlags(TASK_DBG_FLAGS_FAKE_YIELD);
        resumeContext(pAsyncTask->pContext);
#ifdef SERIAL_DEBUG_SCHEDULER_CLI
        if (pAsyncTask->getFlags() & TASK_DBG_FLAGS_FAKE_YIELD) {
            resumeContext(pAsyncTask->pContext);
        }
#endif
#ifdef SERIAL_DEBUG_SCHEDULER_MAX_STACKS
        if (pAsyncTask->maxStackUsed() > pAsyncTask->pContext->stackMax) {
            serialDebugPrintf_P(PSTR("Sched: task %S stack %d > max %d\n"), pAsyncTask->id(), pAsyncTask->maxStackUsed(), pAsyncTask->pContext->stackMax);
            for (;;);
        }
#endif
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
#ifdef SERIAL_DEBUG_SCHEDULER_VALIDATE
    if (taskId < taskCount) {
#endif
    if (microseconds >= TASK_DELAY_MAX) {
        microseconds = TASK_DELAY_MAX - 1;
    }

    time_t endTime = (time_t) (microseconds + micros());
    if (endTime == TASK_DELAY_SUSPENDED) endTime++;
    taskTimes[taskId] = endTime;
#ifdef SERIAL_DEBUG_SCHEDULER_VALIDATE
    }
#endif
}

time_t Scheduler::getResumeMicros(uint8_t taskId) {
#ifdef SERIAL_DEBUG_SCHEDULER_VALIDATE
    if (taskId < taskCount) {
        return taskTimes[taskId];
    }
    return micros();
#else
    return taskTimes[taskId];
#endif
}

uint8_t Scheduler::getCurrentTaskId() {
    return pTask ? pTask->taskId : NULL_TASK;
}

/**
 * Get elapsed micro seconds between start and end micros(). If |diff| > 0x7fffffff, then micros()
 * rolled over and difference will be start - end. Only valid if |actual difference| <= 0x7fffffff.
 *
 * @param startTime     start time micros()
 * @param endTime       end time micros()
 * @return              difference between end and start time, taking possible roll over into account
 *                      result will be in range [-0x80000000, 0x7ffffff]
 */
int32_t elapsed_micros(time_t startTime, time_t endTime) {
    uint32_t diff = endTime - startTime;

    if (diff & 0x80000000) {
        // assume rollover occurred
        diff = -diff;
    }

    return (int32_t) diff;
}

/**
 * Test if endTime has elapsed past now. ie. difference is -ve
 *
 * @param now           now micros()
 * @param endTime       end time micros()
 * @return micros between start and end, taking possibility of micros() rolling over to 0.
 */
uint8_t is_elapsed(time_t now, time_t endTime) {
    uint32_t diff = now - endTime;
    uint8_t nowGtEndTime = 1;

    if (now < endTime) {
        diff = -diff;
        nowGtEndTime = 0;
    }

    if (diff & 0x80000000) {
        // rolled over
        return !nowGtEndTime;
    } else {
        return nowGtEndTime;
    }
}

time_t Task::getCurrentActiveMicros() const {
#ifdef SCHED_TASK_ACTIVE
    return activeTaskMicros + (micros() - scheduler.startTaskMicros);
#else
    return micros();
#endif
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

void AsyncTask::fakeYield() {
    setFlags(TASK_DBG_FLAGS_FAKE_YIELD);
    yieldContext();
    clearFlags(TASK_DBG_FLAGS_FAKE_YIELD);
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
