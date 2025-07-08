
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
extern CByteStream_t * iox_send_out(uint8_t addr, uint8_t reg, uint16_t data);   
extern CByteStream_t *iox_out(uint8_t addr, uint16_t data);   
extern uint8_t iox_out_wait(uint8_t addr, uint16_t data);
extern CByteStream_t *iox_in(uint8_t addr, uint16_t *pData);
extern uint8_t iox_in_wait(uint8_t addr, uint16_t *pData);

#ifdef __cplusplus
};
#endif


#endif //ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_H
