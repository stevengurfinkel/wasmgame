#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"

#define FRAME_RATE 50
#define LOG_INTERVAL 64

#define SPRITE_RADIUS 24
#define SPRITE_DIM 6
#define SPRITE_COUNT (SPRITE_DIM * SPRITE_DIM)

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
} circle_t;

typedef struct cirle_sprite_st
{
    circle_t circle;
    pixel_t color;
    float dx;
    float dy;
    bool hidden;
} circle_sprite_t;

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
    circle_sprite_t *player;
    circle_sprite_t sprites[SPRITE_COUNT + 1];
    keyboard_t keyboard;
    pixel_t color;
    int collision_count;
    uint64_t frameId;
} globals_t;

static globals_t globals = {0};

void spectral_color(pixel_t *p, int bits, int idx);

void circle_init(circle_t *c, float x, float y, float r)
{
    c->x = x;
    c->y = y;
    c->r = r;
}

void pixel_init(pixel_t *p, int r, int g, int b, int a)
{
    p->r = r;
    p->g = g;
    p->b = b;
    p->a = a;
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

void circle_sprite_init(circle_sprite_t *sprite, int idx)
{
    const image_t *img = &globals.image;
    int cell_height = img->height / SPRITE_DIM;
    int cell_width = img->width / SPRITE_DIM;

    int x = cell_width * (idx % SPRITE_DIM) + cell_width / 2;
    int y = cell_height * (idx / SPRITE_DIM) + cell_height / 2;

    circle_init(&sprite->circle, x, y, SPRITE_RADIUS);
    spectral_color(&sprite->color, 2, idx);
}

void globals_init(int width, int height)
{
    set_timer_interval(1000 / FRAME_RATE);
    image_init(&globals.image, width, height);
    globals.player = &globals.sprites[SPRITE_COUNT + 1];
    circle_init(&globals.player->circle, width / 2, height / 2, SPRITE_RADIUS);
    pixel_init(&globals.player->color, 255, 255, 255, 255);
    for (int i = 0; i < SPRITE_COUNT; i++) {
        circle_sprite_init(&globals.sprites[i], i);
    }
}

void draw_rectangle(image_t *img, rect_t *rect, pixel_t *pcolor)
{
    pixel_t color = *pcolor;
    int x_lo = MAX(0, round_down(rect->x));
    int x_hi = MIN(img->width, round_up(rect->x + rect->width));

    int y_lo = MAX(0, round_down(rect->y));
    int y_hi = MIN(img->height, round_up(rect->y + rect->height));

    for (int i = y_lo; i < y_hi; i++) {
        for (int j = x_lo; j < x_hi; j++) {
            pixel_t *p = &img->pixels[i * img->width + j];
            *p = color;
        }
    }
}

void image_clear(image_t *img)
{
    pixel_t p = { .r = 0, .g = 0, .b = 0, .a = 255 };
    for (int i = 0; i < img->width * img->height; i++) {
        img->pixels[i] = p;
    }
}

void draw_circle(image_t *img, circle_t *c, pixel_t *pcolor)
{
    pixel_t color = *pcolor;

    int x_lo = MAX(0, round_down(c->x - c->r));
    int x_hi = MIN(img->width, round_up(c->x + c->r));

    int y_lo = MAX(0, round_down(c->y - c->r));
    int y_hi = MIN(img->height, round_up(c->y + c->r));

    float r2 = c->r * c->r;

    for (int i = y_lo; i < y_hi; i++) {
        float y2 = (i - c->y) * (i - c->y);

        for (int j = x_lo; j < x_hi; j++) {
            float x2 = (j - c->x) * (j - c->x);
            pixel_t *p = &img->pixels[i * img->width + j];
            if (x2 + y2 < r2) {
                *p = color;
            }
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

bool circle_dir_detect_oob(float *pos, float *dpos, float r, int limit)
{
    float p = *pos;
    float d = *dpos;
    bool is_oob = false;

    if (p - r < 0) {
        p = r;
        d *= -1;
        is_oob = true;
    }
    else if (p + r > limit) {
        p = limit - r;
        d *= -1;
        is_oob = true;
    }

    *pos = p;
    *dpos = d;
    return is_oob;
}

bool sprite_detect_oob(circle_sprite_t *sprite)
{
    circle_t *circle = &sprite->circle;
    bool x = circle_dir_detect_oob(&circle->x, &sprite->dx, circle->r, globals.image.width);
    bool y = circle_dir_detect_oob(&circle->y, &sprite->dy, circle->r, globals.image.height);
    return x || y;
}

int spectral_color_value(int count, int max_value)
{
    int period = 3 * max_value;
    int i = count % period;
    if (i < max_value) {
        return 0;
    }
    if (i < 2 * max_value) {
        return i - max_value;
    }
    return period - i;
}

void spectral_color(pixel_t *p, int bits, int idx)
{
    int max_value = (1 << bits) - 1;
    int shift = 8 - bits;

    log_int(spectral_color_value(idx, max_value));
    p->r = spectral_color_value(idx, max_value) << shift;
    p->g = spectral_color_value(idx + max_value, max_value) << shift;
    p->b = spectral_color_value(idx + 2 * max_value, max_value) << shift;
    p->a = 0xFF;
}

void change_color(pixel_t *p, int count)
{
    spectral_color(p, 2, count);
}

bool circle_sprite_move(circle_sprite_t *sprite)
{
    float drag = 1 - (0.0001 * (sprite->dx * sprite->dx + sprite->dy * sprite->dy));
    sprite->dx *= drag;
    sprite->dy *= drag;

    circle_t *circle = &sprite->circle;
    circle->x += sprite->dx;
    circle->y += sprite->dy;

    return sprite_detect_oob(sprite);
}

void player_move(circle_sprite_t *player)
{
    keyboard_t *keys = &globals.keyboard;
    player->dx += 0.03 * keys->x_dir;
    player->dy += 0.03 * keys->y_dir;
}

bool sprite_check_collision(circle_sprite_t *s1, circle_sprite_t *s2)
{
    float dx = s1->circle.x - s2->circle.x;
    float dy = s1->circle.y - s2->circle.y;

    float r = s1->circle.r + s2->circle.r;

    return dx * dx + dy * dy < r * r;
}

void on_timer(float now)
{
    circle_sprite_t *player = globals.player;
    player_move(player);

    circle_sprite_move(player);

    image_t *img = &globals.image;
    image_clear(img);
    for (int i = 0; i < SPRITE_COUNT; i++) {
        circle_sprite_t *sprite = &globals.sprites[i];
        if (sprite_check_collision(player, sprite)) {
            sprite->hidden = true;
        }
        if (!sprite->hidden) {
            draw_circle(img, &sprite->circle, &sprite->color);
        }
    }

    draw_circle(img, &player->circle, &player->color);
    globals.frameId++;
}

void on_mousedown(int x, int y)
{
}
