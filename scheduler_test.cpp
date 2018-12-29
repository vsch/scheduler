//---------------------------------------------------------------------------
// GLOBAL DEFINES
#define F_CPU       (16000000L/2)       // run CPU at 8 MHz or 16 MHz
#define LED         4                   // LED on pin 4

#define PORTB_OUT 0x2E

// ---------------------------------------------------------------------------
// INCLUDES
#include <Arduino.h>
#include <avr/io.h>         // deal with port registers
#include <avr/interrupt.h>  // deal with interrupt calls
#include <util/delay.h>     // used for _delay_ms function
#include <string.h>         // string manipulation routines
#include <avr/sleep.h>      // used for sleep functions
#include <stdlib.h>
#include <ssd1306.h>
#include <dht22.h>
#include <scheduler.h>

#define DISPLAY_TYPE SSD1306_TYPE_OLED_096 /*| SSD1306_INVERTED*/ /*| SSD1306_EXTERNALVCC*/

#include <ssd1306_display.h>

uint8_t pageBuffer[DISPLAY_XSIZE];
Ssd1306 tft = Ssd1306(pageBuffer, DISPLAY_TYPE);
Dht22 dht = Dht22(3);

#define SHOW_TIMING
#define PWM

#define MIN_PWM    30
#define JOG_PWM    196
#define JOG_TIME   250

//#define DEBUG_LED
//#define DEBUG_PWM

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
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

public:
    LedFlasher() {
        flashCount = 0;
    }

    void flash(uint8_t count) {
#ifdef DEBUG_LED
        Serial.print("flash ");
        Serial.print(count);
        Serial.print(" was ");
        Serial.print(flashCount);
#endif
        if (flashCount) {
            // already flashing, add to count
            flashCount += static_cast<uint8_t>((count << 1)); // odd is on, even is off
        } else {
            flashCount = static_cast<uint8_t>((count << 1)); // odd is on, even is off
            resume(0);
        }

#ifdef DEBUG_LED
        Serial.print(" -> ");
        Serial.println(flashCount);
#endif
    }

    const char *id() {
        return "LedFlasher";
    }

private:
    void init() {
        flashCount = 0;
        pinMode(LED, OUTPUT);
        suspend();
    }

    void run() {
        if (flashCount == 0) {
#ifdef DEBUG_LED
            Serial.println("flashing done");
#endif
            digitalWrite(LED, 0);
            suspend();
        } else {
#ifdef DEBUG_LED
            Serial.print("flashing ");
            Serial.print(flashCount);
#endif

            if (flashCount & 1) {
#ifdef DEBUG_LED
                Serial.print(" off ");
#endif
                digitalWrite(LED, 0);
            } else {
#ifdef DEBUG_LED
                Serial.print(" on ");
#endif
                digitalWrite(LED, 1);
            }

            flashCount--;

#ifdef DEBUG_LED
            Serial.println(flashCount);
#endif
            delay(150);            // wait 150ms
        }
    }
} ledFlasher = LedFlasher();

class Counter1 : public Task {
public:
    Counter1() {
        count = 0;
    }

    uint8_t getCount() {
        return count;
    }

    const char *id() {
        return "Counter1";
    }

private:
    uint8_t count;

    void init() {
        count = 0;
        delay(250);
    }

    void run() {
        count++;
        ledFlasher.flash(1);
        delay(2000);
    }
} counter1 = Counter1();

class Counter2 : public Task {
public:

    Counter2() {
        count = 0;
    }

    uint8_t getCount() {
        return count;
    }

    const char *id() {
        return "Counter2";
    }

private:
    uint8_t count;

    void init() {
        count = 0;
        delay(500);
    }

    void run() {
        count++;
        ledFlasher.flash(2);
        delay(4000);
    }
} counter2 = Counter2();

class Counter3 : public PeriodicTask {
public:

    Counter3() {
        count = 0;
    }

    uint8_t getCount() {
        return count;
    }

    const char *id() {
        return "Counter3";
    }

private:
    uint8_t count;

    void init() {
        count = 0;
        delay(750);
    }

    void run() {
        markRun();

        count++;
        ledFlasher.flash(3);
        delay(10000);
    }
} counter3 = Counter3();

class TempHumidity : public Task {
public:

    TempHumidity() {
        temp = 0;
        humidity = 0;
    }

    float getTemp() {
        return temp;
    }

    float getHumidity() {
        return humidity;
    }

    const char *id() {
        return "TempHumidity";
    }

private:
    float temp;
    float humidity;

    void init() {
        delay(500);
    }

    void run() {
        temp = dht.readTemperature(false, false);
        humidity = dht.readHumidity(false);

        delay(2000);
    }
} tempHumidity = TempHumidity();

#ifdef PWM

class MotorPWM : public Task {
    uint8_t pwm;
    uint8_t nextPwm;

public:
    MotorPWM() {
        pwm = 0;
        nextPwm = 0;
    }

    void setPwm(uint8_t pwmValue) {
        bool suspended = pwm == nextPwm;

        nextPwm = pwmValue < MIN_PWM ? 0 : pwmValue;

        if (nextPwm != pwm && suspended) {
#ifdef DEBUG_PWM
            Serial.println("PWM resumed");
#endif
            resume(10);
        }
    }

    uint8_t getPWM() {
        return pwm;
    }

    const char *id() {
        return "MotorPWM";
    }

private:
    void init() {
        pwm = 0;
        nextPwm = 0;
        suspend();
    }

    void run() {
        if (pwm == nextPwm) {
#ifdef DEBUG_PWM
            Serial.println("PWM suspended");
#endif
            suspend();
        } else {
#ifdef DEBUG_PWM
            Serial.print("PWM running ");
            Serial.print(pwm);
            Serial.print("->");
            Serial.println(nextPwm);
#endif

            if (!pwm && nextPwm) {
                pwm = JOG_PWM;
                OCR0A = pwm;
                delay(JOG_TIME);
            } else {
                OCR0A = nextPwm;
                pwm = nextPwm;
#ifdef DEBUG_PWM
                Serial.println("PWM suspended");
#endif
                suspend();            // wait 10ms
            }
        }
    }
} motorPWM = MotorPWM();

#endif

class Updater : public Task {
public:
    Updater() {
        iter = 0;
        lastTime = 0;
    }

    const char *id() {
        return "Updater";
    }

private:
    uint8_t iter;
    unsigned long lastTime;

    void init() {
        ledFlasher.flash(1);                                // indicate program start
        tft.clearScreen();
    }

    void run() {
#ifdef SHOW_TIMING
        int totalLines = 8;
        unsigned long start = micros();
#else
        int totalLines = 7;
#endif

        int totalColumns = 15;
        int col0 = (tft.maxCols - totalColumns) / 2;
        int line0 = (tft.maxRows - totalLines + 1) / 2;

//        Serial.print("maxCols: ");
//        Serial.print(tft.maxCols);
//        Serial.print(" maxRows: ");
//        Serial.println(tft.maxRows);

#ifdef PWM
        motorPWM.setPwm(iter * 5);
#endif

        float temp = tempHumidity.getTemp();
        float humidity = tempHumidity.getHumidity();

        tft.startUpdate();
        while (tft.nextPage()) {
//            tft.fillRect(col0 * SSD1306_CHAR_WIDTH - 1, line0 * SSD1306_CHAR_HEIGHT - 1,
//                         (col0 + totalColumns) * SSD1306_CHAR_WIDTH, (line0 + totalLines) * SSD1306_CHAR_HEIGHT,
//                         tft.background);
//
            int line = line0;
            int col = col0;

#ifdef PWM
            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("   PWM ");
            float pwmP = motorPWM.getPWM() * 100.0 / 255.0;
            tft.write((int) pwmP);
            tft.write('.');
            tft.write(((int) (pwmP * 10)) % 10);
            tft.write('%');
#endif

#ifdef SHOW_TIMING
            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("  Time ");
            tft.write(lastTime);
#endif
            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("  Temp ");
            tft.write((int) (temp));
            tft.write('.');
            tft.write((int) (temp * 10) % 10);
            tft.write(" C");

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write(" Humid ");
            tft.write((int) (humidity));
            tft.write('.');
            tft.write((int) (humidity * 10) % 10);
            tft.write("%");

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("Count1 ");
            tft.write(counter1.getCount());

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("Count2 ");
            tft.write(counter2.getCount());

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("Count3 ");
            tft.write(counter3.getCount());
            tft.write(" ");
            tft.write(millis()/10000);

            tft.gotoCharXY(col, line++);                                 // position text cursor
            tft.write("---------------");
        }

        iter++;

#ifdef SHOW_TIMING
        lastTime = micros() - start;
#endif
        delay(1000);
    }
} updater = Updater();

Task *tasks[] = {
        &updater,
        &counter1,
        &counter2,
//        &counter3,
        &ledFlasher,
        &tempHumidity,
#ifdef PWM
        &motorPWM,
#endif
};

uint32_t delays[sizeof(tasks) / sizeof(Task *)];
Scheduler scheduler = Scheduler(sizeof(tasks) / sizeof(Task *), tasks, delays);

void setup() {
#ifdef PWM
    cli();

    // set compare match register to 0, no output
    OCR0A = 0;// = (16*10^6) / (2000*64) - 1 (must be <256)

    // set fast PWM mode 3 so as not to affect 1ms timer, gives 488Hz PWM cycle
    setBit(TCCR0A, WGM00);
    setBit(TCCR0A, WGM01);

    // clear OC0A on match, set at bottom, non-inverting mode
    clearBit(TCCR0A, COM0A0);
    setBit(TCCR0A, COM0A1);

    sei();
#endif
//    Serial.begin(57600);
    Serial.begin(256000);

    DDRB = PORTB_OUT; // 0010.1110; set B1, B2-B3, B5 as outputs
    DDRC = 0x00; // 0000.0000; set PORTC as inputs
    DDRD = 0xF0; // 0111.0000; set PORTD 4,5,6,7 as output
    PORTD = 0;

    setBit(SSD1306_RST_PORT, SSD1306_RST_BIT); // start with TFT reset line inactive high
    setBit(SSD1306_CS_PORT, SSD1306_CS_BIT);  // deselect TFT CS
    clearBit(SSD1306_SCK_PORT, SSD1306_SCK_BIT);  // TFT SCK Low

    dht.begin();

    Serial.println("Setup ports");
    tft.openSPI();                              // start communication to TFT
    Serial.println("Started spi");

    tft.initDisplay(SSD1306_ROT_0);                              // initialize TFT controller
    Serial.println("Initialized display");

    scheduler.start();
}

void loop() {
    scheduler.run(100);
}
