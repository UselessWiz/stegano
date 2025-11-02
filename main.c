#include "stegano.h"
#include <stdio.h> /* printf, sscanf, fgets, fopen, fprintf, fclose,  */
#include <string.h> /* strcmp, strcpy, strlen, strrchr */

/* ERROR CODES */
#define INVALIDARGUMENTSERROR -1
#define FILENOTFOUNDERROR -2
#define INVALIDINPUTERROR -3

/* MENU OPTIONS*/
#define MENUENCODE 1
#define MENUDECODE 2
#define MENUVIEWRECENT 3
#define MENUEXIT 4

/* CONST PARAMETERS */
#define MAXFILELEN 256
#define MAXMESSAGELEN 256

#define DATAFILE "stegano.dat"

void printMenu(void);
void printHelp(void);
int menuEncodeSelected(queue_t* queue_p);
int menuDecodeSelected(queue_t* queue_p);
int menuViewRecentFiles(queue_t* queue);
void stringInput(char prompt[], int maxResponseLen, char response[]);
int processArgs(int argc, char* argv[], queue_t* queue);
int readQueueFromFile(queue_t *q, const char *filename);
int writeQueueToFile(queue_t *q, const char *filename);

/* - MAIN FUNCTION - */
int main(int argc, char* argv[])
{
    /* If no recently accessed file list exists, use a new queue.*/
    queue_t queue;
    if (readQueueFromFile(&queue, DATAFILE) == FILENOTFOUNDERROR)
    {
        initialiseQueue(&queue); 
    }

    /* If there are any cmd arguments passed, process them and act on them.*/
    if (argc > 1)
    {
        return processArgs(argc, argv, &queue);
    }

    /* Otherwise, run interactively, continuing indefinitely. */
    printf("Stegano - Image steganography in C.\n");
    while (1)
    {
        int input;
        char inputStr[3];

        printMenu();
        
        /* Process user input for menu option handling. */
        printf("Select an option: ");
        fgets(inputStr, 3, stdin);
        sscanf(inputStr, "%1d", &input);
        
        switch (input)
        {
            case MENUEXIT:
                /* Save recently accessed files. */
                writeQueueToFile(&queue, DATAFILE);
                return 0;
            case MENUENCODE:
                menuEncodeSelected(&queue);
                break;
            case MENUDECODE:
                menuDecodeSelected(&queue);
                break;
            case MENUVIEWRECENT:
                menuViewRecentFiles(&queue);
                break;
            default:
                printf("Invalid item.");
                break;
        }
    }
    return 0;
}

/* 
Processes the commandline arguments passed to the program.

Should be run at the start of the program, and processes if the program should 
run in interactive mode or not (and if not, processes what needs to happen 
based on cmd instructions)

Note that this assumes that the expected order of arguments is followed.
Parameters:
    - argc (int): the number of arguments passed.
    - argv (char**): an array of pointers to where those arguments 
    are stored in memory
    - queue_p (queue_t*): a pointer to the queue used by the application.

Returns (int):
    The status of argument processing. 0 if everything is successful, 
    < 0 if there is an error.
*/
int processArgs(int argc, char* argv[], queue_t* queue_p)
{
    /* Help argument */
    if (strcmp(argv[1], "-h") == 0)
    {
        printHelp();
        return 0;
    }

    /* stegano -e -i input.bmp -o output.bmp -m "Test Message" */
    if (strcmp(argv[1], "-e") == 0)
    {
        /* Find all other arguments */
        if (argc < 8 || \
            !(strcmp(argv[2], "-i") == 0 && strcmp(argv[4], "-o") == 0 && \
            strcmp(argv[6], "-m") == 0))
        {
            printf("Invalid flag, please check and try again.");
            printHelp();
            return INVALIDARGUMENTSERROR;
        }

        /* Add the output file to queue. */
        enqueue(queue_p, argv[5]);

        int validFile = checkFileType(argv[3]);
        if (validFile == 0)
        {
            encode(argv[3], argv[5], argv[7]);
        }   
        else 
        {
            return INVALIDINPUTERROR;
        }
        return 0;
    }

    /* stegano -d -i input.bmp [-o fileOutput.txt]*/
    else if (strcmp(argv[1], "-d") == 0)
    {
        if ((argc < 4 && strcmp(argv[2], "-i") != 0) || \
            (argc < 6 && strcmp(argv[2], "-i") != 0 && \
            strcmp(argv[4], "-o") != 0))
        {
            printf("Invalid flag, please check and try again.");
            printHelp();
            return INVALIDARGUMENTSERROR;
        }

        char message[MAXMESSAGELEN];
        decode(argv[3], message);

        /* Assume outfile was passed */
        if (argc >= 5)
        {
            FILE* file = fopen(argv[5], "w+");
            fprintf(file, "%s", message);
            fclose(file);
            enqueue(queue_p, argv[5]); /* Adds the output file to the queue. */
        }
        else 
        {
            enqueue(queue_p, argv[3]); /* Adds teh input file to the queue. */
        }

        return 0;
    }

    /* If you make it here, assume that the arguments weren't valid. */
    printHelp();
    return INVALIDARGUMENTSERROR;
}

/*
Prints the help text. This is a static string that doesn't change and lists all
command line options and usecases.

Parameters:
    - void

Returns:
    void
*/
void printHelp(void)
{
    printf("Stegano - Image steganography in C.\n" \
    "Options:\n" \
    "\t-e: Encode the given message into the provided input file, placing " \
    "the result into the output file. Requires -i, -o and -m flags.\n" \
    "\t-d: Decode a message hidden within the given image. Requires the -i " \
    "flag and optionally takes the -o flag to output the decoded message " \
    "into a text file.\n" \
    "\t-i [filename]: The input file. This must be an image in BMP format.\n" \
    "\t-o [filename]: The output file. This can be any file type, but it's" \
    "recommended that when encoding the output file is a .bmp file and when" \
    "decoding this is a .txt file.\n" \
    "\t-m [message]: The message to hide in the image.\n"
    "\t-h: Displays this help message.\n\n" \
    "If no flags are provided, the program will run in interactive mode.\n" \
    "Note that order matters when flags are used\n");
}

/*
When in interactive mode, prints the main menu which displays the options that 
users can choose in interactive mode.

Parameters:
    - void

Returns
    void
*/
void printMenu(void)
{
    printf("1. Encode a message into an image\n" \
    "2. Decode a message from an image\n" \
    "3. View recent files\n" \
    "4. Exit\n");
}

/*
Processes what should happen when the encode option is selected from the menu
in interactive mode.

Parameters:
    - queue_p (queue_t*): a pointer to the application's queue.

Returns (int):
    - The status of the corresponding encode function call. 0 if encoding was
    successful, < 0 if not.
*/
int menuEncodeSelected(queue_t* queue_p)
{
    char infile[MAXFILELEN];
    stringInput("What input file should we use (this should be a BMP image): "\
        , MAXFILELEN, infile);

    char outfile[MAXFILELEN];
    stringInput("What should we call the new file (leave blank for default): "\
        , MAXFILELEN, outfile);

    char message[MAXMESSAGELEN];
    stringInput("What mesasge would you like to encode into the image? ", \
        MAXMESSAGELEN, message);

    if (outfile[0] == '\0')
    {
        /* Use a default file name if no file name is entered */
        strcpy(outfile, "encoded.bmp");
    }

    enqueue(queue_p, outfile);

    printf("\n");

    int validFile = checkFileType(infile);
    if (validFile == 0)
    {
        encode(infile, outfile, message);
    }   
    else 
    {
        return INVALIDINPUTERROR;
    }
    
    return 0;
}

/*
Processes what should happen when the decode option is selected from the menu
in interactive mode.

Parameters:
    - queue_p (queue_t*): a pointer to the application's queue.

Returns (int):
    - The status of the corresponding decode function call. 0 if encoding was
    successful, > 0 if not.
*/
int menuDecodeSelected(queue_t* queue_p)
{
    char message[MAXMESSAGELEN];

    char infile[MAXFILELEN];
    stringInput("What input file should we use (this should be a BMP image): "\
        , MAXFILELEN, infile);

    char outfile[MAXFILELEN];
    stringInput("What should we call the new file (leave blank to display " \
        "message in the terminal): ", MAXFILELEN, outfile);

    decode(infile, message);

    if (outfile[0] == '\0')
    {
        printf("Resulting Message: %s\n", message);
    }
    else 
    {
        FILE* file = fopen(outfile, "w+");
        fprintf(file, "%s", message);
        fclose(file);
    }

    /* If an outfile was requested, queue it. Otherwise, queue the input file. 
    This corresponds to the most recent file accessed by the application.*/
    if (outfile[0] != '\0')
    {
        enqueue(queue_p, outfile);
    }
    else
    {
        enqueue(queue_p, infile);
    }

    printf("\n");

    return 0;
}

/*
Processes what should happen when the menu option to select recent files is 
picked.

Parameters:
    - queue_p (queue_t*): a pointer to the queue used by the application.
*/
int menuViewRecentFiles(queue_t* queue_p)
{
    if (isEmpty(queue_p))
    {
        printf("No Recent Files.\n");
        return 0;
    }

    printf("Recent Files:\n");
    printQueue(queue_p);
    printf("\n");
    return 0;
}

/*
A helper function to easily get string input from the user when the program is
running interactively. 

Parameters:
    - prompt (char[]): A short message describing the field the user is 
    providing a response to.
    - maxResponseLen (int): The maximum length a response can be. Used to 
    prevent buffer overflows.
    - response (char[]): A buffer to store the user's response in. Terminated
    by a null character.
*/
void stringInput(char prompt[], int maxResponseLen, char response[])
{
    printf("%s", prompt);
    
    fgets(response, maxResponseLen, stdin);

    /* Remove the newline from the string */
    if (strlen(response) > 0)
    {
        char* nl = strrchr(response, '\n');
        *nl = '\0';
    }
}

int writeQueueToFile(queue_t *q, const char *filename)
{
    FILE *fptr = fopen(filename, "w");

    if (fptr == NULL)
    {
        return FILENOTFOUNDERROR;
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
    return 0;
}

int readQueueFromFile(queue_t *q, const char *filename)
{
    FILE *fptr = fopen(filename, "r");

    if (fptr == NULL)
    {
        return FILENOTFOUNDERROR;
    }

    initialiseQueue(q);

    char buffer[MAXFILELEN];
    while (fgets(buffer, sizeof(buffer), fptr) != NULL)
    {
        if (isFull(q))
        {
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        enqueue(q, buffer);
    }

    fclose(fptr);
    return 0;
}
