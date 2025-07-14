
#ifndef SCHEDULER_DEBUG_CONFIG_H
#define SCHEDULER_DEBUG_CONFIG_H

#ifdef CONSOLE_DEBUG
#define printf_P(...)    addActualOutput(__VA_ARGS__)
#else

#include <stdio.h>

#endif

#ifdef RESOURCE_TRACE
#define resourceTracePrintf_P(...) printf_P(__VA_ARGS__)
#define resourceTracePuts_P(...) puts_P(__VA_ARGS__)
#else
#define resourceTracePrintf_P(...) ((void)0)
#define resourceTracePuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_RESOURCE_TRACE
#define serialDebugResourceTracePrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugResourceTracePuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugResourceTracePrintf_P(...) ((void)0)
#define serialDebugResourceTracePuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_RESOURCE_DETAIL_TRACE
#define serialDebugResourceDetailTracePrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugResourceDetailTracePuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugResourceDetailTracePrintf_P(...) ((void)0)
#define serialDebugResourceDetailTracePuts_P(...) ((void)0)
#endif

#endif //SCHEDULER_DEBUG_CONFIG_H
