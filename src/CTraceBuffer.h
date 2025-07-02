
#ifndef ARDUINOPROJECTMODULE_DEBUG_CTRACEBUFFER_H
#define ARDUINOPROJECTMODULE_DEBUG_CTRACEBUFFER_H

// Simple queueing both read and write for use in C interrupts and C code, provided from C/C++ code
// has the same layout as Queue
typedef struct CTraceBuffer
{
    uint8_t nCapacity;
    uint8_t nReadCapacity;
    uint8_t *pPos;
    uint8_t data[TWI_TRACE_SIZE];
    uint8_t traceByte;
    uint8_t traceCount;
} CTwiTraceBuffer_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void twi_trace(CTwiTraceBuffer_t* thizz, uint8_t data); // write byte 7 bits, and add count if repeating

#ifdef __cplusplus
}

#endif

#endif //ARDUINOPROJECTMODULE_DEBUG_CTRACEBUFFER_H
