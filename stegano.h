#ifndef STEGANO
#define STEGANO
#include <stdio.h>

#define FILEHEADER_SIZE 14
#define BFTYPE_SIZE 2
#define IMAGEHEADER_SIZE 40
#define HEADER_SIZE (FILEHEADER_SIZE + IMAGEHEADER_SIZE)
#define BITS_PER_BYTE 8
#define RGB_PER_PIXEL 3

#define MAX_MESSAGE_SIZE 256

#define MAX_SIZE 10

/***** Encode, decode *****/
typedef struct {
    unsigned char bfType[BFTYPE_SIZE];
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} fileheader_t;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} imageheader_t;

typedef struct {
    unsigned char red, green, blue;
} rgb_t;

typedef struct {
    int width;
    int height;
    unsigned int offset;
    rgb_t *rgb;
} image_t;

/* QUEUE */
typedef struct Queue
{
    char items[MAX_SIZE];
    int front;
    int back;
    int count;
} queue_t;

/* Read image's binary information */
image_t readImage(char *infile);

/* Encode message */
void encode(char *infile, char *outfile, char *message);

/* Decode message */
void decode(char *infile, char *outstring);

/* Prepare the given queue to be used initially. */
void initialiseQueue(queue_t *q);

/* Checks if the provided queue is empty */
int isEmpty(queue_t *q);

/* Checks if the provided queue is full */
int isFull(queue_t *q);

/* Places the given item into the given queue */
void enqueue(queue_t *q, char value);

/* Removes the last item from the provided queue */
void dequeue(queue_t *q);

/* Returns the item at the front of the given queue */
char peek(queue_t *q);

void printQueue(queue_t *q);

#endif
