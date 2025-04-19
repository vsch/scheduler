//---------------------------------------------------------------------------
// GLOBAL DEFINES
#include "Arduino.h"
#include "HardwareSerial.h"

// ---------------------------------------------------------------------------
// INCLUDES
#include <Arduino.h>
#include "src/Scheduler.h"
#include "src/Mutex.h"
#include "src/Streams.h"
#include "src/Stream.h"

#define LED (13)

#ifdef QUEUE_WORD_FUNCS
#define QUEUE_WORDS
#endif

uint8_t qData[sizeOfQueue(32, uint8_t)];
Queue qRequests(qData, sizeof(qData));

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
            ByteStream bs(&qRequests, STREAM_FLAGS_WR);
            // qRequests.addTail('L');
            // qRequests.addTail(getIndex() + '0');
            // qRequests.addTail(':');
#ifdef QUEUE_WORDS
            bs.putW(flashCount << 8 | flashDelay);
#else
            bs.put('L');
            bs.put(getIndex() + '0');
            bs.put(':');
#endif
            qRequests.updateStreamed(&bs);
        }

        yieldResume(500);
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

LedFlasher ledFlasher1 = LedFlasher(2, 250, flasherStack1, lengthof(flasherStack1));
LedFlasher ledFlasher2 = LedFlasher(4, 125, flasherStack2, lengthof(flasherStack2));

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
    scheduler.loop(10);

    if (micros() - lastPrint >= 5 * 1000L * 1000L) {
        lastPrint = micros();

        uint8_t stackUsed1 = ledFlasher1.maxStackUsed();
        uint8_t stackUsed2 = ledFlasher2.maxStackUsed();

        Serial.print(F("Task 1 max stack: "));
        Serial.println(stackUsed1);
        Serial.print(F("Task 2 max stack: "));
        Serial.println(stackUsed2);

        if (!qRequests.isEmpty()) {
            ByteStream bs(&qRequests, STREAM_FLAGS_RD);
            Serial.print(F("Requests: "));
            while (!bs.is_empty()) {
#ifdef QUEUE_WORDS
                Serial.print(F("0x"));
                Serial.print(bs.getW(), HEX);
                Serial.print(' ');
#else
                Serial.print((char) bs.get());
#endif

            }
            Serial.println();
            qRequests.updateStreamed(&bs);
        }
    }
}
