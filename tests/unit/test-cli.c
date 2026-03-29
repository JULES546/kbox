/* SPDX-License-Identifier: MIT */

#include "kbox/cli.h"
#include "test-runner.h"

/* On Linux, rewrite.c provides kbox_parse_syscall_mode.
 * On macOS (unit-test-only builds), provide a minimal stub. */
#ifndef __linux__
int kbox_parse_syscall_mode(const char *value, enum kbox_syscall_mode *out)
{
    if (!value || !out)
        return -1;
    if (strcmp(value, "auto") == 0)
        *out = KBOX_SYSCALL_MODE_AUTO;
    else if (strcmp(value, "seccomp") == 0)
        *out = KBOX_SYSCALL_MODE_SECCOMP;
    else if (strcmp(value, "trap") == 0)
        *out = KBOX_SYSCALL_MODE_TRAP;
    else if (strcmp(value, "rewrite") == 0)
        *out = KBOX_SYSCALL_MODE_REWRITE;
    else
        return -1;
    return 0;
}
#endif

static void test_parse_args_partition_valid(void)
{
    char *argv[] = {"kbox", "image", "-r", "rootfs.ext4", "-p", "7"};
    struct kbox_args args;

    ASSERT_EQ(kbox_parse_args(6, argv, &args), 0);
    ASSERT_EQ(args.mode, KBOX_MODE_IMAGE);
    ASSERT_EQ(args.image.part, 7);
}

static void test_parse_args_partition_overflow_rejected(void)
{
    char *argv[] = {"kbox", "image", "-r", "rootfs.ext4", "-p", "4294967296"};
    struct kbox_args args;

    ASSERT_EQ(kbox_parse_args(6, argv, &args), -1);
}

void test_cli_init(void)
{
    TEST_REGISTER(test_parse_args_partition_valid);
    TEST_REGISTER(test_parse_args_partition_overflow_rejected);
}
