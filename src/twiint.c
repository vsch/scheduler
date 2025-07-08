/*
 * twiint.c
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



#include <avr/io.h>         //hardware registers
#include <avr/interrupt.h>  //interrupt vectors
#include <util/twi.h>       //TWI status masks
#include "twiint.h"
#include "CByteBuffer.h"

//default to Arduino oscillator
#ifndef F_CPU
#define F_CPU 16000000UL
#warning "F_CPU not defined! Assuming 16MHz."
#endif


//set prescaler so that the TWBR value is as large as possible
//and at least 10 (frequency error below 5%)
//https://www.nongnu.org/avr-libc/user-manual/group__twi__demo.html Note[5]
#if (F_CPU / TWI_FREQUENCY - 16) / (2 * 1) >= 2 \
 && (F_CPU / TWI_FREQUENCY - 16) / (2 * 1) <= 0xFF
#define TWI_PRESCALER 1
#define TWPS0_VALUE 0
#define TWPS1_VALUE 0
#elif (F_CPU / TWI_FREQUENCY - 16) / (2 * 4) >= 10 \
 && (F_CPU / TWI_FREQUENCY - 16) / (2 * 4) <= 0xFF
#define TWI_PRESCALER 4
#define TWPS0_VALUE 1
#define TWPS1_VALUE 0
#elif (F_CPU/TWI_FREQUENCY - 16) / (2 * 16) >= 10 \
        && (F_CPU/TWI_FREQUENCY - 16) / (2 * 16) <= 0xFF
#define TWI_PRESCALER 16
#define TWPS0_VALUE 0
#define TWPS1_VALUE 1
#elif (F_CPU/TWI_FREQUENCY - 16) / (2 * 64) >= 10 \
        && (F_CPU/TWI_FREQUENCY - 16) / (2 * 64) <= 0xFF
#define TWI_PRESCALER 64
#define TWPS0_VALUE 1
#define TWPS1_VALUE 1
#else
#error "TWI_FREQUENCY too low!"
#endif

#define TWBR_VALUE ((F_CPU/TWI_FREQUENCY - 16) / (2 * TWI_PRESCALER))

CByteStream_t *pTwiStream;
CByteBuffer_t rdBuffer;
uint8_t haveRead;

void twiint_init(void) {
    TWBR = TWBR_VALUE;
    TWSR = (TWPS1_VALUE << TWPS1) | (TWPS0_VALUE << TWPS0);

    TWCR = (1 << TWINT) | (1 << TWEN);
}

bool twiint_busy(void) {
    return TWCR & (1 << TWIE);
}

void twiint_flush(void) {
    while (TWCR & (1 << TWIE));
}

void twiint_start(CByteStream_t *pStream) {
    twiint_flush();

    pTwiStream = pStream;
    haveRead = 0;
    CByteBuffer_t *pRcvBuffer = pStream->pRcvBuffer;
    if (pRcvBuffer) {
        buffer_copy(&rdBuffer, pRcvBuffer);
        haveRead = 1;
    }
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
}

void twint_cancel_rd(CByteStream_t *pStream) {
    if (haveRead) {
        if (pTwiStream == pStream) {
            haveRead = 0;
        }
    }
}

#ifdef SERIAL_DEBUG_TWI_TRACER

CTwiTraceBuffer_t *twi_trace_buffer;

#endif

uint16_t twiint_errors;

#ifdef SERIAL_DEBUG_TWI_TRACER
#ifdef SERIAL_DEBUG_TWI_RAW_TRACER_WORD
#define twi_raw_tracer(s) twi_trace(twi_trace_buffer, s)
#define twi_tracer(d) ((void)0)
#else
#ifdef SERIAL_DEBUG_TWI_RAW_TRACER
#define twi_raw_tracer(s) twi_trace(twi_trace_buffer, s)
#define twi_tracer(d) ((void)0)
#else
#define twi_raw_tracer(s) ((void)0)
#define twi_tracer(d) twi_trace(twi_trace_buffer, (d))
#endif
#endif
#else
#define twi_raw_tracer(s) ((void)0)
#define twi_tracer(d) ((void)0)
#endif

ISR(TWI_vect) {
#if SERIAL_DEBUG_TWI_TRACER
    uint8_t twsr = TWSR;

    twi_raw_tracer(twsr);
    switch (twsr & TW_STATUS_MASK) {
#else
        twi_raw_tracer(TWSR);
        switch (TW_STATUS) {
#endif
        case TW_REP_START:
            twi_tracer(TRC_REP_START);
            if (haveRead) {
                TWDR = pTwiStream->addr | 0x01;     // make it a read
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
                break;
            } else {
                TWDR = pTwiStream->addr;
                goto start;
            }

        case TW_START:
            twi_tracer(TRC_START);
            TWDR = pTwiStream->addr;

        start:
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            break;

        case TW_MT_SLA_ACK:
            twi_tracer(TRC_MT_SLA_ACK);
            goto ack;

        case TW_MT_DATA_ACK:
            twi_tracer(TRC_MT_DATA_ACK);

        ack:
            if (!stream_is_empty(pTwiStream)) {
                TWDR = stream_get(pTwiStream);
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            } else if (haveRead) {
                // do a repeated start then read into the rcv queue
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
                complete_request(pTwiStream);
                goto stop;
            }
            break;

        case TW_MR_DATA_ACK:
            twi_tracer(TRC_MR_DATA_ACK);
            if (haveRead) {
                uint8_t capacity = buffer_capacity(&rdBuffer);
                if (capacity > 1) {
                    buffer_put(&rdBuffer, TWDR);
                    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
                    break;
                } else {
                    if (capacity) {
                        buffer_put(&rdBuffer, TWDR);
                        complete_request(pTwiStream);
                    }
                    haveRead = 0;
                }
            }
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA) | (1 << TWSTO);
            goto stop;
            
        case TW_MR_SLA_ACK:
            twi_tracer(TRC_MR_SLA_ACK);
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
            break;

        case TW_MR_DATA_NACK:
            twi_tracer(TRC_MR_DATA_NACK);
            twiint_errors++;
            complete_request(pTwiStream);
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
            goto stop;

        case TW_MT_ARB_LOST:
            twi_tracer(TRC_MT_ARB_LOST);
            twiint_errors++;
            complete_request(pTwiStream);
            TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
            break;

        case TW_MT_SLA_NACK:
            twi_tracer(TRC_MT_SLA_NACK);
            goto nack;
        case TW_MT_DATA_NACK:
            twi_tracer(TRC_MT_DATA_NACK);
            goto nack;
        case TW_MR_SLA_NACK:
            twi_tracer(TRC_MR_SLA_NACK);
            goto nack;
        default:
#ifdef SERIAL_DEBUG_TWI_TRACER
            twi_tracer(twsr & TW_STATUS_MASK);
#endif
        nack:
            twiint_errors++;
            complete_request(pTwiStream);
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
            
        stop:
            twi_tracer(TRC_STOP);
            break;
    }
}
