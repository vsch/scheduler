#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include <Arduino.h>
#include "common_defs.h"

typedef struct AsyncContext {
    // AsyncContext data structure
    // This structure holds the context of a thread, including registers and stack pointer.
    volatile uint8_t stackUsed;      // saved stack area
    volatile uint8_t stackMax;       // maximum stack area available
    volatile uint8_t stackMaxUsed;   // max stack space actually used
    volatile void (*pEntry)();
    volatile void *pEntryArg;       // entry point for context
    volatile uint8_t stack[];       // stack storage area begins here

} Context;

typedef void (*EntryFunction)(void *);

#define sizeOfStack(s)      (sizeOfPlus(AsyncContext, (s)))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the async context
 * NOTE: EntryFunction takes an optional argument so that C++ member functions can be used as entry functions,
 *     through a static function wrapper
 * @param pContext          async context
 * @param pEntry            pointer to entry function
 * @param pEntryArg         entry function argument
 * @param contextSize       number of bytes allocated to the pContext buffer. NOTE: use sizeOfStack() to declare buffer for the context
 */
extern void initContext(uint8_t *pContext, EntryFunction pEntry, void *pEntryArg, uint8_t contextSize);

/**
 *
 * test if in an async context
 *
 * @return  0 if not, otherwise we are
 */
extern uint8_t isInAsyncContext();

/**
 * Resume execution in the async context. Will set current context pointer to be used by potential call to yieldContext
 * NOTE: This function will not return until yieldContext is called or the callee returns from the entry function
 *
 * @param pContext  pointer to async context
 */
extern void resumeContext(AsyncContext *pContext);

// will yield back to caller of resumeContext and clear current context pointer so no more yields could be done
extern void yieldContext();

#ifdef __cplusplus
}
#endif

#endif // CONTEXT_SWITCH_H
