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
        return 1;
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
