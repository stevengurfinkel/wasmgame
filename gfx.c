#include "gfx.h"

void circle_init(circle_t *c, float x, float y, float r)
{
    c->pos[X] = x;
    c->pos[Y] = y;
    c->r = r;
}

void pixel_init(pixel_t *p, int r, int g, int b, int a)
{
    p->r = r;
    p->g = g;
    p->b = b;
    p->a = a;
}

void image_draw_bitmap(image_t *img, const int *offset, const bitmap_t *bmp)
{
    int count[DIM];
    for (int i = 0; i < DIM; i++) {
        count[i] = MIN(bmp->size[i], img->size[i] - offset[i]);
    }

    for (int i = 0; i < count[Y]; i++) {
        const pixel_t *row_in = &bmp->pixels[i * bmp->pixel_stride_x];
        pixel_t *row_out = &img->pixels[(i * offset[Y]) * img->size[X] + offset[X]];

        for (int j = 0; j < count[X]; j++) {
            if (row_in[j].a != 0) {
                row_out[j] = row_in[j];
                row_out[j].a = 255;
            }
        }
    }
}

void draw_rectangle(image_t *img, rect_t *rect, pixel_t *pcolor)
{
    pixel_t color = *pcolor;
    int x_lo = MAX(0, round_down(rect->start[X]));
    int x_hi = MIN(img->size[X], round_up(rect->start[X] + rect->size[X]));

    int y_lo = MAX(0, round_down(rect->start[Y]));
    int y_hi = MIN(img->size[Y], round_up(rect->start[Y] + rect->size[Y]));

    for (int i = y_lo; i < y_hi; i++) {
        for (int j = x_lo; j < x_hi; j++) {
            pixel_t *p = &img->pixels[i * img->size[X] + j];
            *p = color;
        }
    }
}

void image_clear(image_t *img)
{
    pixel_t p = { .r = 0, .g = 0, .b = 0, .a = 255 };
    for (int i = 0; i < img->size[X] * img->size[Y]; i++) {
        img->pixels[i] = p;
    }
}

void draw_circle(image_t *img, circle_t *c, pixel_t *pcolor)
{
    pixel_t color = *pcolor;

    int lo[DIM];
    int hi[DIM];

    float r2 = c->r * c->r;

    for (int i = 0; i < DIM; i++) {
        lo[i] = MAX(0, round_down(c->pos[i] - c->r));
        hi[i] = MIN(img->size[i], round_up(c->pos[i] + c->r));
    }

    for (int i = lo[Y]; i < hi[Y]; i++) {
        float y2 = (i - c->pos[Y]) * (i - c->pos[Y]);

        for (int j = lo[X]; j < hi[X]; j++) {
            float x2 = (j - c->pos[X]) * (j - c->pos[X]);
            pixel_t *p = &img->pixels[i * img->size[X] + j];
            if (x2 + y2 < r2) {
                *p = color;
            }
        }
    }
}


