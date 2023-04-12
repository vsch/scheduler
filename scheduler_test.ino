//---------------------------------------------------------------------------
// GLOBAL DEFINES
#define F_CPU       (16000000L/2)       // loop CPU at 8 MHz or 16 MHz
#define LED         4                   // LED on id 4
#define PORTB_OUT 0x2E

// ---------------------------------------------------------------------------
// INCLUDES
#include <Arduino.h>
#include <st7735.h>
#include <dht22.h>
#include <Scheduler.h>

#define DISPLAY_TYPE ST7735_TYPE_OLED_096 /*| SSD1306_INVERTED*/ /*| SSD1306_EXTERNALVCC*/

#include <st7735_display.h>

//uint8_t pageBuffer[DISPLAY_XSIZE];
//Ssd1306 tft = Ssd1306(pageBuffer, DISPLAY_TYPE);
St7735 tft = St7735(DISPLAY_TYPE);
Dht22 dht = Dht22(3);

#define SHOW_TIMING
#define PWM

#define MIN_PWM     30
#define MIN_JOG_PWM 115
#define JOG_PWM     191
#define JOG_TIME    250

#define THERMISTOR_PIN 1  // analog 1

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

            digitalWrite(LED, static_cast<uint8_t>(flashCount & 1));
            flashCount--;

#ifdef DEBUG_LED
            Serial.println(flashCount);
#endif
            resume(flashCount ? 100 : 250);
        }
    }

    defineSchedulerTaskId("LedFlasher");

public:
    LedFlasher() { flashCount = 0; }

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
        resume(1750);
    }

    void loop() {
        count++;
        ledFlasher.flash(1);
        resume(2000);
    }

    defineSchedulerTaskId("Counter1");

public:
    Counter1() { count = 0; }

    uint8_t getCount() { return count; }
} counter1 = Counter1();

class Counter2 : public Task {
    uint8_t count;

    void begin() {
        resume(2750);
    }

    void loop() {
        count++;
        ledFlasher.flash(2);
        resume(3000);
    }

    defineSchedulerTaskId("Counter2");

public:
    Counter2() { count = 0; }

    uint8_t getCount() { return count; }
} counter2 = Counter2();

class Counter3 : public Task {
    uint8_t count;

    void begin() {
        resume(4750);
    }

    void loop() {
        count++;
        ledFlasher.flash(3);
        resume(5000);
    }

    defineSchedulerTaskId("Counter3");

public:
    Counter3() { count = 0; }

    uint8_t getCount() { return count; }

} counter3 = Counter3();

class TempHumidity : public Task {
    void begin() {
        dht.begin();
        resume(1000);
    }

    void loop() {
        dht.read();
        resume(2000);
    }

    defineSchedulerTaskId("TempHumidity");

public:
    TempHumidity() = default;

    float getTemperature(bool isFahrenheit = false) { return dht.getTemperature(isFahrenheit); }

    float getHumidity() { return dht.getHumidity(); }
} tempHumidity = TempHumidity();

extern float thermistorTemp(int RawADC);

class Thermistor : public Task {
    int16_t temps[4];
    uint8_t index;

    void begin() {
        resume(1000);
    }

    void loop() {
        temps[index++] = static_cast<int16_t>(thermistorTemp(analogRead(THERMISTOR_PIN)) * 10);
        index &= 3;

        resume(1000);
    }

    defineSchedulerTaskId("Thermistor");

public:
    Thermistor() {
        temps[0] = temps[1] = temps[2] = temps[3] = 0;
        index = 0;
    };

    float getTemperature(bool isFahrenheit = false) {
        float temp = (temps[0] + temps[1] + temps[2] + temps[3]) / 40.0;
        return isFahrenheit ? Dht22::convertCtoF(temp) : temp;
    }
} thermistor = Thermistor();

#ifdef PWM
//#define DEBUG_PWM_VALUES

class MotorPWM : public Task {
    uint8_t port;
    uint8_t pwm;
    uint8_t nextPwm;

    inline void setPwmReg(uint8_t value) {
#ifdef DEBUG_PWM_VALUES
        Serial.print(F("PWM "));
        Serial.print(port, HEX);
        Serial.print(F(" "));
        Serial.println(value);
#endif
        _MMIO_BYTE(port) = value;
    }

    void begin() {
        suspend();
    }

    void loop() {
        if (pwm == nextPwm) {
            suspend();
#ifdef DEBUG_PWM
            Serial.print(F("PWM "));
            Serial.print(port, HEX);
            Serial.println(F(" suspended"));
#endif
        } else {
#ifdef DEBUG_PWM
            Serial.print(F("PWM "));
            Serial.print(port, HEX);
            Serial.print(F(" running "));
            Serial.print(pwm);
            Serial.print(F("->"));
            Serial.println(nextPwm);
#endif

            if (!pwm && nextPwm && nextPwm < MIN_JOG_PWM) {
                pwm = JOG_PWM;
                setPwmReg(pwm);
                resume(JOG_TIME);
            } else {
                pwm = nextPwm;
                setPwmReg(pwm);
                suspend();
#ifdef DEBUG_PWM
                Serial.print(F("PWM "));
                Serial.print(port, HEX);
                Serial.println(F(" suspended"));
#endif
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
#ifdef DEBUG_PWM
            Serial.print(F("PWM "));
            Serial.print(port, HEX);
            Serial.println(F(" resumed"));
#endif
            resume(10);
        }
    }
};

MotorPWM motorPWM1 = MotorPWM(OCR0A);
MotorPWM motorPWM2 = MotorPWM(OCR0B);

#endif

class Updater : public Task {
    uint8_t iter;
    unsigned long lastTime;

    void begin() {
        ledFlasher.flash(1);                                // indicate program start
        tft.clearScreen();
    }

    void write(float value, const __FlashStringHelper *suffix) {
        tft.write((long) (value * 10), 1, '.');
        if (suffix) {
            tft.write(suffix);
        }
    }

    void loop() {
#ifdef SHOW_TIMING
        int totalLines = 7;
        unsigned long start = micros();
#else
        int totalLines = 6;
#endif

        int totalColumns = 20;
        int col0 = (tft.maxCols - totalColumns) / 2;
        int line0 = (tft.maxRows - totalLines) / 2;

//        Serial.print(F("maxCols: "));
//        Serial.print(tft.maxCols);
//        Serial.print(F(" maxRows: "));
//        Serial.println(tft.maxRows);

#ifdef PWM
        motorPWM1.setPwm(iter * 5);
        motorPWM2.setPwm(iter * 1);
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

#ifdef SHOW_TIMING
            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.foreground = WHITE;
            tft.write(F("  Time "));
            tft.write(lastTime, 3, '.');
#endif
            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.foreground = ROSE;
            tft.write(F("  Temp "));
            write(temp, F(" C  "));

            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.foreground = CYAN;
            tft.write(F(" Humid "));
            write(humidity, F("%  "));

            tft.foreground = YELLOW;
#ifdef PWM
            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.write(F("  PWM1 "));
            write(motorPWM1.getPWM() * 100.0 / 255.0, F("%  "));

            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.write(F("  PWM2 "));
            write(motorPWM2.getPWM() * 100.0 / 255.0, F("%  "));
#endif

            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.write(F(" Therm "));
            write(thermistor.getTemperature(false), F(" C  "));

            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
            tft.write(F("Counts "));
            tft.write(counter1.getCount());
            tft.write(' ');
            tft.write(counter2.getCount());
            tft.write(' ');
            tft.write(counter3.getCount());
            tft.write(' ');
            tft.write(millis() / 5000);
            tft.write(' ', 2);

//            tft.gotoCharXY(col, line++);                                 // startPosition text cursor
//            tft.write(F("---------------"));
        }

        iter++;

#ifdef SHOW_TIMING
        lastTime = micros() - start;
#endif
        resume(1000);
    }

    defineSchedulerTaskId("Updater");

public:
    Updater() {
        iter = 0;
        lastTime = 0;
    }

} mainScreen = Updater();

Task *const taskTable[] PROGMEM = {
        &mainScreen,
        &counter1,
        &counter2,
        &counter3,
        &tempHumidity,
        &thermistor,
        &ledFlasher,
#ifdef PWM
        &motorPWM1,
        &motorPWM2,
#endif
};

uint16_t delayTable[sizeof(taskTable) / sizeof(*taskTable)];
Scheduler scheduler = Scheduler(sizeof(taskTable) / sizeof(*taskTable), reinterpret_cast<PGM_P>(taskTable), delayTable);

void setup() {
//    Serial.before(57600);
    Serial.begin(256000);

    DDRB = PORTB_OUT; // 0010.1110; set B1, B2-B3, B5 as outputs
    DDRC = 0x01; // 0000.0001; set PORTC as inputs, B0 as output
    DDRD = 0xF0; // 0111.0000; set PORTD 4,5,6,7 as output
    PORTC = 0;
    PORTD = 0;

    cli();

    // set compare match register to 0, no output
    OCR0A = 0;
    OCR0B = 0;

    // set fast PWM mode 3 so as not to affect 1ms timer, gives 488Hz PWM cycle
    setBit(TCCR0A, WGM00);
    setBit(TCCR0A, WGM01);

    // reset OCR0A and OCR0B on match, set at bottom, non-inverting mode
    clearBit(TCCR0A, COM0A0);
    setBit(TCCR0A, COM0A1);
    clearBit(TCCR0A, COM0B0);
    setBit(TCCR0A, COM0B1);

    sei();

//    setBit(SSD1306_RST_PORT, SSD1306_RST_BIT); // start with TFT reset line inactive high
//    setBit(SSD1306_CS_PORT, SSD1306_CS_BIT);  // deselect TFT CS
//    clearBit(SSD1306_SCK_PORT, SSD1306_SCK_BIT);  // TFT SCK Low
    setBit(ST7735_RST_PORT, ST7735_RST_BIT); // start with TFT reset line inactive high
    setBit(ST7735_CS_PORT, ST7735_CS_BIT);  // deselect TFT CS
    clearBit(ST7735_SCK_PORT, ST7735_SCK_BIT);  // TFT SCK Low

    Serial.println(F("Setup"));
    tft.openSPI();                              // start communication to TFT
    Serial.println(F("Started SPI"));
/*
    Serial.print(F("sizeof(St7735) "));
    Serial.println(sizeof(St7735));
    Serial.print(F("sizeof(Dht22) "));
    Serial.println(sizeof(Dht22));
    Serial.print(F("sizeof(Task) "));
    Serial.println(sizeof(Task));
    Serial.print(F("sizeof(Updater) "));
    Serial.println(sizeof(Updater));
    Serial.print(F("sizeof(Counter1) "));
    Serial.println(sizeof(Counter1));
    Serial.print(F("sizeof(Counter2) "));
    Serial.println(sizeof(Counter2));
    Serial.print(F("sizeof(Counter3) "));
    Serial.println(sizeof(Counter3));
    Serial.print(F("sizeof(TempHumidity) "));
    Serial.println(sizeof(TempHumidity));
    Serial.print(F("sizeof(LedFlasher) "));
    Serial.println(sizeof(LedFlasher));
    Serial.print(F("sizeof(MotorPWM) "));
    Serial.println(sizeof(MotorPWM));
    Serial.print(F("sizeof(Scheduler) "));
    Serial.println(sizeof(Scheduler));
*/

    Serial.print(F("OCR0A "));
    Serial.println(&OCR0A - (volatile unsigned char *) 0);

    scheduler.begin();

//    tft.initDisplay(ST7735_ROT_0);                             // initialize TFT controller
    tft.initDisplay(ST7735_ROT_90);                              // initialize TFT controller
    tft.foreground = YELLOW;
    Serial.println(F("Initialized display"));

    tft.clearScreen();
}

void loop() {
    scheduler.loop(10);
}
