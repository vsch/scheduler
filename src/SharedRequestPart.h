#ifndef SCHEDULER_SHAREDREQUESTPART_H
#define SCHEDULER_SHAREDREQUESTPART_H

typedef struct SharedRequestPart {
    uint8_t nTotal;         // total amount to be sent for this request
    uint8_t nSize;          // total amount to be sent for this request part, if nSize < nTotal, then there are more parts to the request
    uint8_t *pData;         // pointer to the current part of the data, contiguous
} SharedRequestPart_t;

#endif //SCHEDULER_SHAREDREQUESTPART_H
