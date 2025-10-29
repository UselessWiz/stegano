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

    /* Checks for correct header information. */
    if((ih.biSize != 40) || (ih.biCompression != 0) || (ih.biBitCount != 24)) {
        fclose(image);
        printf("Image header error\n");
        return 1;
    }

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
    pic.rgb = NULL;
    if(!image) {
        printf("readImage open error: %s\n", infile);
        return pic;
    }

    /* Seeks specific positions in the image to find the offset, width, 
       height, and header data. */
    fseek(image, START_BYTE, SEEK_SET);
    fread(pic.header, 1, HEADER_SIZE, image);
    fseek(image, OFFSET_BYTE, SEEK_SET);
    fread(&pic.offset, sizeof(pic.offset), 1, image);
    fseek(image, WIDTH_BYTE, SEEK_SET);
    fread(&pic.width, sizeof(pic.width), 1, image);
    fseek(image, HEIGHT_BYTE, SEEK_SET);
    fread(&pic.height, sizeof(pic.height), 1, image); 
    
    /* Skips to byte 54 (where the RGB sequence starts). */
    fseek(image, HEADER_SIZE, SEEK_SET);
    
    /* Allocates memory for total RGB values. */
    pic.rgb = malloc(pic.width * pic.height * RGB_PER_PIXEL);
    if(!pic.rgb) {
        printf("pic.rgb malloc failed\n");
        fclose(image);
        return pic;
    }

    /* Padding and temporary array. */
    int padding = calcPadding(pic.width);
    unsigned char channel[RGB_PER_PIXEL];
    /* Loops through image dimensions from (height - 1) as BMP files 
       store information from bottom to top, left to right. */
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
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outfile: Pointer to char outfile, signifies the output file
 *                   to write.
 *  - char *message: Pointer to char (string) message.
 * Output:
 *  - Function of type void, no output (except the new image).
 */
void encode(char *infile, char *outfile, char *message) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);

    int i, j, k;
    int char_index = 0, bit_index = 0, bits_hidden = 0;
    int len = strlen(message);
    /* +1 to account for null terminator. */
    int total_bits = (len + 1) * BITS_PER_BYTE;

    /* Loops through the image dimensions until all bits are hidden. */
    for(i = 0; i < pic.height && bits_hidden < total_bits; i++) {
        for(j = 0; j < pic.width && bits_hidden < total_bits; j++) {
            /* Index for RGB value and temporary array. */
            int index = i * pic.width + j;
            unsigned char *channels[RGB_PER_PIXEL] = {
                &pic.rgb[index].red,
                &pic.rgb[index].green,
                &pic.rgb[index].blue
            };
            /* Loops through 3 color values of a pixel until all bits are hidden. */
            for(k = 0; k < RGB_PER_PIXEL && bits_hidden < total_bits; k++) {
                unsigned char current_char;
                /* Assigns each character and null terminator. */
                if(char_index < len) {
                    current_char = message[char_index];
                } else {
                    current_char = '\0';
                }

                /* Uses bitwise operator & (AND) to compare against 
                   number 1 to get each bit. */
                int bit = (current_char >> (7 - bit_index)) & 1;

                /* Sets the LSB of the color value to 1 (makes it odd). */
                if(bit == 1) *channels[k] |= 1;

                /* Sets the LSB of the color value to 0 (makes it even). */
                else *channels[k] &= ~1;

                /* Increments after each pixel assignment. */
                bits_hidden++;
                bit_index++;

                /* Resets and increments after every 8-bit cycle. */
                if(bit_index == BITS_PER_BYTE) {
                    bit_index = 0;
                    char_index++;
                }
            }
        }
    }
    
    /* Creates new image. */
    FILE *newimage = fopen(outfile, "wb");
    if(newimage == NULL) {
        printf("Encode function open error: %s\n", outfile);
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
    free(pic.rgb);
}

/* Decodes the message from the image using the reserved logic
 * of encoding and puts the message in a string.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outstring: Pointer to char outstring (decoded string).
 * Output:
 *  - Function of type void, no output.
 * 
 */
void decode(char *infile, char *outstring) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);

    /* Initialises variables and stop flag. */
    int i, j, k;
    int char_index = 0, bit_index = 0, flag = 0;
    unsigned char current_char = 0;

    /* readImage() already reordered pixel rows and RGB values,
       decode() can loop from top to bottom until stop flag. */
    for(i = 0; i < pic.height && !flag; i++) {
        for(j = 0; j < pic.width && !flag; j++) {
            /* Index and temporary array. */
            int index = i * pic.width + j;
            unsigned char *channels[RGB_PER_PIXEL] = {
                &pic.rgb[index].red,
                &pic.rgb[index].green,
                &pic.rgb[index].blue
            };

            /* Reconstructs the hidden message bit by bit. 
             * Extracts each value's LSB, shifts current_char left. 
             * Appends new bit.
             */
            for(k = 0; k < RGB_PER_PIXEL && !flag; k++) {
                int bit = *channels[k] & 1;
                current_char = (current_char << 1) | bit;
                bit_index++;

                /* Stores one character every 8-bit cycle and updates indices. */
                if(bit_index == BITS_PER_BYTE) {
                    outstring[char_index] = current_char;
                    char_index++;

                    /* Triggers stop flag if null terminator (8-bit 0 sequence) is found. */
                    if(current_char == '\0') {
                        flag = 1;
                        break;
                    }
                    bit_index = 0;
                    current_char = 0;
                }
            }
        }
    }

    /* Adds a null terminator at the end of the decoded string. */
    outstring[char_index] = '\0';
    free(pic.rgb);
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

/**Set all values to 0 in Struct */
void initialiseQueue(queue_t *q)
{
    q->front = 0;
    q->back = 0;
    q->count = 0;
}

//**Check to see if Queue is empty*/
int isEmpty(queue_t *q)
{
    if (q->count == 0)
    {
        return 1; /**Return 1 if empty */
    }
    return 0; //**Return 0 if not empty */
}

//**Check to see if Queue is full */
int isFull(queue_t *q)
{
    if (q->count == MAX_SIZE)
    {
        return 1; //**Return 1 if full */
    }
    return 0;
}
//**Adds a new element to the queue */
void enqueue(queue_t *q, char value[])
{
    if (isFull(q))
    {
        printf("Queue is full\n");
        return;
    }
    strcpy(q->items[q->back], value);
    q->back = (q->back + 1) % MAX_SIZE;
    q->count++;
}

//**Removes the top element from the queue (First in First out)*/
void dequeue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }
    q->front = (q->front + 1) % MAX_SIZE;
    q->count--;
}

/**Gets the top element from the queue */
char *peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return NULL;
    }
    return q->items[q->front];
}

/**Prints the whole queue starting at first added element until last added element */
void printQueue(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }

    printf("Current Queue: ");
    int i, current = q->front;
    for (i = 0; i < q->count; i++)
    {
        printf("%s ", q->items[current]);
        current = (current + 1) % MAX_SIZE;
    }
    printf("\n");
}
