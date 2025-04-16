//---------------------------------------------------------------------------
// GLOBAL DEFINES
#include "Arduino.h"
#include "HardwareSerial.h"

// ---------------------------------------------------------------------------
// INCLUDES
#include <Arduino.h>
#include "src/Scheduler.h"
#include "src/ResourceLock.h"

#define LED (13)

//#define DEBUG_LED
#define lengthof(a)  (sizeof(a)/sizeof((a)[0]))     // get length of array
//#define DEBUG_PWM

#ifdef DEBUG_LED
#define debugLedPrintf_P(...) printf_P(__VA_ARGS__)
#define debugLedPuts_P(...) puts_P(__VA_ARGS__)
#else
#define debugLedPrintf_P(...) ((void)0)
#define debugLedPuts_P(...) ((void)0)
#endif

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

uint8_t queueData[8];
ResourceLock ledLock(queueData, sizeof(queueData));

class LedFlasher : public YieldingTask {
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
        ledLock.reserveResource(this);

        Serial.print(F("LED Loop"));
        Serial.println(flashCount);

        for (uint8_t i = 0; i < flashCount; i++) {
            digitalWrite(LED, 1);
            yieldResume(flashDelay);
            digitalWrite(LED, 0);
            yieldResume(flashDelay);
        }

        yieldResume(1000);
        ledLock.releaseResource();
        yield();

        Serial.print(F("LED Loop"));
        Serial.print(flashCount);
        Serial.println(F(" done."));
    }

    defineSchedulerTaskId("LedFlasher");

public:
    LedFlasher(uint8_t flashCount, uint16_t flashDelay, uint8_t *pStack, uint8_t stackMax)
            : flashDelay(flashDelay), YieldingTask(pStack, stackMax) {
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

void loop() {
    scheduler.loop(10);
}
