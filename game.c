#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE (64 * 1024)

typedef struct pixel_st
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} pixel_t;

static pixel_t *pixels = NULL;
static int width = 0;
static int height = 0;

void set_image_buffer(pixel_t *pixels);
void *memory_grow_pages(size_t bytes);

void globals_init(pixel_t *p, int w, int h)
{
    pixels = p;
    width = w;
    height = h;

    size_t size = w * h * sizeof pixels[0];
    size_t page_count = (size + PAGE_SIZE) / PAGE_SIZE;
    pixels = memory_grow_pages(page_count);
    set_image_buffer(pixels);
}

void *memset(void *s, int c, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ((char *)s)[i] = c;
    }
    return s;
}

void on_mousedown(int x, int y)
{
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pixel_t *p = &pixels[i * width + j];
            int ry = i - y;
            int rx = j - x;

            uint8_t color = 0;
            if (rx * rx + ry * ry < 120 * 120) {
                color = 255;
            }
            p->r = p->b = p->g = color;
            p->a = 255;
        }
    }
}
