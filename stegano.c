#include "stegano.h"

#define MAXQUEUESIZE 5

struct queue
{
    char** items;
    int front;
    int back;
};
