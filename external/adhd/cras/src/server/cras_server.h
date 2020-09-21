/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * The CRAS server instance.
 */
#ifndef CRAS_SERVER_H_
#define CRAS_SERVER_H_

/*
 * Bitmask for cras_server_run() argument profile_disable_mask
 */
#define CRAS_SERVER_PROFILE_MASK_HFP	(1 << 0)
#define CRAS_SERVER_PROFILE_MASK_HSP	(1 << 1)
#define CRAS_SERVER_PROFILE_MASK_A2DP	(1 << 2)

struct cras_client_message;

/* Initialize some server setup. Mainly to add the select handler first
 * so that client callbacks can be registered before server start running.
 */
int cras_server_init();

/* Runs the CRAS server.  Open the main socket and begin listening for
 * connections and for messages from clients that have connected.
 */
int cras_server_run(unsigned int profile_disable_mask);

/* Send a message to all attached clients. */
void cras_server_send_to_all_clients(const struct cras_client_message *msg);

#endif /* CRAS_SERVER_H_ */
