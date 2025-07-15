#ifdef INCLUDE_IOX_MODULE

#include "CIOExpander.h"
#include "CTwiController.h"
#include "twiint.h"

void iox_prep_write(uint8_t addr, uint8_t reg) {
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
}

CByteStream_t *iox_send_word(uint8_t addr, uint8_t reg, uint16_t data) {
    iox_prep_write(addr, reg);
    stream_put(twiStream, (data & 0x00ff));
    stream_put(twiStream, (data >> 8) & 0x00ff);
    return twi_process(twiStream);
}

CByteStream_t *iox_send_byte(uint8_t addr, uint8_t reg, uint8_t data) {
    iox_prep_write(addr, reg);
    stream_put(twiStream, data);
    return twi_process(twiStream);
}

CByteStream_t *iox_rcv_data(uint8_t addr, uint8_t reg, void *pData, uint8_t len) {
    CByteBuffer_t rdBuffer;
    buffer_init(&rdBuffer, 0, pData, len);

    iox_prep_write(addr, reg);
    return twi_process_rcv(twiStream, &rdBuffer);
}

uint8_t iox_rcv_data_wait(uint8_t addr, uint8_t reg, void *pData, uint8_t len) {
    CByteStream_t *pStream = iox_rcv_data(addr, reg, pData, len);
    return twi_wait_sent(pStream);
}

CByteStream_t *iox_rcv_byte(uint8_t addr, uint8_t reg, uint8_t *pData) {
    return iox_rcv_data(addr, reg, pData, 1);
}

uint8_t iox_rcv_byte_wait(uint8_t addr, uint8_t reg, uint8_t *pData) {
    return iox_rcv_data_wait(addr, reg, pData, 1);
}

CByteStream_t *iox_rcv_word(uint8_t addr, uint8_t reg, uint16_t *pData) {
    return iox_rcv_data(addr, reg, pData, 2);
}

uint8_t iox_rcv_word_wait(uint8_t addr, uint8_t reg, uint16_t *pData) {
    return iox_rcv_data_wait(addr, reg, pData, 2);
}

// IOX Tilt Tower Module
CByteStream_t *ciox_init(CIOExpander_t *thizz, uint8_t addressVar, uint8_t extraOutputs) {
    thizz->flags = (addressVar & IOX_FLAGS_ADDRESS) | IOX_FLAGS_STEPPER_PHASE;
    thizz->outputs = 0x00;
    thizz->inputs = 0xff;
    thizz->lastInputs = 0xff;
    return iox_init(IOX_I2C_ADDRESS(thizz->flags & IOX_FLAGS_ADDRESS), TILT_CONFIGURATION, extraOutputs | TILT_INVERT_OUT);
}

/*
 *      A1 A2 B1 B2
 * Step C0 C1 C2 C3
 *    1  1  0  1  0
 *    2  0  1  1  0
 *    3  0  1  0  1
 *    4  1  0  0  1
 */
const uint8_t phases[] PROGMEM = {
        IOX_OUT_MOT_A1 | IOX_OUT_MOT_B1, // 1  0  1  0        
        IOX_OUT_MOT_A2 | IOX_OUT_MOT_B1, // 0  1  1  0        
        IOX_OUT_MOT_A2 | IOX_OUT_MOT_B2, // 0  1  0  1        
        IOX_OUT_MOT_A1 | IOX_OUT_MOT_B2, // 1  0  0  1        
};

void ciox_update(CIOExpander_t *thizz) {
    // update if motor is off, assuming no stepping
    if (!(thizz->outputs & IOX_OUT_MOT_EN)) {
        // turn it off rightaway since there may not be any stepping for a while.
        iox_send_byte(IOX_I2C_ADDRESS(thizz->flags & IOX_FLAGS_ADDRESS), IOX_REG_OUTPUT_PORT0, thizz->outputs);
    }
}

void ciox_stepper_power(CIOExpander_t *thizz, uint8_t enable) {
    if (enable) enable = IOX_OUT_MOT_EN;
    if ((thizz->outputs & IOX_OUT_MOT_EN) != enable) {
        thizz->outputs ^= IOX_OUT_MOT_EN;
        ciox_update(thizz);
    }
}

void ciox_led_color(CIOExpander_t *thizz, uint8_t ledColor) {
    ledColor = (ledColor) ? (ledColor & IOX_OUT_LED) ? (ledColor & IOX_OUT_LED) : TILT_LED_COLOR_WHITE : TILT_LED_COLOR_BLACK;
    ledColor ^= TILT_INVERT_OUT;

    if ((thizz->outputs & IOX_OUT_LED) != ledColor) {
        thizz->outputs &= ~IOX_OUT_LED;
        thizz->outputs |= ledColor;
        ciox_update(thizz);
    }
}

CByteStream_t *ciox_step(CIOExpander_t *thizz, uint8_t ccw) {
    uint8_t phase = IOX_FLAGS_TO_STEPPER_PHASE(thizz->flags);
    if (ccw) {
        phase--;
    } else {
        phase++;
    }
    phase &= IOX_STEPPER_PHASE_MASK;
    uint8_t motOut = pgm_read_byte(phases);
    thizz->flags &= IOX_FLAGS_STEPPER_PHASE;

    // enable motor output by default
    thizz->flags |= IOX_STEPPER_PHASE_TO_FLAGS(motOut) | IOX_OUT_MOT_EN;
    return iox_send_byte(IOX_I2C_ADDRESS(thizz->flags & IOX_FLAGS_ADDRESS), IOX_REG_OUTPUT_PORT0, thizz->outputs);
}

CByteStream_t *ciox_step_cw(CIOExpander_t *thizz) {
    return ciox_step(thizz, 0);
}

CByteStream_t *ciox_step_ccw(CIOExpander_t *thizz) {
    return ciox_step(thizz, 1);
}

CByteStream_t *ciox_in(CIOExpander_t *thizz) {
    thizz->lastInputs = thizz->inputs;
    CByteStream_t *pStream = iox_rcv_byte(IOX_I2C_ADDRESS(thizz->flags & IOX_FLAGS_ADDRESS), IOX_REG_INPUT_PORT1, &thizz->inputs);
    twi_wait_sent(pStream);
    return pStream;
}

CByteStream_t *iox_init(uint8_t addr, uint16_t rw_config, uint16_t data) {
    // set the output value
    iox_send_word(addr, IOX_REG_CONFIGURATION_PORT0, rw_config);
    return iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
}

CByteStream_t *iox_out(uint8_t addr, uint16_t data) {
    return iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
}

uint8_t iox_out_wait(uint8_t addr, uint16_t data) {
    CByteStream_t *pStream = iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
    return twi_wait_sent(pStream);
}

CByteStream_t *iox_in(uint8_t addr, uint16_t *pData) {
    return iox_rcv_word(addr, IOX_REG_INPUT_PORT0, pData);
}

uint8_t iox_in_wait(uint8_t addr, uint16_t *pData) {
    return iox_rcv_word_wait(addr, IOX_REG_INPUT_PORT0, pData);
}

#endif
