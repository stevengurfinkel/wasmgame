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

void log_str(const char *msg)
{
    log_str_with_size(msg, strlen(msg));
}
