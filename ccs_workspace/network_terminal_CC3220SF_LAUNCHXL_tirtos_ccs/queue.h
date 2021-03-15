/*
 * queue.h
 *
 *  Created on: Mar 2, 2021
 *      Author: NNobi
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#define OUR_MAX_QUEUE_SIZE          300
#define MAX_ELEM_ARR_SIZE           5

#include <stdint.h>
#include "network_terminal.h"

typedef struct{
    int32_t front, back, size;
    uint32_t capacity;
    int32_t arr[OUR_MAX_QUEUE_SIZE][MAX_ELEM_ARR_SIZE];
} queue_t;

int32_t initQueue(queue_t * q);

//int32_t freeQueue(queue_t * q);

int32_t enque(queue_t * q, int32_t * vals);

int32_t * deque(queue_t * q);

int32_t qFull(queue_t * q);

int32_t qEmpty(queue_t * q);

int32_t * qFront(queue_t * q);

int32_t * qBack(queue_t * q);

int32_t qIndex(queue_t * q, int32_t i);


#endif /* QUEUE_H_ */
