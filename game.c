#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"
#include "gfx.h"

#define FRAME_RATE 50
#define LOG_INTERVAL 64
#define ONE_OVER_ROOT_TWO 0.7071067811865475

#define DRAG_COEF 0.0005
#define PLAYER_ACCEL 0.08

#define PLAYER_RADIUS 36
#define PLAYER_MASS 2

#define SPRITE_ACCEL 0.12
#define SPRITE_RADIUS 24
#define SPRITE_DIM 6
#define SPRITE_MAX_DAMAGE 4
#define SPRITE_COUNT (SPRITE_DIM * SPRITE_DIM)

typedef struct keyboard_st
{
    bool is_pressed[KEY_CODE_COUNT];
    float dir[DIM];
} keyboard_t;

typedef struct cirle_sprite_st
{
    circle_t circle;
    pixel_t color;
    float vel[DIM];
    float accel_norm[DIM];
    float mass;
    int damage_count;
} circle_sprite_t;

typedef struct globals_st
{
    image_t image;
    circle_sprite_t *player;
    int sprite_count;
    circle_sprite_t *sprites[SPRITE_COUNT + 1];
    circle_sprite_t all_sprites[SPRITE_COUNT + 1];
    keyboard_t keyboard;
    pixel_t color;
    int collision_count;
    uint64_t frameId;
    int win_frame_id;
} globals_t;

static globals_t globals = {0};

void spectral_color(pixel_t *p, int bits, int idx);



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
    globals.player = &globals.all_sprites[SPRITE_COUNT];
    circle_init(&globals.player->circle, width / 2, height / 2, PLAYER_RADIUS);
    pixel_init(&globals.player->color, 255, 255, 255, 255);
    globals.player->mass = PLAYER_MASS;
    for (int i = 0; i < SPRITE_COUNT; i++) {
        circle_sprite_init(&globals.all_sprites[i], i);
    }
    globals.sprite_count = ARRAY_COUNT(globals.sprites);
    for (int i = 0; i < globals.sprite_count; i++) {
        globals.sprites[i] = &globals.all_sprites[i];
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
    for (int i = 0; i < DIM; i++) {
        player->vel[i] += PLAYER_ACCEL * keys->dir[i];
        player->accel_norm[i] = keys->dir[i];
    }
}

void sprite_escape(circle_sprite_t *sprite)
{
    circle_sprite_t *player = globals.player;
    float away[DIM];
    float dist = 0;
    bool player_accel = false;
    for (int i = 0; i < DIM; i++) {
        player_accel = player_accel || player->accel_norm[i] != 0;
        away[i] = sprite->circle.pos[i] - player->circle.pos[i];
        dist += away[i] * away[i];
    }
    dist = sqrtf(dist);
    if (player_accel && dist < 2 * (player->circle.r + sprite->circle.r)) {
        for (int i = 0; i < DIM; i++) {
            sprite->accel_norm[i] = (away[i] / dist);
            sprite->vel[i] += SPRITE_ACCEL * sprite->accel_norm[i];
        }
    }
    else {
        sprite->accel_norm[X] = sprite->accel_norm[Y] = 0;
    }
}

void circle_sprite_update_damage(circle_sprite_t *s)
{
    s->damage_count++;
    if (s->damage_count < SPRITE_MAX_DAMAGE) {
        pixel_t *color = &s->color;
        int impact = 16;
        color->r = MAX(color->r - impact, 0);
        color->g = MAX(color->g - impact, 0);
        color->b = MAX(color->b - impact, 0);
    }
    globals.player->circle.r++;
}

void circle_sprite_collide(circle_sprite_t *s1, circle_sprite_t *s2)
{
    float dpos[DIM]; // distance between center points
    float dist_sq = 0; // square of the distance
    float dr = s1->circle.r + s2->circle.r; // collision distance

    // calculate the distance between sprites
    for (int i = 0; i < DIM; i++) {
        dpos[i] = s1->circle.pos[i] - s2->circle.pos[i];
        dist_sq += dpos[i] * dpos[i];
    }

    // if they're too close, it's a collision
    if (dist_sq < dr * dr) {
        // calculate the extent to which the collision plane
        // and the sprite's relative velocities are in the same direction
        // using a dot-product
        float v_dot_p = 0;
        for (int i = 0; i < DIM; i++) {
            v_dot_p += dpos[i] * (s1->vel[i] - s2->vel[i]);
        }

        // They're already moving away from eachother
        // don't change the vector or they might get stuck
        if (v_dot_p > 0) {
            return;
        }

        // factor-out some common fields
        float factor = 2 * v_dot_p / (dist_sq * (s1->mass + s2->mass));

        // update the velocities
        for (int i = 0; i < DIM; i++) {
            s1->vel[i] -= s2->mass * factor * dpos[i];
            s2->vel[i] += s1->mass * factor * dpos[i];
        }

        if (s1 == globals.player) {
            circle_sprite_update_damage(s2);
        }
        else if (s2 == globals.player) {
            circle_sprite_update_damage(s1);
        }
    }
}

void win_animation_play(image_t *img, circle_sprite_t *player)
{
    circle_t circle = {0};
    pixel_t black = { .r = 0, .g = 0, .b = 0, .a = 255 };
    pixel_t white = { .r = 255, .g = 255, .b = 255, .a = 255 };

    int thickness = 20;
    int max_radius = MAX(img->size[X], img->size[Y]);
    int frame = globals.win_frame_id += 8;

    circle_init(&circle, player->circle.pos[X], player->circle.pos[Y], 0);

    if (frame < max_radius) {
        circle.r = frame;
        draw_circle(img, &circle, &white);
        if (frame > thickness) {
            circle.r -= thickness;
        }
        draw_circle(img, &circle, &black);
    }
    else if (frame < 2 * max_radius) {
        frame -= max_radius;
        circle.r = frame;
        draw_circle(img, &circle, &white);
    }
    else if (frame < 3 * max_radius) {
        player->circle.r = PLAYER_RADIUS;
        frame -= 2 * max_radius;
        circle.r = max_radius;
        draw_circle(img, &circle, &white);
        circle.r = frame;
        draw_circle(img, &circle, &black);
    }
    else {
        globals.win_frame_id = 0;
    }
}

void draw_jet(image_t *img, circle_sprite_t *sprite)
{
    float sizes[] = {0.25, 0.30, 0.35};
    float dist = sprite->circle.r;
    pixel_t yellow = { .r = 255, .g = 255, .b = 0, .a = 255 };

    for (int i = 0; i < ARRAY_COUNT(sizes); i++) {
        circle_t jet = {0};
        sizes[i] *= SPRITE_RADIUS;
        for (int i = 0; i < DIM; i++) {
            jet.pos[i] = sprite->circle.pos[i] - dist * sprite->accel_norm[i];
        }
        jet.r = sizes[i];
        draw_circle(img, &jet, &yellow);
        dist += sizes[i];
    }
}

void on_timer(float now)
{
    circle_sprite_t *player = globals.player;
    player_move(player);

    int count = globals.sprite_count;

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < i; j++) {
            circle_sprite_collide(globals.sprites[i], globals.sprites[j]);
        }
    }

    image_t *img = &globals.image;
    image_clear(img);

    if (globals.win_frame_id > 0) {
        win_animation_play(img, player);
    }

    for (int i = 0; i < count; i++) {
        circle_sprite_t *sprite = globals.sprites[i];
        if (sprite != player) {
            sprite_escape(sprite);
        }
        circle_sprite_move(sprite);
        if (globals.win_frame_id == 0) {
            draw_jet(img, sprite);
        }
    }

    for (int i = 0; i < count; i++) {
        circle_sprite_t *sprite = globals.sprites[i];
        draw_circle(img, &sprite->circle, &sprite->color);
    }

    int new_count = 0;
    for (int i = 0; i < count; i++) {
        if (globals.sprites[i]->damage_count < SPRITE_MAX_DAMAGE) {
            globals.sprites[new_count++] = globals.sprites[i];
        }
    }

    // if we just won, start the win animation
    if (new_count == 1 && globals.sprite_count > 1) {
        globals.win_frame_id = 1;
    }

    globals.sprite_count = new_count;
    globals.frameId++;
}

void on_mousedown(int x, int y)
{
    if (globals.sprite_count == 1) {
        globals.win_frame_id = 1;
    }
}
