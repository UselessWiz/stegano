#ifndef STEGANO
#define STEGANO
#include <stdio.h>

#define MAX_SIZE 10

/*********************************************************/
#define FILEHEADER_SIZE 14
#define BFTYPE_SIZE 2
#define IMAGEHEADER_SIZE 40
#define HEADER_SIZE (FILEHEADER_SIZE + IMAGEHEADER_SIZE)
#define BITS_PER_BYTE 8
#define RGB_PER_PIXEL 3

#define START_BYTE 0
#define OFFSET_BYTE 10
#define WIDTH_BYTE 18
#define HEIGHT_BYTE 22

#define MAX_MESSAGE_SIZE 256

/***** Encode, decode *****/
typedef struct {
    unsigned char bfType[BFTYPE_SIZE];
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} fileheader_t;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} imageheader_t;

typedef struct {
    unsigned char red, green, blue;
} rgb_t;

typedef struct {
    int width;
    int height;
    unsigned int offset;
    rgb_t *rgb;
    unsigned char header[HEADER_SIZE];
} image_t;
/*********************************************************/

/*********************************************************/
/* Calculates padding needed for each row in the image */
int calcPadding(int width);

/* Checks for correct BMP file type and format */
int checkFileType(char *filename);

/* Read image, check for correct file format */
image_t readImage(char *infile);

/* Encode message into image */
void encode(char *infile, char *outfile, char *message);

/* Decode message from image */
void decode(char *infile, char *outstring);
/*********************************************************/
#endif
