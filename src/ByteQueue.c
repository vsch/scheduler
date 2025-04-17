#include "ByteQueue.h"

// Data versions
uint8_t byteQueueData_getCount(const ByteQueueData_t *thizz) {
    return queue_getCount(thizz);
}

uint8_t byteQueueData_isEmpty(const ByteQueueData_t *thizz) {
    return queue_isEmpty(thizz);
}

uint8_t byteQueueData_isFull(const ByteQueueData_t *thizz) {
    return queue_isFull(thizz);
}

uint8_t byteQueueData_peekHead(const ByteQueueData_t *this) {
    uint8_t index = queue_peekHead(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

uint8_t byteQueueData_peekTail(const ByteQueueData_t *this) {
    uint8_t index = queue_peekTail(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

// enqueue/dequeue methods
uint8_t byteQueueData_addTail(ByteQueueData_t *this, uint8_t byte) {
    uint8_t index = queue_addTail(this);
    return index == NULL_BYTE ? NULL_BYTE : (this->data[index] = byte);
}

uint8_t byteQueueData_removeTail(ByteQueueData_t *this) {
    uint8_t index = queue_removeTail(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

uint8_t byteQueueData_addHead(ByteQueueData_t *this, uint8_t byte) {
    uint8_t index = queue_addHead(this);
    return index == NULL_BYTE ? NULL_BYTE : (this->data[index] = byte);
}

uint8_t byteQueueData_removeHead(ByteQueueData_t *this) {
    uint8_t index = queue_removeHead(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

// data pointer versions
uint8_t byteQueuePtr_peekHead(const ByteQueuePtr_t *this) {
    uint8_t index = queue_peekHead(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

uint8_t byteQueuePtr_peekTail(const ByteQueuePtr_t *this) {
    uint8_t index = queue_peekTail(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

// enqueue/dequeue methods
uint8_t byteQueuePtr_addTail(ByteQueuePtr_t *this, uint8_t byte) {
    uint8_t index = queue_addTail(this);
    return index == NULL_BYTE ? NULL_BYTE : (this->data[index] = byte);
}

uint8_t byteQueuePtr_removeTail(ByteQueuePtr_t *this) {
    uint8_t index = queue_removeTail(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

uint8_t byteQueuePtr_addHead(ByteQueuePtr_t *this, uint8_t byte) {
    uint8_t index = queue_addHead(this);
    return index == NULL_BYTE ? NULL_BYTE : (this->data[index] = byte);
}

uint8_t byteQueuePtr_removeHead(ByteQueuePtr_t *this) {
    uint8_t index = queue_removeHead(this);
    return index == NULL_BYTE ? NULL_BYTE : this->data[index];
}

