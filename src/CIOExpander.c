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

CByteStream_t *iox_init(uint8_t addr, uint16_t rw_config, uint16_t data){
    // set the output value
    iox_send_word(addr, IOX_REG_CONFIGURATION_PORT0, rw_config);
    return iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
}

CByteStream_t *iox_out(uint8_t addr, uint16_t data){
    return iox_send_word(addr, IOX_REG_OUTPUT_PORT0, data);
}

uint8_t iox_out_wait(uint8_t addr, uint16_t data){
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
