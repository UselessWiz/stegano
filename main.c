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

/*
Writes the elements in the application's queue into a .txt file.
The .txt file is specified by the user
Checks whether the queue is empty before writing into the file

Parameters:

q (queue_t*): a pointer to the application's queue.
filename (const char*): a pointer to the designated file

Returns (int):

Whether the function succesfully writes to the designated file
0 - Unsuccessful
1 - Successful
*/
int writeQueueToFile(queue_t *q, const char *filename)
{
    FILE *fptr = fopen(filename, "w"); /* Opens the designated file and assigns it to fptr */

    if (fptr == NULL) /* Check if the file has opened succesfully */
    {
        printf("ERROR: Could not open '%s' file\n", filename);
        return 0; /* Return 0 is not opened succesfully */
    }

    if (isEmpty(q)) /* Check if there is anything stored in the Queue array*/
    {
        fclose(fptr); /* Successfull, clears the existing file of all data*/
        return 1;
    }

    int i, current = q->front;     /* Initialise i (for loop), Initialise current to front of queue */
    for (i = 0; i < q->count; i++) /* Run until reaching end */
    {
        fprintf(fptr, "%s\n", q->items[current]); /* Writes the current value into the designated file */
        current = (current + 1) % MAX_SIZE;       /* Increments current to the next position in the queue*/
    }

    fclose(fptr);                                                /* Close */
    printf("Queue successfully written to '%s' file", filename); /* Message to user to ensure successful file writing*/
    return 1;
}

/*
Reads the elements from the designated .txt file and loads the applications queue with that data
The .txt file is specified by the user

Parameters:

q (queue_t*): a pointer to the application's queue.
filename (const char*): a pointer to the designated file

Returns (int):

Whether the function succesfully writes to the designated file
0 - Unsuccessful
1 - Successful
*/
int readQueueFromFile(queue_t *q, const char *filename)
{
    FILE *fptr = fopen(filename, "r"); /* Opens the designated file and assigns it to fptr */

    if (fptr == NULL) /* Check if the file has opened succesfully */
    {
        printf("ERROR: Could not open '%s' file\n", filename);
        return 0; /* Return 0 is not opened succesfully */
    }

    initialiseQueue(q); /* Clears queue and reinitialises all values to = 0*/

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

void printMenu(void)
{
    printf("\n=== STRING QUEUE MENU ===\n");
    printf("1. Enqueue string\n");
    printf("2. Dequeue\n");
    printf("3. Print all\n");
    printf("4. Peek\n");
    printf("5. Save all\n");
    printf("6. Read file\n");
    printf("999. Exit\n");
    printf("Please enter option: ");
}

/* Should be run at the start of the program, and processes if the program should run in
interactive mode or not (and if not, processes what needs to happen based on cmd instructions) */
void processArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    queue_t q;
    initialiseQueue(&q);

    int choice;
    char input[MAX_STRING_LENGTH];
    char filename[100];

    printf("Welcome to String Queue Manager!\n");
    printf("Now you can store complete strings like filenames!\n");

    while (1)
    {
        printMenu();
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            if (isFull(&q))
            {
                printf("Queue is full! Cannot add more elements.\n");
            }
            else
            {
                printf("Enter string to add: ");
                scanf("%s", input);
                enqueue(&q, input);
                printf("Added \"%s\" to queue\n", input);
            }
            break;

        case 2:
            if (isEmpty(&q))
            {
                printf("Queue is empty! Cannot remove elements.\n");
            }
            else
            {
                char *removed = peek(&q);
                printf("Removed \"%s\" from queue\n", removed);
                dequeue(&q);
            }
            break;

        case 3:
            printf("\n");
            printQueue(&q);
            break;

        case 4:
            if (isEmpty(&q))
            {
                printf("Queue is empty! Nothing to peek at.\n");
            }
            else
            {
                char *frontElement = peek(&q);
                printf("Front element is: \"%s\"\n", frontElement);
            }
            break;

        case 5:
            printf("Enter filename to save to: ");
            scanf("%s", filename);
            writeQueueToFile(&q, filename);
            break;

        case 6:
            printf("Enter filename to load from: ");
            scanf("%s", filename);
            readQueueFromFile(&q, filename);
            break;

        case 999:
            printf("Goodbye!\n");
            return 0;

        default:
            printf("Invalid option! Please try again.\n");
            break;
        }
    }

    return 0;
}
