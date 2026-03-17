# kbox

kbox boots a real Linux kernel as an in-process library ([LKL](https://github.com/lkl/linux)) and routes intercepted syscalls to it via [seccomp-unotify](https://man7.org/linux/man-pages/man2/seccomp_unotify.2.html). It provides a rootless chroot/proot alternative with kernel-level syscall accuracy.

## How it works

1. The supervisor opens a rootfs disk image and registers it as an LKL block device.
2. LKL boots a real Linux kernel inside the process (no VM, no separate process tree).
3. The filesystem is mounted via LKL, and the supervisor sets the guest's virtual root via LKL's internal `chroot` (no host privileges needed).
4. A child process is forked with a seccomp BPF filter that delivers all syscalls (except a minimal allow-list: `sendmsg`, `exit`, `exit_group`) as notifications.
5. The supervisor intercepts each notification, translates paths and file descriptors, and forwards the syscall to either LKL (filesystem, identity, metadata) or the host kernel (scheduling, signals, memory, I/O multiplexing).
6. Results are injected back into the child via `seccomp_notif_resp`.

This gives programs running inside the image a complete Linux kernel environment -- real VFS, real ext4, real procfs -- without root privileges, containers, or ptrace.

## Building

Linux only (host kernel 5.0+ for seccomp-unotify). Requires GCC, GNU Make, and a pre-built `liblkl.a`. No `libseccomp` dependency -- the BPF filter is compiled natively.

```bash
make                        # debug build (ASAN + UBSAN enabled)
make BUILD=release          # release build
```

LKL is fetched automatically from GitHub Actions artifacts on first build (requires `gh` CLI). To use a custom LKL:

```bash
make LKL_DIR=/path/to/lkl
```

## Quick start

Build a test rootfs image (requires `e2fsprogs` for `mkfs.ext4`/`debugfs`, no root needed):

```bash
make rootfs                 # creates rootfs.ext4 with busybox
```

## Usage

```bash
# Boot an Alpine Linux ext4 image, run /bin/sh
./kbox image -r alpine.ext4

# Same, with recommended mounts (proc, sysfs, devtmpfs, devpts, tmpfs)
./kbox image -R alpine.ext4

# System root: recommended mounts + uid=0/gid=0 inside the guest (no host privileges)
./kbox image -S alpine.ext4

# Run a specific command (args after -- become the command)
./kbox image -S alpine.ext4 -- /bin/ls -la /

# Minimal mount profile (proc + tmpfs only)
./kbox image -S alpine.ext4 --mount-profile minimal

# Custom kernel cmdline, bind mount, explicit identity
./kbox image -r alpine.ext4 -k "mem=2048M loglevel=7" \
    -b /home/user/data:/mnt/data --change-id 1000:1000
```

Run `./kbox image --help` for the full option list.

## Testing

```bash
make check                  # all tests (unit + integration + stress)
make check-unit             # 74 unit tests under ASAN/UBSAN
make check-integration      # ~45 integration tests against a rootfs image
make check-stress           # 5 stress test programs
```

Unit tests have no LKL dependency and run on any Linux host. Integration and stress tests require a rootfs image (built via `make rootfs`).

## Architecture

```
                 ┌────────────────┐
                 │  guest child   │  (seccomp BPF installed)
                 └──────┬─────────┘
                        │ syscall notification
                 ┌──────▼──────────┐
                 │  supervisor     │  seccomp_dispatch.c
                 │  (dispatch)     │  114 syscall entries
                 └────┬───────┬────┘
          LKL path    │       │  host path (CONTINUE)
          ┌───────────▼──┐ ┌──▼──────────┐
          │  LKL kernel  │ │ host kernel │
          │  (in-proc)   │ │             │
          └──────────────┘ └─────────────┘
```

Syscalls are routed to one of three dispositions:

- LKL forward: filesystem ops, metadata, identity -- the supervisor reads arguments from tracee memory, calls into LKL, writes results back.
- Host CONTINUE: scheduling, signals, memory management, I/O multiplexing -- the host kernel handles these natively.
- Emulated: process info (getpid returns 1), uname (synthetic), getrandom (LKL /dev/urandom).

Key subsystems:
- Virtual FD table (`fd_table.c`): maps guest FDs to LKL-internal FDs. Two ranges -- low FDs (0..1023) for dup2 redirects, high FDs (32768+) for normal allocation.
- Shadow FDs (`shadow_fd.c`): O_RDONLY files get a host-visible memfd copy, enabling native mmap for dynamic linking.
- Path translation (`path.c`): routes /proc, /sys, /dev to the host kernel; everything else to LKL.
- ELF extraction (`elf.c`, `image.c`): binaries are copied from LKL into memfds for fexecve. Dynamic binaries get PT_INTERP patched to `/proc/self/fd/N`.

## GDB integration

kbox doubles as an educational sandbox. Because LKL runs in-process, you can set GDB breakpoints on kernel functions, inspect live procfs data, and trace syscalls end-to-end.

```bash
# Load custom helpers
source scripts/gdb/kbox-gdb.py

# Break on a specific syscall entering dispatch
kbox-break-syscall openat

# Print the virtual FD table
kbox-fdtable

# Trace a guest path through the translation logic
kbox-vfs-path /usr/bin/ls

# Coordinated breakpoints across seccomp dispatch and LKL entry
kbox-syscall-trace
```

See `docs/gdb-workflow.md` for the full workflow.

## Project layout

```
src/                    18 C source files (~8300 lines)
include/kbox/           14 headers (~1000 lines)
tests/unit/             6 unit test suites (74 tests)
tests/guest/            5 guest test programs (run inside kbox)
tests/stress/           5 stress test programs
scripts/gdb/            GDB helpers + 48 VFS path tests
scripts/                build, fetch, integration test runner
docs/                   syscall parity spec, GDB workflow
```

## Targets

- x86_64 (primary, fully tested)
- aarch64 (syscall tables populated, end-to-end verification pending)

## License

MIT
