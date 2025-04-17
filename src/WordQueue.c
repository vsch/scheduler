#include "WordQueue.h"

// Data versions
uint8_t wordQueueData_getCount(const WordQueueData_t *thizz) {
    return queue_getCount(thizz);
}

uint8_t wordQueueData_isEmpty(const WordQueueData_t *thizz) {
    return queue_isEmpty(thizz);
}

uint8_t wordQueueData_isFull(const WordQueueData_t *thizz) {
    return queue_isFull(thizz);
}

uint16_t wordQueueData_peekHead(const WordQueueData_t *this) {
    uint8_t index = queue_peekHead(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

uint16_t wordQueueData_peekTail(const WordQueueData_t *this) {
    uint8_t index = queue_peekTail(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

// enqueue/dequeue methods
uint16_t wordQueueData_addTail(WordQueueData_t *this, uint16_t word) {
    uint8_t index = queue_addTail(this);
    return index == NULL_BYTE ? NULL_WORD : (this->data[index] = word);
}

uint16_t wordQueueData_removeTail(WordQueueData_t *this) {
    uint8_t index = queue_removeTail(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

uint16_t wordQueueData_addHead(WordQueueData_t *this, uint16_t word) {
    uint8_t index = queue_addHead(this);
    return index == NULL_BYTE ? NULL_WORD : (this->data[index] = word);
}

uint16_t wordQueueData_removeHead(WordQueueData_t *this) {
    uint8_t index = queue_removeHead(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

// data pointer versions
uint16_t wordQueuePtr_peekHead(const WordQueuePtr_t *this) {
    uint8_t index = queue_peekHead(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

uint16_t wordQueuePtr_peekTail(const WordQueuePtr_t *this) {
    uint8_t index = queue_peekTail(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

// enqueue/dequeue methods
uint16_t wordQueuePtr_addTail(WordQueuePtr_t *this, uint16_t word) {
    uint8_t index = queue_addTail(this);
    return index == NULL_BYTE ? NULL_WORD : (this->data[index] = word);
}

uint16_t wordQueuePtr_removeTail(WordQueuePtr_t *this) {
    uint8_t index = queue_removeTail(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

uint16_t wordQueuePtr_addHead(WordQueuePtr_t *this, uint16_t word) {
    uint8_t index = queue_addHead(this);
    return index == NULL_BYTE ? NULL_WORD : (this->data[index] = word);
}

uint16_t wordQueuePtr_removeHead(WordQueuePtr_t *this) {
    uint8_t index = queue_removeHead(this);
    return index == NULL_BYTE ? NULL_WORD : this->data[index];
}

