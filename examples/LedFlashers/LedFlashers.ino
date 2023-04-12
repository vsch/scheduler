#include <Arduino.h>
#include "Scheduler.h"

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

    defineSchedulerTaskId("LedFlasher");

public:
    LedFlasher() { flashCount = 0; }

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

    defineSchedulerTaskId("Counter1");

public:
    Counter1() { count = 0; }
    uint8_t getCount() { return count; }
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

    defineSchedulerTaskId("Counter2");

public:
    Counter2() { count = 0; }
    uint8_t getCount() { return count; }

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

    defineSchedulerTaskId("Updater");

public:
    Updater() = default;
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

