#include "stegano.h"

#define MAXQUEUESIZE 5

struct queue
{
    char **items;
    int front;
    int back;
};
/*This is a test for queue branch*/
