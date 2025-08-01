
#ifndef ARDUINOPROJECTMODULE_CIOEXPANDER_H
#define ARDUINOPROJECTMODULE_CIOEXPANDER_H

#include <stddef.h>     //size_t type, NULL pointer
#include <stdint.h>     //uint8_t type
#include "CByteStream.h"
#include "CIOExpander_cmd.h"
#include "tilt_tower_config.h"

#define IOX_OUT_MOT_B1          TILT_MOT_B1
#define IOX_OUT_MOT_A1          TILT_MOT_A1
#define IOX_OUT_MOT_B2          TILT_MOT_B2
#define IOX_OUT_MOT_A2          TILT_MOT_A2
#define IOX_OUT_MOT_EN          TILT_MOT_EN
#define IOX_OUT_LED_RED         TILT_LED_RED
#define IOX_OUT_LED_GREEN       TILT_LED_GREEN
#define IOX_OUT_LED_BLUE        TILT_LED_BLUE

#define IOX_OUT_LED             (TILT_LED_COLOR_WHITE)
#define IOX_OUT_MOT_PHASES      (IOX_OUT_MOT_B1 | IOX_OUT_MOT_A1 | IOX_OUT_MOT_B2 | IOX_OUT_MOT_A2)

#define IOX_IN_BTN_HIGH_LIMIT   (TILT_BTN_HIGH_LIMIT >> 8)
#define IOX_IN_BTN_LOW_LIMIT    (TILT_BTN_LOW_LIMIT >> 8)
#define IOX_IN_FEED_MOTOR_IN    (TILT_FEED_MOTOR_IN >> 8)
#define IOX_IN_FEED_VALVE_IN    (TILT_FEED_VALVE_IN >> 8)

#define IOX_I2C_ADDRESS(v)      (XL9535_BASE_ADDRESS | ((v) & 0x07))

#define IOX_FLAGS_ADDRESS       (0x07)
#define IOX_FLAGS_VALID_INPUTS  (0x08)   // got input complete
#define IOX_FLAGS_STEPPER_PHASE (0x30)
#define IOX_FLAGS_STEPPING      (0x40)   // stepping is active
#define IOX_FLAGS_LATEST_INPUTS (0x80)   // latest input request is complete

#define IOX_STEPPER_PHASE_TO_FLAGS(p) (((p) << 4) & IOX_FLAGS_STEPPER_PHASE)
#define IOX_FLAGS_TO_STEPPER_PHASE(f) ((((f) & IOX_FLAGS_STEPPER_PHASE) >> 4))

#define IOX_STEPPER_PHASE_MASK (0x03)

struct CIOExpander;
typedef void (*CIoxCallback_t)(const struct CByteStream *pStream, struct CIOExpander *pIox);

typedef struct CIOExpander {
    uint8_t flags;
    uint8_t outputs;
    uint8_t inputs;
    uint8_t lastInputs;
    uint8_t lastOutputs;

    time_t stepStartTick;           // micros of start of step
    CIoxCallback_t fStepCallback;
} CIOExpander_t;


#ifdef __cplusplus
extern "C" {
#endif

extern CByteStream_t *ciox_init(CIOExpander_t *thizz, uint8_t addressVar, uint8_t extraOutputs);
extern void ciox_led_color(CIOExpander_t *thizz, uint8_t ledColor);

#ifdef INCLUDE_STP_MODULE
extern void ciox_stepper_power(CIOExpander_t *thizz, uint8_t enable);
extern CByteStream_t *ciox_step(CIOExpander_t *thizz, uint8_t ccwDir);
extern CByteStream_t *ciox_step_cw(CIOExpander_t *thizz);
extern CByteStream_t *ciox_step_ccw(CIOExpander_t *thizz);
extern CByteStream_t *ciox_in(CIOExpander_t *thizz);
extern time_t ciox_rpm_to_step_micros(uint8_t reduction, uint8_t rpm);
extern uint16_t ciox_step_micros_to_rpmX10(uint8_t reduction, uint32_t stepMicros);
#endif // INCLUDE_STP_MODULE

extern CByteStream_t *iox_init(uint8_t addr, uint16_t rw_config, uint16_t data);
extern CByteStream_t *iox_send_word(uint8_t addr, uint8_t reg, uint16_t data);
extern CByteStream_t *iox_send_byte(uint8_t addr, uint8_t reg, uint8_t data);

extern CByteStream_t *iox_rcv_data(uint8_t addr, uint8_t reg, void *pData, uint8_t len);
extern uint8_t iox_rcv_data_wait(uint8_t addr, uint8_t reg, void *pData, uint8_t len);
extern CByteStream_t *iox_rcv_byte(uint8_t addr, uint8_t reg, uint8_t *pData);
extern uint8_t iox_rcv_byte_wait(uint8_t addr, uint8_t reg, uint8_t *pData);
extern CByteStream_t *iox_rcv_word(uint8_t addr, uint8_t reg, uint16_t *pData);
extern uint8_t iox_rcv_word_wait(uint8_t addr, uint8_t reg, uint16_t *pData);

extern CByteStream_t *iox_out(uint8_t addr, uint16_t data);
extern uint8_t iox_out_wait(uint8_t addr, uint16_t data);
extern CByteStream_t *iox_in(uint8_t addr, uint16_t *pData);
extern uint8_t iox_in_wait(uint8_t addr, uint16_t *pData);


#ifdef __cplusplus
};
#endif

#endif //ARDUINOPROJECTMODULE_CIOEXPANDER_H
