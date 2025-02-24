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

float fabsf(float x)
{
    return x >= 0 ? x : -x;
}

int round_down(float f)
{
    return (int)f;
}

int round_up(float f)
{
    int i = round_down(f);
    return i + (i != f);
}

void log_str(const char *msg)
{
    log_str_with_size(msg, strlen(msg));
}
