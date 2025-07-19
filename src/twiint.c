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
 * Author:     Vladimir Schneider
 *
 * Modified for CByteStream and CTwiController handling
 * Fixed write followed by read handling
 * Added read Buffer after write request handing
 * Added TWI status tracing for debugging drivers
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

#ifndef CONSOLE_DEBUG

#include <avr/io.h>         //hardware registers
#include <avr/interrupt.h>  //interrupt vectors
#include <util/twi.h>       //TWI status masks

#endif

#include "twiint.h"
#include "CByteBuffer.h"

CByteStream_t *pTwiStream;
CByteBuffer_t rdBuffer;
uint16_t twiint_errors;
uint8_t twiint_flags;

#ifdef CONSOLE_DEBUG

void twiint_init(void) {
    twiint_flags = 0;
}

bool twiint_busy(void) {
    return 0;
}

void twiint_flush(void) {
}

void twiint_start(CByteStream_t *pStream) {
    twiint_flush();
}

void twint_cancel_rd(CByteStream_t *pStream) {
    if (twiint_flags & TWI_FLAGS_HAVE_READ) {
        if (pTwiStream == pStream) {
            twiint_flags &= ~TWI_FLAGS_HAVE_READ;
        }
    }
}

#else

void twiint_init(void) {
    twiint_flags = 0;

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
    twiint_flags &= ~TWI_FLAGS_HAVE_READ;

    if (pStream->nRdSize && pStream->pRdData) {
        buffer_init(&rdBuffer, pStream->flags & STREAM_FLAGS_BUFF_REVERSE, pStream->pRdData, pStream->nRdSize);
        twiint_flags |= TWI_FLAGS_HAVE_READ;
    }

#ifdef SERIAL_DEBUG_TWI_REQ_TIMING
    if (!pStream->startTime) {
        pStream->startTime = micros();
        if (!pStream->startTime) pStream->startTime = 1;
    }
#endif

    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA);
}

void twint_cancel_rd(CByteStream_t *pStream) {
    twiint_flags &= ~TWI_FLAGS_HAVE_READ;
}

#endif // CONSOLE_DEBUG

#ifdef SERIAL_DEBUG_TWI_TRACER

CTwiTraceBuffer_t *twi_trace_buffer;

#ifndef SERIAL_DEBUG_TWI_RAW_TRACER
const char sSTR_BUS_ERROR[] PROGMEM = STR_TRC_BUS_ERROR;
const char sSTR_START[] PROGMEM = STR_TRC_START;
const char sSTR_REP_START[] PROGMEM = STR_TRC_REP_START;
const char sSTR_MT_SLA_ACK[] PROGMEM = STR_TRC_MT_SLA_ACK;
const char sSTR_MT_DATA_ACK[] PROGMEM = STR_TRC_MT_DATA_ACK;
const char sSTR_MR_DATA_ACK[] PROGMEM = STR_TRC_MR_DATA_ACK;
const char sSTR_MR_SLA_ACK[] PROGMEM = STR_TRC_MR_SLA_ACK;
const char sSTR_MR_DATA_NACK[] PROGMEM = STR_TRC_MR_DATA_NACK;
const char sSTR_MT_ARB_LOST[] PROGMEM = STR_TRC_MT_ARB_LOST;
const char sSTR_MT_SLA_NACK[] PROGMEM = STR_TRC_MT_SLA_NACK;
const char sSTR_MT_DATA_NACK[] PROGMEM = STR_TRC_MT_DATA_NACK;
const char sSTR_MR_SLA_NACK[] PROGMEM = STR_TRC_MR_SLA_NACK;
const char sSTR_STOP[] PROGMEM = STR_TRC_STOP;

#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
const char sSTR_TRC_RCV_OVR1[] PROGMEM = STR_TRC_RCV_OVR1;
const char sSTR_TRC_RCV_OVR2[] PROGMEM = STR_TRC_RCV_OVR2;
const char sSTR_TRC_RCV_ADDR[] PROGMEM = STR_TRC_RCV_ADDR;
#endif

PGM_P const trcStrings[] PROGMEM = {
        sSTR_BUS_ERROR,
        sSTR_START,
        sSTR_REP_START,
        sSTR_MT_SLA_ACK,
        sSTR_MT_DATA_ACK,
        sSTR_MR_DATA_ACK,
        sSTR_MR_SLA_ACK,
        sSTR_MR_DATA_NACK,
        sSTR_MT_ARB_LOST,
        sSTR_MT_SLA_NACK,
        sSTR_MT_DATA_NACK,
        sSTR_MR_SLA_NACK,
        sSTR_STOP,
#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
        sSTR_TRC_RCV_OVR1,
        sSTR_TRC_RCV_OVR2,
        sSTR_TRC_RCV_ADDR,
#endif // SERIAL_DEBUG_WI_TRACE_OVERRUNS
};
#endif //SERIAL_DEBUG_TWI_RAW_TRACER

#ifdef SERIAL_DEBUG_TWI_RAW_TRACER
#define twi_raw_tracer(s) twi_trace(twi_trace_buffer, s)
#define twi_tracer(d) ((void)0)
#define twi_tracer_start() ((void)0)
#define twi_tracer_stop() ((void)0)
#else
#define twi_raw_tracer(s) ((void)0)
#define twi_tracer(d) twi_trace(twi_trace_buffer, (d))

#ifdef DEBUG_MODE_TWI_TRACE_TIMEIT
uint32_t startTime;
uint16_t elapsedTime;

#define twi_tracer_start() startTime = micros(); twi_trace(twi_trace_buffer, TRC_START)
#define twi_tracer_stop() elapsedTime = (uint16_t)(micros() - startTime); twi_trace_bytes(twi_trace_buffer, TRC_STOP, &elapsedTime, sizeof(elapsedTime))
#else  // DEBUG_MODE_TWI_TRACE_TIMEIT
#define twi_tracer_start() twi_trace(twi_trace_buffer, TRC_START)
#define twi_tracer_stop() twi_trace(twi_trace_buffer, TRC_STOP)
#endif // DEBUG_MODE_TWI_TRACE_TIMEIT

#endif //SERIAL_DEBUG_TWI_RAW_TRACER

#else  // SERIAL_DEBUG_TWI_TRACER
#define twi_raw_tracer(s) ((void)0)
#define twi_tracer(d) ((void)0)
#define twi_tracer_start() ((void)0)
#define twi_tracer_stop() ((void)0)
#endif // SERIAL_DEBUG_TWI_TRACER

#ifndef CONSOLE_DEBUG

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
            if (twiint_flags & TWI_FLAGS_HAVE_READ) {
                TWDR = pTwiStream->addr | 0x01;     // make it a read
                goto rep_start;
                // } else {
                //     TWDR = pTwiStream->addr;
            }
            goto start;

        case TW_START:
            twi_tracer_start();

        start:
            TWDR = pTwiStream->addr;

        rep_start:
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
            } else if (twiint_flags & TWI_FLAGS_HAVE_READ) {
                // do a repeated start then read into the rcv queue
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA);
            } else {
                goto complete;
            }
            break;

        case TW_MR_DATA_NACK:
            // this is generated after the last byte is received
            twi_tracer(TRC_MR_DATA_NACK);

#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
            {
                uint8_t capacity = buffer_capacity(&rdBuffer);
                if (!capacity) {
                    // this is an error, should have generatged MR_DATA_NACK when receiving second to last byte
                    twi_tracer(TRC_RCV_OVR1);
                    TWCR = (1 << TWINT) | (1 << TWEN) /*| (1 << TWEA)*/ | (1 << TWSTO);
                    twi_tracer_stop();
                    twi_complete_request(pTwiStream);
                    break;
                }

            }
#ifdef DEBUG_MODE_TWI_TRACE_RCVADDR
            twi_trace_bytes(twi_trace_buffer, TRC_RCV_ADDR, &rdBuffer.pData, 2); // write address of pData
#endif //DEBUG_MODE_TWI_TRACE_RCVADDR
#endif //SERIAL_DEBUG_WI_TRACE_OVERRUNS

            buffer_put(&rdBuffer, TWDR);
            goto complete;

        case TW_MR_DATA_ACK: {
            twi_tracer(TRC_MR_DATA_ACK);
            uint8_t capacity = buffer_capacity(&rdBuffer);

#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
            if (!capacity) {
                // this is an error, should have generatged MR_DATA_NACK when receiving second to last byte
                twi_tracer(TRC_RCV_OVR1);
                TWCR = (1 << TWINT) | (1 << TWEN) /*| (1 << TWEA)*/ | (1 << TWSTO);
                twi_tracer_stop();
                twi_complete_request(pTwiStream);
                break;
            }
#endif // SERIAL_DEBUG_WI_TRACE_OVERRUNS

            buffer_put(&rdBuffer, TWDR);
            capacity--;

            if (capacity > 1) {
                // data byte received, send ack since we have room for more
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA);
            } else {
#ifdef SERIAL_DEBUG_WI_TRACE_OVERRUNS
                if (!capacity) {
                    // this is an error, should have generatged MR_DATA_NACK when receiving second to last byte
                    twi_tracer(TRC_RCV_OVR2);
                    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTO);
                    twi_tracer_stop();
                    twi_complete_request(pTwiStream);
                    break;
                }
#endif //SERIAL_DEBUG_WI_TRACE_OVERRUNS
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
                //
                // // TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) /*| (1 << TWEA)*/; // data byte received, send nack
                // TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) /*| (1 << TWSTO)*/;
                // //complete_request(pTwiStream);
                // // twi_tracer_stop();
                // break;
            }
        }
            break;

        case TW_MR_SLA_ACK:
            twi_tracer(TRC_MR_SLA_ACK);
            if (buffer_capacity(&rdBuffer) > 1) {
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA); // data byte will be received, send ACK
            } else {
                // this is a 1 byte buffer, we need to have NACK generated so only 1 byte is transfered.
                TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // data byte will be received, send NACK
            }
            break;

        case TW_MT_ARB_LOST:
            twi_tracer(TRC_MT_ARB_LOST);
            twiint_errors++;
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA);
            // Arbitration loss restarts attempt
            // TODO: need to reset the stream condition because it is unknown when the arbitration was lost
            //       it could have been detected after transmission of some data.
            //       as an alternative, just handle it as an error and need for to signal the requestor that
            //       an error has occurred.
            // TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
            // complete_request(pTwiStream);
            break;

            // TODO: some of these may not be errors but SOP for I2C protocol. Like the MR_DATA_NACK used to receive
            //     the last byte, and not an error.
        case TW_MT_SLA_NACK:
            twi_tracer(TRC_MT_SLA_NACK);
            goto error;

        case TW_MT_DATA_NACK:
            twi_tracer(TRC_MT_DATA_NACK);
            goto error;

        case TW_MR_SLA_NACK:
            twi_tracer(TRC_MR_SLA_NACK);
            goto error;

        case TW_BUS_ERROR:
            twi_tracer(TRC_BUS_ERROR);
            goto error;

        default:
#ifdef SERIAL_DEBUG_TWI_TRACER
            twi_tracer(twsr);
            //twi_tracer(twsr & TW_STATUS_MASK);
#endif
        error:
            twiint_errors++;

        complete:
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
            twi_tracer_stop();
            twi_complete_request(pTwiStream);
    }
}

#endif // CONSOLE_DEBUG
