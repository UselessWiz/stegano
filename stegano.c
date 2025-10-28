#include "stegano.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************************************************************/

/* Calculates the number of bytes to pad to align with the
 * natural padding of .bmp files.
 * Making sure the number of per each row is a multiple of 4.
 * 
 * Input:
 *  - int width: The width of the image (for width of row).
 * Output:
 *  - int padding: Total bytes needs to pad a row.
*/
int calcPadding(int width) {
    const int alignment = 4;
    int row_bytes = width * RGB_PER_PIXEL;
    /* How many bytes the row needs to be a multiple of 4. */
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

    /* Opens the .bmp file, reads the first 14 bytes in binary and
    saves them to fileheader_t struct and imageheader_t struct. */
    fileheader_t fh;
    imageheader_t ih;

    fread(fh.bfType, sizeof(fh.bfType), 1, image);
    fread(&fh.bfSize, sizeof(fh.bfSize), 1, image);
    fread(&fh.bfReserved1, sizeof(fh.bfReserved1), 1, image);
    fread(&fh.bfReserved2, sizeof(fh.bfReserved2), 1, image);
    fread(&fh.bfOffBits, sizeof(fh.bfOffBits), 1, image);
    fread(&ih, sizeof(imageheader_t), 1, image);

    /* Checks the first 2 indexes (bytes) of char bfType for 'B' or 'M'. */
    if((fh.bfType[0] != 'B') | (fh.bfType[1] != 'M')) {
        printf("File not .bmp\n");
        return 1;
    }

    /* Checks for correct header size of 40 bytes, correct compression type of 0, 
    and correct color format of 24-bit (RGB for each pixel). */
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
    /* Assigns the picture variables to 0 to return the empty
    image if the function cannot open it */
    image_t pic;
    pic.width = 0;
    pic.height = 0;
    pic.offset = 0;
    pic.rgb = NULL;

    if(!image) {
        printf("readImage open error: %s\n", infile);
        return pic;
    }

    /* Seeks specific postions in the image to find the offset, width, 
    height, and header data. */
    fseek(image, START_BYTE, SEEK_SET);
    fread(pic.header, 1, HEADER_SIZE, image);
    fseek(image, OFFSET_BYTE, SEEK_SET);
    fread(&pic.offset, sizeof(pic.offset), 1, image);
    fseek(image, WIDTH_BYTE, SEEK_SET);
    fread(&pic.width, sizeof(pic.width), 1, image);
    fseek(image, HEIGHT_BYTE, SEEK_SET);
    fread(&pic.height, sizeof(pic.height), 1, image); 
    
    /* Skips to byte 54 (where the RGB sequence starts) to store RGB
    values of every pixel. */
    fseek(image, HEADER_SIZE, SEEK_SET);
    
    /* Allocates memory for total RGB values. */
    pic.rgb = malloc(pic.width * pic.height * RGB_PER_PIXEL);
    if(!pic.rgb) {
        printf("pic.rgb malloc failed\n");
        fclose(image);
        return pic;
    }

    /* Calls to calcPadding function with local int padding. */
    int padding = calcPadding(pic.width);
    /* Temporary array to store the RGB values. */
    unsigned char channel[RGB_PER_PIXEL];
    /* Loops and read the RGB values from (height - 1) as BMP files 
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
        /* Set cursor position to skip the padding. */
        fseek(image, padding, SEEK_CUR);
    }
    
    fclose(image);
    return pic;
}

/* Encode the message into each color value of the pixels and
 * creates a new image with the altered pixels.
 * 
 * Input:
 *  - char *infile: Pointer to char infile, signifies the input file
 *                  to read.
 *  - char *outfile: Pointer to char outfile, signifies the output file
 *                   to write.
 * Output:
 *  - Function of type void, no actual output except the new image.
*/
void encode(char *infile, char *outfile, char *message) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);

    /* Initialises variables. */
    int i, j, k;
    /* Index of character of the message, index of the character's bit and bits hidden. */
    int char_index = 0, bit_index = 0, bits_hidden = 0;
    int len = strlen(message);
    /* +1 to account for null terminator. */
    int total_bits = (len + 1) * BITS_PER_BYTE;

    /* Loops through the image dimensions until all bits are hidden. */
    for(i = 0; i < pic.height && bits_hidden < total_bits; i++) {
        for(j = 0; j < pic.width && bits_hidden < total_bits; j++) {
            /* Index for each value of each pixel. */
            int index = i * pic.width + j;
            /* Temporary pointer to char channel to process the RGB values 
            easier. */
            unsigned char *channels[RGB_PER_PIXEL] = {
                &pic.rgb[index].red,
                &pic.rgb[index].green,
                &pic.rgb[index].blue
            };
            /* Loops through 3 color values of a pixel until all bits are hidden. */
            for(k = 0; k < RGB_PER_PIXEL && bits_hidden < total_bits; k++) {
                unsigned char current_char;
                if(char_index < len) {
                    current_char = message[char_index];
                } else {
                    /* Accounts for null terminator since len used strlen. */
                    current_char = '\0';
                }

                /* Uses bitwise operator & (AND) to compare last bit of character against 
                number 1 (binary of 00000001) to get the bit. */
                int bit = (current_char >> (7 - bit_index)) & 1;
                /* For bit of 1, changes the LSB of the RGB value to 1 using 
                bitwise operator | (OR), changing the RGB value to odd. */
                if(bit == 1) *channels[k] |= 1;
                /* For bit of 0, changes the LSB of the RGB value to 0 using 
                bitwise operator & (AND) and compares to ~1 (NOT), changing 
                the RGB value to even. */
                else *channels[k] &= ~1;

                /* Increments bits_hidden and bit_index after each RGB assignment. */
                bits_hidden++;
                bit_index++;

                /* Assign bit_index to 0 every 8-bit cycle. */
                if(bit_index == BITS_PER_BYTE) {
                    bit_index = 0;
                    /* Increments char_index for each character hidden. */
                    char_index++;
                }
            }
        }
    }
    
    /* Creates new image using fopen "wb". */
    FILE *newimage = fopen(outfile, "wb");
    if(newimage == NULL) {
        printf("Encode function open error: %s\n", outfile);
        return;
    }

    /* Write the header data from old image to new image. */
    fwrite(pic.header, pic.offset, 1, newimage);
    
    /* Calls to calcPadding function with local int padding. */
    int padding = calcPadding(pic.width);
    /* Empty padding char. */
    unsigned char pad[RGB_PER_PIXEL] = {0, 0, 0};
    /* Temporary array to store the RGB values. */
    unsigned char channel[RGB_PER_PIXEL];
    /* Loops bottom to top, left to right. */
    for(i = pic.height - 1; i >= 0; i--) {
        for(j = 0; j < pic.width; j++) {
            /* Index for each value of each pixel. */
            int index = i * pic.width + j;
            /* Assigns RGB value to temporary char channel. */
            channel[2] = pic.rgb[index].red;
            channel[1] = pic.rgb[index].green;
            channel[0] = pic.rgb[index].blue;
            /* Write the new RGB values into new image. */
            fwrite(channel, 1, RGB_PER_PIXEL, newimage);
        }
        /* Writes padding after each row. */
        fwrite(pad, 1, padding, newimage);
    }

    fclose(newimage);
    /* Free allocated memory. */
    free(pic.rgb);
}

/* 
 * 
 * 
 * 
 * Input:
 *  - 
 * Output:
 *  - 
 * 
*/
void decode(char *infile, char *outstring) {
    /* Calls the readImage function with local instance pic of image_t struct. */
    image_t pic = readImage(infile);

    int i, j, k;
    int char_index = 0, bit_index = 0, flag = 0;
    unsigned char current_char = 0;

    for(i = 0; i < pic.height && !flag; i++) {
        for(j = 0; j < pic.width && !flag; j++) {
            int index = i * pic.width + j;
            unsigned char *channels[RGB_PER_PIXEL] = {
                &pic.rgb[index].red,
                &pic.rgb[index].green,
                &pic.rgb[index].blue
            };

            for(k = 0; k < RGB_PER_PIXEL && !flag; k++) {
                int bit = *channels[k] & 1;
                current_char = (current_char << 1) | bit;
                bit_index++;

                if(bit_index == BITS_PER_BYTE) {
                    outstring[char_index] = current_char;
                    char_index++;

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

    outstring[char_index] = '\0';
    free(pic.rgb);
}
/********************************************************************/

void initialiseQueue(queue_t *q)
{
    q->front = 0;
    q->back = 0;
    q->count = 0;
}

int isEmpty(queue_t *q)
{
    if (q->count == 0)
    {
        return 1;
    }
    return 0;
}

int isFull(queue_t *q)
{
    if (q->count == MAX_SIZE)
    {
        return 1;
    }
    return 0;
}

void enqueue(queue_t *q, char value)
{
    if (isFull(q))
    {
        printf("Queue is full\n");
        return;
    }
    q->items[q->back] = value;
    q->back = (q->back + 1) % MAX_SIZE;
    q->count++;
}

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

char peek(queue_t *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return -1;
    }
    return q->items[q->front];
}

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
        printf("%c ", q->items[current]);
        current = (current + 1) % MAX_SIZE;
    }
    printf("\n");
}
