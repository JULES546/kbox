/* SPDX-License-Identifier: MIT */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kbox/util.h"

int kbox_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);

    if (n < 0 || (size_t) n >= size)
        return -1;
    return 0;
}

char *kbox_strdup(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup)
        memcpy(dup, s, len);
    return dup;
}

char *kbox_strjoin(const char *const *parts, int count, char sep)
{
    if (!parts || count <= 0)
        return NULL;

    /* Calculate total length: sum of all strings + separators + NUL */
    size_t total = 0;
    for (int i = 0; i < count; i++) {
        if (!parts[i])
            continue;
        total += strlen(parts[i]);
    }
    /* Add room for separators between elements and trailing NUL */
    total += (size_t) (count - 1) + 1;

    char *buf = malloc(total);
    if (!buf)
        return NULL;

    char *p = buf;
    for (int i = 0; i < count; i++) {
        if (i > 0)
            *p++ = sep;
        if (parts[i]) {
            size_t len = strlen(parts[i]);
            memcpy(p, parts[i], len);
            p += len;
        }
    }
    *p = '\0';
    return buf;
}
