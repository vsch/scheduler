#ifdef INCLUDE_IOX_MODULE

#include "Arduino.h"
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

CByteStream_t *iox_init(uint8_t addr, uint16_t rw_config, uint16_t data){
    // set the output value
    iox_send_word(addr, IOX_REG_CONFIGURATION_PORT0, rw_config);
    return iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
}

CByteStream_t *iox_out_byte(uint8_t addr, uint8_t data){
    return iox_send_byte(addr, IOX_REG_OUTPUT_PORT0, data);
}

uint8_t iox_out_byte_wait(uint8_t addr, uint8_t data){
    CByteStream_t *pStream = iox_send_byte(addr, IOX_REG_OUTPUT_PORT0, data);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

CByteStream_t *iox_in_byte(uint8_t addr, uint8_t *pData) {
    CByteBuffer_t rdBuffer;
    buffer_init(&rdBuffer, 0, pData, sizeof(*pData));

    iox_prep_write(addr, IOX_REG_INPUT_PORT0);
    return twi_process_rcv(twiStream, &rdBuffer);
}

uint8_t iox_in_byte_wait(uint8_t addr, uint8_t *pData) {
    CByteStream_t *pStream = iox_in_byte(addr, pData);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

CByteStream_t *iox_out_word(uint8_t addr, uint16_t data){
    return iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
}

uint8_t iox_out_word_wait(uint8_t addr, uint16_t data){
    CByteStream_t *pStream = iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

CByteStream_t *iox_in_word(uint8_t addr, uint16_t *pData) {
    CByteBuffer_t rdBuffer;
    buffer_init(&rdBuffer, 0, pData, sizeof(*pData));

    iox_prep_write(addr, IOX_REG_INPUT_PORT0);
    return twi_process_rcv(twiStream, &rdBuffer);
}

uint8_t iox_in_word_wait(uint8_t addr, uint16_t *pData) {
    CByteStream_t *pStream = iox_in_word(addr, pData);
    return twi_wait_sent(pStream, TWI_WAIT_TIMEOUT);
}

#endif
