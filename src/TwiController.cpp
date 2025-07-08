
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
void twi_dump_trace(uint8_t noWait) {
    cli();
    twiController.dumpTrace(noWait);
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
uint8_t twi_wait_sent(CByteStream_t *pStream, uint8_t timeoutMs) {
    uint32_t start = micros();
    uint32_t timeoutMic = timeoutMs * 1000L;

    while (stream_is_pending(pStream)) {
        if (timeoutMs) {
            uint32_t diff = micros() - start;
            if (diff >= timeoutMic) {
                cli();
                twint_cancel_rd(pStream);
                sei();
#ifdef SERIAL_DEBUG_TWI_TRACER
                twi_dump_trace(1);
#endif
                PRINTF_SERIAL_DEBUG_TWI_STATS(PSTR("  TWI: %d wait_sent timed out %ld.\n"), twiController.getReadStreamId((ByteStream *)pStream), diff / 1000L);
                return 0;
            }
        }
    }
#ifdef SERIAL_DEBUG_TWI_TRACER
    twi_dump_trace(1);
#endif
    return 1;
}

CByteStream_t *twi_process_stream() {
    // send the accumulated buffer
    CByteStream_t *pStream = NULL;

    START_SERIAL_DEBUG_TWI_STATS();
#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
    uint16_t reqSize = stream_count(twiStream);
#endif
    // returns the same stream but updated head/tail, so address is unchanged
    pStream = twi_process(twiStream);

    END_SERIAL_DEBUG_TWI_STATS(reqSize);

    PRINTF_SERIAL_DEBUG_TWI_STATS(PSTR("%8ld: TWI %d command bytes, new bytes %d in %ld usec %d\n"),
                                      start / 1000L, twi_send_bytes, stream_count(twiStream), twi_send_time, twi_send_errors);
#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
    twi_send_bytes = 0;
#endif
    return pStream;
}

