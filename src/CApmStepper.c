#ifdef INCLUDE_STP_MODULE

#include <Arduino.h>
#include "CApmStepper.h"
#include "common_defs.h"

#ifdef INCLUDE_DAC_MODULE

#include "CDac53401.h"

#endif

void capm_init(CApmStepper_t *thizz, uint8_t pinA1, uint8_t pinA2, uint8_t pinB1, uint8_t pinB2) {
    thizz->flags = 0;
    thizz->pinA1 = pinA1;
    thizz->pinA2 = pinA2;
    thizz->pinB1 = pinB1;
    thizz->pinB2 = pinB2;
}

/*
 *      A1 A2 B1 B2
 * Step C0 C1 C2 C3
 *    1  1  0  1  0
 *    2  0  1  1  0
 *    3  0  1  0  1
 *    4  1  0  0  1
 */
const uint8_t capm_stepper_phases[] PROGMEM = {
        APM_OUT_MOT_A1 | APM_OUT_MOT_B1,
        APM_OUT_MOT_A2 | APM_OUT_MOT_B1,
        APM_OUT_MOT_A2 | APM_OUT_MOT_B2,
        APM_OUT_MOT_A1 | APM_OUT_MOT_B2,
};

void capm_step(CApmStepper_t *thizz, uint8_t ccwDir) {
    uint8_t phase = APM_FLAGS_TO_STEPPER_PHASE(thizz->flags);
    if (ccwDir) {
        phase--;
    } else {
        phase++;
    }
    phase &= APM_STEPPER_PHASE_MASK;
    thizz->flags &= ~APM_FLAGS_STEPPER_PHASE;
    thizz->flags |= APM_STEPPER_PHASE_TO_FLAGS(phase);

    uint8_t motOut = pgm_read_byte(capm_stepper_phases + phase);

    digitalWrite(thizz->pinA1, motOut & APM_OUT_MOT_A1);
    digitalWrite(thizz->pinA2, motOut & APM_OUT_MOT_A2);
    digitalWrite(thizz->pinB1, motOut & APM_OUT_MOT_B1);
    digitalWrite(thizz->pinB2, motOut & APM_OUT_MOT_B2);
}

void capm_step_cw(CApmStepper_t *thizz) {
    capm_step(thizz, 0);
}

void capm_step_ccw(CApmStepper_t *thizz) {
    capm_step(thizz, 1);
}

// micros per step for 28BYJ-48 motor, 32 steps/rev with 1:64 reduction, resulting in output of 2048 steps/rev
const time_t rpmMicros[] PROGMEM = {
        APM_STEPPER_MICROS_RPM(32, 1, 1),
        APM_STEPPER_MICROS_RPM(32, 2, 1),
        APM_STEPPER_MICROS_RPM(32, 3, 1),
        APM_STEPPER_MICROS_RPM(32, 4, 1),
        APM_STEPPER_MICROS_RPM(32, 5, 1),
        APM_STEPPER_MICROS_RPM(32, 6, 1),
        APM_STEPPER_MICROS_RPM(32, 7, 1),
        APM_STEPPER_MICROS_RPM(32, 8, 1),
        APM_STEPPER_MICROS_RPM(32, 9, 1),
        APM_STEPPER_MICROS_RPM(32, 10, 1),
        APM_STEPPER_MICROS_RPM(32, 11, 1),
        APM_STEPPER_MICROS_RPM(32, 12, 1),
        APM_STEPPER_MICROS_RPM(32, 13, 1),
        APM_STEPPER_MICROS_RPM(32, 14, 1),
        APM_STEPPER_MICROS_RPM(32, 15, 1),
        APM_STEPPER_MICROS_RPM(32, 16, 1),
        APM_STEPPER_MICROS_RPM(32, 17, 1),
        APM_STEPPER_MICROS_RPM(32, 18, 1),
        APM_STEPPER_MICROS_RPM(32, 19, 1),
        APM_STEPPER_MICROS_RPM(32, 20, 1),
};

time_t raw_rpm_to_step_micros(uint8_t reduction, uint8_t rpm) {
    if (rpm > lengthof(rpmMicros)) {
        rpm = lengthof(rpmMicros);
    } else if (!rpm) {
        rpm = 1;
    }

    time_t stepMicros = pgm_read_dword(rpmMicros + rpm - 1);

    if (reduction > 1) {
        stepMicros /= reduction;
    }
    return stepMicros;
}

uint16_t raw_step_micros_to_rpmX10(uint8_t reduction, uint32_t stepMicros) {
    uint16_t rpmX10 = APM_STEPPER_MICROS_RPM((uint16_t) reduction << 5, stepMicros, 10);
    return rpmX10;
}

time_t capm_rpm_to_step_micros(uint8_t reduction, uint8_t rpm) {
    return raw_rpm_to_step_micros(reduction, rpm) - 100;
}

uint16_t capm_step_micros_to_rpmX10(uint8_t reduction, uint32_t stepMicros) {
    return raw_step_micros_to_rpmX10(reduction, stepMicros + 100);
}

#ifdef INCLUDE_DAC_MODULE
// micros per step for 28BYJ-48 motor, 32 steps/rev with 1:64 reduction, resulting in output of 2048 steps/rev
//4 rpm - 4.5v, 9 rpm - 5v, 13 rmp 5.5 v, 15 rmp 6 v, 16 rmp 6.5 v, 17 rmp 7 v, 18 rmp 7.3 v, 20 rmp 7.5 v
#define VM_ADD (0)
const uint16_t rpmVDac[] PROGMEM = {
        // minimalist
        VM_TO_DAC_DATA(4.0 + VM_ADD), // 1
        VM_TO_DAC_DATA(4.0 + VM_ADD), // 2
        VM_TO_DAC_DATA(4.0 + VM_ADD), // 3
        VM_TO_DAC_DATA(4.0 + VM_ADD), // 4
        VM_TO_DAC_DATA(4.0 + VM_ADD), // 5
        VM_TO_DAC_DATA(4.4 + VM_ADD), // 6
        VM_TO_DAC_DATA(4.5 + VM_ADD), // 7
        VM_TO_DAC_DATA(4.6 + VM_ADD), // 8
        VM_TO_DAC_DATA(4.7 + VM_ADD), // 9
        VM_TO_DAC_DATA(4.8 + VM_ADD), // 10
        VM_TO_DAC_DATA(5.0 + VM_ADD), // 11
        VM_TO_DAC_DATA(5.1 + VM_ADD), // 12
        VM_TO_DAC_DATA(5.5 + VM_ADD), // 13
        VM_TO_DAC_DATA(5.5 + VM_ADD), // 14
        VM_TO_DAC_DATA(5.8 + VM_ADD), // 15
        VM_TO_DAC_DATA(6.2 + VM_ADD), // 16
        VM_TO_DAC_DATA(6.8 + VM_ADD), // 17
        VM_TO_DAC_DATA(7.2 + VM_ADD), // 18
        VM_TO_DAC_DATA(7.3 + VM_ADD), // 19
        VM_TO_DAC_DATA(7.6 + VM_ADD), // 20
};

uint16_t capm_rpm_to_vdac(uint8_t rpm) {
    if (rpm > lengthof(rpmVDac)) {
        rpm = lengthof(rpmVDac);
    } else if (!rpm) {
        rpm = 1;
    }

    uint16_t vDac = pgm_read_word(rpmVDac + rpm - 1);
    return vDac;
}

#endif //INCLUDE_DAC_MODULE

#endif // INCLUDE_STP_MODULE
