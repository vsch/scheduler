
#ifndef ARDUINOPROJECTMODULE_CAPMSTEPPER_H
#define ARDUINOPROJECTMODULE_CAPMSTEPPER_H

#include "common_defs.h"

#define APM_FLAGS_STEPPER_PHASE         (0x03)

#define APM_STEPPER_PHASE_TO_FLAGS(p)   ((p) & APM_FLAGS_STEPPER_PHASE)
#define APM_FLAGS_TO_STEPPER_PHASE(f)   (((f) & APM_FLAGS_STEPPER_PHASE))

#define APM_STEPPER_PHASE_MASK          (0x03)

#define APM_STEPPER_MICROS_RPM(strev, rpm, scale)    ((60000000UL * (scale) / (strev) / (rpm)))

#define APM_OUT_MOT_A1  (0x01)
#define APM_OUT_MOT_B1  (0x02)
#define APM_OUT_MOT_A2  (0x04)
#define APM_OUT_MOT_B2  (0x08)

typedef struct CApmStepper {
    uint8_t flags;
    uint8_t pinA1;
    uint8_t pinA2;
    uint8_t pinB1;
    uint8_t pinB2;
} CApmStepper_t;

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t capm_stepper_phases[];
extern void capm_init(CApmStepper_t *thizz, uint8_t pinA1, uint8_t pinA2, uint8_t pinB1, uint8_t pinB2);

extern void capm_step(CApmStepper_t *thizz, uint8_t ccwDir);
extern void capm_step_cw(CApmStepper_t *thizz);
extern void capm_step_ccw(CApmStepper_t *thizz);

extern time_t raw_rpm_to_step_micros(uint8_t reduction, uint8_t rpm);
extern uint16_t raw_step_micros_to_rpmX10(uint8_t reduction, uint32_t stepMicros);

extern time_t capm_rpm_to_step_micros(uint8_t reduction, uint8_t rpm);
extern uint16_t capm_step_micros_to_rpmX10(uint8_t reduction, uint32_t stepMicros);

#ifdef INCLUDE_DAC_MODULE
extern uint16_t capm_rpm_to_vdac(uint8_t rpm, int16_t vDacDelta);
#endif

#ifdef __cplusplus
};
#endif

#endif //ARDUINOPROJECTMODULE_CAPMSTEPPER_H
