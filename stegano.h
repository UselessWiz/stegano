#ifndef STEGANO
#define STEGANO
#include <stdio.h>

#define MAX_SIZE 10
#define MAX_STRING_LENGTH 100

#define FILEHEADER_SIZE 14
#define BFTYPE_SIZE 2
#define IMAGEHEADER_SIZE 40
#define HEADER_SIZE (FILEHEADER_SIZE + IMAGEHEADER_SIZE)
#define BITS_PER_BYTE 8
#define RGB_PER_PIXEL 3

#define START_BYTE 0
#define OFFSET_BYTE 10
#define WIDTH_BYTE 18
#define HEIGHT_BYTE 22

#define MAX_MESSAGE_SIZE 256

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
    unsigned char header[HEADER_SIZE];
} image_t;

/* QUEUE */
typedef struct Queue
{
    char items[MAX_SIZE][MAX_STRING_LENGTH];
    int front;
    int back;
    int count;
} queue_t;

typedef struct huffmanNode{
    char ch;
    int freq;
    struct huffmanNode *left, *right;
} huffmanNode_t;

/* Calculates padding needed for each row in the image */
int calcPadding(int width);

/* Checks for correct BMP file type and format */
int checkFileType(char *filename);

/* Read image, check for correct file format */
image_t readImage(char *infile);

/* Encode message into image */
void encode(char *infile, char *outfile, char *message);

/* Decode message from image */
void decode(char *infile, char *outstring);

/* Prepare the given queue to be used initially. */
void initialiseQueue(queue_t *q);

/* Checks if the provided queue is empty */
int isEmpty(queue_t *q);

/* Checks if the provided queue is full */
int isFull(queue_t *q);

/* Places the given item into the given queue */
void enqueue(queue_t *q, char value[]);

/* Removes the last item from the provided queue */
void dequeue(queue_t *q);

/* Returns the item at the front of the given queue */
char *peek(queue_t *q);

void printQueue(queue_t *q);

/* Takes a string in and returns a compressed version of it - most gcclikely with RLE (Sam)*/ 
char* compressMessage(char message[]);

/* Takes a compressed string in and returns the decompressed version of it (Sam) */
char* decompressMessage(const char compressed[], const int freqTable[256], int messageLength);

/*helper functions for compression and decompression*/

/*Counts how many times each byte appears in message and fills the freqTable*/
void buildFrequencyTable(const char message[], int freqTable[256]);

/*Allocates leaves for non-zero freqs and inserts them into nodeList in ascending freq*/
void createSortedNodeList(const int freqTable[256], huffmanNode_t* nodeList[256], int *outSize);

/*Repeatedly merges the two smallest nodes and reinserts the parent until a single root remains*/
huffmanNode_t* buildHuffmanTree(huffmanNode_t* nodeList[256], int size);

/*Recursively free all nodes*/
void freeHuffmanTree(huffmanNode_t* root);

/*Depth First Search assigns codes, 0 = left, 1 = right, storing strings and lengths*/
void buildCode(huffmanNode_t* node, char *path, int depth, char *codeTable[256], int codeLen[256]);

#endif
