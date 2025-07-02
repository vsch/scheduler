
#ifndef ARDUINOPROJECTMODULE_CTWICONTROLLER_H
#define ARDUINOPROJECTMODULE_CTWICONTROLLER_H

#include <stdint.h>     //uint8_t type

#ifdef __cplusplus
extern "C" {
#endif

extern CByteStream_t *twiStream;

// internally used functions for communication to SSD1306 via TWI
// command transaction markers
extern void twi_start_cmd_frame();
extern CByteStream_t * twi_end_frame();            // processes the twiStream and requests a new twiStream without the need to call gfx_start_twi_cmd_frame() after the call
extern CByteStream_t * twi_send_cmd(uint8_t cmd);

// sending operations
extern void twi_add_byte(uint8_t byte);
extern void twi_add_pgm_byte_list(const uint8_t *bytes, uint16_t count);
extern void twi_wait_sent(CByteStream_t *pStream, uint8_t timeoutMs);
/**
 * Send given buffered data as self-buffered twi request
 * 
 * @param addr   twi address, including read flag
 * @param pData  pointer to byte buffer
 * @param len    length of data to send
 * @return       pointer to last request, can be used to wait for completion of the send
 */
extern CByteStream_t *twi_unbuffered_request(uint8_t addr, uint8_t *pData, uint8_t nSize, CByteQueue_t *pRcvQ);

#ifdef __cplusplus
}
#endif

#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
extern uint32_t twi_send_time;
extern uint16_t twi_send_bytes;
extern uint8_t twi_send_errors;

#define RESET_SERIAL_DEBUG_GFX_TWI_STATS() \
    twi_send_time = 0; \
    twi_send_bytes = 0; \
    twi_send_errors = 0

#define START_SERIAL_DEBUG_GFX_TWI_STATS() \
        uint32_t start = micros()

#define RESTART_SERIAL_DEBUG_GFX_TWI_STATS() \
        start = micros()

#define END_SERIAL_DEBUG_GFX_TWI_STATS(bytes_sent) \
        twi_send_time += micros() - start; \
        twi_send_bytes += (bytes_sent)

#define PRINTF_SERIAL_DEBUG_GFX_TWI_STATS(...) printf_P(__VA_ARGS__)

#else
#define RESET_SERIAL_DEBUG_GFX_TWI_STATS()                  ((void)0)
#define START_SERIAL_DEBUG_GFX_TWI_STATS()                  ((void)0)
#define END_SERIAL_DEBUG_GFX_TWI_STATS(bytes_sent)          ((void)0)
#define PRINTF_SERIAL_DEBUG_GFX_TWI_STATS(...)              ((void)0)
#endif // SERIAL_DEBUG_DETAIL_TWI_STATS

#endif //ARDUINOPROJECTMODULE_CTWICONTROLLER_H
