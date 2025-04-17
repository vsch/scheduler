//---------------------------------------------------------------------------
// GLOBAL DEFINES
#include "Arduino.h"
#include "HardwareSerial.h"

// ---------------------------------------------------------------------------
// INCLUDES
#include "src/Scheduler.h"
#include "src/Request.h"
#include "src/Mutex.h"

#define LED (13)

class RequestTest : public Request {
public:
    inline RequestTest() : Request() {}
};

RequestTest requestTest;

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is loop between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
    while (Serial.available()) {
        // drain port
        Serial.read();
    }
}

uint8_t ledLock_queue[sizeOfByteQueue(8)];
Mutex ledLock(ledLock_queue, sizeof(ledLock_queue));

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

    void activating() {
        Serial.print(F("Activating LED Loop"));
        Serial.println(flashCount);
    }

    void deactivating() {
        Serial.print(F("Deactivating LED Loop"));
        Serial.println(flashCount);
    }

    void loop() {
        Serial.print(F("LED Loop"));
        Serial.println(flashCount);

        reserveResource(ledLock);

        Serial.println(F("reserved LED"));

        for (uint8_t i = 0; i < flashCount; i++) {
            digitalWrite(LED, 1);
            yieldResume(flashDelay);
            digitalWrite(LED, 0);
            yieldResume(flashDelay);
        }

        yieldResume(1000);
        releaseResource(ledLock);
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

uint8_t flasherStack1[sizeOfStack(32)];
uint8_t flasherStack2[sizeOfStack(32)];

LedFlasher ledFlasher1 = LedFlasher(4, 250, flasherStack1, lengthof(flasherStack1));
LedFlasher ledFlasher2 = LedFlasher(8, 125, flasherStack2, lengthof(flasherStack2));

Task *const taskTable[] PROGMEM = {
        &ledFlasher1,
        &ledFlasher2,
};

uint16_t delayTable[lengthof(taskTable)];
Scheduler scheduler = Scheduler(lengthof(taskTable), reinterpret_cast<PGM_P>(taskTable), delayTable);

void setup() {
    Serial.begin(57600);
    pinMode(LED, OUTPUT);

    Serial.println(F("Setup"));

    scheduler.begin();
}

unsigned long lastPrint = 0;

void loop() {
    RequestTest *pReq = &requestTest;

    scheduler.loop(10);

    if (micros() - lastPrint >= 1000L * 1000L) {
        lastPrint = micros();

        uint8_t stackUsed1 = ledFlasher1.maxStackUsed();
        uint8_t stackUsed2 = ledFlasher2.maxStackUsed();

        Serial.print(F("Task 1 max stack: "));
        Serial.println(stackUsed1);
        Serial.print(F("Task 2 max stack: "));
        Serial.println(stackUsed2);
    }
}
