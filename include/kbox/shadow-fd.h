/* SPDX-License-Identifier: MIT */
#ifndef KBOX_SHADOW_FD_H
#define KBOX_SHADOW_FD_H

#include "kbox/syscall-nr.h"

/*
 * Maximum file size for shadow FD creation (256 MB).
 * Files larger than this get virtual FDs and cannot be mmapped.
 * Dynamic linker .so files are typically < 10 MB.
 */
#define KBOX_SHADOW_MAX_SIZE (256L * 1024 * 1024)

/*
 * Create a host-visible memfd populated with the contents of an LKL file.
 *
 * Reads the entire file via LKL pread64 in chunks and writes it into a
 * memfd_create'd descriptor.  The memfd is seeked back to position 0
 * before return so it is ready for mmap.
 *
 * Returns the host memfd on success, or -errno on failure.
 * Caller is responsible for closing the returned fd when done.
 */
int kbox_shadow_create(const struct kbox_sysnrs *s, long lkl_fd);

#endif /* KBOX_SHADOW_FD_H */
