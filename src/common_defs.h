
#ifndef SCHEDULER_COMMON_DEFS_H
#define SCHEDULER_COMMON_DEFS_H

#define INFINITE_DELAY  (-1)
#define NULL_BYTE  ((uint8_t)-1)
#define NULL_WORD  ((uint16_t)0) // for words use 0 so compatible with storing NULL as no value
#define NULL_TASK  NULL_BYTE

#define offsetof(p, m)    (&((*(p *)(0)).m))
#define lengthof(p)    (sizeof(p)/sizeof(*(p)))
#define sizeOfPlus(t, b, e)    (sizeof(t)+sizeof(e)*(b))

typedef uint8_t TaskId_t;

#ifdef __cplusplus
class Task;
class SharedRequestManager;
typedef Task *pTask_t;
typedef SharedRequestManager *pSharedRequestManager_t;
#else
// for C based functions
typedef void *pTask_t;
typedef void *pCommonRequestManager_t;
#endif

#endif //SCHEDULER_COMMON_DEFS_H
