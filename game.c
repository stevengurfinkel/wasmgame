#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"

#define FRAME_RATE 50
#define LOG_INTERVAL 64

typedef struct image_st
{
    pixel_t *pixels;
    int width;
    int height;
} image_t;

typedef struct keyboard_st
{
    bool is_pressed[KEY_CODE_COUNT];
    int x_dir;
    int y_dir;
} keyboard_t;

typedef struct circle_st
{
    float x;
    float y;
    float r;

    float dx;
    float dy;
} circle_t;

typedef struct rect_st
{
    float x;
    float y;
    float width;
    float height;
} rect_t;

typedef struct globals_st
{
    image_t image;
    circle_t circle;
    keyboard_t keyboard;
    uint64_t frameId;
} globals_t;

static globals_t globals = {0};

void circle_init(circle_t *c, float x, float y, float r)
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
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            image->pixels[i * width + j].a = 255;
        }
    }
}

void globals_init(int width, int height)
{
    set_timer_interval(1000 / FRAME_RATE);
    image_init(&globals.image, width, height);
    circle_init(&globals.circle, width / 2, height / 2, 120);
}

void draw_rectangle(image_t *img, rect_t *rect)
{
    int x_lo = MAX(0, round_down(rect->x));
    int x_hi = MIN(img->width, round_up(rect->x + rect->width));

    int y_lo = MAX(0, round_down(rect->y));
    int y_hi = MIN(img->height, round_up(rect->y + rect->height));

    for (int i = y_lo; i < y_hi; i++) {
        for (int j = x_lo; j < x_hi; j++) {
            pixel_t *p = &img->pixels[i * img->width + j];
            p->r = p->g = p->b = 0;
        }
    }
}

void draw_circle(image_t *img, circle_t *c)
{
    float r2 = c->r * c->r;

    for (int i = 0; i < img->height; i++) {
        float y2 = (i - c->y) * (i - c->y);

        for (int j = 0; j < img->width; j++) {
            float x2 = (j - c->x) * (j - c->x);
            pixel_t *p = &img->pixels[i * img->width + j];

            uint8_t color = 0;
            if (x2 + y2 < r2) {
                color = 255;
            }
            p->r = p->b = p->g = color;
        }
    }
}

void keyboard_calc_dir(keyboard_t *k)
{
    k->x_dir = k->is_pressed[KEY_CODE_ARROW_RIGHT] - k->is_pressed[KEY_CODE_ARROW_LEFT];
    k->y_dir = k->is_pressed[KEY_CODE_ARROW_DOWN] - k->is_pressed[KEY_CODE_ARROW_UP];
}

void on_keydown(int key_code)
{
    keyboard_t *k = &globals.keyboard;
    k->is_pressed[key_code] = true;
    keyboard_calc_dir(&globals.keyboard);
}

void on_keyup(int key_code)
{
    keyboard_t *k = &globals.keyboard;
    k->is_pressed[key_code] = false;
    keyboard_calc_dir(&globals.keyboard);
}

void circle_dir_detect_oob(float *pos, float *dpos, float r, int limit)
{
    float p = *pos;
    float d = *dpos;

    if (p - r < 0) {
        p = r;
        d *= -1;
    }
    else if (p + r > limit) {
        p = limit - r;
        d *= -1;
    }

    *pos = p;
    *dpos = d;
}

void circle_detect_oob(circle_t *circle, image_t *image)
{
    circle_dir_detect_oob(&circle->x, &circle->dx, circle->r, image->width);
    circle_dir_detect_oob(&circle->y, &circle->dy, circle->r, image->height);
}

void on_timer(float now)
{
    circle_t *c = &globals.circle;
    keyboard_t *k = &globals.keyboard;

    c->dx += 0.03 * k->x_dir;
    c->dy += 0.03 * k->y_dir;

    float drag = 1 - (0.0001 * (c->dx * c->dx + c->dy * c->dy));
    c->dx *= drag;
    c->dy *= drag;

    c->x += c->dx;
    c->y += c->dy;

    image_t *img = &globals.image;
    circle_detect_oob(c, img);

    draw_circle(img, c);
    rect_t r = { .x = c->x - 10,
                 .y = c->y - 10,
                 .width = 20,
                 .height = 20 };
    draw_rectangle(img, &r);

    if (globals.frameId % LOG_INTERVAL == 0) {
    }
    globals.frameId++;
}

void on_mousedown(int x, int y)
{
}
