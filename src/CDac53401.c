#ifdef INCLUDE_DAC_MODULE

#include "Arduino.h"
#include "CDac53401.h"
#include "CTwiController.h"
#include "twiint.h"

void dac_init() {
    static const DacWriteEntry_t PROGMEM init1[] = {
            {REG_TRIGGER,        WR_TRIGGER_DEVICE_CONFIG_RESET(1)},
            {REG_GENERAL_CONFIG, WR_DAC_POWER(DAC_POWER_UP) | WR_GENERAL_CONFIG_REF_EN(DAC_VREF_VDD) /*| WR_GENERAL_CONFIG_DAC_SPAN(DAC_VREF_GAIN_1_5X)*/ },
            {REG_DATA,           WR_DATA_DAC(DATA_DAC_MAX)}, // output max so VM voltage is min
    };

    CByteStream_t *pStream = dac_send_byte_list(init1, sizeof(init1));
    twi_wait_sent(pStream);
}

CByteStream_t *dac_send_byte_list(const uint8_t *bytes, uint16_t count) {
    CByteStream_t *pStream = NULL;

    while (count >= 3) {
        uint8_t reg = pgm_read_byte(bytes++);
        uint16_t val = pgm_read_word(bytes++);
        pStream = dac_write(DAC53401_ADDRESS, reg, val);
        bytes++;
        count -= 3;
    }
    return pStream;
}

CByteStream_t *dac_write(uint8_t addr, uint8_t reg, uint16_t value) {
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
    stream_put(twiStream, (value & 0xff00) >> 8);
    stream_put(twiStream, (value & 0x00ff));
    return twi_process_stream();
}

CByteStream_t *dac_write_read(uint8_t addr, uint8_t reg, uint16_t value, uint16_t *pValue) {
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
    stream_put(twiStream, (value & 0xff00) >> 8);
    stream_put(twiStream, (value & 0x00ff));

    // CAVEAT: reverse flag is needed if the CPU is little endian, while the dac is bigendian
    twi_set_rd_buffer(1, pValue, sizeof(*pValue));

    return twi_process_stream();
}

CByteStream_t *dac_read(uint8_t addr, uint8_t reg, uint16_t *pValue) {
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);

    // CAVEAT: reverse flag is needed if the CPU is little endian, while the dac is bigendian
    twi_set_rd_buffer(1, pValue, sizeof(*pValue));

    return twi_process_stream();
}

CByteStream_t *dac_power_up(uint8_t addr) {
}

CByteStream_t *dac_power_down(uint8_t addr, uint8_t flags) {
}

CByteStream_t *dac_output(uint8_t addr, uint16_t value) {
    return dac_write(addr, REG_DATA, WR_DATA_DAC(value));
}

#endif
