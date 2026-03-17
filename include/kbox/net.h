/* SPDX-License-Identifier: MIT */
#ifndef KBOX_NET_H
#define KBOX_NET_H

/*
 * net.h - Networking support via minislirp.
 *
 * Provides user-mode networking for the LKL guest using SLIRP.
 * Creates a virtio-net device backed by SLIRP, configures the
 * guest interface, and bridges LKL I/O with SLIRP's event loop.
 *
 * Guest network config:
 *   IP:      10.0.2.15/24
 *   Gateway: 10.0.2.2
 *   DNS:     10.0.2.3
 *
 * Limitations:
 *   - Server-side sockets (bind/listen/accept) are not supported
 *     in the initial implementation.
 *   - Unprivileged ICMP: minislirp synthesizes replies for
 *     localhost; external ICMP may not work without CAP_NET_RAW.
 */

#include "kbox/syscall-nr.h"

/*
 * Initialize SLIRP networking.
 *
 * Creates the LKL virtio-net device, starts the SLIRP instance,
 * configures the guest interface (IP, route, DNS).
 *
 * Must be called after kernel boot and sysnrs detection,
 * before the supervisor fork.
 *
 * Returns 0 on success, -1 on error.
 */
int kbox_net_init(const struct kbox_sysnrs *sysnrs);

/*
 * Tear down SLIRP networking.
 *
 * Shuts down the SLIRP instance and frees resources.
 * Called during cleanup, after the supervisor loop exits.
 */
void kbox_net_cleanup(void);

#endif /* KBOX_NET_H */
