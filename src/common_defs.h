
#ifndef SCHEDULER_COMMON_DEFS_H
#define SCHEDULER_COMMON_DEFS_H

#define NULL_BYTE  ((uint8_t)-1)
#define NULL_WORD  ((uint16_t)-1)
#define NULL_TASK  NULL_BYTE

#ifndef offsetof
#define offsetof(p, m)    (&((*(p *)(0)).m))
#endif

#ifndef lengthof
#define lengthof(p)    (sizeof(p)/sizeof(*(p)))
#endif

#ifndef CONSOLE_DEBUG
typedef uint32_t time_t;
#endif

#define sizeOfArray(b, e)    (sizeof(e)*(b))
#define sizeOfPlus(t, b, e)    (sizeof(t)+sizeof(e)*(b))

#ifndef CONSOLE_DEBUG
#ifdef SERIAL_DEBUG_SCHEDULER_CLI
extern const char *pCliFile;
extern uint16_t nCliLine;
extern uint16_t nSeiLine;
#define CLI()   uint8_t oldSREG = SREG; pCliFile = PSTR(__FILE__); nCliLine = __LINE__; cli()
#define CLI_ONLY()   pCliFile = PSTR(__FILE__); nCliLine = __LINE__; cli()
#define SEI()   SREG = oldSREG; nSeiLine = __LINE__
#else
#define CLI()   uint8_t oldSREG = SREG; cli()
#define CLI_ONLY()   cli()
#define SEI()   SREG = oldSREG
#endif
#else
#define CLI()   ((void)0)
#define SEI()   ((void)0)
#endif

#ifdef CONSOLE_DEBUG
#define NO_DISCARD      [[nodiscard]]
#else
#define NO_DISCARD
#endif

#define TO_MICROS(ms)   ((ms) * 1000UL)

#ifdef SERIAL_DEBUG
#define serialDebugPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugPuts_P(...) puts_P(__VA_ARGS__)
#else
#ifdef CONSOLE_DEBUG
#define serialDebugPrintf_P(...) addActualOutput(__VA_ARGS__)
#define serialDebugPuts_P(...) addActualOutput(__VA_ARGS__)
#else
#define serialDebugPrintf_P(...) ((void)0)
#define serialDebugPuts_P(...) ((void)0)
#endif
#endif

#ifdef SERIAL_DEBUG_DUMP
#define serialDebugDumpPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugDumpPuts_P(...) puts_P(__VA_ARGS__)
#else
#ifdef CONSOLE_DEBUG
#define serialDebugDumpPrintf_P(...) addActualOutput(__VA_ARGS__)
#define serialDebugDumpPuts_P(...) addActualOutput(__VA_ARGS__)
#else
#define serialDebugDumpPrintf_P(...) ((void)0)
#define serialDebugDumpPuts_P(...) ((void)0)
#endif
#endif

#ifdef SERIAL_DEBUG_TWI_DATA
#define serialDebugTwiDataPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugTwiDataPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugTwiDataPrintf_P(...) ((void)0)
#define serialDebugTwiDataPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_DAC_MUTEX
#define serialDebugDacMutexPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugDacMutexPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugDacMutexPrintf_P(...) ((void)0)
#define serialDebugDacMutexPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_DETAIL_TWI_STATS
#define serialDebugGfxTwiStatsPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugGfxTwiStatsPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugGfxTwiStatsPrintf_P(...) ((void)0)
#define serialDebugGfxTwiStatsPuts_P(...) ((void)0)
#endif

#ifdef SERIAL_DEBUG_GFX_STATS
#define serialDebugGfxStatsPrintf_P(...) printf_P(__VA_ARGS__)
#define serialDebugGfxStatsPuts_P(...) puts_P(__VA_ARGS__)
#else
#define serialDebugGfxStatsPrintf_P(...) ((void)0)
#define serialDebugGfxStatsPuts_P(...) ((void)0)
#endif

#endif //SCHEDULER_COMMON_DEFS_H
