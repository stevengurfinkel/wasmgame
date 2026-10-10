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

void log_str(const char *msg)
{
    log_str_with_size(msg, strlen(msg));
}
