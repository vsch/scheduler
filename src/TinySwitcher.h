#ifndef SCHEDULER_TINYSWITCHER_H
#define SCHEDULER_TINYSWITCHER_H

#ifdef CONSOLE_DEBUG
#include <coroutine>
#include <cstdint>

// Define the type for the entry function of a task
typedef void (*EntryFunction)(void *arg);

// Forward declaration of the coroutine handle type
struct ContextPromise;
using ContextHandle = std::coroutine_handle<ContextPromise>;

// Declare the MainContextPromise structure
struct MainContextPromise;

// Declare the global mainContext handle
extern std::coroutine_handle<MainContextPromise> mainContext;

// Declare the createMainContext function
std::coroutine_handle<MainContextPromise> createMainContext();

// Structure to represent an asynchronous context
struct AsyncContext {
    uint8_t stackUsed;          // Placeholder for stack usage (not used in coroutines)
    uint8_t stackMax;           // Placeholder for maximum stack size (not used in coroutines)
    uint8_t stackMaxUsed;       // Placeholder for maximum stack usage (not used in coroutines)
    ContextHandle coroutineHandle; // Coroutine handle for the task
    EntryFunction entryFunction;   // Entry function for the task
    void *entryArg;               // Argument for the entry function
};

#define sizeOfStack(s)      (sizeOfPlus(AsyncContext, 0, uint8_t))
#else // CONSOLE_DEBUG

#include "Arduino.h"
#include "common_defs.h"

typedef struct AsyncContext {
    // AsyncContext data structure
    // This structure holds the context of a thread, including registers and stack pointer.
    volatile uint8_t stackUsed;      // saved stack area
    volatile uint8_t stackMax;       // maximum stack area available
    volatile uint8_t stackMaxUsed;   // max stack space actually used
    void (*pEntry)();
    void *pEntryArg;       // entry point for context
    uint8_t stack[];       // stack storage area begins here

} AsyncContext;

#define sizeOfStack(s)      (sizeOfPlus(AsyncContext, (s), uint8_t))
#endif // CONSOLE_DEBUG

typedef void (*EntryFunction)(void *);

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

#endif // SCHEDULER_TINYSWITCHER_H
