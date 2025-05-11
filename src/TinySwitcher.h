// TinySwitcher.h
#ifndef SCHEDULER_TINYSWITCHER_H
#define SCHEDULER_TINYSWITCHER_H

#ifdef CONSOLE_DEBUG
#include <boost/context/fiber.hpp>
#include <boost/context/continuation.hpp>
#include <cstdint>
#include <functional>

namespace ctx = boost::context;

typedef void (*EntryFunction)(void *arg);

struct AsyncContext {
    uint8_t stackUsed;
    uint8_t stackMax;
    uint8_t stackMaxUsed;
    EntryFunction entryFunction;
    void *entryArg;
    //ctx::fiber fiber;      // Boost fiber for context switching
    boost::context::continuation continuation;
    boost::context::continuation caller;
};

#define sizeOfStack(s)      (sizeof(AsyncContext))

#else // CONSOLE_DEBUG

#include "Arduino.h"
#include "common_defs.h"

typedef struct AsyncContext {
    volatile uint8_t stackUsed;
    volatile uint8_t stackMax;
    volatile uint8_t stackMaxUsed;
    void (*pEntry)();
    void *pEntryArg;
    uint8_t stack[];
} AsyncContext;

#define sizeOfStack(s)      (sizeOfPlus(AsyncContext, (s), uint8_t))
#endif // CONSOLE_DEBUG

#ifdef __cplusplus

extern "C" {
#endif

extern AsyncContext *initContext(void *pContextBuff, EntryFunction entryFunction, void *entryArg, uint16_t stackSize);
extern uint8_t isInAsyncContext();
extern void resumeContext(AsyncContext *pContext);
extern void yieldContext();

#ifdef __cplusplus
}
#endif

#endif // SCHEDULER_TINYSWITCHER_H
