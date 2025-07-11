/*
 * twiint.h
 *
 * Buffered, interrupt based TWI driver.
 *
 * Author:      Sebastian Goessl
 * Hardware:    ATmega328P
 *
 * LICENSE:
 * MIT License
 *
 * Copyright (c) 2018 Sebastian Goessl
 * 
 * Modified for CByteStream and TwiController handling
 * Author:     Vladimir Schneider
 * 
 * Copyright (c) 2025 Vladimir Schneider
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#ifndef TWIINT_H_
#define TWIINT_H_

#include "Arduino.h"
#include <stdbool.h>    //bool type
#include <stddef.h>     //size_t type
#include <stdint.h>     //uint8_t type
#include "CByteStream.h"

//default to Fast Mode
#ifndef TWI_FREQUENCY
#define TWI_FREQUENCY 400000
#endif

/** Slave address with write intend. */
#define TWI_ADDRESS_W(x)    (((x) << 1) & ~0x01)
/** Slave address with read intend. */
#define TWI_ADDRESS_R(x)    (((x) << 1) | 0x01)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Errors during interrupt processing will increment twiint_errors
 */
extern uint16_t twiint_errors;
extern uint8_t twiint_flags;

/**
 * Initializes the TWI hardware for master mode operating at TWI_FREQUENCY.
 * Configures SCL and SDA (PC5 and PC4) as outputs.
 * Interrupts have to be enabled.
 */
void twiint_init(void);

/**
 * Returns true if currently a transmission is ongoing.
 *
 * @return if currently a transmission is ongoing
 */
bool twiint_busy(void);

/**
 * Blocks until the transmission is completed.
 */
void twiint_flush(void);

/**
 * cancel read operation (if the current stream equals given stream
 * 
 * NOTE: will disable then enable interrupts
 * 
 * @param pStream 
 */
void twint_cancel_rd(CByteStream_t *pStream);



/**
 * Starts a TWI transmission writing or reading multiple bytes.
 * The address byte should be provided already manipulated by
 * TWI_ADDRESS_W if data should be written
 * or TWI_ADDRESS_R if data should be read,
 * both slave address in 7-bit format.
 * This function then returns while the transmission continues,
 * getting handled by interrupts routines.
 * The data location must not be changed while the transmission is ongoing
 * as the bytes are read/written as they are needed.
 * When an transmission is still ongoing, this function blocks until the
 * transmission is completed before starting a new one.
 *
 * @param address slave address byte, use the result of TWI_ADDRESS_W for write
 * intend or TWI_ADDRESS_R for read intend applied on the 7-bit address
 * @param data location of the bytes to transmit
 * or location for the read in bytes to be written to
 * @param len number of bytes to write/read
 */
void twiint_start(CByteStream_t *pStream);

/**
 * Implemented by TwiController as a C callable function
 * 
 * @param pStream pointer to stream whose processing was completed 
 */
void complete_request(CByteStream_t *pStream);

#define TWI_CO_0_DC_1 (0x40) // Co = 0, D/C = 1
#define TWI_CO_0_DC_0 (0x00) // Co = 0, D/C = 0

CByteStream_t *twi_get_write_buffer(uint8_t addr);

#define TWI_FLAGS_HAVE_READ           (0x01)
#define TWI_FLAGS_TRC_PENDING         (0x02)          // trace pending, do not initiate new twi requests
#define TWI_FLAGS_TRC_HAD_EMPTY       (0x04)          // only dump empty if had non-empty before

#define TWI_WAIT_TIMEOUT        (50)

#ifdef SERIAL_DEBUG_TWI_TRACER
#include "CTraceBuffer.h"

#ifndef TWI_TRACE_SIZE
#define TWI_TRACE_SIZE (32)
#endif

#include <util/twi.h>

#ifndef SERIAL_DEBUG_TWI_RAW_TRACER
#define TRC_BUS_ERROR    (0x00)   
#define TRC_START        (0x01)   
#define TRC_REP_START    (0x02)   
#define TRC_MT_SLA_ACK   (0x03)   
#define TRC_MT_DATA_ACK  (0x04)   
#define TRC_MR_DATA_ACK  (0x05)   
#define TRC_MR_SLA_ACK   (0x06)   
#define TRC_MR_DATA_NACK (0x07)   
#define TRC_MT_ARB_LOST  (0x08)   
#define TRC_MT_SLA_NACK  (0x09)   
#define TRC_MT_DATA_NACK (0x0A)   
#define TRC_MR_SLA_NACK  (0x0B)   
#define TRC_STOP         (0x0C)   
#ifndef SERIAL_DEBUG_WI_TRACE_OVERRUNS
#define TRC_MAX          (0x0D)   
#else
#define TRC_RCV_OVR1     (0x0D)
#define TRC_RCV_OVR2     (0x0E)
#define TRC_MAX          (0x0F)
#endif // SERIAL_DEBUG_WI_TRACE_OVERRUNS

#ifdef SERIAL_DEBUG_TWI_TRACER_CODES
#define TO_STRING2(x) #x
#define TO_STRING(x) TO_STRING2(x)

// Usage:
#define STR_TRC_BUS_ERROR    "BUS_ERROR(0x00)"   
#define STR_TRC_START        "START(" TO_STRING(TW_START) ")"
#define STR_TRC_REP_START    "REP_START(" TO_STRING(TW_REP_START) ")"
#define STR_TRC_MT_SLA_ACK   "MT_SLA_ACK(" TO_STRING(TW_MT_SLA_ACK) ")"
#define STR_TRC_MT_DATA_ACK  "MT_DATA_ACK(" TO_STRING(TW_MT_DATA_ACK) ")"
#define STR_TRC_MR_DATA_ACK  "MR_DATA_ACK(" TO_STRING(TW_MR_DATA_ACK) ")"
#define STR_TRC_MR_SLA_ACK   "MR_SLA_ACK(" TO_STRING(TW_MR_SLA_ACK) ")"
#define STR_TRC_MR_DATA_NACK "MR_DATA_NACK(" TO_STRING(TW_MR_DATA_NACK) ")"
#define STR_TRC_MT_ARB_LOST  "MT_ARB_LOST(" TO_STRING(TW_MT_ARB_LOST) ")"
#define STR_TRC_MT_SLA_NACK  "MT_SLA_NACK(" TO_STRING(TW_MT_SLA_NACK) ")"
#define STR_TRC_MT_DATA_NACK "MT_DATA_NACK(" TO_STRING(TW_MT_DATA_NACK) ")"
#define STR_TRC_MR_SLA_NACK  "MR_SLA_NACK(" TO_STRING(TW_MR_SLA_NACK) ")"
#define STR_TRC_STOP         "STOP()"
#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
#define STR_TRC_RCV_OVR1     "RCV_OVERRUN1()"
#define STR_TRC_RCV_OVR2     "RCV_OVERRUN2()"
#endif
#else // SERIAL_DEBUG_TWI_TRACER_CODES
#define STR_TRC_BUS_ERROR    "BUS_ERROR"   
#define STR_TRC_START        "START"   
#define STR_TRC_REP_START    "REP_START"   
#define STR_TRC_MT_SLA_ACK   "MT_SLA_ACK"   
#define STR_TRC_MT_DATA_ACK  "MT_DATA_ACK"   
#define STR_TRC_MR_DATA_ACK  "MR_DATA_ACK"   
#define STR_TRC_MR_SLA_ACK   "MR_SLA_ACK"   
#define STR_TRC_MR_DATA_NACK "MR_DATA_NACK"   
#define STR_TRC_MT_ARB_LOST  "MT_ARB_LOST"   
#define STR_TRC_MT_SLA_NACK  "MT_SLA_NACK"   
#define STR_TRC_MT_DATA_NACK "MT_DATA_NACK"   
#define STR_TRC_MR_SLA_NACK  "MR_SLA_NACK"   
#define STR_TRC_STOP         "STOP"   
#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
#define STR_TRC_RCV_OVR1     "RCV_OVERRUN1"
#define STR_TRC_RCV_OVR2     "RCV_OVERRUN2"
#endif
#endif //SERIAL_DEBUG_TWI_TRACER_CODES
#endif // SERIAL_DEBUG_TWI_RAW_TRACER

extern CTwiTraceBuffer_t *twi_trace_buffer;
extern PGM_P const trcStrings[] PROGMEM;
#endif

#ifdef __cplusplus
}
#endif

//default to Arduino oscillator
#ifndef F_CPU
#define F_CPU 16000000UL
#warning "F_CPU not defined! Assuming 16MHz."
#endif

#ifndef TWI_FREQUENCY
#define TWI_FREQUENCY 400000UL
#warning "TWI_FREQUENCY not defined! Assuming 400kHz."
#endif

//set prescaler so that the TWBR value is as large as possible
//and at least 10 (frequency error below 5%)
//https://www.nongnu.org/avr-libc/user-manual/group__twi__demo.html Note[5]
#define TWI_TWBR_VALUE(c, t, p)             (((c) / (t) - 16) / (2 * (p)))
#define TWI_TWBR_IN_RANGE(c, t, p, l, h)    TWI_TWBR_VALUE(c, t, p) >= (l) && TWI_TWBR_VALUE(c, t, p) <= (h)
#define TWI_COMPUTED_FREQUENCY(c, p, v)     ((c) / (16L + 2L * (p) * (v)))

#if TWI_TWBR_IN_RANGE(F_CPU, TWI_FREQUENCY, 1, 2, 0xff)
#define TWI_PRESCALER 1
#define TWPS0_VALUE 0
#define TWPS1_VALUE 0
#elif TWI_TWBR_IN_RANGE(F_CPU, TWI_FREQUENCY, 4, 10, 0xff)
#define TWI_PRESCALER 4
#define TWPS0_VALUE 1
#define TWPS1_VALUE 0
#elif TWI_TWBR_IN_RANGE(F_CPU, TWI_FREQUENCY, 16, 10, 0xff)
#define TWI_PRESCALER 16
#define TWPS0_VALUE 0
#define TWPS1_VALUE 1
#elif TWI_TWBR_IN_RANGE(F_CPU, TWI_FREQUENCY, 64, 10, 0xff)
#define TWI_PRESCALER 64
#define TWPS0_VALUE 1
#define TWPS1_VALUE 1
#else
#if TWI_TWBR_VALUE(F_CPU, TWI_FREQUENCY, 1) < 2
#error "TWI_FREQUENCY too high!"
#else
#error "TWI_FREQUENCY too low!"
#endif
#endif

// #if (F_CPU / TWI_FREQUENCY - 16) / (2 * 1) >= 2 \
//  && (F_CPU / TWI_FREQUENCY - 16) / (2 * 1) <= 0xFF
// #define TWI_PRESCALER 1
// #define TWPS0_VALUE 0
// #define TWPS1_VALUE 0
// #elif (F_CPU / TWI_FREQUENCY - 16) / (2 * 4) >= 10 \
//  && (F_CPU / TWI_FREQUENCY - 16) / (2 * 4) <= 0xFF
// #define TWI_PRESCALER 4
// #define TWPS0_VALUE 1
// #define TWPS1_VALUE 0
// #elif (F_CPU/TWI_FREQUENCY - 16) / (2 * 16) >= 10 \
//         && (F_CPU/TWI_FREQUENCY - 16) / (2 * 16) <= 0xFF
// #define TWI_PRESCALER 16
// #define TWPS0_VALUE 0
// #define TWPS1_VALUE 1
// #elif (F_CPU/TWI_FREQUENCY - 16) / (2 * 64) >= 10 \
//         && (F_CPU/TWI_FREQUENCY - 16) / (2 * 64) <= 0xFF
// #define TWI_PRESCALER 64
// #define TWPS0_VALUE 1
// #define TWPS1_VALUE 1
// #else
// #error "TWI_FREQUENCY too low!"
// #endif
//
#define TWBR_VALUE TWI_TWBR_VALUE(F_CPU, TWI_FREQUENCY, TWI_PRESCALER)
#define TWI_ACTUAL_FREQUENCY TWI_COMPUTED_FREQUENCY(F_CPU, TWI_PRESCALER, TWBR_VALUE)      

#endif /* TWIINT_H_ */
