#ifndef STEGANO
#define STEGANO
#define MAXQUEUESIZE 5

/* QUEUE */
struct queue;
typedef struct queue queue_t;
typedef struct huffmanNode{
    char ch;
    int freq;
    struct huffmanNode *left, *right;
} huffmanNode_t;

struct queue
{
    char** items;
    int front;
    int back;
};

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

/* Takes a string in and returns a compressed version of it - most gcclikely with RLE (Sam)*/ 
char* compressMessage(char message[]);

/* Takes a compressed string in and returns the decompressed version of it (Sam) */
char* decompressMessage(const char compressed[], const int freqTable[256], int messageLength);

/*helper functions for compression and decompression*/

/*Counts how many times each byte appears in message and fills the freqTable*/
static void buildFrequencyTable(const char message[], int freqTable[256]);

/*Allocates leaves for non-zero freqs and inserts them into nodeList in ascending freq*/
static void createSortedNodeList(const int freqTable[256], huffmanNode_t* nodeList[256], int *outSize);

/*Repeatedly merges the two smallest nodes and reinserts the parent until a single root remains*/
static huffmanNode_t* buildHuffmanTree(huffmanNode_t* nodeList[256], int size);

/*Recursively free all nodes*/
static void freeHuffmanTree(huffmanNode_t* root);

/*Depth First Search assigns codes, 0 = left, 1 = right, storing strings and lengths*/
static void buildCode(huffmanNode_t* node, char *path, int depth, char *codeTable[256], int codeLen[256]);

#endif