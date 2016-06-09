#include <stddef.h>

#ifndef fb_h
#define fb_h

typedef enum {
    NONE,
    VERTICAL,
    HORIZONTAL
} direction;

typedef struct {
    uint16_t bfType;   /* "BM" */
    uint32_t bfSize;           /* Size of file in bytes */
    uint16_t bfReserved;       /* set to 0 */
    uint16_t bgReserved;       /* set to 0 */
    uint32_t bfOffBits;        /* Byte offset to actual bitmap data (= 54) */
    uint32_t biSize;           /* Size of BITMAPINFOHEADER, in bytes (= 40) */
    uint32_t biWidth;          /* Width of image, in pixels */
    uint32_t biHeight;         /* Height of images, in pixels */
    uint16_t biPlanes;       /* Number of planes in target device (set to 1) */
    uint16_t biBitCount;     /* Bits per pixel (24 in this case) */
    uint32_t biCompression;    /* Type of compression (0 if no compression) */
    uint32_t biSizeImage;      /* Image size, in bytes (0 if no compression) */
    uint32_t biXPelsPerMeter;  /* Resolution in pixels/meter of display device */
    uint32_t biYPelsPerMeter;  /* Resolution in pixels/meter of display device */
    uint32_t biClrUsed;        /* Number of colors in the color table (if 0, use 
                             maximum allowed by biBitCount) */
    uint32_t biClrImportant;   /* Number of important colors.  If 0, all colors 
                             are important */
}__attribute__((packed)) bitmap_header_t;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
}__attribute__((packed)) rgb_t;

typedef struct {
    rgb_t A;
    rgb_t B;
    direction dir;
} __attribute__((packed)) gradient_t;

void fb_init(void);
void draw_bitmap(const char *img, size_t x, size_t y, int scale);
size_t width(void);
size_t height(void);
void draw_rect(gradient_t, size_t, size_t, size_t, size_t);
void draw_circle(rgb_t, size_t, size_t, size_t);


#endif
