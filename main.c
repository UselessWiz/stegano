#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stegano.h"

int main() {
    char infile[] = "image.bmp";
    char outfile[] = "new.bmp";
    char message[MAX_MESSAGE_SIZE] = "hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0! hello world 0!";
    printf("Message before compress: %s\n", message);
    printf("Length before: %d bytes\n", (int) (strlen(message)));

    checkFileType(infile);

    /* Returns total bits to then be passed to decode() */
    int total_bits = encode(infile, outfile, message);

    /* Returns pointer to array to then be passed to decompressMessage() */
    char* compressed = decode(outfile, total_bits);
    printf("Message after decode (compressed): %s\n", compressed);
    
    printf("Length after: %d bytes\n", (int) (strlen(compressed) + 7)/ 8);

    /* Sam's code sequence to create frequency table and reversing huffman tree logic, decompressing the message (don't put this comment in final code) */
    int freqTable[256] = {0};
    buildFrequencyTable(message, freqTable);

    int messageLength = strlen(message);

    char *decompressed = decompressMessage(compressed, freqTable, messageLength);
    if(!decompressed){
        printf("Decompression failed!\n");
        free(compressed);
        return 1;
    }
    printf("Message after decompress: %s\n", decompressed);
    free(decompressed);

    free(compressed);

    return 0;
}
