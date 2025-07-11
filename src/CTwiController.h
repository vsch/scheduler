
#ifndef ARDUINOPROJECTMODULE_CTWICONTROLLER_H
#define ARDUINOPROJECTMODULE_CTWICONTROLLER_H

#include <stdint.h>     //uint8_t type
#include "CByteStream.h"

#define TWI_WAIT_TIMEOUT    (50)

#ifdef __cplusplus
extern "C" {
#endif

extern CByteStream_t *twiStream;

CByteStream_t *twi_get_write_buffer(uint8_t addr);

// process accumulated twiStream, with debug stats and prep twiStream for next accumulation.
extern CByteStream_t *twi_process_stream();            // processes the twiStream and requests a new twiStream without the need to call gfx_start_twi_cmd_frame() after the call
extern CByteStream_t *twi_process_stream_rcv(CByteBuffer_t *pBuffer);

CByteStream_t *twi_process(CByteStream_t *pStream);
CByteStream_t *twi_process_rcv(CByteStream_t *pStream, CByteBuffer_t *pRcvBuffer);

// sending operations
extern void twi_add_byte(uint8_t byte);
extern void twi_add_pgm_byte_list(const uint8_t *bytes, uint16_t count);

// wait for stream to be sent, if timeout !=0 then wait that many ms before giving up
// return 0 if timeed out, 1 if sent (may be with twiint_errors)
extern uint8_t twi_wait_sent(CByteStream_t *pStream);
/**
 * Send given buffered data as self-buffered twi request
 * 
 * @param addr   twi address, including read flag
 * @param pData  pointer to byte buffer
 * @param len    length of data to send
 * @return       pointer to last request, can be used to wait for completion of the send
 */
extern CByteStream_t *twi_unbuffered_request(uint8_t addr, uint8_t *pData, uint8_t nSize, CByteBuffer_t *pRcvBuffer);

#ifdef SERIAL_DEBUG_TWI_TRACER
extern void twi_dump_trace();
#endif

#ifdef __cplusplus
}
#endif

#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
extern uint32_t twi_send_time;
extern uint16_t twi_send_bytes;
extern uint8_t twi_send_errors;

#define RESET_SERIAL_DEBUG_TWI_STATS() \
    twi_send_time = 0; \
    twi_send_bytes = 0; \
    twi_send_errors = 0

#define START_SERIAL_DEBUG_TWI_STATS() \
        uint32_t start = micros()

#define RESTART_SERIAL_DEBUG_TWI_STATS() \
        start = micros()

#define END_SERIAL_DEBUG_TWI_STATS(bytes_sent) \
        twi_send_time += micros() - start; \
        twi_send_bytes += (bytes_sent)

#define PRINTF_SERIAL_DEBUG_TWI_STATS(...) printf_P(__VA_ARGS__)

#else
#define RESET_SERIAL_DEBUG_TWI_STATS()                  ((void)0)
#define START_SERIAL_DEBUG_TWI_STATS()                  ((void)0)
#define END_SERIAL_DEBUG_TWI_STATS(bytes_sent)          ((void)0)
#define PRINTF_SERIAL_DEBUG_TWI_STATS(...)              ((void)0)
#endif // SERIAL_DEBUG_DETAIL_TWI_STATS

#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
#define serialDebugDetailTwiStatsPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugDetailTwiStatsPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugDetailTwiStatsPrintf_P(...) ((void)0)
#define serialDebugDetailTwiStatsPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_TWI
#define serialDebugTwiPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugTwiPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugTwiPrintf_P(...) ((void)0)
#define serialDebugTwiPuts_P(...) ((void)0)
#endif

#endif //ARDUINOPROJECTMODULE_CTWICONTROLLER_H
