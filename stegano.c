#include "stegano.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("checkFileType open error: %s\n", filename);
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
        printf("File not .bmp\n");
        return 1;
    }

    /* Checks for correct 24-bit, uncompressed format. */
    if((ih.biBitCount != 24) || (ih.biCompression != 0)) {
        fclose(image);
        printf("Image header error\n"
               "Must be 24-bit color format and uncompressed\n");
        return 1;
    }

    /* Check for correct bottom-up BMP format. */
    if((ih.biHeight < 0)) {
        printf("Top-down BMP not supported\n");
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
        printf("readImage open error: %s\n", infile);
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
        printf("pic.header malloc failed\n");
        fclose(image);
        return pic;
    }
    /* Skips to start to read full header and stores in struct variable. */
    fseek(image, START_BYTE, SEEK_SET);
    fread(pic.header, 1, pic.offset, image);
    
    /* Allocates memory for total RGB values. */
    pic.rgb = malloc(pic.width * pic.height * RGB_PER_PIXEL);
    if(!pic.rgb) {
        printf("pic.rgb malloc failed\n");
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

/* Encodes the message into each color value of the pixels.
 * Creates a new image, writes the modified RGB values into 
 * the output image.
 * encode() allocates and frees internal memory.
 * Caller is not responsible for freeing.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outfile: Pointer to char outfile, signifies the output file
 *                   to write.
 *  - char *message: Pointer to char (string) message.
 * Output:
 *  - return total_bits: Total number of bits that the huffman compress 
 *                       returns, later be used in decode to stop decode after
 *                       loop reaches total bits.
 */
int encode(char *infile, char *outfile, char *message) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);
    /* Stops the program if readImage returns corrupted image. */
    if(!pic.rgb || !pic.header) {
        printf("Failed to allocate memory for pic.rgb and pic.header in encode()\n");
        return 1;
    }

    /* Initialising variables. */
    int max_bits = pic.width * pic.height * RGB_PER_PIXEL;
    int total_bits = 0;
    /* Call to compressMessage(), compress message and access total bits from the function. */
    char *compressed = compressMessage(message, &total_bits);
    if(!compressed) {
        printf("Compression failed in encode()\n");
        free(pic.header);
        free(pic.rgb);
        return 1;
    }
    /* Checks if the Huffman string is too long (or image is too small). */
    if(total_bits > max_bits) {
        printf("Message is too large for image in encode()\n");
        free(pic.header);
        free(pic.rgb);
        free(compressed);
        return 1;
    }
    
    int i;
    /* Loops through each character in the Huffman string and alter RGB channels accordingly. */
    for(i = 0; i < total_bits; i++) {
        char current_char = compressed[i];
        int bit;
        /* Checks whether the character in the string is 0, or 1. */
        if(current_char == '1') {
            bit = 1;
        } else {
            bit = 0;
        }
        
        /* Indices and temporary array. 
         * Pixel index increments after every 3 channels.
         * Modulus always returns 0, 1, or 2, returning channel index.
         */
        int pixel_index = i / RGB_PER_PIXEL;
        int channel_index = i % RGB_PER_PIXEL;
        unsigned char *channels[RGB_PER_PIXEL] = {
            &pic.rgb[pixel_index].red,
            &pic.rgb[pixel_index].green,
            &pic.rgb[pixel_index].blue
        };

        /* Modifying the LSB of each channel based on each Huffman character assessed. */
        if(bit == 1) {
            *channels[channel_index] |= 1;
        } else {
            *channels[channel_index] &= ~1;
        }
    }
    
    /* Creates new image. */
    FILE *newimage = fopen(outfile, "wb");
    if(newimage == NULL) {
        printf("Encode function open error: %s\n", outfile);
        free(pic.header);
        free(pic.rgb);
        free(compressed);
        return 1;
    }

    /* Writes the header data from old image. */
    fwrite(pic.header, pic.offset, 1, newimage);
    
    /* Padding and temporary array. */
    int padding = calcPadding(pic.width);
    unsigned char pad[RGB_PER_PIXEL] = {0, 0, 0};
    unsigned char channel[RGB_PER_PIXEL];

    int x, y;
    /* Loops bottom to top, left to right. */
    for(y = pic.height - 1; y >= 0; y--) {
        for(x = 0; x < pic.width; x++) {
            /* Index for each value of each pixel. */
            int index = y * pic.width + x;
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
    return total_bits;
}

/* Decodes the message from the image using the reserved logic
 * of encoding and puts the message in a string.
 * decode() allocates the compressed data and returns it.
 * Caller (main) is responsible for freeing it.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outstring: Pointer to char outstring (decoded string).
 * Output:
 *  - return compressed: Returns compressed huffman string, can then 
 *                       be used to decompresss.
 */
char *decode(char *infile, int total_bits) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);
    /* Stops the program if readImage returns corrupted image. */
    if(!pic.rgb || !pic.header) {
        printf("Failed to allocate memory for pic.rgb and pic.header in decode()\n");
        return NULL;
    }

    /* Initialises variable. */
    char *compressed = malloc(total_bits + 1);
    unsigned char current_char = 0;
    int i;
    int bits_read = 0;

    /* Loops through the pixel and channels to decode the compressed Huffman string. */
    for(i = 0; i < total_bits && bits_read < total_bits; i++) {
        /* Indices and temporary array. */
        int pixel_index = i / RGB_PER_PIXEL;
        int channel_index = i % RGB_PER_PIXEL;
        unsigned char *channels[RGB_PER_PIXEL] = {
            &pic.rgb[pixel_index].red,
            &pic.rgb[pixel_index].green,
            &pic.rgb[pixel_index].blue
        };

        /* Extracts the LSB of the channel.
           Assigns current_char as '0' or '1' accordingly. */
        int bit = *channels[channel_index] & 1;
        if(bit == 1) current_char = '1';
        else current_char = '0';

        /* Appends and increments. */
        compressed[i] = current_char;
        bits_read++;
    }

    /* Adds a null terminator at the end of the decoded string. */
    compressed[bits_read] = '\0';
    free(pic.header);
    free(pic.rgb);
    return compressed;
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