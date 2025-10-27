#include "stegano.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



int main(int argc, char* argv[]){

    char message[256];
    printf("Enter the message you want to compress: \n");
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\n")] = '\0';


    printf("Original message: %s\n", message);

    char *compressed = compressMessage(message);
    

    if(!compressed){
        printf("Compression failed.\n");
        return 1;
    }

    printf("Compressed output: %s\n", compressed);
    printf("Length before: %zu bytes\n", (strlen(message)));
    printf("Length after: %zu bytes\n", (strlen(compressed) + 7)/ 8);

    char choice;
    printf("Do you want to decompress your message? (y,n)\n");
    scanf(" %c", &choice);
    if(choice == 'Y' || choice == 'y'){
        int freqTable[256] = {0};
        buildFrequencyTable(message, freqTable);
    
        int messageLength = strlen(message);
    
        char *decompressed = decompressMessage(compressed, freqTable, messageLength);
        if(!decompressed){
            printf("Decompression failed!\n");
            free(compressed);
            return 1;
        }
        printf("Decompressed message: %s\n", decompressed);
        free(decompressed);
    } else{
        printf("Program exited!");
    }

    free(compressed);
    return 0;
}


