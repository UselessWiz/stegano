#include "stegano.h"
#include <stdlib.h> /*malloc(), free()*/
#include <string.h> /*strdup()*/

/*
Builds a frequency table of all characters in the given message.

Parameters:
message (const char[]): 
- input string whose character frequencies are to be counted.

freqTable (int[256]): 
- An integer array used to store the frequency count of each possible characters (0-255). 
- 256 including the null terminator.
- array should be initialized to zero before calling this function.

Returns (void):
- This function does not return a value.
- The resulting frequency counts are stored in the provided freqTable array.
*/
void buildFrequencyTable(const char message[], int freqTable[256]){
    /*Create a frequency table*/
    int i;
    for(i = 0; message[i] != '\0'; i++){
        freqTable[(unsigned char)message[i]]++;
    }
}

/*
Creates a sorted list of huffman tree leaf nodes based on character frequencies.

Parameters:
freqTable (const int[256]):
- An integer array containing the frequency of each possible character, 
  where the index corresponds to the character's ASCII value. 

nodeList (huffmanNode_t*[256]):
- An array of pointers where the function will store the addresses of dynamically allocated huffman nodes,
  each node represents a character that appears in the message

outSize (int*):
- A pointer to an integer where the function will store the number of nodes created and inserted into nodeList.
- If memory allocation fails, this value is set to -1.

Returns (void):
- This function does not return a value.
- The sorted list of nodes is stored in nodeList,
  and the count of nodes is stored in the integer pointed to by outSize.
*/
void createSortedNodeList(const int freqTable[256], huffmanNode_t* nodeList[256], int *outSize){
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

/*
Builds a Huffman tree from a sorted list of leaf nodes.

Parameters:

nodeList (huffmanNode_t*[256]):
- An array of pointers to huffman nodes, each representing a character and its frequency.
- Sorted in ascending order based on frequency.
- The two smallest frequency nodes will repeatedly merge until a single root node remains.

size (int):
- The number of nodes currently in nodeList.

Returns (huffmanNode_t*):
- A pointer to the root node of the constructed Huffman tree.
- Returns NULL if memory allocation fails or if the input size is zero.
*/
huffmanNode_t* buildHuffmanTree(huffmanNode_t* nodeList[256], int size){
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

/*
Recursively frees all memory associated with the Huffman tree.

Parameters:

root (huffmanNode_t*):
- A pointer to the root node of the Huffman tree to be freed.
- May be NULL, in which case the function does nothing.

Returns (void):
- This function does not return a value.
- All dynamically allocated nodes within the tree are freed from memory.
*/
void freeHuffmanTree(huffmanNode_t* root){
    if(!root){
        return;
    }
    /*Using recursion*/
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    free(root);
}

/*
Recursively generates huffman codes for each character in the huffman tree.

Parameters:

node (huffmanNode_t*):
- A pointer to the current node in the Huffman tree.
- The function traverses this node and its children to generate binary codes.

path (char*):
- A character array that stores the current binary path during taversal.\
- Each left branch appends '0' and each right branch appends '1'.

depth (int):
- The current depth of traversal within the huffman tree.
- Used to track the length of the binary code being generated.

codeTable (char*[256]):
- An array of string pointers used to store the generated binary codes for each character indexed by ASCII value.
- Memory of each code is dynamically allocated using strdup().

codeLen (int[256]):
- An interger array storing the length of each generated code,
  where the index corresponds to the character's ASCII value.

Returns (void):
- This function does not return a value.
- The resulting huffman codes and their lengths are stored in codeTable and codeLen respectively.
*/
void buildCode(huffmanNode_t* node, char *path, int depth, char *codeTable[256], int codeLen[256]){
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

/*
Compresses a given message using huffman encoding and returns the encoded bitstring.

Parameters:
message (char[]):
- the input string to be compressed.

Returns (char*):
- A dynamically allocated string containing the compressed message 
  represented as a sequence of '0's and '1's.
- Returns NULL if compression fails due to memory allocation issues or other errors.

Notes:
- The caller is responsible for freeing the returned string.
- This function internally builds the frequency table, 
  constructs the huffman tree, and generates the huffman codes, 
  and encodes the message.
*/

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

    /*Calculate total number of bits required for the compressed output*/
    int totalBits = 0;
    int i;
    for(i = 0; i < 256; i++){
        if(freqTable[i] && codeLen[i] > 0){
            totalBits += freqTable[i]*codeLen[i];
        }
    }

    /*Allocate memory for compressed output string*/
    char *output = malloc(totalBits + 1);
    if(!output){
        for(i = 0; i < 256; i++){
            free(codes[i]);
        }
        freeHuffmanTree(root);
        return NULL;
    }

    /*Encode message using generated huffman codes*/
    char *writePos = output;
    const unsigned char *inputPtr;

    for(inputPtr = (const unsigned char*) message; *inputPtr; inputPtr++){
        const char *currentCode = codes[*inputPtr];

        if(!currentCode){
            continue;
        }

        /*Write each bit ('0' or '1') of the current code to output*/
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

/*
Decompresses a huffman encoded bitstring back to its original message.

Parameters:
compressed (const char[]):
- The huffman encoded bitstring consisting of '0's and '1's characters.

freqTable (const int[256]):
- An integer frequency table that was originally used to build the huffman tree
  during compression. Each index corresponds to a character's ASCII value
  and its frequency count.

messageLength (int):
- The expected number of characters in the decompressed message.
- Must be non-negative.

Returns (char*):
- A dynamically allocated string containing the decompressed original message.
- Returns NULL if:
  - the input parameters are invalid
  - memory allocation fails
  - the huffman tree cannot be reconstructed
  - the bitstring does not decode correctly to the expected message length.

Notes:
- The caller is responsible for freeing the returned memory.
- This function reconstructs the huffman tree using the frequency table,
  then traverses it according to each bit in the compressed input.
*/
char* decompressMessage(const char compressed[], const int freqTable[256], int messageLength){
    /*Validate input parameters*/
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

    /*Recreate huffman leaf nodes from the frequency table*/
    huffmanNode_t *nodeList[256];
    int size = 0;
    createSortedNodeList(freqTable, nodeList, &size);

    if(size <= 0 && messageLength > 0){
        return NULL;
    }

    /*Rebuild the huffman tree from the sorted node list*/
    huffmanNode_t *root = buildHuffmanTree(nodeList, size);

    if(!root && messageLength > 0){
        return NULL;
    }

    /*Allocate memory for the decompressed output string*/
    char *output = malloc((int)messageLength + 1);
    if(!output){
        freeHuffmanTree(root);
        return NULL;
    }

    /*Handle single-node tree case*/
    if(!root->left && !root->right){
        int i;
        for(i = 0; i < messageLength; i++){
            output[i] = root->ch;
        }

        output[messageLength] = '\0';
        freeHuffmanTree(root);
        return output;
    }

    /*Traverse the huffman tree to decode each bit*/
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

    /*Verify that the number of decoded characters matches message length*/
    if(decodedCount != messageLength){
        free(output);
        freeHuffmanTree(root);
        return NULL;
    }

    output[decodedCount] = '\0';
    freeHuffmanTree(root);
    return output;

}





