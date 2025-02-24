#pragma once

// C standard library
size_t strlen(const char *s);
void *memset(void *s, int c, size_t n);
float fabsf(float x);
float sqrtf(float x);

// Custom
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ARRAY_COUNT(array) (sizeof (array) / sizeof (array)[0])

void log_str(const char *msg);
int round_down(float f);
int round_up(float f);

// Implemented in javascript
#define PAGE_SIZE (64 * 1024)

#define KEY_CODE_ARROW_LEFT 37
#define KEY_CODE_ARROW_UP 38
#define KEY_CODE_ARROW_RIGHT 39
#define KEY_CODE_ARROW_DOWN 40
#define KEY_CODE_COUNT 256

typedef struct pixel_st
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} pixel_t;

void set_image_buffer(pixel_t *pixels);
void *memory_grow_pages(size_t bytes);
void log_int(long value);
void log_float(float value);
void log_i64(int64_t value);
void log_str_with_size(const char *msg, size_t size);
void set_timer_interval(int ms);
