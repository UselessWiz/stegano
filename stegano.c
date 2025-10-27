#include "stegano.h"
#include <stdio.h>

/**Set all values to 0 in Struct */
void initialiseQueue(queue_t *q)
{
    q->front = 0;
    q->back = 0;
    q->count = 0;
}

//**Check to see if Queue is empty*/
int isEmpty(queue_t *q)
{
    if (q->count == 0)
    {
        return 1; /**Return 1 if empty */
    }
    return 0; //**Return 0 if not empty */
}

//**Check to see if Queue is full */
int isFull(queue_t *q)
{
    if (q->count == MAX_SIZE)
    {
        return 1; //**Return 1 if full */
    }
    return 0;
}
//**Adds a new element to the queue */
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

//**Removes the top element from the queue (First in First out)*/
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

/**Gets the top element from the queue */
char peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return -1;
    }
    return q->items[q->front];
}

/**Prints the whole queue starting at first added element until last added element */
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
