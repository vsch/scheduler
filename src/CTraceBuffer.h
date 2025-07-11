
#ifdef SERIAL_DEBUG_TWI_TRACER

#ifndef ARDUINOPROJECTMODULE_DEBUG_CTRACEBUFFER_H
#define ARDUINOPROJECTMODULE_DEBUG_CTRACEBUFFER_H

// #ifndef TWI_TRACE_SIZE
// #define TWI_TRACE_SIZE
// #endif

// Simple queueing both read and write for use in C interrupts and C code, provided from C/C++ code
// has the same layout as Queue
typedef struct CTraceBuffer
{
    uint8_t nCapacity;
    uint8_t nReadCapacity;
    uint8_t *pPos;
    uint8_t traceByte;
    uint8_t traceCount;
    uint8_t haveByte;
    uint8_t nextAfterStop;           // index of trace after last stop
    uint8_t data[TWI_TRACE_SIZE];
} CTwiTraceBuffer_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void twi_trace(CTwiTraceBuffer_t* thizz, uint8_t traceByte); // write byte 7 bits, and add count if repeating
extern void twi_trace_bytes(CTwiTraceBuffer_t* thizz, uint8_t traceByte, void *data, uint8_t count); // write byte 7Bits, and add given data afterwards

#ifdef SERIAL_DEBUG_TWI_RAW_TRACER
#define TRACER_BYTE_COUNT_MARKER    (0x01)
#else
#define TRACER_BYTE_COUNT_MARKER    (0x80)
#endif

#ifdef __cplusplus
}

#endif

#endif //ARDUINOPROJECTMODULE_DEBUG_CTRACEBUFFER_H

#endif //SERIAL_DEBUG_TWI_TRACER
