# Arduino Scheduler library

Cooperative round-robin scheduler using a simple task model with
`begin()` and `loop()` methods.

[TOC]: #

- [Overview](#overview)
- [Example](#example)
  - [PWM Motor Controller](#pwm-motor-controller)


## Overview

Tasks can use scheduler to `suspend()`, `resume(uin16_t milliseconds)`
after suspension or to schedule next execution after delay. If
`milliseconds` is given as `0` then the current task will be rescheduled
to run in the next scheduler loop invocation and after all ready tasks
in the current invocation have been run.

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
    const __FlashStringHelper *id() { return F("LedFlasher"); }

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
    const __FlashStringHelper *id() { return F("Counter1"); }
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
    const __FlashStringHelper *id() { return F("Counter2"); }

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
    const __FlashStringHelper *id() { return F("Updater"); }
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

    const __FlashStringHelper *id() { return F("MotorPWM"); }

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
   scheduler.loop();
}
```

The `motorPWM1` and `motorPWM2` tasks, if added to the tasks table will
be run whenever their `.setPWM()` is called with a new pwm setting and
suspend its execution when there is no pwm motor activity needed.

