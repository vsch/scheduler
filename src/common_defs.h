
#ifndef SCHEDULER_COMMON_DEFS_H
#define SCHEDULER_COMMON_DEFS_H

#define INFINITE_DELAY  ((uint16_t)-1)
#define NULL_BYTE  ((uint8_t)-1)
#define NULL_WORD  ((uint16_t)-1)
#define NULL_TASK  NULL_BYTE

#ifndef offsetof
#define offsetof(p, m)    (&((*(p *)(0)).m))
#endif

#ifndef lengthof
#define lengthof(p)    (sizeof(p)/sizeof(*(p)))
#endif

#define sizeOfArray(b, e)    (sizeof(e)*(b))
#define sizeOfPlus(t, b, e)    (sizeof(t)+sizeof(e)*(b))

#endif //SCHEDULER_COMMON_DEFS_H
