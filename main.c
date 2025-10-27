#include "stegano.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct huffmanNode{
    char ch;
    int freq;
    struct huffmanNode *left, *right;
} huffmanNode_t;


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


static void buildFrequencyTable(const char message[], int freqTable[256]){
    /*Create a frequency table*/
    int i;
    for(i = 0; message[i] != '\0'; i++){
        freqTable[(unsigned char)message[i]]++;
    }
}
static void createSortedNodeList(const int freqTable[256], huffmanNode_t* nodeList[256], int *outSize){
    /*Create nodes for characters that appear*/
    int size = 0;
    int i;
    for(i = 0; i < 256; i++){
        if(freqTable[i] > 0){
            /*Allocate a new node for this character*/
            huffmanNode_t* node = malloc(sizeof(huffmanNode_t));
            if(!node){
                *outSize = -1;
                return;
            }
    
            node->ch = (unsigned char) i;
            node->freq = freqTable[i];
            node->left = node->right = NULL;
            
            /*Insert node into sorted position by frequency*/
            int j = size++;
            while(j > 0 && nodeList[j-1]->freq > node->freq){
                nodeList[j] = nodeList[j-1];
                j--;
            }
            nodeList[j] = node;
        }
    }
    /*Return how many nodes were created*/
    *outSize = size;
}

static huffmanNode_t* buildHuffmanTree(huffmanNode_t* nodeList[256], int size){
    if(size == 0){
        return NULL;
    }

    if(size == 1){
        return nodeList[0];
    }

    /*Merge nodes until one remains*/
    while(size > 1){
        /*take the 2 least frequent*/
        huffmanNode_t* left = nodeList[0];
        huffmanNode_t* right = nodeList[1];
    
        /*Create parent*/
        huffmanNode_t* parent = malloc(sizeof(huffmanNode_t));
        if(!parent){
            return NULL;
        }
    
        parent->ch = '\0';
        parent->freq = left->freq + right->freq;
        parent->left = left;
        parent->right = right;
    
        /*remove left,right, then shift array by 2*/
        int k;
        for(k = 2; k < size; k++){
            nodeList[k-2] = nodeList[k];
        }
        size -= 2;
    
        /*Insert parent back into sorted array*/
        int j = size++;
        while(j > 0 && nodeList[j-1]->freq > parent->freq){
            nodeList[j] = nodeList[j-1];
            j--;
        }
        nodeList[j] = parent;
    }
    /*Return the final returning node*/
    return nodeList[0];
}

static void freeHuffmanTree(huffmanNode_t* root){
    if(!root){
        return;
    }
    /*Using recursion*/
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    free(root);
}

static void buildCode(huffmanNode_t* node, char *path, int depth, char *codeTable[256], int codeLen[256]){
    if(!node){
        return;
    }

    /*Store the current path as the code for this character*/
    if(!node->left && !node->right){
        if(depth == 0){
            path[0] = '0';
            path[1] = '\0';
            codeLen[(unsigned char)node->ch] = 1;
            codeTable[(unsigned char)node->ch] = strdup("0"); /*Duplicate and store code*/
        } else{
            path[depth] = '\0'; /*End the string at current depth*/
            codeLen[(unsigned char)node->ch] = depth;
            codeTable[(unsigned char)node->ch] = strdup(path); /*Store a copy of the code path*/
        }
        return;
    }
    /*Traverse left and right, 0 for left, 1 for right*/
    path[depth] = '0';
    buildCode(node->left, path, depth + 1, codeTable, codeLen);

    path[depth] = '1';
    buildCode(node->right, path, depth + 1, codeTable, codeLen);
}

char* compressMessage(char message[]){
    /*Build frequency table from input*/
    int freqTable[256] = {0};
    buildFrequencyTable(message, freqTable);

    /*Create sorted list of leaf nodes by ascending freq*/
    huffmanNode_t* nodeList[256];
    int size = 0;
    createSortedNodeList(freqTable, nodeList, &size);

    if(size < 0){
        return  NULL;
    }

    /*Build huffman treee from the sorted leaves*/
    huffmanNode_t* root = buildHuffmanTree(nodeList, size);
    if(!root && message[0] != '\0'){
        return NULL;
    }

    /*Derive code table from Depth First Search*/
    char *codes[256] = {0};
    int codeLen[256] = {0};
    char path[256] = {0};
    buildCode(root, path, 0, codes, codeLen);

    int totalBits = 0;
    int i;
    for(i = 0; i < 256; i++){
        if(freqTable[i] && codeLen[i] > 0){
            totalBits += freqTable[i]*codeLen[i];
        }
    }

    char *output = malloc(totalBits + 1);
    if(!output){
        for(i = 0; i < 256; i++){
            free(codes[i]);
        }
        freeHuffmanTree(root);
        return NULL;
    }

    char *writePos = output;
    const unsigned char *inputPtr;

    for(inputPtr = (const unsigned char*) message; *inputPtr; inputPtr++){
        const char *currentCode = codes[*inputPtr];

        if(!currentCode){
            continue;
        }

        const char *bitPtr;
        for(bitPtr = currentCode; *bitPtr; bitPtr++){
            *writePos++ = *bitPtr;
        }
    }
    *writePos = '\0';

    for(i = 0; i < 256; i++){
        free(codes[i]);
    }
    freeHuffmanTree(root);

    return output;

}


char* decompressMessage(const char compressed[], const int freqTable[256], int messageLength){
    if(!compressed || !freqTable || messageLength < 0){
        return NULL;
    }

    /*Check for empty message*/
    if(messageLength == 0){
        char *empty = malloc(1);
        if(empty){
            empty[0] = '\0';
            return empty;
        }
        return NULL;
    }

    huffmanNode_t *nodeList[256];
    int size = 0;
    createSortedNodeList(freqTable, nodeList, &size);

    if(size <= 0 && messageLength > 0){
        return NULL;
    }

    huffmanNode_t *root = buildHuffmanTree(nodeList, size);

    if(!root && messageLength > 0){
        return NULL;
    }

    char *output = malloc((int)messageLength + 1);
    if(!output){
        freeHuffmanTree(root);
        return NULL;
    }

    if(!root->left && !root->right){
        int i;
        for(i = 0; i < messageLength; i++){
            output[i] = root->ch;
        }

        output[messageLength] = '\0';
        freeHuffmanTree(root);
        return output;
    }

    int decodedCount = 0;
    huffmanNode_t *currentNode = root;

    for(const char *ptr = compressed; *ptr && decodedCount < messageLength; ptr++){
        if(*ptr == '0'){
            currentNode = currentNode->left;
        } else if(*ptr == '1'){
            currentNode = currentNode->right;
        } else{
            continue;
        }

        if(!currentNode){
            free(output);
            freeHuffmanTree(root);
            return NULL;
        }

        if(!currentNode->left && !currentNode->right){
            output[decodedCount++] = currentNode->ch;
            currentNode = root;
        }
    }

    if(decodedCount != messageLength){
        free(output);
        freeHuffmanTree(root);
        return NULL;
    }

    output[decodedCount] = '\0';
    freeHuffmanTree(root);
    return output;

}
