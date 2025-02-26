#pragma once
#include "wasmlib.h"

#define X 0
#define Y 1
#define DIM 2

typedef struct image_st
{
    pixel_t *pixels;
    int size[DIM];
} image_t;

typedef struct circle_st
{
    float pos[DIM];
    float r;
} circle_t;

typedef struct bitmap_st
{
    pixel_t *pixels;
    int size[DIM];
    int pixel_stride_x;
} bitmap_t;

typedef struct rect_st
{
    float start[DIM];
    float size[DIM];
} rect_t;

void circle_init(circle_t *c, float x, float y, float r);
void pixel_init(pixel_t *p, int r, int g, int b, int a);
void image_draw_bitmap(image_t *img, const int *offset, const bitmap_t *bmp);
void draw_rectangle(image_t *img, rect_t *rect, pixel_t *pcolor);
void image_clear(image_t *img);
void draw_circle(image_t *img, circle_t *c, pixel_t *pcolor);
