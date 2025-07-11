# Arduino Scheduler library

Cooperative round-robin scheduler using a simple task model with
`begin()` and `loop()` methods.

Version 2 includes `AsyncTask` type which can use yielding methods
from within its loop to release the CPU to other tasks or the main loop.
Execution of the code will continue after the call to the yielding
method, when the scheduler resumes the task.

This eliminates the need to convert the code to a state machine with one
entry and one exit, by using yield functions to relinquish the CPU, the
state is preserved on the stack and variables of the running code.

[TOC]: #

- [Overview](#overview)
- [Implementation Details for `AsyncTask`](#implementation-details-for-asynctask)
- [Example](#example)
  - [PWM Motor Controller](#pwm-motor-controller)

## Overview

Tasks can use scheduler to `suspend()`, `resume(uin16_t milliseconds)`
after suspension or to schedule next execution after delay. If
`milliseconds` is given as `0` then the current task will be rescheduled
to run in the next scheduler loop invocation and after all ready tasks
in the current invocation have been run.

In version 2 of the library a `AsyncTask` type was added which can
use yielding methods from within its `loop()` to release the CPU to
other tasks or the main loop. When the scheduler resumes the task,
execution will continue after the call to the yielding method.

The resume/suspend will return to the caller immediately, with the delay
taking effect for the next call of the task `loop()` function. Multiple
calls in the same `loop()` invocation overwrite previous call values. To
implement `delay()` functionality which suspends the `loop()` function
requires converting the function to a state machine which sets the
delay, next state and return immediately then on the next invocation
resumes at the new state.

Each task can set a delay time of up to 65534 ms (0xfffe), with 65535
(0xffff) reserved to mark a task as suspended, trying to use 65535 with
the `resume()` function will result in 65534 ms delay time. Use
`suspend()` explicitly to suspend a task.

A task can initialize its state in the `begin()` method and use
`resume()` method of its super class `Task` to schedule the start of its
`loop()` or it can use `suspend()` if its scheduling will be triggered
by another task.

The scheduler uses `micros()` internally to eliminate accumulated
microseconds truncation error when using `millis()` to reduce task
delays.

The scheduler has two public methods `begin()` for initialization and
`loop(uint16 timeSliceMilliseconds = 0)` for executing the task loop.

The `begin()` method will initialize the scheduler and invoke all tasks'
`begin()` methods.

The `loop()` method will execute all ready tasks once or until current
run time has exceeded the given `timeSliceMilliseconds`. Passing default
of `0` means no time limit, execute all ready tasks once and return.

The scheduler constructor takes a table of pointers to tasks in
`PROGMEM` and a task delay table in RAM. No dynamic memory allocation is
used by design. All tasks are defined at compile time. Since, only
active tasks are run, there is minimal overhead for inactive tasks other
than scanning the delay table looking for ready tasks.

The total RAM overhead per task is 5 bytes, with a fixed overhead of 10
bytes for the scheduler.

For `AsyncTask` the overhead adds a stack buffer, to hold the task
specific stack contents. This only includes stack data between the point
on the stack when it was resumed and when yield was called. This data is
removed from the stack into the buffer at time of yield, and restored to
the stack when the task is resumed.

`AsyncTask` methods:

* `uint8_t yieldSuspend()` - yield and suspend the task. It will not
  return until task is resumed via `resume()` call.
* `uint8_t yieldResume(uint16_t milliseconds)` - Will set the task to
  resume in `milliseconds` and yield. Function will return when the
  timeout has elapsedTime.
* `void yield()` - sets resume delay to `0` and yields. Useful for
  breaking up long tasks to allow other functions to be performed.
* `uint8_t hasYielded() const` - returns true if the task has returned
  via one of the yield methods, `0` if the task has exited its `loop()`
  function.
* `uint8_t maxStackUsed() const` - maximum bytes used of the task's
  stack buffer. Can be used to reduce the buffer size, if it isn't
  needed.

## Implementation Details for `AsyncTask`

The implementation manipulates the contents of the stack to allow
suspending and resuming tasks, in-situ. This requires assembly code
which is highly compiler and MCU dependent. The current implementation
of this code is in the `TinySwitcher.S` `avr-gcc` assembly and is only
for the avr gcc toolchain and only for the AT328p and compatible MCU's.
Although, only the 328p was actually used and tested to work.

The trick involves removing data from the stack to a buffer when the
task is yielded and restoring it to the stack when the task is resumed.
Execution of each task proceeds on the main stack and the buffer only
needs sufficient space to store stack data pushed between the call to a
task's `loop` method and when the `yieldContext` was called in the
`TinySwitcher` assembly code. How deep the call hierarchy is within the
task has no influence. It is only how deep into the hierarchy is the
call to yield context is made. If the task always yields from within
it's `loop` method, then 32 bytes for the stack buffer, plus any local
variable space definitions in the function, will be plenty.

This works because the data on the stack is not position dependent.
Except, in the case where a pointer to a local variable is also stored
on the stack. In this case the code will break if the resumed stack is
in a slightly different position, but that would be a very rare use
case.

## Example

The scheduler allows separating independent tasks into their own state
machines which can delay their own execution without hogging CPU time
and blocking other tasks.

The following will create 4 tasks:

* LedFlasher - flashes LED given number of times with 100ms on/off time
  and 250 ms at the end of a string of flashes.
* Counter1 - increments 8 bit counter with 2000 ms delay, flashes LED 1
  time when run
* Counter2 - increments 8 bit counter with 3000 ms delay, flashes LED 2
  times when run
* Updater - Prints update to Serial port with 1000ms delay.

The scheduler is invoked with no maximum time slice length, which means
it will run all tasks once before returning. If given a maximum time
slice, it will run tasks once, but will return after the a task
completion exceeds the allotted time.

```cpp
#define LED         4                   // LED on pin 4

class LedFlasher : public Task {
    uint8_t flashCount;

    void begin() {
        pinMode(LED, OUTPUT);
        suspend();
    }

    void loop() {
        if (flashCount == 0) {
            digitalWrite(LED, 0);
            suspend();
        } else {
            digitalWrite(LED, !(flashCount & 1));
            flashCount--;
            resume(flashCount ? 100 : 250);   // longer pause at end of flashes
        }
    }
    
public:
    LedFlasher() { flashCount = 0; }
    defineSchedulerTaskId("LedFlasher");

    void flash(uint8_t count) {
        if (!isSuspended()) {
            // already flashing, add to count
            flashCount += static_cast<uint8_t>((count << 1));
        } else {
            flashCount = static_cast<uint8_t>((count << 1));
            resume(0);
        }
    }
} ledFlasher = LedFlasher();

class Counter1 : public Task {
    uint8_t count;

    void begin() {
        resume(2250);  // start in 2.25 seconds 
    }

    void loop() {
        count++;
        ledFlasher.flash(1);
        resume(2000); // resume in 2 seconds
    }
    
public:
    Counter1() { count = 0; } 
    uint8_t getCount() { return count; } 
    defineSchedulerTaskId("Counter1");
} counter1 = Counter1();

class Counter2 : public Task {
    uint8_t count;

    void begin() {
        resume(2750);  // start in 2.75 seconds
    }

    void loop() {
        count++;
        ledFlasher.flash(2);
        resume(3000); // resume in 3 seconds
    }
    
public:
    Counter2() { count = 0; }
    uint8_t getCount() { return count; }
    defineSchedulerTaskId("Counter2");

} counter2 = Counter2();

class Updater : public Task {
    void begin() { 
       // nothing to initialize, run right away
    }

    void loop() {
        Serial.print(F("Update["));
        Serial.print(millis());
        Serial.print(F("] count1: "));
        Serial.print(counter1.getCount());
        Serial.print(F(" "));
        Serial.println(counter2.getCount());
        resume(1000);  // resume in a second
    }
    
public:
    Updater() = default;
    defineSchedulerTaskId("Updater");
} updater = Updater();

// Scheduler task table
Task *const tasks[] PROGMEM = {
        &updater,
        &counter1,
        &counter2,
        &ledFlasher,
};

// scheduler task delays table
uint16_t delays[sizeof(tasks) / sizeof(Task *)];
Scheduler scheduler = Scheduler(sizeof(tasks) / sizeof(*tasks), reinterpret_cast<PGM_P>(tasks), delays);

void setup() {
   // setup ports
   // ...
   Serial.begin(57600);
   
   scheduler.begin();
}

void loop() {
   scheduler.loop();    // execute all ready tasks once before returning
}

```

In version 2, it is possible to have an LED flashing loop that does not
exit between flashes, but yields.

```cpp
uint8_t queueData[8];
Mutex ledLock(queueData, sizeof(queueData));

class LedFlasher : public AsyncTask {
    uint8_t flashCount;
    const uint16_t flashDelay;

    void begin() {
        if (flashCount) {
            resume(100);
        } else {
            suspend();
        }
    }

    void loop() {
        ledLock.reserve();

        Serial.print(F("LED Loop"));
        Serial.println(flashCount);

        for (uint8_t i = 0; i < flashCount; i++) {
            digitalWrite(LED, 1);
            yieldResume(flashDelay);
            digitalWrite(LED, 0);
            yieldResume(flashDelay);
        }

        yieldResume(1000);
        ledLock.release();
        yield();

        Serial.print(F("LED Loop"));
        Serial.print(flashCount);
        Serial.println(F(" done."));
    }

    defineSchedulerTaskId("LedFlasher");

public:
    LedFlasher(uint8_t flashCount, uint16_t flashDelay, uint8_t *pStack, uint8_t stackMax)
            : flashDelay(flashDelay), AsyncTask(pStack, stackMax) {
        this->flashCount = flashCount;
    }

    void flash(uint8_t count) {
        flashCount = static_cast<uint8_t>(count); // odd is on, even is off
        if (isSuspended()) {
            resume(0);
        }
    }
};

uint8_t flasherStack1[128];
uint8_t flasherStack2[128];

LedFlasher ledFlasher1 = LedFlasher(4, 250, flasherStack1, lengthof(flasherStack1));
LedFlasher ledFlasher2 = LedFlasher(8, 125, flasherStack2, lengthof(flasherStack2));

```

These flashers will flash the LED given number of flashes and then exit
the loop, while not blocking the CPU in between. The code also
demonstrates the use of a `ResourceLock` to arbitrate between tasks for
competing resources.

### PWM Motor Controller

Another example is a PWM motor controller which will set the motor PWM
within a few milliseconds of the request. If the motor is stopped and a
low PWM setting is requested, will "jog" the motor for 250ms at 75% of
full power before switching to the requested setting to eliminate motor
stall.

At other times the task is suspended and does not run.

Here timer 0 is setup in Fast PWM mode so as not to affect the
`millis()` and `micros()` functions and allowing use of comparator A and
B as independent PWM channels.

```cpp
// these will depend on fan/motor characteristics
#define MIN_PWM     30        // 11.7% lowest allowed setting, below this treated as 0 
#define MIN_JOG_PWM 115       // 45% lowest self start, lower uses JOG
#define JOG_PWM     191       // 75% setting used to jog motor from full stop
#define JOG_TIME    250       // jog duration ms

class MotorPWM : public Task {
    uint8_t port;
    uint8_t pwm;
    uint8_t nextPwm;

    inline void setPwmReg(uint8_t value) {
        _MMIO_BYTE(port) = value;
    }

    void begin() {
        suspend();
    }

    void loop() {
        if (pwm == nextPwm) {
            suspend();
        } else {
            if (!pwm && nextPwm && nextPwm < MIN_JOG_PWM) {
                pwm = JOG_PWM;
                setPwmReg(pwm);
                resume(JOG_TIME);
            } else {
                pwm = nextPwm;
                setPwmReg(pwm);
                suspend();
            }
        }
    }

    
    defineSchedulerTaskId("MotorPWM");

public:
    MotorPWM(volatile uint8_t &portReg) {
        port = static_cast<uint8_t>(&portReg - (volatile unsigned char *) 0);
        pwm = 0;
        nextPwm = 0;
    }

    uint8_t getPWM() { return pwm; }

    void setPwm(uint8_t pwmValue) {
        nextPwm = pwmValue < MIN_PWM ? 0 : pwmValue;

        if (nextPwm != pwm && isSuspended()) {
            resume(10);
        }
    }
};
 
MotorPWM motorPWM1 = MotorPWM(OCR0A);
MotorPWM motorPWM2 = MotorPWM(OCR0B);

// add to setup() to configure OCR0A to output PWM on PORT D[6], PIN 6
// and OCR0B to output PWM on PORT D[5], PIN 5
void setup() {
    cli();

    // uses Timer 0 shared for millis() and micros() function
    // set compare match register to 0, no output
    OCR0A = 0;
    OCR0B = 0;

    // set fast PWM mode 3 so as not to affect 1ms timer, gives 488Hz PWM cycle on 8MHz and 976Hz on 16MHz CPUs
    setBit(TCCR0A, WGM00);
    setBit(TCCR0A, WGM01);

    // clear OC0A on match, set at bottom, non-inverting mode
    clearBit(TCCR0A, COM0A0);
    setBit(TCCR0A, COM0A1);

    // clear OC0B on match, set at bottom, non-inverting mode
    clearBit(TCCR0A, COM0B0);
    setBit(TCCR0A, COM0B1);

    sei();
}

void loop() {
   scheduler.loop(10);
}
```

The `motorPWM1` and `motorPWM2` tasks, if added to the tasks table will
be run whenever their `.setPWM()` is called with a new pwm setting and
suspend its execution when there is no pwm motor activity needed.

