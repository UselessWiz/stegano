#include "stegano.h"
#include <stdio.h>
#include <string.h>

/*
Sets all values in the queue to 0
This function is used before adding any values to a new queue or to reset all values in a queue

Parameters:

q (queue_t*): a pointer to the designated queue.

Returns (void):

Nothing is returned
*/
void initialiseQueue(queue_t *q)
{
    q->front = 0;
    q->back = 0;
    q->count = 0;
}

/*
Checks if there are any values in the current queue

Parameters:

q (queue_t*): a pointer to the designated queue.

Returns (int):

Whether the queue is empty or not
0 - Not empty
1 - Empty
*/
int isEmpty(queue_t *q)
{
    if (q->count == 0)
    {
        return 1; /**Return 1 if empty */
    }
    return 0; /**Return 0 if not empty */
}

/*
Checks if the queue has reached the maximum amount of elements
Max is set by developer - Located in stegano.h (MAX_SIZE)

Parameters:

q (queue_t*): a pointer to the designated queue.
Returns (int):

Whether the function succesfully writes to the designated file
0 - Not full
1 - Full
*/
int isFull(queue_t *q)
{
    if (q->count == MAX_SIZE)
    {
        return 1; /**Return 1 if full */
    }
    return 0;
}

/*
Adds a new element to the back of the queue
The element would be most recent file that got used

Parameters:

q (queue_t*): a pointer to the application's queue.
value[] (char): rray of characters (string) added to the queue

Returns (void):

Nothing is returned
*/
void enqueue(queue_t *q, char value[])
{
    if (isFull(q))
    {
        printf("Queue is full\n");
        return;
    }
    strcpy(q->items[q->back], value);
    q->back = (q->back + 1) % MAX_SIZE;
    q->count++;
}

/*
Removes the element that is currently at the front of the queue (First In First Out)

Parameters:

q (queue_t*): a pointer to the application's queue.

Returns (void):

Nothing is returned
*/
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

/*
Peeks at the element that is currently at the front of the queue

Parameters:

q (queue_t*): a pointer to the application's queue.

Returns (char*):

Points to the value at the front of the queue
*/
char *peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return NULL;
    }
    return q->items[q->front];
}

/*
Prints all values in the queue starting at the front until the back is reached

Parameters:

q (queue_t*): a pointer to the application's queue.

Returns (void):

Nothing is returned
All values are printed to the terminal
*/
void printQueue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }

    printf("Current Queue: ");
    int i, current = q->front;
    for (i = 0; i < q->count; i++) /* Iterating through the queue */
    {
        printf("%s ", q->items[current]);
        current = (current + 1) % MAX_SIZE;
    }
    printf("\n");
}
