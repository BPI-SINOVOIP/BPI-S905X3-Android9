/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_MAIN_MESSAGE_H_
#define CRAS_MAIN_MESSAGE_H_

#include <stdio.h>

#include "utlist.h"

/* The types of message main thread can handle. */
enum CRAS_MAIN_MESSAGE_TYPE {
	/* Audio thread -> main thread */
	CRAS_MAIN_A2DP,
	CRAS_MAIN_BT,
	CRAS_MAIN_METRICS,
	CRAS_MAIN_MONITOR_DEVICE,
};

/* Structure of the header of the message handled by main thread.
 * Args:
 *    length - Size of the whole message.
 *    type - Type of the message.
 */
struct cras_main_message {
	size_t length;
	enum CRAS_MAIN_MESSAGE_TYPE type;
};

/* Callback function to handle main thread message. */
typedef void (*cras_message_callback)(struct cras_main_message *msg,
				      void *arg);

/* Sends a message to main thread. */
int cras_main_message_send(struct cras_main_message *msg);

/* Registers the handler function for specific type of message. */
int cras_main_message_add_handler(enum CRAS_MAIN_MESSAGE_TYPE type,
				  cras_message_callback callback,
				  void *callback_data);

/* Initialize the message handling mechanism in main thread. */
void cras_main_message_init();

#endif /* CRAS_MAIN_MESSAGE_H_ */
