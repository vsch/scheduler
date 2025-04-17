
#ifndef SCHEDULER_COMMON_DEFS_H
#define SCHEDULER_COMMON_DEFS_H

#define offsetof(p, m)    (&((*(p *)(0)).m))
#define lengthof(p)    (sizeof(p)/sizeof(*(p)))
#define sizeOfPlus(t, b)    (sizeof(t)+(b))

#endif //SCHEDULER_COMMON_DEFS_H
