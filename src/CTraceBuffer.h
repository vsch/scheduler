
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
#ifdef SERIAL_DEBUG_TWI_RAW_TRACER_WORD   
    uint16_t *pPos;
    uint16_t data[TWI_TRACE_SIZE];
#else
    uint8_t *pPos;
    uint8_t traceByte;
    uint8_t traceCount;
    uint8_t haveByte;
#ifdef DEBUG_MODE_TWI_TRACE_TIMEIT
#endif
    uint8_t data[TWI_TRACE_SIZE];
#endif
} CTwiTraceBuffer_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SERIAL_DEBUG_TWI_RAW_TRACER_WORD
extern void twi_trace(CTwiTraceBuffer_t* thizz, uint16_t data);
#else
extern void twi_trace(CTwiTraceBuffer_t* thizz, uint8_t data); // write byte 7 bits, and add count if repeating
extern void twi_trace_start(CTwiTraceBuffer_t* thizz); // write byte 7 bits, and add count if repeating
extern void twi_trace_stop(CTwiTraceBuffer_t* thizz); // write byte 7 bits, and add count if repeating
#endif

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
