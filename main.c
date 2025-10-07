#include "stegano.h"
#include <stdio.h>
#include <string.h>

/* ERROR CODES */
#define INVALIDARGUMENTSERROR -1

/* CONST PARAMETERS */
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
char* compressMessage(char message[]);

/* Takes a compressed string in and returns the decompressed version of it (Sam) */
char* decompressMessage(char compressed[]);

/* Using the queue structure, get recently accessed files in order */
char** getRecentFiles(queue_t* queue);

void printMenu(void);

void printHelp(void);

/* Should be run at the start of the program, and processes if the program should run in 
interactive mode or not (and if not, processes what needs to happen based on cmd instructions) */
int processArgs(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    /* If there are any cmd arguments passed, process them and act on them.*/
    if (argc > 1)
    {
        return processArgs(argc, argv);
    }

    /* Otherwise, run interactively. */
    printf("Running interactively\n");
    return 0;
}

/* Order of arguments matters because it makes life easier*/
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
        printf("encoding checkpoint 1\n");
        /* Find all other arguments */
        if (argc < 8 || \
            !(strcmp(argv[2], "-i") == 0 && strcmp(argv[4], "-o") == 0 && strcmp(argv[6], "-m") == 0))
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
        printf("decoding checkpoint 1\n");
        /* TODO: Check this. Non zero chance i've messed this up*/
        if ((argc < 4 && strcmp(argv[2], "-i") != 0) || \
            (argc < 6 && strcmp(argv[2], "-i") != 0 && strcmp(argv[4], "-o") != 0))
        {
            printHelp();
            return INVALIDARGUMENTSERROR;
        }

        char outString[MAXMESSAGELEN];
        int result = decode(argv[3], outString);

        /* Assume outfile was passed */
        if (result == 0 && argc >= 5)
        {
            FILE* file = fopen(argv[5], "w+");
            fprintf(file, "%s", outString);
            fclose(file);
        }

        return result;
    }

    /* If you make it here, assume that the arguments weren't valid. */
    printHelp();
    return INVALIDARGUMENTSERROR;
}

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

int encode(char infile[], char outfile[], char message[])
{
    printf("Encoding\n");
    return 0;
}

int decode(char infile[], char outstring[])
{
    printf("Decoding\n");
    return 0;
}