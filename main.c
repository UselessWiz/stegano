#include "stegano.h"
#include <stdio.h>
#include <string.h>

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
int writeQueueToFile(queue_t *q, const char *filename)
{
    FILE *fptr = fopen(filename, "w");

    if (fptr == NULL)
    {
        printf("ERROR: Could not open '%s' file\n", filename);
        return 0;
    }

    if (isEmpty(q))
    {
        fclose(fptr);
        return 1;
    }

    int i, current = q->front;
    for (i = 0; i < q->count; i++)
    {
        fprintf(fptr, "%s\n", q->items[current]);
        current = (current + 1) % MAX_SIZE;
    }

    fclose(fptr);
    printf("Queue successfully written to '%s' file", filename);
    return 1;
}

int readQueueFromFile(queue_t *q, const char *filename)
{
    FILE *fptr = fopen(filename, "r");

    if (fptr == NULL)
    {
        printf("ERROR: Could not open '%s' file\n", filename);
        return 0;
    }

    initialiseQueue(q);

    char buffer[MAX_STRING_LENGTH];
    while (fgets(buffer, sizeof(buffer), fptr) != NULL)
    {
        if (isFull(q))
        {
            printf("Warning: Queue full, couldn't load all elements\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        enqueue(q, buffer);
    }

    fclose(fptr);
    printf("Queue successfully read from '%s' file\n", filename);
    return 1;
}

char **getRecentFiles(queue_t *q);

/* Should be run at the start of the program, and processes if the program should run in
interactive mode or not (and if not, processes what needs to happen based on cmd instructions) */
void processArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
}
