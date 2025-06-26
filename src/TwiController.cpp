
#include "TwiController.h"
#include "twiint.h"

void complete_request(CByteStream_t *pStream) {
    // force waiting for completion of processing of the request
#ifndef CONSOLE_DEBUG   
    twiint_flush();
#endif    
    
    twiController.endProcessingRequest((ByteStream *) pStream);
}

CByteStream_t *twi_get_write_buffer(uint8_t addr, uint8_t coDC) {
    ByteStream *pStream = twiController.getWriteStream();

    pStream->set_address(addr);
    pStream->put(coDC);
    return (CByteStream_t *) pStream;
}

CByteStream_t * twi_process(CByteStream_t *pStream) {
    return (CByteStream_t *) twiController.processStream((ByteStream *)pStream);
}

CByteStream_t *twi_unbuffered_request(uint8_t addr, uint8_t *pData, uint16_t len, int maxSize) {
    return (CByteStream_t *) twiController.processRequest(addr, pData, len, maxSize);
}
