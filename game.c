#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"

#define FRAME_RATE 50
#define LOG_INTERVAL 64
#define ONE_OVER_ROOT_TWO 0.7071067811865475

#define DRAG_COEF 0.0005
#define PLAYER_ACCEL 0.08

#define PLAYER_RADIUS 48
#define PLAYER_MASS 4

#define SPRITE_RADIUS 24
#define SPRITE_DIM 6
#define SPRITE_COUNT (SPRITE_DIM * SPRITE_DIM)

#define X 0
#define Y 1
#define DIM 2

typedef struct image_st
{
    pixel_t *pixels;
    int size[DIM];
} image_t;

typedef struct keyboard_st
{
    bool is_pressed[KEY_CODE_COUNT];
    float dir[DIM];
} keyboard_t;

typedef struct circle_st
{
    float pos[DIM];
    float r;
} circle_t;

typedef struct cirle_sprite_st
{
    circle_t circle;
    pixel_t color;
    float vel[DIM];
    float mass;
} circle_sprite_t;

typedef struct rect_st
{
    float start[DIM];
    float size[DIM];
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

void image_init(image_t *image, int width, int height)
{
    image->size[X] = width;
    image->size[Y] = height;

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
    int cell_height = img->size[Y] / SPRITE_DIM;
    int cell_width = img->size[X] / SPRITE_DIM;

    int x = cell_width * (idx % SPRITE_DIM) + cell_width / 2;
    int y = cell_height * (idx / SPRITE_DIM) + cell_height / 2;

    circle_init(&sprite->circle, x, y, SPRITE_RADIUS);
    spectral_color(&sprite->color, 2, idx);
    sprite->mass = 1;
}

void globals_init(int width, int height)
{
    set_timer_interval(1000 / FRAME_RATE);
    image_init(&globals.image, width, height);
    globals.player = &globals.sprites[SPRITE_COUNT];
    circle_init(&globals.player->circle, width / 2, height / 2, PLAYER_RADIUS);
    pixel_init(&globals.player->color, 255, 255, 255, 255);
    globals.player->mass = PLAYER_MASS;
    for (int i = 0; i < SPRITE_COUNT; i++) {
        circle_sprite_init(&globals.sprites[i], i);
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

void keyboard_calc_dir(keyboard_t *k)
{
    k->dir[X] = k->is_pressed[KEY_CODE_ARROW_RIGHT] - k->is_pressed[KEY_CODE_ARROW_LEFT];
    k->dir[Y] = k->is_pressed[KEY_CODE_ARROW_DOWN] - k->is_pressed[KEY_CODE_ARROW_UP];
    if (k->dir[X] != 0 && k->dir[Y] != 0) {
        k->dir[X] *= ONE_OVER_ROOT_TWO;
        k->dir[Y] *= ONE_OVER_ROOT_TWO;
    }
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

bool shape_bounce_off_edge(float *pos, float *vel, float r, const int *limit)
{
    bool is_oob = false;
    for (int i = 0; i < DIM; i++) {
        if (pos[i] - r < 0) {
            pos[i] = r;
            vel[i] *= -1;
            is_oob = true;
        }
        else if (pos[i] + r > limit[i]) {
            pos[i] = limit[i] - r;
            vel[i] *= -1;
            is_oob = true;
        }
    }
    return is_oob;
}

bool circle_sprite_bounce_off_edge(circle_sprite_t *sprite)
{
    circle_t *circle = &sprite->circle;
    return shape_bounce_off_edge(circle->pos, sprite->vel, circle->r, globals.image.size);
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
    float vel2 = sprite->vel[X] * sprite->vel[X] + sprite->vel[Y] * sprite->vel[Y];
    if (vel2 > 0) {
        float speed = sqrtf(vel2);
        float norm_x = sprite->vel[X] / speed;
        float norm_y = sprite->vel[Y] / speed;

        sprite->vel[X] -= DRAG_COEF * norm_x * vel2;
        sprite->vel[Y] -= DRAG_COEF * norm_y * vel2;
    }

    circle_t *circle = &sprite->circle;
    circle->pos[X] += sprite->vel[X];
    circle->pos[Y] += sprite->vel[Y];

    return circle_sprite_bounce_off_edge(sprite);
}

void player_move(circle_sprite_t *player)
{
    keyboard_t *keys = &globals.keyboard;
    player->vel[X] += PLAYER_ACCEL * keys->dir[X];
    player->vel[Y] += PLAYER_ACCEL * keys->dir[Y];
}

void circle_sprite_collide(circle_sprite_t *s1, circle_sprite_t *s2)
{
    float dpos[DIM];
    float dist_sq = 0;
    float dr = s1->circle.r + s2->circle.r;

    float masses = s1->mass + s2->mass;

    for (int i = 0; i < DIM; i++) {
        dpos[i] = s1->circle.pos[i] - s2->circle.pos[i];
        dist_sq += dpos[i] * dpos[i];
    }

    if (dist_sq < dr * dr) {
        float v_dot_p = 0;
        for (int i = 0; i < DIM; i++) {
            v_dot_p += dpos[i] * (s1->vel[i] - s2->vel[i]);
        }

        float factor = v_dot_p / dist_sq;

        for (int i = 0; i < DIM; i++) {
            s1->vel[i] -= 2 * s2->mass * factor * dpos[i] / masses;
            s2->vel[i] += 2 * s1->mass * factor * dpos[i] / masses;
        }
    }
}

void on_timer(float now)
{
    circle_sprite_t *player = globals.player;
    player_move(player);

    int count = ARRAY_COUNT(globals.sprites);

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < i; j++) {
            circle_sprite_collide(&globals.sprites[i], &globals.sprites[j]);
        }
    }

    image_t *img = &globals.image;
    image_clear(img);

    for (int i = 0; i < count; i++) {
        circle_sprite_t *sprite = &globals.sprites[i];
        circle_sprite_move(sprite);
        draw_circle(img, &sprite->circle, &sprite->color);
    }

    globals.frameId++;
}

void on_mousedown(int x, int y)
{
}
