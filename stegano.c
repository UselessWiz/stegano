#include "stegano.h"
#include <stdio.h>
#include <stdlib.h> /*malloc(), free()*/
#include <string.h> /*strdup()*/

/********************************************************************/
/* Calculates the number of padding bytes so each image row aligns.
 * Ensures the total bytes per row is a multiple of 4.
 * 
 * Input:
 *  - int width: The width of the image (for width of row).
 * Output:
 *  - int padding: Total bytes needs to pad a row.
*/
int calcPadding(int width) {
    const int alignment = 4;
    int row_bytes = width * RGB_PER_PIXEL;

    /* How many bytes the row is short from a multiple of 4. */
    int remainder = alignment - (row_bytes % alignment);

    /* Checks for already aligned (e.g. 4 % 4 = 0). */
    int padding = remainder % alignment;

    return padding;
}

/* Checks whether the file is in BMP format.
 * 
 * Input:
 *  - char *filename: Pointer to char of filename, which is the
 *                    string that contains the file name.
 * Output:
 *  - 0: If correct file format.
 *  - 1: If open error or incorrect file format.
*/
int checkFileType(char *filename) {
    FILE *image = fopen(filename, "rb");
    if(!image) {
        printf("Couldn't open file %s.\n", filename);
        return 1;
    }

    /* Reads the file and image header into their respective structs. */
    fileheader_t fh;
    imageheader_t ih;

    fread(fh.bfType, sizeof(fh.bfType), 1, image);
    fread(&fh.bfSize, sizeof(fh.bfSize), 1, image);
    fread(&fh.bfReserved1, sizeof(fh.bfReserved1), 1, image);
    fread(&fh.bfReserved2, sizeof(fh.bfReserved2), 1, image);
    fread(&fh.bfOffBits, sizeof(fh.bfOffBits), 1, image);
    fread(&ih, sizeof(imageheader_t), 1, image);

    /* Checks the first 2 bytes (indices of bfType). */
    if((fh.bfType[0] != 'B') || (fh.bfType[1] != 'M')) {
        printf("Incorrect file format.\n");
        return 1;
    }

    /* Checks for correct 24-bit, uncompressed format. */
    if((ih.biBitCount != 24) || (ih.biCompression != 0)) {
        fclose(image);
        printf("Incorrect image format. "
               "Must be 24-bit color format and uncompressed\n");
        return 1;
    }

    /* Check for correct bottom-up BMP format. */
    if((ih.biHeight < 0)) {
        printf("Incorrect image format. "
            "Top-down BMP not supported.\n");
        return 1;
    }

    fclose(image);
    return 0;
}

/* Reads the image in binary and store its information in image_t
 * struct.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 * Output:
 *  - image_t pic: Returns the instance pic of image_t struct along
 *                 with its data.
*/
image_t readImage(char *infile) {
    int i, j;

    FILE *image = fopen(infile, "rb");
    /* Initialises image data to 0 to safely return the empty
       image if opening fails. */
    image_t pic;
    pic.width = 0;
    pic.height = 0;
    pic.offset = 0;
    pic.header = NULL;
    pic.rgb = NULL;
    if(!image) {
        printf("Couldn't open image %s.\n", infile);
        return pic;
    }

    /* Seeks specific positions in the image to find the offset, width, 
       height, and header data. */
    fseek(image, OFFSET_BYTE, SEEK_SET);
    fread(&pic.offset, sizeof(pic.offset), 1, image);
    fseek(image, WIDTH_BYTE, SEEK_SET);
    fread(&pic.width, sizeof(pic.width), 1, image);
    fseek(image, HEIGHT_BYTE, SEEK_SET);
    fread(&pic.height, sizeof(pic.height), 1, image);

    /* Memory allocates *header with size of offset (total size of header in bytes). */
    pic.header = malloc(pic.offset);
    if(!pic.header) {
        printf("Memory Allocation Error.\n");
        fclose(image);
        return pic;
    }
    /* Skips to start to read full header and stores in struct variable. */
    fseek(image, START_BYTE, SEEK_SET);
    fread(pic.header, 1, pic.offset, image);
    
    /* Allocates memory for total RGB values. */
    pic.rgb = malloc(pic.width * pic.height * RGB_PER_PIXEL);
    if(!pic.rgb) {
        printf("Memory Allocation Error.\n");
        free(pic.header);
        fclose(image);
        pic.header = NULL;
        return pic;
    }

    /* Skips to offset byte, indicator of where the RGB sequence starts. */
    fseek(image, pic.offset, SEEK_SET);
    /* Padding and temporary array. */
    int padding = calcPadding(pic.width);
    unsigned char channel[RGB_PER_PIXEL];

    /* Loops through image dimensions from (height - 1), following bottom-up convention, left to right. */
    for(i = pic.height - 1; i >= 0; i--) {
        for(j = 0; j < pic.width; j++) {
            /* Index for each value of each pixel. */
            int index = i * pic.width + j;
            fread(channel, 1, RGB_PER_PIXEL, image);
            /* Reassigns as RGB for readability, since the initial 
               BMP order is BGR. */
            pic.rgb[index].red = channel[2];
            pic.rgb[index].green = channel[1];
            pic.rgb[index].blue = channel[0];
        }
        /* Moves the file pointer forward to skip padding byte(s). */
        fseek(image, padding, SEEK_CUR);
    }
    
    fclose(image);
    return pic;
}

/* Calculates indices and accesses RGB channels. Changes the LSB 
 * of the channel according to the bit.
 * 
 * Input:
 *  - image_t *pic: Pointer to struct pic.
 *  - int bit_index: Index position in the image (which pixel/channel).
 *  - int bit: The value of the bit, 0 or 1.
 * Output:
 *  - Function of type void.
 */
void setLSBPixel(image_t *pic, int bit_index, int bit) {
    /* Pixel position increases after every 3 channels accessed. 
       Channel index always 0, 1, or 2. */
    int pixel_index = bit_index / RGB_PER_PIXEL;
    int channel_index = bit_index % RGB_PER_PIXEL;

    /* Pointer to the channel, accessing memory address of each channel based on index. */
    unsigned char *channel;
    if(channel_index == 0) channel = &pic->rgb[pixel_index].red;
    else if(channel_index == 1) channel = &pic->rgb[pixel_index].green;
    else channel = &pic->rgb[pixel_index].blue;

    /* Bitwise operations, setting LSB. */
    if(bit == 1) *channel |= 1;
    else *channel &= ~1;
}

/* Calculates indices and accesses RGB channels. Extracts the LSB
 * of the channel then compares it with binary 1.
 * 
 * Input:
 *  - image_t *pic: Pointer to struct pic.
 *  - int bit_index: Index position of the image (which pixel/channel).
 *  - int *bit: Pointer to the integer bit, modifying the integer.
 * Output:
 *  - Function of type void.
 */
void getLSBPixel(image_t *pic, int bit_index, int *bit) {
    /* Pixel position increases after every 3 channels accessed. 
       Channel index always 0, 1, or 2. */
    int pixel_index = bit_index / RGB_PER_PIXEL;
    int channel_index = bit_index % RGB_PER_PIXEL;

    /* Pointer to channel, accessing memory address of each channel based on index. */
    unsigned char *channel;
    if(channel_index == 0) channel = &pic->rgb[pixel_index].red;
    else if(channel_index == 1) channel = &pic->rgb[pixel_index].green;
    else channel = &pic->rgb[pixel_index].blue;

    /* Performs bitwise AND with 00000001, isolating LSB. */
    *bit = *channel & 1;
}

/* Encodes the total bits, message length, Huffman's frequency 
 * table, and Huffman compressed message into image.
 * Creates a new image, writes the information including the modified
 * RGB values into the image.
 * 
 * encode() allocates and frees memory to compress the
 * message internally, no manual/external memory management.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outfile: Pointer to char outfile, signifies the output file
 *                   to write.
 *  - char *message: Pointer to char (string) message.
 * Output:
 *  - Function of type void.
 */
void encode(char *infile, char *outfile, char *message) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);
    /* Stops the program if readImage returns corrupted image. */
    if(!pic.rgb || !pic.header) {
        printf("Failed to allocate memory.\n");
        return;
    }

    /* Initialising variables. */
    int i, j;
    int total_bits = 0;

    /* Checks for empty string. */
    int message_len = strlen(message);
    if(message_len == 0) {
        printf("Message is empty.\n");
        free(pic.header);
        free(pic.rgb);
        return;
    }

    /* Call to compressMessage(), compress message and access total bits from the function. */
    char *compressed = compressMessage(message, &total_bits);
    if(!compressed) {
        printf("Compression failed.\n");
        free(pic.header);
        free(pic.rgb);
        return;
    }

    /* printf("Compressed message: %s\n", compressed); */

    /* Initialising essential variables. */
    int tree_bits = BITS_PER_BYTE * 2 + MAX_MESSAGE_SIZE * BITS_PER_BYTE; /* Bits required for the Huffman tree. */
    int required_bits = tree_bits + total_bits;
    int max_bits = pic.width * pic.height * RGB_PER_PIXEL;

    /* Checks if image is too small (or message too large). */
    if(total_bits > (MAX_MESSAGE_SIZE - 1) || required_bits > max_bits) {
        if(total_bits > (MAX_MESSAGE_SIZE - 1)) printf("Message is too large.\n");
        else printf("Image is too small.\n");
        free(pic.header);
        free(pic.rgb);
        free(compressed);
        return;
    }

    /* Stores the total bits in the first 8 LSBs, since max string length after compression is 256. */
    for(i = 0; i < BITS_PER_BYTE; i++) {
        /* Performs bitwise >> and AND with 00000001, isolating each bit. */
        int bit = (total_bits >> (7 - i)) & 1;
        
        /* Call to function, modifying LSB according to bit value. */
        setLSBPixel(&pic, i, bit);
    }

    /* Set start position after first byte (dedicated for total bits). */
    int start_meslen = BITS_PER_BYTE;
    for(i = 0; i < BITS_PER_BYTE; i++) {
        int bit = (message_len >> (7 - i)) & 1;
        int j = i + start_meslen;

        setLSBPixel(&pic, j, bit);
    }

    /* Initialising and building frequency table using original message and its length. */
    int freqTable[MAX_MESSAGE_SIZE] = {0};
    buildFrequencyTable(message, freqTable);

    /* Set start position for frequency table after first 2 bytes (for total bits and message length). */
    int start_freq = BITS_PER_BYTE * 2;
    for(i = 0; i < MAX_MESSAGE_SIZE; i++) {
        /* Buffer char for frequency table element. */
        unsigned char buffer = freqTable[i];
        for(j = 0; j < BITS_PER_BYTE; j++) {
            /* Performs bitwise operations and set frequency index, each element takes 8 bits. */
            int bit = (buffer >> (7 - j)) & 1;
            int freq_index = start_freq + i * BITS_PER_BYTE + j;
            
            setLSBPixel(&pic, freq_index, bit);
        }
    }

    /* Loops through each character in the Huffman string and alters RGB channels accordingly. */
    for(i = 0; i < total_bits; i++) {
        /* Starts after total bits, message length, and frequency table. */
        int j = i + tree_bits;
        /* Accesses each element of the Huffman compressed continuous string of 0s and 1s. */
        char current_char = compressed[i];
        int bit;
        /* Checks whether the character in the string is 0, or 1. */
        if(current_char == '1') bit = 1;
        else bit = 0;

        setLSBPixel(&pic, j, bit);
    }
    
    /* Creates new image. */
    FILE *newimage = fopen(outfile, "wb");
    if(newimage == NULL) {
        printf("Couldn't create file %s.\n", outfile);
        free(pic.header);
        free(pic.rgb);
        free(compressed);
        return;
    }

    /* Writes the header data from old image. */
    fwrite(pic.header, pic.offset, 1, newimage);
    
    /* Padding and temporary array. */
    int padding = calcPadding(pic.width);
    unsigned char pad[RGB_PER_PIXEL] = {0, 0, 0};
    unsigned char channel[RGB_PER_PIXEL];

    /* Loops bottom to top, left to right. */
    for(i = pic.height - 1; i >= 0; i--) {
        for(j = 0; j < pic.width; j++) {
            /* Index for each value of each pixel. */
            int index = i * pic.width + j;
            channel[2] = pic.rgb[index].red;
            channel[1] = pic.rgb[index].green;
            channel[0] = pic.rgb[index].blue;
            /* Writes the new RGB values into new image. */
            fwrite(channel, 1, RGB_PER_PIXEL, newimage);
        }
        /* Writes padding after each row. */
        fwrite(pad, 1, padding, newimage);
    }

    fclose(newimage);
    free(pic.header);
    free(pic.rgb);
    free(compressed);
}

/* Decodes the compressed message from the image using the reversed logic
 * of encoding.
 * Rebuilds Huffman frequency table with total bits and message length
 * encoded in the image.
 * Decompresses the Huffman string, returning the original message.
 * 
 * decode() allocates and frees memory to decompress the
 * message internally, no manual or external memory management.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outstring: Pointer to char outstring (decoded string).
 * Output:
 *  - Function of type void.
 */
void decode(char *infile, char *outstring) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);
    /* Stops the program if readImage returns corrupted image. */
    if(!pic.rgb || !pic.header) {
        printf("Failed to allocate memory.\n");
        return;
    }

    /* Initialising variables. Some variables are to be received from the image. */
    int i, j;
    int total_bits = 0, message_len = 0;
    int tree_bits = BITS_PER_BYTE * 2 + MAX_MESSAGE_SIZE * BITS_PER_BYTE;
    int max_bits = pic.width * pic.height * RGB_PER_PIXEL;

    /* Extracts the number of total bits in the first byte. */
    for(i = 0; i < BITS_PER_BYTE; i++) {
        int bit;
        /* Calls getLSBPixel. */
        getLSBPixel(&pic, i, &bit);
        /* Performs bitwise << and |, extracting the 8-bit sequence. */
        total_bits = (total_bits << 1) | bit;
    }

    /* printf("total bits: %d\n", total_bits); */

    /* Safety check to make sure encoded data is valid and doesn't overflow image's capacity. */
    if(total_bits <= 0 || tree_bits + total_bits > max_bits) {
        printf("Invalid image data.\n");
        free(pic.header);
        free(pic.rgb);
        return;
    }
    
    /* Sets start position after first byte (containing total bits). 
       Extracts message length. */
    int start_meslen = BITS_PER_BYTE;
    for(i = 0; i < BITS_PER_BYTE; i++) {
        int bit;
        /* Initialise index, calls getLSBPixel function, 
           and performs bitwises operators, extracting the 8-bit sequence. */
        int j = i + start_meslen;
        getLSBPixel(&pic, j, &bit);
        message_len = (message_len << 1) | bit;
    }
    
    /* printf("message len: %d\n", message_len); */

    /* Sets start position after first 2 bytes (containing total bits and message length). 
       Extracts frequency table to rebuild. */
    int start_freq = BITS_PER_BYTE * 2;
    /* Initialising frequency table. */
    int freqTable[MAX_MESSAGE_SIZE] = {0};
    for(i = 0; i < MAX_MESSAGE_SIZE; i++) {
        int current_freq = 0;
        for(j = 0; j < BITS_PER_BYTE; j++) {
            int bit;
            int freq_index = start_freq + i * BITS_PER_BYTE + j;
            getLSBPixel(&pic, freq_index, &bit);
            current_freq = (current_freq << 1) | bit;
        }
        /* Appends the value into the frequency table. */
        freqTable[i] = current_freq;
    }

    /* Allocates memory for reconstructed (compressed) string (+1 for null terminator). */
    char *compressed = malloc(total_bits + 1);
    if(!compressed) {
        printf("Decompression failed.\n");
        free(pic.header);
        free(pic.rgb);
        return;
    }

    /* Loops through each character in the Huffman string and alter RGB channels accordingly. */
    for(i = 0; i < total_bits; i++) {
        int j = i + tree_bits;
        int bit;
        getLSBPixel(&pic, j, &bit);
        
        /* Checks for bit value and appends to compressed array. */
        if(bit == 1) compressed[i] = '1';
        else compressed[i] = '0';
    }

    /* Null terminates array at correct end position. */
    compressed[total_bits] = '\0';

    /* Allocates memory for decompressed string. 
       Reconstructs the original message with encoded frequency table and message length. */
    char *decompressed = decompressMessage(compressed, freqTable, message_len);
    if(!decompressed) {
        printf("Decompression failed.\n");
        free(pic.header);
        free(pic.rgb);
        free(compressed);
        return;
    }

    /* Copies decompressed into outstring, which contains 
       the original message before compression and encryption. */
    strcpy(outstring, decompressed);

    free(pic.header);
    free(pic.rgb);
    free(compressed);
    free(decompressed);
}

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
            codeTable[(unsigned char)node->ch] = malloc(2);

            if(codeTable[(unsigned char)node->ch]){
                strcpy(codeTable[(unsigned char)node->ch], "0");
            }
        } else{
            path[depth] = '\0'; /*End the string at current depth*/
            codeLen[(unsigned char)node->ch] = depth;
            codeTable[(unsigned char)node->ch] = malloc(strlen(path) + 1);

            if(codeTable[(unsigned char)node->ch]){
                strcpy(codeTable[(unsigned char)node->ch], path);
            }
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

int *out_totalBits:
- the number of total bits needed for encode/decode.

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

char* compressMessage(char message[], int *out_totalBits){
    /*Build frequency table from input*/
    int freqTable[256] = {0};
    buildFrequencyTable(message, freqTable);

    /*Create sorted list of leaf nodes by ascending freq*/
    huffmanNode_t* nodeList[256];
    int size = 0;
    createSortedNodeList(freqTable, nodeList, &size);

    if(size < 0){
        return NULL;
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
    *out_totalBits = totalBits;
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

    const char *ptr;
    for (ptr = compressed; *ptr && decodedCount < messageLength; ptr++){
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

/*Set all values to 0 in Struct */
void initialiseQueue(queue_t *q)
{
    q->front = 0;
    q->back = 0;
    q->count = 0;
}

/*Check to see if Queue is empty*/
int isEmpty(queue_t *q)
{
    if (q->count == 0)
    {
        return 1; /*Return 1 if empty */
    }
    return 0; /*Return 0 if not empty */
}

/*Check to see if Queue is full */
int isFull(queue_t *q)
{
    if (q->count == MAX_SIZE)
    {
        return 1; /*Return 1 if full */
    }
    return 0;
}

/*Adds a new element to the queue */
void enqueue(queue_t *q, char value[])
{
    if (isFull(q))
    {
        printf("Queue is full.\n");
        return;
    }
    strcpy(q->items[q->back], value);
    q->back = (q->back + 1) % MAX_SIZE;
    q->count++;
}

/*Removes the top element from the queue (First in First out)*/
void dequeue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty.\n");
        return;
    }
    q->front = (q->front + 1) % MAX_SIZE;
    q->count--;
}

/*Gets the top element from the queue */
char *peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty.\n");
        return NULL;
    }
    return q->items[q->front];
}

/*Prints the whole queue starting at first added element until last added element */
void printQueue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty.\n");
        return;
    }

    int i, current = q->front;
    for (i = 0; i < q->count; i++)
    {
        printf("%s ", q->items[current]);
        current = (current + 1) % MAX_SIZE;
    }
    printf("\n");
}
