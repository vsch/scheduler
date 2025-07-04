#include "Arduino.h"
#include "CDac53401.h"
#include "CTwiController.h"
#include "twiint.h"

void dac_init(uint8_t addr, uint8_t flags) {
    static const DacWriteEntry_t PROGMEM init1[] = {
            {REG_TRIGGER,        WR_TRIGGER_DEVICE_CONFIG_RESET(1)},
            {REG_GENERAL_CONFIG, WR_DAC_POWER(DAC_POWER_UP) | WR_GENERAL_CONFIG_REF_EN(DAC_VREF_VDD) /*| WR_GENERAL_CONFIG_DAC_SPAN(DAC_VREF_GAIN_1_5X)*/ },
            {REG_DATA,           WR_DATA_DAC(DATA_DAC_MAX)}, // output max so VM voltage is min
    };

    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    dac_send_byte_list(init1, sizeof(init1));
    CByteStream_t *pStream = twi_process_stream();
    twi_wait_sent(pStream, DAC_WAIT_TIMEOUT);
}

void dac_send_byte_list(const uint8_t *bytes, uint16_t count) {
    while (count >= 3) {
        uint8_t reg = pgm_read_byte(bytes++);
        uint16_t val = pgm_read_word(bytes++);
        if (!dac_write_wait(DAC53401_ADDRESS, reg, val)) {
#ifdef SERIAL_DEBUG_TWI_TRACER
            twi_dump_trace(1);
#endif
            break;
        }
        bytes++;
        count -= 3;
    }
}

CByteStream_t *dac_write(uint8_t addr, uint8_t reg, uint16_t value) {
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
    stream_put(twiStream, (value & 0xff00) >> 8);
    stream_put(twiStream, (value & 0x00ff));
    return twi_process_stream();
}

uint8_t dac_write_wait(uint8_t addr, uint8_t reg, uint16_t value) {
    CByteStream_t *pStream = dac_write(addr, reg, value);
    if (twi_wait_sent(pStream, DAC_WAIT_TIMEOUT)) {
        return 1;
    }
#ifdef SERIAL_DEBUG_TWI_TRACER
    twi_dump_trace(1);
#endif
    return 0;
}

uint8_t dac_write_read_wait(uint8_t addr, uint8_t reg, uint16_t value, uint16_t *pValue) {
    CByteBuffer_t rdBuffer;

    // CAVEAT: reverse flag is needed if the CPU is little endian, while the dac is bigendian
    buffer_init(&rdBuffer, BUFFER_PUT_REVERSE, pValue, sizeof(*pValue));

    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
    stream_put(twiStream, (value & 0xff00) >> 8);
    stream_put(twiStream, (value & 0x00ff));

    CByteStream_t *pStream = twi_process_rcv(twiStream, &rdBuffer);
    uint8_t retVal = twi_wait_sent(pStream, DAC_WAIT_TIMEOUT);
    if (!retVal) {
#ifdef SERIAL_DEBUG_TWI_TRACER
        twi_dump_trace(1);
#endif
    }
    return retVal;
}

uint8_t dac_read_wait(uint8_t addr, uint8_t reg, uint16_t *pValue) {
    CByteBuffer_t rdBuffer;

    // CAVEAT: reverse flag is needed if the CPU is little endian, while the dac is bigendian
    buffer_init(&rdBuffer, BUFFER_PUT_REVERSE, pValue, sizeof(*pValue));

    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
    CByteStream_t *pStream = twi_process_rcv(twiStream, &rdBuffer);
    uint8_t retVal = twi_wait_sent(pStream, DAC_WAIT_TIMEOUT);
    if (!retVal) {
#ifdef SERIAL_DEBUG_TWI_TRACER
        twi_dump_trace(1);
#endif
    }
    return retVal;
}

void dac_power_up(uint8_t addr) {
}

void dac_power_down(uint8_t addr, uint8_t flags) {
}

void dac_output(uint8_t addr, uint16_t value) {
}
