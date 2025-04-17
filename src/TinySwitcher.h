#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include <Arduino.h>

#define offsetof(p,m)    (&((*(p *)(0)).m))
#define lengthof(p)    (sizeof(p)/sizeof(*(p)))

typedef struct Context {
    // Context data structure
    // This structure holds the context of a thread, including registers and stack pointer.
    uint8_t stackUsed;      // saved stack area
    uint8_t stackMax;       // maximum stack area available
    uint8_t stackMaxUsed;   // max stack space accually used
    void (*pEntry)();
    void *pEntryArg;       // entry point for context
    uint8_t *pStack;        // stack storage area
} Context;

typedef void (*EntryFunction)(void *);

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: EntryFunction takes an optional argument so that C++ member functions can be used as entry functions,
//     through a static function wrapper
extern void initContext(Context *pContext, EntryFunction pEntry, void *pEntryArg, uint8_t *pStack, uint8_t stackMax);

/**
 *
 * test if in an async context
 *
 * @return  0 if not, otherwise we are
 */
extern uint8_t isAsyncContext();

// resume execution per context. Will set current context pointer to be used by potential call to yieldContext
// NOTE: This function will not return until yieldContext is called or the callee returns from the entry function
extern void resumeContext(Context *pContext);

// will yield back to caller of resumeContext and clear current context pointer so no more yields could be done
extern void yieldContext();

#ifdef __cplusplus
}
#endif

#endif // CONTEXT_SWITCH_H
