#pragma once

// C standard library
size_t strlen(const char *s);
void *memset(void *s, int c, size_t n);

// Custom
void log_str(const char *msg);

// Implemented in javascript

typedef struct pixel_st
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} pixel_t;

void set_image_buffer(pixel_t *pixels);
void *memory_grow_pages(size_t bytes);
void log_int(int value);
void log_str_with_size(const char *msg, size_t size);
