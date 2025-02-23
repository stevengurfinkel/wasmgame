#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"

#define PAGE_SIZE (64 * 1024)


static pixel_t *pixels = NULL;
static int width = 0;
static int height = 0;

void globals_init(int w, int h)
{
    width = w;
    height = h;

    size_t size = w * h * sizeof pixels[0];
    size_t page_count = (size + PAGE_SIZE) / PAGE_SIZE;
    pixels = memory_grow_pages(page_count);
    set_image_buffer(pixels);
    log_str("Allocating memory for with and height...");
    log_int(width);
    log_int(height);
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
