/*
 * queue.c
 *
 *  Created on: Mar 2, 2021
 *      Author: NNobi
 */

#include "queue.h";

int32_t initQueue(queue_t * q)
{
//    int32_t i;

//    queue_t * q = (queue_t *)malloc(sizeof(queue_t));
//    int32_t ** arr = (int32_t **)malloc(capacity*sizeof(int32_t *));
//    for(i=0;i<MAX_ELEM_ARR_SIZE;i++)
//        arr[i] = (int32_t *)malloc(MAX_ELEM_ARR_SIZE*sizeof(int32_t));

    q->capacity = OUR_MAX_QUEUE_SIZE;
    q->size = 0;
    q->front = 0;
    q->back = q->capacity-1;

    return 0;
}

//int32_t freeQueue(queue_t * q)
//{
//    int32_t i;
//    for(i=0;i<MAX_ELEM_ARR_SIZE;i++)
//        free(q->arr[i]);
//    free(q->arr);
//    free(q);
//
//    return 0;
//}

int32_t enque(queue_t * q, int32_t * vals)
{
    if(qFull(q))
        return -1;

    int32_t i;

    UART_PRINT("start looping for enque\n\r");
    UART_PRINT("%i = (%i + 1) %% %i\n\r", q->back, q->back, q->capacity);
    q->back = (q->back + 1) % OUR_MAX_QUEUE_SIZE;
//    q->back = (q->back + 1) % q->capacity;
    for(i=0;i<MAX_ELEM_ARR_SIZE;i++){
        UART_PRINT("%i\n\r", q->back);
        q->arr[q->back][i] = vals[i];
    }
    q->size++;
    UART_PRINT("end looping for enque\n\r");

    return 0;
}

int32_t * deque(queue_t * q)
{
    if(qEmpty(q))
        return -1;

    int32_t * itemsPtr = q->arr[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    return itemsPtr;
}

int32_t qFull(queue_t * q)
{
    if(q->size==q->capacity)
        return 1;
    return 0;
}

int32_t qEmpty(queue_t * q)
{
    if(q->size==0)
        return 1;
    return 0;
}

int32_t * qFront(queue_t * q)
{
    if(qEmpty(q))
        return -1;
    return q->arr[q->front];
}

int32_t * qBack(queue_t * q)
{
    if(qEmpty(q))
        return -1;
    return q->arr[q->back];
}

int32_t qIndex(queue_t * q, int32_t i)
{
    if(qEmpty(q))
        return -1;
    return (q->front + i) % q->capacity;
}

