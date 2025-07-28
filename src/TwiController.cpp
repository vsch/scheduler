
#include "TwiController.h"
#include "CTwiController.h"
#include "twiint.h"

void twi_complete_request(CByteStream_t *pStream) {
    twiController.endProcessingRequest((ByteStream *) pStream);
}

void twi_fresh_stream() {
    twiStream = (CByteStream_t *) twiController.getWriteStream();
}

CByteStream_t *twi_get_write_buffer(uint8_t addr) {
    ByteStream *pStream = twiController.getWriteStream();
    pStream->set_address(addr);
    return twiStream = (CByteStream_t *) pStream;
}

CByteStream_t *twi_process(CByteStream_t *pStream) {
    return (CByteStream_t *) twiController.processStream((ByteStream *) pStream);
}

CByteStream_t *twi_unbuffered_request(uint8_t addr, uint8_t *pData, uint8_t nSize) {
    ((ByteStream *)twiStream)->setOwnBuffer(pData, nSize);
    twiStream->addr = addr;
    return (CByteStream_t *) twiController.processStream((ByteStream *)twiStream);
}

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

uint8_t twi_wait_sent(CByteStream_t *pStream) {
    return twi_wait((TwiWaitCallback) stream_is_pending, pStream);

}
// IMPORTANT: assumes: interrupts are enabled, processing of requests should be done by interrupt
//            routine sequentially sending all pending requests.
//            i.e. set CTR_FLAGS_REQ_AUTO_START in controller constructor flags
uint8_t twi_wait(TwiWaitCallback callback, void *pParam) {
    uint32_t start = micros();
    uint32_t diff = 0;
    uint32_t timeoutMic = TWI_WAIT_TIMEOUT_MS * 1000L;

    while (callback(pParam)) {
        diff = micros() - start;
        if (diff >= timeoutMic) {
#ifdef SERIAL_DEBUG_TWI_TRACER
            TraceBuffer::dumpTrace();
#endif
            serialDebugTwiPrintf_P(PSTR("  TWI: #%d twi_wait timed out %ld.\n"), twiController.getReadStreamId((ByteStream *) pParam), diff / 1000L);

#ifdef SERIAL_DEBUG
            ((ByteStream *)pParam)->serialDebugDump(twiController.getReadStreamId((ByteStream *)pParam));
#endif
            return 0;
        }
    }

    if (diff) {
        serialDebugTwiPrintf_P(PSTR("  TWI: #%d twi_wait done %ld.\n"), twiController.getReadStreamId((ByteStream *) pParam), diff / 1000L);
    }

#ifdef SERIAL_DEBUG_TWI_TRACER
    TraceBuffer::dumpTrace();
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
    pStream = (CByteStream_t *) twiController.processStream((ByteStream *) twiStream);

    END_SERIAL_DEBUG_TWI_STATS(reqSize);

    serialDebugDetailTwiStatsPrintf_P(PSTR("%8ld: TWI %d command bytes, new bytes %d in %ld usec %d\n"), start / 1000L, twi_send_bytes, stream_count(twiStream), twi_send_time, twi_send_errors);
#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
    twi_send_bytes = 0;
#endif
    return pStream;
}

void twi_set_own_buffer(uint8_t *pData, uint8_t nSize) {
    ((ByteStream *) twiStream)->setOwnBuffer(pData, nSize);
}

void twi_set_rd_buffer(uint8_t rdReverse, uint8_t *pRdData, uint8_t nRdSize) {
    ((ByteStream *) twiStream)->setRdBuffer(rdReverse, pRdData, nRdSize);
}


void TwiController::startProcessingRequest(ByteStream *pStream) {
    CLI();
    // output stream content and call endProcessingRequest

#ifndef CONSOLE_DEBUG

#ifdef SERIAL_DEBUG_TWI_DATA
    pStream->serialDebugDump(getReadStreamId(pStream));
#endif

#endif
    this->resume(0);

    SEI();

#ifndef CONSOLE_DEBUG
    twiint_start((CByteStream_t *) pStream);
#else
    pStream->flags |= STREAM_FLAGS_PROCESSING;
#endif
}
