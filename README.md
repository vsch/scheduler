# Arduino Scheduler library

Cooperative scheduler using a simple task model with `begin()` and
`loop()` methods. Uses the `micros()` function for computing delay
expiration.

Task can use scheduler to `suspend()`, `resume(uin16_t milliseconds)`
after suspension, or schedule next execution with `delay(uin16_t
milliseconds)`. If milliseconds is given as `0` then the current task
will be rescheduled after all ready tasks have been run but without
additional delay.

A task can initialize state in its `begin()` method and use `delay()`
method of its super class `Task` to schedule the start of its `loop()` or
`suspend()` if its scheduling will be triggered by another task.

Each task is an instance, holding its own state and handling its own
re-scheduling. This allows adding new tasks to the project without
having to propagate new state information to the main loop.

As far as each task is concerned it is running the main loop,
independent of other tasks.

The scheduler has two public methods `begin()` for initialization and
`loop(uint16 timeSliceMilliseconds)` for executing the task loop.

The `begin()` method will initialize the scheduler and invoke all tasks'
`begin()` methods.

The `loop()` method will execute all ready tasks once or until current
run time has exceeded the given `timeSliceMilliseconds`. Passing default
of `0` means no time limit, execute all ready tasks once and return.

The scheduler constructor takes a table of pointers to tasks in
`PROGMEM` and a task delay table in RAM. No dynamic memory allocation is
used by design. All tasks are defined at compile time. Since, only active
tasks are run, there is minimal overhead for inactive tasks.

The RAM overhead per task in the scheduler + Task is 4 bytes, with a
fixed overhead of 11 bytes. So 8 tasks will use an extra 44 bytes of
RAM.

## Usage

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

    void init() {
        pinMode(LED, OUTPUT);
        suspend();
    }

    void run() {
        if (flashCount == 0) {
            digitalWrite(LED, 0);
            suspend();
        } else {
            digitalWrite(LED, !(flashCount & 1));
            flashCount--;
            delay(flashCount ? 100 : 250);   // longer pause at end of flashes
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
        delay(2250);  // start in 2.25 seconds 
    }

    void loop() {
        count++;
        ledFlasher.flash(1);
        delay(2000); // run every 2 seconds
    }
    
public:
    Counter1() { count = 0; } 
    uint8_t getCount() { return count; } 
    const __FlashStringHelper *id() { return F("Counter1"); }
} counter1 = Counter1();

class Counter2 : public Task {
    uint8_t count;

    void begin() {
        delay(2750);  // start in 2.75 seconds
    }

    void loop() {
        count++;
        ledFlasher.flash(2);
        delay(3000); // run every 3 seconds
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
        delay(1000);  // run every second
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

```cpp
#define MIN_PWM     25        // 9.8% lowest allowed setting
#define MIN_JOG_PWM 115       // 45% lowest self start, lower uses JOG
#define JOG_PWM     191       // 75% setting for jog
#define JOG_TIME    250       // jog duration ms

class MotorPWM : public Task {
    uint8_t pwm;
    uint8_t nextPwm;

    void begin() {
        cli();

        // uses Timer 0 shared for millis() and micros() function
        // set compare match register to 0, no output
        OCR0A = 0;

        // set fast PWM mode 3 so as not to affect 1ms timer, gives 488Hz PWM cycle on 8MHz and 976Hz on 16MHz CPUs
        setBit(TCCR0A, WGM00);
        setBit(TCCR0A, WGM01);

        // clear OC0A on match, set at bottom, non-inverting mode
        clearBit(TCCR0A, COM0A0);
        setBit(TCCR0A, COM0A1);

        sei();
        suspend();
    }

    void loop() {
        if (pwm == nextPwm) {
            suspend();
        } else {
            if (!pwm && nextPwm && nextPwm < MIN_JOG_PWM) {
                pwm = JOG_PWM;
                OCR0A = pwm;
                delay(JOG_TIME);
            } else {
                OCR0A = nextPwm;
                pwm = nextPwm;
                suspend();
            }
        }
    }
    
public:
    MotorPWM() {
        pwm = 0;
        nextPwm = 0;
    }
    
    const __FlashStringHelper *id() { return F("MotorPWM"); }

    uint8_t getPWM() { return pwm; }
    
    void setPwm(uint8_t pwmValue) {
        nextPwm = pwmValue < MIN_PWM ? 0 : pwmValue;

        if (nextPwm != pwm && isSuspended()) {
            resume(10);  // resume in 10ms and apply new setting
        }
    }

} motorPWM = MotorPWM();

```

The `motorPWM` task, if added to the tasks table will be run whenever
the `motorPWM.setPWM()` is called with a new pwm setting and suspend its
execution when there is no pwm motor activity needed.

