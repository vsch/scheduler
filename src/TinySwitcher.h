#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include <Arduino.h>

#define offsetof(p, m)    (&((*(p *)(0)).m))
#define lengthof(p)    (sizeof(p)/sizeof(*(p)))

typedef struct Context {
    // Context data structure
    // This structure holds the context of a thread, including registers and stack pointer.
    uint8_t stackUsed;      // saved stack area
    uint8_t stackMax;       // maximum stack area available
    uint8_t stackMaxUsed;   // max stack space accually used
    void (*pEntry)();
    void *pEntryArg;       // entry point for context
    uint8_t stack[];       // stack storage area begins here
} Context;

typedef void (*EntryFunction)(void *);

#define sizeOfPlus(t, b)    (sizeof(t)+(b))
#define sizeOfStack(s)      (sizeOfPlus(Context, (s)))

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
extern uint8_t isAsyncContext();

/**
 * Resume execution in the async context. Will set current context pointer to be used by potential call to yieldContext
 * NOTE: This function will not return until yieldContext is called or the callee returns from the entry function
 *
 * @param pContext  pointer to async context
 */
extern void resumeContext(Context *pContext);

// will yield back to caller of resumeContext and clear current context pointer so no more yields could be done
extern void yieldContext();

#ifdef __cplusplus
}
#endif

#endif // CONTEXT_SWITCH_H
