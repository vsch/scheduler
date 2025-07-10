
#ifndef ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_H
#define ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_H

#include <stddef.h>     //size_t type, NULL pointer
#include <stdint.h>     //uint8_t type
#include "CByteStream.h"
#include "CIOExpander_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CByteStream_t *iox_init(uint8_t addr, uint16_t rw_config, uint16_t data);
extern CByteStream_t *iox_send_word(uint8_t addr, uint8_t reg, uint16_t data);
extern CByteStream_t *iox_send_byte(uint8_t addr, uint8_t reg, uint8_t data);
extern CByteStream_t *iox_out_byte(uint8_t addr, uint8_t data);
extern uint8_t iox_out_byte_wait(uint8_t addr, uint8_t data);
extern CByteStream_t *iox_in_byte(uint8_t addr, uint8_t *pData);
extern uint8_t iox_in_byte_wait(uint8_t addr, uint8_t *pData);
extern CByteStream_t *iox_out_word(uint8_t addr, uint16_t data);
extern uint8_t iox_out_word_wait(uint8_t addr, uint16_t data);
extern CByteStream_t *iox_in_word(uint8_t addr, uint16_t *pData);
extern uint8_t iox_in_word_wait(uint8_t addr, uint16_t *pData);

#ifdef __cplusplus
};
#endif

#endif //ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_H
