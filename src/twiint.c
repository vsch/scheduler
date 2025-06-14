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

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
}

uint16_t twiint_errors;

// added to introduce delays that exist in the non-interrupt driven version
// interrupt driven version does not initialize OLED-091 unless delays are introduced.
ISR(TWI_vect) {
    switch (TW_STATUS) {
        case TW_START:
        case TW_REP_START:
            TWDR = pTwiStream->addr;
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            break;

        case TW_MT_SLA_ACK:
        case TW_MT_DATA_ACK:
            if (!stream_is_empty(pTwiStream)) {
                TWDR = stream_get(pTwiStream);
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
                complete_request(pTwiStream);
            }
            break;

        case TW_MR_DATA_ACK:
            if (!stream_is_full(pTwiStream)) {
                stream_put(pTwiStream, TWDR);
            } else {
                // TODO: handle buffer full condition
            }
        case TW_MR_SLA_ACK:
            // TODO: validate the end condition is correct
            if (stream_count(pTwiStream) > 1) {
                TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
            } else {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
            }
            break;

        case TW_MR_DATA_NACK:
            if (!stream_is_full(pTwiStream)) {
                stream_put(pTwiStream, TWDR);
            } else {
                // TODO: handle buffer full condition
            }
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
            break;

        case TW_MT_ARB_LOST:
            //case TW_MR_ARB_LOST:
            TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
            twiint_errors++;
            break;

        case TW_MT_SLA_NACK:
        case TW_MT_DATA_NACK:
        case TW_MR_SLA_NACK:
        default:
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
            twiint_errors++;
    }
}
