#include "stegano.h"
#include <stdio.h>

/* Takes an input file (.bmp image), an output file name to create and the message the user wishes to compress
and encode into the image. Outputs a status based on if the image was successfully encoded or not. (Khanh) */
int encode(char infile[], char outfile[], char message[]);

/* Takes an input .bmp file and places the decoded message into the outstring char array.
Outputs a status based on if the image was successfully encoded or not. (Khanh) */
int decode(char infile[], char outstring[]);

/* Simple check to see if the provided filename ends in ".bmp\0". If it does, all good, if not warn the
user that there might be errors and ask if they want to proceed. Can also check first two bytes for
0x42 0x4D as per format standard. Only prompt in interactive mode */
int checkFileType(char filename[]);

/* Takes a string in and returns a compressed version of it - most likely with RLE (Sam)*/
char *compressMessage(char message[]);

/* Takes a compressed string in and returns the decompressed version of it (Sam) */
char *decompressMessage(char compressed[]);

/* Using the queue structure, get recently accessed files in order */
char **getRecentFiles(queue_t *q);

void printMenu(void);

/* Should be run at the start of the program, and processes if the program should run in
interactive mode or not (and if not, processes what needs to happen based on cmd instructions) */
void processArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    printf("--Testing Queue--\n");
    queue_t q;
    initialiseQueue(&q);

    enqueue(&q, 'q');
    printQueue(&q);
    enqueue(&q, '2');
    printQueue(&q);
    enqueue(&q, '3');
    printQueue(&q);
    enqueue(&q, '4');
    printQueue(&q);
    enqueue(&q, '5');
    printQueue(&q);
    enqueue(&q, '6');
    printQueue(&q);
    dequeue(&q);
    printf("Removed element\n");
    dequeue(&q);
    printf("Removed element\n");
    printQueue(&q);
    enqueue(&q, '7');
    printQueue(&q);
    enqueue(&q, '8');
    printQueue(&q);
    enqueue(&q, '1');
    printQueue(&q);
    enqueue(&q, '2');
    printQueue(&q);
    dequeue(&q);
    printf("Removed element\n");
    printQueue(&q);

    return 0;
}