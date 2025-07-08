#ifdef INCLUDE_IOX_MODULE

#include "Arduino.h"
#include "CIOExpander.h"
#include "CTwiController.h"
#include "twiint.h"

void iox_prep_write(uint8_t addr, uint8_t reg) {
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(addr));
    stream_put(twiStream, reg);
}

CByteStream_t *iox_send_out(uint8_t addr, uint8_t reg, uint16_t data) {
    iox_prep_write(addr, reg);
    stream_put(twiStream, (data >> 8) & 0x00ff);
    stream_put(twiStream, (data & 0x00ff));
    return twi_process(twiStream);
}

uint8_t iox_write_out_wait(uint8_t addr, uint8_t reg, uint16_t data) {
    CByteStream_t *pStream = iox_send_out(addr, reg, data);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

CByteStream_t *iox_init(uint8_t addr, uint16_t rw_config, uint16_t data){
    // set the output value
    iox_send_out(addr, IOX_REG_OUTPUT_PORT0, data);
    return iox_send_out(addr, IOX_REG_CONFIGURATION_PORT0, rw_config);
}

CByteStream_t *iox_out(uint8_t addr, uint16_t data){
    return iox_send_out(addr, IOX_REG_OUTPUT_PORT0, data);
}

uint8_t iox_out_wait(uint8_t addr, uint16_t data){
    CByteStream_t *pStream = iox_send_out(addr, IOX_REG_OUTPUT_PORT0, data);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

CByteStream_t *iox_in(uint8_t addr, uint16_t *pData) {
    CByteBuffer_t rdBuffer;
    buffer_init(&rdBuffer, 0, pData, sizeof(*pData));

    iox_prep_write(addr, IOX_REG_INPUT_PORT0);
    return twi_process_rcv(twiStream, &rdBuffer);
}

uint8_t iox_in_wait(uint8_t addr, uint16_t *pData) {
    CByteStream_t *pStream = iox_in(addr, pData);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

#endif
