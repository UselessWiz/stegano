#include "stegano.h"
#include <stdio.h>
#define MAX_SIZE 5

void initialiseQueue(queue_t *q)
{
    q->front = -1;
    q->back = 0;
}

int isEmpty(queue_t *q)
{
    if (q->front == q->back - 1)
    {
        return 1;
    }
    return 0;
}

int isFull(queue_t *q)
{
    if (q->back == MAX_SIZE)
    {
        return 1;
    }
    return 0;
}

void enqueue(queue_t *q, int value)
{
    if (isFull(q))
    {
        printf("Queue is full\n");
        return;
    }
    q->items[q->back] = value;
    q->back++;
}

void dequeue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }
    q->front++;
}

int peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return -1;
    }
    return q->items[q->front + 1];
}

void printQueue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }

    printf("Current Queue: ");
    int i;
    for (i = q->front + 1; i < q->back; i++)
    {
        printf("%d ", q->items[i]);
    }
    printf("\n");
}
