//---------------------------------------------------------------------------
// GLOBAL DEFINES
#define F_CPU       (16000000L/2)       // loop CPU at 8 MHz or 16 MHz
#define LED         4                   // LED on pin 4
#define PORTB_OUT 0x2E

// ---------------------------------------------------------------------------
// INCLUDES
#include <Arduino.h>
#include <st7735.h>
#include <dht22.h>
#include <scheduler.h>

#define DISPLAY_TYPE ST7735_TYPE_OLED_096 /*| SSD1306_INVERTED*/ /*| SSD1306_EXTERNALVCC*/

#include <st7745_display.h>

//uint8_t pageBuffer[DISPLAY_XSIZE];
//Ssd1306 tft = Ssd1306(pageBuffer, DISPLAY_TYPE);
St7735 tft = St7735(DISPLAY_TYPE);
Dht22 dht = Dht22(3);

#define SHOW_TIMING
#define PWM

#define MIN_PWM     25
#define MIN_JOG_PWM 115
#define JOG_PWM     191
#define JOG_TIME    250

//#define DEBUG_LED
//#define DEBUG_PWM

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

class LedFlasher : public Task {
    uint8_t flashCount;

    void begin() {
        pinMode(LED, OUTPUT);
        suspend();
    }

    void loop() {
        if (flashCount == 0) {
#ifdef DEBUG_LED
            Serial.println(F("flashing done"));
#endif
            digitalWrite(LED, 0);
            suspend();
        } else {
#ifdef DEBUG_LED
            Serial.print(F("flashing "));
            Serial.print(flashCount);
#endif

            digitalWrite(LED, !(flashCount & 1));
            flashCount--;

#ifdef DEBUG_LED
            Serial.println(flashCount);
#endif
            delay(flashCount ? 100 : 250);
        }
    }

public:
    LedFlasher() { flashCount = 0; }
    const __FlashStringHelper *id() { return F("LedFlasher"); }

    void flash(uint8_t count) {
#ifdef DEBUG_LED
        Serial.print(F("flash "));
        Serial.print(count);
        Serial.print(F(" was "));
        Serial.print(flashCount);
#endif
        if (!isSuspended()) {
            // already flashing, add to count
            flashCount += static_cast<uint8_t>((count << 1)); // odd is on, even is off
        } else {
            flashCount = static_cast<uint8_t>((count << 1)); // odd is on, even is off
            resume(0);
        }

#ifdef DEBUG_LED
        Serial.print(F(" -> "));
        Serial.println(flashCount);
#endif
    }
} ledFlasher = LedFlasher();

class Counter1 : public Task {
    uint8_t count;

    void begin() {
        delay(1750);
    }

    void loop() {
        count++;
        ledFlasher.flash(1);
        delay(2000);
    }

public:
    Counter1() { count = 0; }
    uint8_t getCount() { return count; }
    const __FlashStringHelper *id() { return F("Counter1"); }
} counter1 = Counter1();

class Counter2 : public Task {
    uint8_t count;

    void begin() {
        delay(2750);
    }

    void loop() {
        count++;
        ledFlasher.flash(2);
        delay(3000);
    }

public:
    Counter2() { count = 0; }
    uint8_t getCount() { return count; }
    const __FlashStringHelper *id() { return F("Counter2"); }
} counter2 = Counter2();

class Counter3 : public Task {
    uint8_t count;

    void begin() {
        delay(4750);
    }

    void loop() {
        count++;
        ledFlasher.flash(3);
        delay(5000);
    }

public:
    Counter3() { count = 0; }
    uint8_t getCount() { return count; }
    const __FlashStringHelper *id() { return F("Counter3"); }
} counter3 = Counter3();

class TempHumidity : public Task {
    void begin() {
        dht.begin();
        delay(1000);
    }

    void loop() {
        dht.read();
        delay(2000);
    }

public:
    TempHumidity() = default;
    const __FlashStringHelper *id() { return F("TempHumidity"); }

    float getTemperature(bool isFahrenheit = false) { return dht.getTemperature(isFahrenheit); }
    float getHumidity() { return dht.getHumidity(); }
} tempHumidity = TempHumidity();

#ifdef PWM

class MotorPWM : public Task {
    uint8_t pwm;
    uint8_t nextPwm;

    void begin() {
        cli();

        // set compare match register to 0, no output
        OCR0A = 0;

        // set fast PWM mode 3 so as not to affect 1ms timer, gives 488Hz PWM cycle
        setBit(TCCR0A, WGM00);
        setBit(TCCR0A, WGM01);

        // clear OC0A on match, set at bottom, non-inverting mode
        clearBit(TCCR0A, COM0A0);
        setBit(TCCR0A, COM0A1);

        sei();

        pwm = 0;
        nextPwm = 0;
        suspend();
    }

    void loop() {
        if (pwm == nextPwm) {
#ifdef DEBUG_PWM
            Serial.println(F("PWM suspended"));
#endif
            suspend();
        } else {
#ifdef DEBUG_PWM
            Serial.print(F("PWM running "));
            Serial.print(pwm);
            Serial.print(F("->"));
            Serial.println(nextPwm);
#endif

            if (!pwm && nextPwm && nextPwm < MIN_JOG_PWM) {
                pwm = JOG_PWM;
                OCR0A = pwm;
                delay(JOG_TIME);
            } else {
                OCR0A = nextPwm;
                pwm = nextPwm;
#ifdef DEBUG_PWM
                Serial.println(F("PWM suspended"));
#endif
                suspend();            // wait 10ms
            }
        }
    }

public:
    MotorPWM() { pwm = 0; nextPwm = 0; }
    const __FlashStringHelper *id() { return F("MotorPWM"); }

    uint8_t getPWM() { return pwm; }

    void setPwm(uint8_t pwmValue) {
        bool suspended = isSuspended();

        nextPwm = pwmValue < MIN_PWM ? 0 : pwmValue;

        if (nextPwm != pwm && suspended) {
#ifdef DEBUG_PWM
            Serial.println(F("PWM resumed"));
#endif
            resume(10);
        }
    }

} motorPWM = MotorPWM();

#endif

class Updater : public Task {
    uint8_t iter;
    unsigned long lastTime;

    void begin() {
        ledFlasher.flash(1);                                // indicate program start
        tft.clearScreen();
    }

    void loop() {
#ifdef SHOW_TIMING
        int totalLines = 8;
        unsigned long start = micros();
#else
        int totalLines = 7;
#endif

        int totalColumns = 15;
        int col0 = (tft.maxCols - totalColumns) / 2;
        int line0 = (tft.maxRows - totalLines + 1) / 2;

//        Serial.print(F("maxCols: "));
//        Serial.print(tft.maxCols);
//        Serial.print(F(" maxRows: "));
//        Serial.println(tft.maxRows);

#ifdef PWM
        motorPWM.setPwm(iter * 5);
#endif

        float temp = tempHumidity.getTemperature();
        float humidity = tempHumidity.getHumidity();

        tft.startUpdate();
        while (tft.nextPage()) {
//            tft.fillRect(col0 * CHAR_WIDTH - 1, line0 * CHAR_HEIGHT - 1,
//                         (col0 + totalColumns) * CHAR_WIDTH, (line0 + totalLines) * CHAR_HEIGHT,
//                         tft.background);

            int line = line0;
            int col = col0;

#ifdef PWM
            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("   PWM "));
            float pwmP = motorPWM.getPWM() * 100.0 / 255.0;
            tft.write((int) pwmP);
            tft.write('.');
            tft.write(((int) (pwmP * 10)) % 10);
            tft.write(F("%  "));
#endif

#ifdef SHOW_TIMING
            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("  Time "));
            tft.write(lastTime);
#endif
            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("  Temp "));
            tft.write((int) (temp));
            tft.write('.');
            tft.write((int) (temp * 10) % 10);
            tft.write(F(" C  "));

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F(" Humid "));
            tft.write((int) (humidity));
            tft.write('.');
            tft.write((int) (humidity * 10) % 10);
            tft.write(F("%  "));

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("Count1 "));
            tft.write(counter1.getCount());
            tft.write(F("  "));

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("Count2 "));
            tft.write(counter2.getCount());
            tft.write(F("  "));

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("Count3 "));
            tft.write(counter3.getCount());
            tft.write(F(" "));
            tft.write(millis() / 5000);
            tft.write(F("  "));

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(F("---------------"));
        }

        iter++;

#ifdef SHOW_TIMING
        lastTime = micros() - start;
#endif
        delay(1000);
    }

public:
    Updater() { iter = 0; lastTime = 0; }
    const __FlashStringHelper *id() { return F("Updater"); }
} updater = Updater();

Task *const tasks[] PROGMEM = {
        &updater,
        &counter1,
        &counter2,
        &counter3,
        &ledFlasher,
        &tempHumidity,
#ifdef PWM
        &motorPWM,
#endif
};

uint16_t delays[sizeof(tasks) / sizeof(*tasks)];
Scheduler scheduler = Scheduler(sizeof(tasks) / sizeof(*tasks), reinterpret_cast<PGM_P>(tasks), delays);

void setup() {
//    Serial.begin(57600);
    Serial.begin(256000);

    DDRB = PORTB_OUT; // 0010.1110; set B1, B2-B3, B5 as outputs
    DDRC = 0x00; // 0000.0000; set PORTC as inputs
    DDRD = 0xF0; // 0111.0000; set PORTD 4,5,6,7 as output
    PORTD = 0;

//    setBit(SSD1306_RST_PORT, SSD1306_RST_BIT); // start with TFT reset line inactive high
//    setBit(SSD1306_CS_PORT, SSD1306_CS_BIT);  // deselect TFT CS
//    clearBit(SSD1306_SCK_PORT, SSD1306_SCK_BIT);  // TFT SCK Low
    setBit(ST7735_RST_PORT, ST7735_RST_BIT); // start with TFT reset line inactive high
    setBit(ST7735_CS_PORT, ST7735_CS_BIT);  // deselect TFT CS
    clearBit(ST7735_SCK_PORT, ST7735_SCK_BIT);  // TFT SCK Low


    Serial.println(F("Setup ports"));
    tft.openSPI();                              // start communication to TFT
    Serial.println(F("Started SPI"));
    Serial.print(F("sizeof(long) "));
    Serial.println(sizeof(long));
    Serial.print(F("sizeof(Task) "));
    Serial.println(sizeof(Task));
    Serial.print(F("sizeof(Scheduler) "));
    Serial.println(sizeof(Scheduler));

    scheduler.begin();

//    tft.initDisplay(ST7735_ROT_0);                              // initialize TFT controller
    tft.initDisplay(ST7735_ROT_90);                              // initialize TFT controller
    tft.foreground = YELLOW;
    Serial.println(F("Initialized display"));
}

void loop() {
    scheduler.loop(100);
}
