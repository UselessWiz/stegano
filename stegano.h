#ifndef STEGANO
#define STEGANO
#include <stdio.h>

#define MAX_SIZE 10

/* QUEUE */
typedef struct Queue
{
    int items[MAX_SIZE];
    int front;
    int back;
} queue_t;

/* Prepare the given queue to be used initially. */
void initialiseQueue(queue_t *q);

/* Checks if the provided queue is empty */
int isEmpty(queue_t *q);

/* Checks if the provided queue is full */
int isFull(queue_t *q);

/* Places the given item into the given queue */
void enqueue(queue_t *q, int value);

/* Removes the last item from the provided queue */
void dequeue(queue_t *q);

/* Returns the item at the front of the given queue */
int peek(queue_t *q);

void printQueue(queue_t *q);

#endif