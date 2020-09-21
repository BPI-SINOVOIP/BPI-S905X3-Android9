/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "cras_main_message.h"
#include "cras_system_state.h"
#include "cras_util.h"


/* Callback to handle specific type of main thread message. */
struct cras_main_msg_callback {
	enum CRAS_MAIN_MESSAGE_TYPE type;
	cras_message_callback callback;
	void *callback_data;
	struct cras_main_msg_callback *prev, *next;
};

static int main_msg_fds[2];
static struct cras_main_msg_callback *main_msg_callbacks;

int cras_main_message_add_handler(enum CRAS_MAIN_MESSAGE_TYPE type,
				  cras_message_callback callback,
				  void *callback_data)
{
	struct cras_main_msg_callback *msg_cb;

	DL_FOREACH(main_msg_callbacks, msg_cb) {
		if (msg_cb->type == type) {
			syslog(LOG_ERR, "Main message type %u already exists",
			       type);
			return -EEXIST;
		}
	}

	msg_cb = (struct cras_main_msg_callback *)calloc(1, sizeof(*msg_cb));
	msg_cb->type = type;
	msg_cb->callback = callback;
	msg_cb->callback_data = callback_data;

	DL_APPEND(main_msg_callbacks, msg_cb);
	return 0;
}

int cras_main_message_send(struct cras_main_message *msg)
{
	int err;

	err = write(main_msg_fds[1], msg, msg->length);
	if (err < 0) {
		syslog(LOG_ERR, "Failed to send main message, type %u",
		       msg->type);
		return err;
	}
	return 0;
}

static int read_main_message(int msg_fd, uint8_t *buf, size_t max_len) {
	int to_read, nread, rc;
	struct cras_main_message *msg = (struct cras_main_message *)buf;

	nread = read(msg_fd, buf, sizeof(msg->length));
	if (nread < 0)
		return nread;
	if (msg->length > max_len)
		return -ENOMEM;

	to_read = msg->length - nread;
	rc = read(msg_fd, &buf[0] + nread, to_read);
	if (rc < 0)
		return rc;
	return 0;
}

static void handle_main_messages(void *arg)
{
	uint8_t buf[256];
	int rc;
	struct cras_main_msg_callback *main_msg_cb;
	struct cras_main_message *msg = (struct cras_main_message *)buf;

	rc = read_main_message(main_msg_fds[0], buf, sizeof(buf));
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to read main message");
		return;
	}

	DL_FOREACH(main_msg_callbacks, main_msg_cb) {
		if (main_msg_cb->type == msg->type) {
			main_msg_cb->callback(msg, main_msg_cb->callback_data);
			break;
		}
	}
}

void cras_main_message_init() {
	int rc;

	rc = pipe(main_msg_fds);
	if (rc < 0) {
		syslog(LOG_ERR, "Fatal: main message init");
		exit(-ENOMEM);
	}

	/* When full it's preferred to get error instead of blocked. */
	cras_make_fd_nonblocking(main_msg_fds[0]);
	cras_make_fd_nonblocking(main_msg_fds[1]);

	cras_system_add_select_fd(main_msg_fds[0],
				  handle_main_messages,
				  NULL);
}
