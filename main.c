#include "stegano.h"
#include <stdio.h> /* printf, sscanf, fgets, fopen, fprintf, fclose,  */
#include <string.h> /* strcmp, strcpy, strlen, strrchr */

/* ERROR CODES */
#define INVALIDARGUMENTSERROR -1

/* MENU OPTIONS*/
#define MENUENCODE 1
#define MENUDECODE 2
#define MENUEXIT 3

/* CONST PARAMETERS */
#define MAXFILELEN 256
#define MAXMESSAGELEN 256

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
void printHelp(void);
int menuEncodeSelected(void);
int menuDecodeSelected(void);
void stringInput(char prompt[], int maxResponseLen, char response[]);
int processArgs(int argc, char* argv[]);

/* - MAIN FUNCTION - */
int main(int argc, char* argv[])
{
    /* If there are any cmd arguments passed, process them and act on them.*/
    if (argc > 1)
    {
        return processArgs(argc, argv);
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
                return 0;
            case MENUENCODE:
                menuEncodeSelected();
                break;
            case MENUDECODE:
                menuDecodeSelected();
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

Returns (int):
    The status of argument processing. 0 if everything is successful, 
    < 0 if there is an error.
*/
int processArgs(int argc, char* argv[])
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

        return encode(argv[3], argv[5], argv[7]);
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
        int result = decode(argv[3], message);

        /* Assume outfile was passed */
        if (result == 0 && argc >= 5)
        {
            FILE* file = fopen(argv[5], "w+");
            fprintf(file, "%s", message);
            fclose(file);
        }

        return result;
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
    printf("1. Encode a Message into an Image\n" \
    "2. Decode a message from an Image\n" \
    "3. Exit\n");
}

/*
Processes what should happen when the encode option is selected from the menu
in interactive mode.

Parameters:
    - void

Returns (int):
    - The status of the corresponding encode function call. 0 if encoding was
    successful, > 0 if not.
*/
int menuEncodeSelected(void)
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

    return encode(infile, outfile, message);
}

/*
Processes what should happen when the decode option is selected from the menu
in interactive mode.

Parameters:
    - void

Returns (int):
    - The status of the corresponding decode function call. 0 if encoding was
    successful, > 0 if not.
*/
int menuDecodeSelected(void)
{
    char message[MAXMESSAGELEN];

    char infile[MAXFILELEN];
    stringInput("What input file should we use (this should be a BMP image): "\
        , MAXFILELEN, infile);

    char outfile[MAXFILELEN];
    stringInput("What should we call the new file (leave blank to display \
        message in the terminal): ", MAXFILELEN, outfile);

    int result = decode(infile, message);

    if (result == 0)
    {
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
    }

    return result;
}

/* 
Dummy function
*/
int encode(char infile[], char outfile[], char message[])
{
    printf("[DEBUG] Encoding\n");
    printf("[DEBUG] INPUTFILE: %s\n[DEBUG] OUTPUTFILE: %s\n[DEBUG] MESSAGE: \
        %s\n", infile, outfile, message);
    return 0;
}

/* 
Dummy function
*/
int decode(char infile[], char outstring[])
{
    printf("[DEBUG] Decoding\n");
    strcpy(outstring, "Decoding functionality isn't fully functional yet...");
    printf("[DEBUG] MESSAGE: %s\n", outstring);
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
