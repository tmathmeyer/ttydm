#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/fb.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include "fb.h"

static int fb_fd = 0;
static uint8_t *fbp = 0;
static size_t WIDTH, HEIGHT, __INIT__ = 0;

static struct fb_fix_screeninfo finfo;
static struct fb_var_screeninfo vinfo;

size_t width(void) {
    return WIDTH;
}

size_t height(void) {
    return HEIGHT;
}

void fb_init(void) {
    fb_fd = open("/dev/fb0",O_RDWR);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    vinfo.grayscale=0;
    vinfo.bits_per_pixel=32;

    ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

    ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
    long screensize = vinfo.yres_virtual * finfo.line_length;
    WIDTH = vinfo.xres;
    HEIGHT = vinfo.yres;

    fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
    __INIT__ = 1;
}

#define IC() do { \
    if (!__INIT__) { \
        perror("FB not initialized\n"); \
        exit(1); \
    } \
} while(0)

static inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo *vinfo) {
    return (r<<vinfo->red.offset) | (g<<vinfo->green.offset) | (b<<vinfo->blue.offset);
}

static inline void framebuffer(int x, int y, rgb_t draw) {
    if (x<0 || y<0 || x>=WIDTH || y>=HEIGHT) return;
    long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8)
        + (y+vinfo.yoffset) * finfo.line_length;
    *((uint32_t*)(fbp + location))=pixel_color(draw.r, draw.g, draw.b, &vinfo);
}

void draw_circle(rgb_t color, size_t x, size_t y, size_t r) {
    int R = (int)r;
    for(int a=-R; a<R; a++) {
        for(int b=-R; b<R; b++) {
            if (a*a+b*b<r*r) {
                framebuffer(x+a, y+b, color);
            }
        }
    }
}

void draw_rect(gradient_t color, size_t x, size_t y, size_t w, size_t h) {
    switch(color.dir) {
        case NONE: break;
        case VERTICAL:
        case HORIZONTAL:
             perror("NOT IMPLEMENTED\n");
             exit(1);
    }

    for(ssize_t a=0; a<w; a++) {
        for(ssize_t b=0; b<h; b++) {
            framebuffer(x+a, y+b, color.A);
        }
    }
}

void draw_bitmap(const char *img, size_t at_x, size_t at_y, int scale) {
    IC();

    FILE *bitmap = fopen(img, "rb");
    if (!bitmap) {
        fprintf(stderr, "ERROR: bitmap file (%s) not accessable\n", img);
        return;
    }

    bitmap_header_t file_header;
    fread(&file_header, sizeof(bitmap_header_t), 1, bitmap);

    if (file_header.bfType != 0x4d42) {
        fprintf(stderr, "ERROR: bitmap file (%s) has invalid header\n", img);
        fclose(bitmap);
        return;
    }

    fseek(bitmap, file_header.bfOffBits, SEEK_SET);
    //printf("bits = %i\n", file_header.biBitCount);
    //printf("size = (%ix%i)\n", file_header.biWidth, file_header.biHeight);
    uint8_t *im = (uint8_t *)malloc(file_header.biSizeImage);
    if (!im) {
        perror("out of memory");
        fclose(bitmap);
        return;
    }
    fread(im, 1, file_header.biSizeImage, bitmap);

    int bytesPerLine = (file_header.biBitCount*file_header.biWidth + 31) / 32;
    bytesPerLine *= 4;

    for(int py=0; py<file_header.biHeight; py+=scale) {
        for(int px=0; px<file_header.biWidth; px+=scale) {
            int moffset = (py * bytesPerLine) + (px * file_header.biBitCount / 8);
            rgb_t rgb = {
                .r=im[moffset+2],
                .g=im[moffset+1],
                .b=im[moffset+0],
            };

            int fx = (px/scale) + at_x;
            int fy = ((file_header.biHeight - py)/scale) + at_y;
            framebuffer(fx, fy, rgb);
        }
    }
    free(im);   
}


