#ifndef ARDUINOPROJECTMODULE_CDAC53401_H
#define ARDUINOPROJECTMODULE_CDAC53401_H

#include <stddef.h>     //size_t type, NULL pointer
#include <stdint.h>     //uint8_t type
#include "CByteStream.h"
#include "CDac53401_cmd.h"

// DAC computed values
#define VOUT_A      (-134.912959381045)
#define VOUT_B      (1616.85686653772)
#define DAC_DATA_MAX            (1024)
#define VM_TO_DAC_DATA_RAW(v)   ((int32_t)(VOUT_A * (double)(v) + VOUT_B + 0.5))
#define VM_TO_DAC_DATA(v)       (VM_TO_DAC_DATA_RAW(v) >= DAC_DATA_MAX ? DAC_DATA_MAX-1 : VM_TO_DAC_DATA_RAW(v) < 0 ? 0: (uint16_t)VM_TO_DAC_DATA_RAW(v))
#define DAC_DATA_TO_VM(d)       (((double)(d)-VOUT_B)/VOUT_A)
#define DAC_DATA_TO_VM_MV(d)    ((uint16_t)(DAC_DATA_TO_VM(d)*1000+.5))

#define DAC_DATA_TO_VM_MV_ONLY(d)   ((uint16_t)(DAC_DATA_TO_VM(d)*1000 +.5) % 1000)
#define DAC_DATA_TO_VM_V_ONLY(d)    ((uint16_t)(DAC_DATA_TO_VM(d)*1000 +.5) / 1000)

#define VM_MIN                  (DAC_DATA_TO_VM_MV(1023))
#define VM_MAZ                  (DAC_DATA_TO_VM_MV(0))
#define DAC_VREF                (1.21f)

#define DAC_DATA_TO_VDAC_MV(d,vr)      ((uint16_t)((vr)*(d)*1000/DAC_DATA_MAX))
#define DAC_DATA_TO_VDAC_VREFX4_MV(d)  (DAC_DATA_TO_VDAC_MV(d, DAC_VREF*4))

#define WR_DAC_POWER(v)          WR_GENERAL_CONFIG_DAC_PDN(v)
#define RD_DAC_POWER(v)          RD_GENERAL_CONFIG_DAC_PDN(v)

#define DAC_POWER_UP            (WR_DAC_POWER(DAC_PDN_UP))
#define DAC_POWER_DN_10K        (WR_DAC_POWER(DAC_PDN_10K))
#define DAC_POWER_DN_HI_Z       (WR_DAC_POWER(DAC_PDN_HI_Z))
#define DAC_POWER_DN_10K2       (WR_DAC_POWER(DAC_PDN_10K_2))
#define DAC_POWER(f)            (RD_DAC_POWER(f))

#define DAC_DATA_1V_DELTA       ((VM_TO_DAC_DATA(5) - VM_TO_DAC_DATA(10)) / 5)
#define DAC_CONSTRAIN_DATA(d)   ((d) < 0 ? 0 : (d) >= DAC_DATA_MAX ? DAC_DATA_MAX-1 : (d))

// define 3 bytes for entry: register, MSB value, LSZB value
// this one is if the bytes are sent as defined
// #define DAC_WRITE_ENTRY(r,v)          ((uint8_t)(r)), ((uint8_t)((r) & 0xff00)) >> 8, ((uint8_t)((r) & 0x00ff))
// this one is if the bytes are first read into uint16 then sent in big endian order
//#define DAC_WRITE_ENTRY(r,v)          ((uint8_t)(r)), ((uint8_t)((r) & 0x00ff)), ((uint8_t)((r) & 0xff00)) >> 8
typedef struct DacWriteEntry {
    uint8_t reg;
    uint16_t value;
} DacWriteEntry_t;

#ifdef __cplusplus
extern "C" {
#endif

// count must be multiple of 3, only full 3 bytes are sent, if last entry is short it is ignored
extern CByteStream_t *dac_send_byte_list(uint8_t addr, const uint8_t *bytes, uint16_t count);

extern CByteStream_t * dac_init(uint8_t addr);
extern CByteStream_t *dac_power_up(uint8_t addr);
extern CByteStream_t *dac_power_down(uint8_t addr, uint8_t flags);
extern CByteStream_t *dac_output(uint8_t addr, uint16_t value);
extern CByteStream_t *dac_write(uint8_t addr, uint8_t reg, uint16_t value);

extern CByteStream_t *dac_write_read(uint8_t addr, uint8_t reg, uint16_t value, uint16_t *pValue);
/**
 * Write a value to a register then read in the value from the register.
 *
 * @param addr
 * @param reg
 * @param value
 * @param pValue
 * @return 1 if value read, 0 if timed out (50ms) waiting for twi stream to process
 */
extern CByteStream_t *dac_read(uint8_t addr, uint8_t reg, uint16_t *pValue);

#ifdef __cplusplus
};
#endif

#endif //ARDUINOPROJECTMODULE_CDAC53401_H
