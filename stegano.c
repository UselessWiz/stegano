#include "stegano.h"
#include <stdio.h>

void initialiseQueue(queue_t *q)
{
    q->front = 0;
    q->back = 0;
    q->count = 0;
}

int isEmpty(queue_t *q)
{
    if (q->count == 0)
    {
        return 1;
    }
    return 0;
}

int isFull(queue_t *q)
{
    if (q->count == MAX_SIZE)
    {
        return 1;
    }
    return 0;
}

void enqueue(queue_t *q, char value)
{
    if (isFull(q))
    {
        printf("Queue is full\n");
        return;
    }
    q->items[q->back] = value;
    q->back = (q->back + 1) % MAX_SIZE;
    q->count++;
}

void dequeue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }
    q->front = (q->front + 1) % MAX_SIZE;
    q->count--;
}

char peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return -1;
    }
    return q->items[q->front];
}

void printQueue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }

    printf("Current Queue: ");
    int i, current = q->front;
    for (i = 0; i < q->count; i++)
    {
        printf("%c ", q->items[current]);
        current = (current + 1) % MAX_SIZE;
    }
    printf("\n");
}
