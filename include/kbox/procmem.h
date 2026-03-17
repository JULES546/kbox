/* SPDX-License-Identifier: MIT */
#ifndef KBOX_PROCMEM_H
#define KBOX_PROCMEM_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "kbox/lkl-wrap.h"

/*
 * Process memory access wrappers for seccomp-unotify.
 *
 * These use process_vm_readv/writev to read/write the tracee's
 * memory without ptrace.
 */

/* Read exactly `len` bytes from the tracee.  Returns 0 or -errno. */
int kbox_vm_read(pid_t pid, uint64_t remote_addr, void *out, size_t len);

/* Write exactly `len` bytes to the tracee.  Returns 0 or -errno. */
int kbox_vm_write(pid_t pid, uint64_t remote_addr, const void *in, size_t len);

/*
 * Write to the tracee via /proc/pid/mem.  Unlike process_vm_writev,
 * this can write through read-only page protections (the kernel uses
 * FOLL_FORCE internally).  Needed when execve pathnames point into
 * .rodata or other non-writable segments.
 *
 * Returns 0 or -errno.
 */
int kbox_vm_write_force(pid_t pid,
                        uint64_t remote_addr,
                        const void *in,
                        size_t len);

/*
 * Read a NUL-terminated string from the tracee.
 * `buf` must be at least `max_len` bytes.
 * Returns the string length (excluding NUL) on success, or -errno.
 */
int kbox_vm_read_string(pid_t pid,
                        uint64_t remote_addr,
                        char *buf,
                        size_t max_len);

/*
 * Read an open_how structure from the tracee.
 * Returns 0 on success or -errno.
 */
int kbox_vm_read_open_how(pid_t pid,
                          uint64_t remote_addr,
                          uint64_t size,
                          struct kbox_open_how *out);

#endif /* KBOX_PROCMEM_H */
