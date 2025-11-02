#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stegano.h"

int main() {
    char infile[] = "image.bmp";
    char outfile[] = "new.bmp";
    char message[MAX_MESSAGE_SIZE] = "hello world 0! hello helo woij fowjefo wejoif wjeowiejf w";
    char outstring[MAX_MESSAGE_SIZE] = "";
    printf("Message before compress: %s\n", message);
    printf("Length before: %d bytes\n", (int) (strlen(message)));

    checkFileType(infile);

    /* Returns total bits to then be passed to decode() */
    encode(infile, outfile, message);

    /* Returns pointer to array to then be passed to decompressMessage() */
    decode(outfile, outstring);

    printf("Message is: %s\n", outstring);

    return 0;
}
