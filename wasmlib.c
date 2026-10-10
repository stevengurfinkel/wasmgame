#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wasmlib.h"

size_t strlen(const char *s)
{
    size_t i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}

void *memset(void *s, int c, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ((char *)s)[i] = c;
    }
    return s;
}

float sqrtf(float f)
{
    float out;
    asm (
        "local.get %1\n"
        "f32.sqrt\n"
        "local.set %0\n"
        : "=r"(out)
        : "r"(f)
    );
    return out;
}

float fabsf(float x)
{
    float out;
    asm (
        "local.get %1\n"
        "f32.abs\n"
        "local.set %0\n"
        : "=r"(out)
        : "r"(x)
    );

    return out;
}

int round_down(float f)
{
    int out;
    asm (
        "local.get %1\n"
        "f32.floor\n"
        "i32.trunc_f32_s\n"
        "local.set %0\n"
        : "=r"(out)
        : "r"(f)
    );

    return out;
}

int round_up(float f)
{
    int out;
    asm (
        "local.get %1\n"
        "f32.ceil\n"
        "i32.trunc_f32_s\n"
        "local.set %0\n"
        : "=r"(out)
        : "r"(f)
    );
    return out;
}

static size_t memory_grow_pages(size_t pages_to_add)
{
    size_t old_page_count;
    asm volatile (
        "local.get %1\n"
        "memory.grow 0\n"
        "local.set %0\n"
        : "=r"(old_page_count)
        : "r"(pages_to_add)
    );
    return old_page_count;
}

void *memory_alloc(size_t bytes)
{
    bytes = (bytes + PAGE_MASK) & ~PAGE_MASK; // round up to PAGE_SIZE
    int old_page_count = memory_grow_pages(bytes >> PAGE_BITS);
    if (old_page_count < 0) {
        return NULL;
    }

    return (char *)(old_page_count * PAGE_SIZE);
}

void log_str(const char *msg)
{
    log_str_with_size(msg, strlen(msg));
}
