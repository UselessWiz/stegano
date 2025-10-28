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
