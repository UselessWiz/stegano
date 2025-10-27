#include "stegano.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************************************************/
int calcPadding(int width) {
    const int alignment = 4;
    int row_bytes = width * RGB_PER_PIXEL; /* hello */
    int remainder = alignment - (row_bytes % alignment);
    int padding = remainder % alignment;

    return padding;
}

int checkFileType(char *filename) {
    FILE *image = fopen(filename, "rb");
    if(!image) {
        printf("checkFileType open error: %s\n", filename);
        return 1;
    }

    fileheader_t fh;
    imageheader_t ih;

    fread(fh.bfType, sizeof(fh.bfType), 1, image);
    fread(&fh.bfSize, sizeof(fh.bfSize), 1, image);
    fread(&fh.bfReserved1, sizeof(fh.bfReserved1), 1, image);
    fread(&fh.bfReserved2, sizeof(fh.bfReserved2), 1, image);
    fread(&fh.bfOffBits, sizeof(fh.bfOffBits), 1, image);
    fread(&ih, sizeof(imageheader_t), 1, image);

    if((fh.bfType[0] != 'B') | (fh.bfType[1] != 'M')) {
        printf("File not .bmp\n");
        return 1;
    }

    if((ih.biSize != 40) || (ih.biCompression != 0) || (ih.biBitCount != 24)) {
        fclose(image);
        printf("Image header error\n");
        return 1;
    }

    return 0;
}

image_t readImage(char *infile) {
    int i, j;

    FILE *image = fopen(infile, "rb");
    image_t pic;
    pic.width = 0;
    pic.height = 0;
    pic.offset = 0;
    pic.rgb = NULL;

    if(!image) {
        printf("readImage open error: %s\n", infile);
        return pic;
    }

    fseek(image, 0, SEEK_SET);
    fread(pic.header, 1, HEADER_SIZE, image);
    fseek(image, 10, SEEK_SET);
    fread(&pic.offset, sizeof(pic.offset), 1, image);
    fseek(image, 4, SEEK_CUR);
    fread(&pic.width, sizeof(pic.width), 1, image);
    fread(&pic.height, sizeof(pic.height), 1, image);
    printf("width: %d, height: %d, offset: %d\n", pic.width, pic.height, pic.offset);
    fseek(image, HEADER_SIZE, SEEK_SET);
    
    pic.rgb = malloc(pic.width * pic.height * RGB_PER_PIXEL);
    if(!pic.rgb) {
        printf("pic.rgb malloc failed\n");
        fclose(image);
        return pic;
    }

    int padding = calcPadding(pic.width);
    unsigned char channel[RGB_PER_PIXEL];
    for(i = pic.height - 1; i >= 0; i--) {
        for(j = 0; j < pic.width; j++) {
            int index = i * pic.width + j;
            fread(channel, 1, RGB_PER_PIXEL, image);
            /* BGR order */
            pic.rgb[index].red = channel[2];
            pic.rgb[index].green = channel[1];
            pic.rgb[index].blue = channel[0];
        }
        fseek(image, padding, SEEK_CUR);
    }
    
    fclose(image);
    return pic;
}

void encode(char *infile, char *outfile, char *message) {
    image_t pic = readImage(infile);

    int i, j, k;
    int char_index = 0, bit_index = 0, bits_hidden = 0;
    int len = strlen(message);
    int total_bits = (len + 1) * BITS_PER_BYTE;

    for(i = 0; i < pic.height && bits_hidden < total_bits; i++) {
        for(j = 0; j < pic.width && bits_hidden < total_bits; j++) {
            int index = i * pic.width + j;
            unsigned char *channels[RGB_PER_PIXEL] = {
                &pic.rgb[index].red,
                &pic.rgb[index].green,
                &pic.rgb[index].blue
            };
            for(k = 0; k < RGB_PER_PIXEL && bits_hidden < total_bits; k++) {
                unsigned char current_char;
                if(char_index < len) {
                    current_char = message[char_index];
                } else {
                    current_char = '\0';
                }

                int bit = (current_char >> (7 - bit_index)) & 1;

                if(bit == 1) {
                    *channels[k] |= 1;
                } else {
                    *channels[k] &= ~1;
                }

                bits_hidden++;
                bit_index++;

                if(bit_index == BITS_PER_BYTE) {
                    bit_index = 0;
                    char_index++;
                }
            }
        }
    }
    
    FILE *newimage = fopen(outfile, "wb");
    if(newimage == NULL) {
        printf("Encode function open error: %s\n", outfile);
        return;
    }

    fwrite(pic.header, pic.offset, 1, newimage);
    
    int padding = calcPadding(pic.width);
    unsigned char pad[RGB_PER_PIXEL] = {0, 0, 0};
    unsigned char channel[RGB_PER_PIXEL];
    for(i = pic.height - 1; i >= 0; i--) {
        for(j = 0; j < pic.width; j++) {
            int index = i * pic.width + j;
            channel[2] = pic.rgb[index].red;
            channel[1] = pic.rgb[index].green;
            channel[0] = pic.rgb[index].blue;
            fwrite(channel, 1, RGB_PER_PIXEL, newimage);
        }
        fwrite(pad, 1, padding, newimage);
    }

    fclose(newimage);
    free(pic.rgb);
}

void decode(char *infile, char *outstring) {
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
/*********************************************************/

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
