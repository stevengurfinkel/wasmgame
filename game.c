#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"

typedef struct image_st
{
    pixel_t *pixels;
    int width;
    int height;
} image_t;

typedef struct keyboard_st
{
    int x_dir;
    int y_dir;
} keyboard_t;

typedef struct circle_st
{
    int x;
    int y;
    int r;

    int dx;
    int dy;
} circle_t;

typedef struct globals_st
{
    image_t image;
    circle_t circle;
    keyboard_t keyboard;
} globals_t;

static globals_t globals = {0};

void circle_init(circle_t *c, int x, int y, int r)
{
    c->x = x;
    c->y = y;
    c->r = r;
}

void image_init(image_t *image, int width, int height)
{
    image->width = width;
    image->height = height;

    size_t size = width * height * sizeof image->pixels[0];
    size_t page_count = (size + PAGE_SIZE) / PAGE_SIZE;

    image->pixels = memory_grow_pages(page_count);
    set_image_buffer(image->pixels);
}

void globals_init(int width, int height)
{
    set_timer_interval(20);
    image_init(&globals.image, width, height);
    circle_init(&globals.circle, width / 2, height / 2, 120);
}

void draw_circle(image_t *img, circle_t *c)
{
    int r2 = c->r * c->r;

    for (int i = 0; i < img->height; i++) {
        int y2 = (i - c->y) * (i - c->y);

        for (int j = 0; j < img->width; j++) {
            int x2 = (j - c->x) * (j - c->x);
            pixel_t *p = &img->pixels[i * img->width + j];

            uint8_t color = 0;
            if (x2 + y2 < r2) {
                color = 255;
            }
            p->r = p->b = p->g = color;
            p->a = 255;
        }
    }
}

void on_keydown(int key_code)
{
    keyboard_t *k = &globals.keyboard;
    k->x_dir = 0;
    k->y_dir = 0;

    switch (key_code) {
        case KEY_CODE_ARROW_LEFT:
            k->x_dir = -1;
            break;
        case KEY_CODE_ARROW_UP:
            k->y_dir = -1;
            break;
        case KEY_CODE_ARROW_RIGHT:
            k->x_dir = 1;
            break;
        case KEY_CODE_ARROW_DOWN:
            k->y_dir = 1;
            break;
        default:
            break;
    }
}

void on_keyup(int key_code)
{
    keyboard_t *k = &globals.keyboard;
    k->x_dir = 0;
    k->y_dir = 0;
}

void on_timer(int64_t now)
{
    circle_t *c = &globals.circle;
    keyboard_t *k = &globals.keyboard;

    c->dx += k->x_dir;
    c->dy += k->y_dir;

    c->x += c->dx / 32;
    c->y += c->dy / 32;

    image_t *img = &globals.image;
    c->x = MIN(img->width, MAX(0, c->x));
    c->y = MIN(img->height, MAX(0, c->y));

    draw_circle(img, c);
}

void on_mousedown(int x, int y)
{
}
