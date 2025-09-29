#ifndef STEGANO
#define STEGANO

/* QUEUE */
struct queue;
typedef struct queue queue_t;

/* Prepare the given queue to be used initially. */
void intitialiseQueue(queue_t* queue);

/* Checks if the provided queue is empty */
int isQueueEmpty(queue_t* queue);

/* Checks if the provided queue is full */
int isQueueFull(queue_t* queue);

/* Places the given item into the given queue */
int enqueue(queue_t* queue, char item[]);

/* Removes the last item from the provided queue */
int dequeue(queue_t* queue);

/* Returns the item at the front of the given queue */
char* peek(queue_t* queue);

#endif