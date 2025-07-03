
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
    return (CByteStream_t *) twiController.processStream((ByteStream *) pStream);
}

CByteStream_t *twi_unbuffered_request(uint8_t addr, uint8_t *pData, uint8_t nSize, CByteQueue_t *pRcvQ) {
    return (CByteStream_t *) twiController.processRequest(addr, pData, nSize, (ByteQueue *) pRcvQ);
}

#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
uint32_t twi_send_time = 0;
uint16_t twi_send_bytes = 0;
#endif

uint8_t twi_send_errors = 0;

CByteStream_t *twiStream;

void twi_add_pgm_byte_list(const uint8_t *bytes, uint16_t count) {
    while (count--) {
        stream_put(twiStream, pgm_read_byte(bytes++));
    }
}

// IMPORTANT: assumes: interrupts are enabled, processing of requests should be done by interrupt 
//            routine sequentially sending all pending requests. 
//            i.e. set CTR_FLAGS_REQ_AUTO_START in controller constructor flags
void twi_wait_sent(CByteStream_t *pStream, uint8_t timeoutMs) {
    uint32_t start = micros();
    uint32_t timeoutMic = timeoutMs * 1000L;

    while (stream_is_pending(pStream)) {
        if (timeoutMs) {
            uint32_t diff = micros() - start;
            if (diff >= timeoutMic) {
                PRINTF_SERIAL_DEBUG_TWI_STATS(PSTR("     Waiting last req timed out %ld.\n"), diff / 1000L);
                break;
            }
        }
    }
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
    // stream_put(twiStream, TWI_CO_0_DC_0);
    twiStream = twi_get_write_buffer(TWI_ADDRESS_W(DISPLAY_ADDRESS));

    END_SERIAL_DEBUG_TWI_STATS(reqSize);

    PRINTF_SERIAL_DEBUG_TWI_STATS(PSTR("%8ld: TWI %d command bytes, new bytes %d in %ld usec %d\n"),
                                      start / 1000L, twi_send_bytes, stream_count(twiStream), twi_send_time, twi_send_errors);
#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
    twi_send_bytes = 0;
#endif
    return pStream;
}

