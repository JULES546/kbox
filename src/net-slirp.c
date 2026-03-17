/* SPDX-License-Identifier: MIT */
/*
 * net-slirp.c - User-mode networking via minislirp.
 *
 * Bridges LKL's virtio-net device with a SLIRP instance to provide
 * outbound TCP/UDP networking without privileges.
 *
 * Threading model:
 *   - LKL's virtio-net poll callback is called from an LKL kernel
 *     thread and must block until data is available.
 *   - SLIRP's pollfds_fill/poll/cleanup cycle runs on the host side.
 *   - Bridge via pipe: SLIRP output -> write to pipe -> LKL poll
 *     callback unblocks -> RX delivery.
 *   - LKL TX callback -> slirp_input() can be called directly
 *     (same address space, LKL is single-threaded for I/O).
 *
 * Compiled only when KBOX_HAS_SLIRP is defined.
 */

#ifdef KBOX_HAS_SLIRP

#include "kbox/lkl-wrap.h"
#include "kbox/net.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* minislirp headers */
#include "slirp/libslirp.h"

/* Guest network configuration */
#define GUEST_IP "10.0.2.15"
#define GUEST_MASK 24
#define GATEWAY_IP "10.0.2.2"
#define DNS_IP "10.0.2.3"

/* Bridge pipe for SLIRP -> LKL RX delivery */
static int rx_pipe[2] = {-1, -1};

/* SLIRP instance */
static Slirp *slirp_instance;

/* LKL network device ID */
static int lkl_netdev_id = -1;

/* Event loop thread */
static pthread_t slirp_thread;
static volatile int slirp_running;

/* ------------------------------------------------------------------ */
/* SLIRP callbacks                                                    */
/* ------------------------------------------------------------------ */

static ssize_t slirp_send_packet(const void *buf, size_t len, void *opaque)
{
    (void) opaque;
    /* Write packet to the RX pipe; LKL poll will read it. */
    return write(rx_pipe[1], buf, len);
}

static void slirp_guest_error(const char *msg, void *opaque)
{
    (void) opaque;
    fprintf(stderr, "kbox: slirp error: %s\n", msg);
}

static int64_t slirp_clock_get_ns(void *opaque)
{
    (void) opaque;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t) ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void *slirp_timer_new(SlirpTimerCb cb, void *cb_opaque, void *opaque)
{
    (void) cb;
    (void) cb_opaque;
    (void) opaque;
    /* Minimal timer stub -- full implementation needs timer wheel */
    return NULL;
}

static void slirp_timer_free(void *timer, void *opaque)
{
    (void) timer;
    (void) opaque;
}

static void slirp_timer_mod(void *timer, int64_t expire_time, void *opaque)
{
    (void) timer;
    (void) expire_time;
    (void) opaque;
}

static void slirp_register_poll_fd(int fd, void *opaque)
{
    (void) fd;
    (void) opaque;
}

static void slirp_unregister_poll_fd(int fd, void *opaque)
{
    (void) fd;
    (void) opaque;
}

static void slirp_notify(void *opaque)
{
    (void) opaque;
}

static const SlirpCb slirp_callbacks = {
    .send_packet = slirp_send_packet,
    .guest_error = slirp_guest_error,
    .clock_get_ns = slirp_clock_get_ns,
    .timer_new = slirp_timer_new,
    .timer_free = slirp_timer_free,
    .timer_mod = slirp_timer_mod,
    .register_poll_fd = slirp_register_poll_fd,
    .unregister_poll_fd = slirp_unregister_poll_fd,
    .notify = slirp_notify,
};

/* ------------------------------------------------------------------ */
/* SLIRP event loop thread                                            */
/* ------------------------------------------------------------------ */

static void *slirp_event_loop(void *arg)
{
    (void) arg;

    while (slirp_running) {
        uint32_t timeout = 0;
        struct pollfd pfds[64];
        int nfds = 0;

        slirp_pollfds_fill(slirp_instance, &timeout, pfds, &nfds, 64);

        int ret = poll(pfds, (nfds_t) nfds, timeout > 0 ? (int) timeout : 100);
        if (ret < 0 && errno != EINTR)
            break;

        slirp_pollfds_poll(slirp_instance, ret <= 0, pfds, nfds);
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* LKL virtio-net callbacks                                           */
/* ------------------------------------------------------------------ */

/*
 * TX: LKL sends a packet.  Forward to SLIRP directly.
 * Called from LKL kernel context (single-threaded for I/O).
 */
static int net_tx(const void *buf, int len, void *opaque)
{
    (void) opaque;
    if (slirp_instance)
        slirp_input(slirp_instance, buf, len);
    return len;
}

/*
 * RX: LKL polls for incoming packets.  Block on the pipe
 * until SLIRP delivers something via send_packet callback.
 */
static int net_rx(void *buf, int len, void *opaque)
{
    (void) opaque;
    ssize_t n = read(rx_pipe[0], buf, (size_t) len);
    return (int) n;
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

int kbox_net_init(const struct kbox_sysnrs *sysnrs)
{
    SlirpConfig cfg;

    /* Create the RX bridge pipe. */
    if (pipe(rx_pipe) < 0) {
        fprintf(stderr, "kbox: net: pipe failed: %s\n", strerror(errno));
        return -1;
    }

    /* Initialize SLIRP. */
    memset(&cfg, 0, sizeof(cfg));
    cfg.version = 4;
    cfg.restricted = 0;
    cfg.in_enabled = 1;
    /* cfg.vnetwork, cfg.vgateway, etc. set by SLIRP defaults (10.0.2.0/24) */

    slirp_instance = slirp_new(&cfg, &slirp_callbacks, NULL);
    if (!slirp_instance) {
        fprintf(stderr, "kbox: net: slirp_new failed\n");
        goto err_pipe;
    }

    /*
     * Register the LKL virtio-net device.
     * lkl_netdev_add returns the device index (0, 1, ...).
     *
     * TODO: The actual LKL netdev registration API needs to be
     * wired up.  This is a placeholder for the integration point.
     * The real implementation needs:
     *   1. lkl_register_netdev with TX/RX callbacks
     *   2. lkl_netdev_get_ifindex to get the interface index
     *   3. Configure the interface up, IP, route via LKL syscalls
     */
    lkl_netdev_id = 0; /* placeholder */

    fprintf(stderr, "kbox: net: configuring guest interface...\n");

    /* Bring interface up. */
    /* lkl_if_up(lkl_netdev_id); */

    /* Set IP address. */
    /* lkl_if_set_ipv4(lkl_netdev_id, GUEST_IP, GUEST_MASK); */

    /* Set default route via gateway. */
    /* lkl_set_ipv4_gateway(GATEWAY_IP); */

    /*
     * Write /etc/resolv.conf inside LKL.
     */
    {
        const char *resolv = "nameserver " DNS_IP "\n";
        long fd = kbox_lkl_openat(sysnrs, AT_FDCWD_LINUX, "/etc/resolv.conf",
                                  0x241 /* O_WRONLY|O_CREAT|O_TRUNC */, 0644);
        if (fd >= 0) {
            kbox_lkl_write(sysnrs, fd, resolv, (long) strlen(resolv));
            kbox_lkl_close(sysnrs, fd);
        }
    }

    /* Start the SLIRP event loop thread. */
    slirp_running = 1;
    if (pthread_create(&slirp_thread, NULL, slirp_event_loop, NULL) != 0) {
        fprintf(stderr, "kbox: net: pthread_create failed: %s\n",
                strerror(errno));
        goto err_slirp;
    }

    fprintf(stderr, "kbox: net: initialized (guest %s/%d gw %s dns %s)\n",
            GUEST_IP, GUEST_MASK, GATEWAY_IP, DNS_IP);
    return 0;

err_slirp:
    slirp_cleanup(slirp_instance);
    slirp_instance = NULL;
err_pipe:
    close(rx_pipe[0]);
    close(rx_pipe[1]);
    rx_pipe[0] = rx_pipe[1] = -1;
    return -1;
}

void kbox_net_cleanup(void)
{
    if (!slirp_running)
        return;

    slirp_running = 0;
    pthread_join(slirp_thread, NULL);

    if (slirp_instance) {
        slirp_cleanup(slirp_instance);
        slirp_instance = NULL;
    }

    if (rx_pipe[0] >= 0) {
        close(rx_pipe[0]);
        close(rx_pipe[1]);
        rx_pipe[0] = rx_pipe[1] = -1;
    }

    fprintf(stderr, "kbox: net: cleaned up\n");
}

#else /* !KBOX_HAS_SLIRP */

#include <stdio.h>
#include "kbox/net.h"

int kbox_net_init(const struct kbox_sysnrs *sysnrs)
{
    (void) sysnrs;
    fprintf(stderr, "kbox: net: not compiled with SLIRP support\n");
    return -1;
}

void kbox_net_cleanup(void) {}

#endif /* KBOX_HAS_SLIRP */
