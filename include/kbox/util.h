/* SPDX-License-Identifier: MIT */
#ifndef KBOX_UTIL_H
#define KBOX_UTIL_H

#include <stddef.h>
#include <unistd.h>

#define KBOX_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* Close a file descriptor if non-negative. */
static inline void close_fd(int fd)
{
    if (fd >= 0)
        close(fd);
}

/* Safe snprintf: returns 0 on success, -1 on truncation. */
int kbox_snprintf(char *buf, size_t size, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Duplicate a string. Returns NULL on failure. */
char *kbox_strdup(const char *s);

/* Join strings with a separator. Caller frees result. */
char *kbox_strjoin(const char *const *parts, int count, char sep);

#endif /* KBOX_UTIL_H */
