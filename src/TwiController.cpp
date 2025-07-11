
#include "TwiController.h"
#include "CTwiController.h"
#include "twiint.h"

void complete_request(CByteStream_t *pStream) {
    twiController.endProcessingRequest((ByteStream *) pStream);
}

CByteStream_t *twi_get_write_buffer(uint8_t addr) {
    ByteStream *pStream = twiController.getWriteStream();
    pStream->set_address(addr);
    return (CByteStream_t *) pStream;
}

CByteStream_t *twi_process(CByteStream_t *pStream) {
    return (CByteStream_t *) twiController.processStream((ByteStream *) pStream, NULL);
}

CByteStream_t *twi_process_rcv(CByteStream_t *pStream, CByteBuffer_t *pRcvBuffer) {
    return (CByteStream_t *) twiController.processStream((ByteStream *) pStream, pRcvBuffer);
}

CByteStream_t *twi_unbuffered_request(uint8_t addr, uint8_t *pData, uint8_t nSize, CByteBuffer_t *pRcvBuffer) {
    return (CByteStream_t *) twiController.processRequest(addr, pData, nSize, pRcvBuffer);
}

#ifdef SERIAL_DEBUG_TWI_TRACER

void twi_dump_trace() {
    cli();
    twiController.dumpTrace();
    sei();
}

#endif

#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
uint32_t twi_send_time = 0;
uint16_t twi_send_bytes = 0;
#endif

uint8_t twi_send_errors = 0;

CByteStream_t *twiStream;

void twi_add_byte(uint8_t byte) {
    stream_put(twiStream, byte);
}

void twi_add_pgm_byte_list(const uint8_t *bytes, uint16_t count) {
    while (count--) {
        stream_put(twiStream, pgm_read_byte(bytes++));
    }
}

// IMPORTANT: assumes: interrupts are enabled, processing of requests should be done by interrupt 
//            routine sequentially sending all pending requests. 
//            i.e. set CTR_FLAGS_REQ_AUTO_START in controller constructor flags
uint8_t twi_wait_sent(CByteStream_t *pStream) {
    uint32_t start = micros();
    uint32_t timeoutMic = TWI_WAIT_TIMEOUT * 1000L;

    while (stream_is_pending(pStream)) {
        uint32_t diff = micros() - start;
        if (diff >= timeoutMic) {
            cli();
            twint_cancel_rd(pStream);
            sei();
#ifdef SERIAL_DEBUG_TWI_TRACER
            twi_dump_trace();
#endif
            serialDebugTwiPrintf_P(PSTR("  TWI: #%d wait_sent timed out %ld.\n"), twiController.getReadStreamId((ByteStream *) pStream), diff / 1000L);
            return 0;
        }
    }
#ifdef SERIAL_DEBUG_TWI_TRACER
    twi_dump_trace();
#endif
    return 1;
}

CByteStream_t *twi_process_stream_rcv(CByteBuffer_t *pBuffer) {
    // send the accumulated buffer
    CByteStream_t *pStream = NULL;

    START_SERIAL_DEBUG_TWI_STATS();
#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
    uint16_t reqSize = stream_count(twiStream);
#endif
    // returns the same stream but updated head/tail, so address is unchanged
    pStream = (CByteStream_t *) twiController.processStream((ByteStream *) twiStream, pBuffer);

    END_SERIAL_DEBUG_TWI_STATS(reqSize);

    serialDebugDetailTwiStatsPrintf_P(PSTR("%8ld: TWI %d command bytes, new bytes %d in %ld usec %d\n"), start / 1000L, twi_send_bytes, stream_count(twiStream), twi_send_time, twi_send_errors);
#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
    twi_send_bytes = 0;
#endif
    return pStream;
}

CByteStream_t *twi_process_stream() {
    return twi_process_stream_rcv(NULL);
}

#ifdef SERIAL_DEBUG_TWI_TRACER

// IMPORTANT: called with interrupts disabled, so it should return with interrupts disabled
void TwiController::dumpTrace() {
    if (!(flags & CTR_FLAGS_TRC_HAD_EMPTY) || !twiTraceBuffer.isEmpty()) {

        if (twiTraceBuffer.isEmpty()) {
            flags |= CTR_FLAGS_TRC_HAD_EMPTY;
        } else {
            flags &= ~CTR_FLAGS_TRC_HAD_EMPTY;
        }

        // set trace pending and wait for TWI to be idle so we don't mess up the twi interrupt timing
        flags |= CTR_FLAGS_TRC_PENDING;

#ifndef CONSOLE_DEBUG
        serialDebugPrintf_P(PSTR("Waiting for TWI TRACER. "));
        uint32_t start = micros();
        uint32_t timeoutMic = TWI_WAIT_TIMEOUT * 1000L;

        sei();
        while (twiint_busy()) {
            uint32_t diff = micros() - start;
            if (diff >= timeoutMic) {
                serialDebugPrintf_P(PSTR("timed out %dms. "), TWI_WAIT_TIMEOUT);
                break;
            }
        }
        cli();
        serialDebugPrintf_P(PSTR("done.\n"));
#endif

        TraceBuffer traceBuffer;

        // make a copy and clear the trace queue
        traceBuffer.copyFrom(&twiTraceBuffer);
        twiTraceBuffer.reset();

        flags &= ~CTR_FLAGS_TRC_PENDING;

        // enable interrupts so twi processing can proceed
        sei();

        traceBuffer.dump();

        cli();
    }
}

#endif
    
