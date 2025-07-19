#ifndef ARDUINOPROJECTMODULE_STEPPERRPM_H
#define ARDUINOPROJECTMODULE_STEPPERRPM_H

#include <stdint.h>
#include <Arduino.h>
#include "CByteStream.h"

class StepperRPM {
    uint16_t stepsPerRevolution;
    uint8_t rpmX10;

    // speed ramping
    uint8_t startRPMx10;    // starting RPM
    uint8_t endRPMx10;      // ending RPM
    uint16_t rampMs;         // over how many MS to ramp rpm

    time_t rampStartMicros;

public:
    StepperRPM() {
        rpmX10 = 5;
        endRPMx10 = startRPMx10 = 0;
        rampMs = 0;
        rampStartMicros = 0;
        stepsPerRevolution = 2048;
    }

    /**
     * Get microseconds to next step from the end of the last step
     *
     * Delay = 600 / rpmX10 / stepsPerRev;
     *
     * rpmX10 = endRPMx10 * elapsedMs/rampMs + startRPMx10
     *
     * @return delay before the next step in microseconds
     */
    time_t NextStepDelay() {
    }

    uint8_t rampRPMx10(uint16_t rampStart) {
        if (rampStartMicros == 0 || rampMs == 0 || startRPMx10 == endRPMx10) {
            startRPMx10 = endRPMx10;
            return startRPMx10;
        }

        time_t elapsedMicros = micros() - rampStartMicros;
        if (elapsedMicros & 0x8000000) {
            // wrapped around
            elapsedMicros ^= 0xffffffff;
            elapsedMicros++;
        }

        if (!rampStart || !elapsedMicros) {
            // did not start ramping
            return startRPMx10;
        }

        rpmX10 = (((uint32_t) endRPMx10 * elapsedMicros) + (rampMs + 1) * 500) / rampMs / 1000l + startRPMx10;
        if (rpmX10 > endRPMx10 && endRPMx10 > startRPMx10) {
            rpmX10 = endRPMx10;
            startRPMx10 = endRPMx10;
            rampStart = 0;
        } else if (rpmX10 < endRPMx10 && endRPMx10 < startRPMx10) {
            rpmX10 = endRPMx10;
            startRPMx10 = endRPMx10;
            rampStart = 0;
        }
        return rpmX10;
    }

    inline void setRPMx10(uint8_t rpmX10, uint16_t rampMillis = 0) {
        rampMs = rampMillis;
        if (rampMillis) {
            startRPMx10 = this->rpmX10;
            endRPMx10 = rpmX10;
        } else {
            this->rpmX10 = rpmX10;
            rampStartMicros = 0;
        }
    }

};

#endif //ARDUINOPROJECTMODULE_STEPPERRPM_H
