/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "cras_metrics.h"
#include "cras_main_message.h"

const char kNoCodecsFoundMetric[] = "Cras.NoCodecsFoundAtBoot";
const char kStreamTimeoutMilliSeconds[] = "Cras.StreamTimeoutMilliSeconds";

/* Type of metrics to log. */
enum CRAS_SERVER_METRICS_TYPE {
	LONGEST_FETCH_DELAY,
};

struct cras_server_metrics_message {
	struct cras_main_message header;
	enum CRAS_SERVER_METRICS_TYPE metrics_type;
	unsigned data;
};

static void init_longest_fetch_delay_msg(
		struct cras_server_metrics_message *msg,
		enum CRAS_SERVER_METRICS_TYPE type,
		unsigned data)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.type = CRAS_MAIN_METRICS;
	msg->header.length = sizeof(*msg);
	msg->metrics_type = type;
	msg->data = data;
}

int cras_server_metrics_longest_fetch_delay(unsigned delay_msec)
{
	struct cras_server_metrics_message msg;
	int err;

	init_longest_fetch_delay_msg(&msg, LONGEST_FETCH_DELAY, delay_msec);
	err = cras_main_message_send((struct cras_main_message *)&msg);
	if (err < 0) {
		syslog(LOG_ERR, "Failed to send metrics message");
		return err;
	}

	return 0;
}

static void metrics_longest_fetch_delay(unsigned delay_msec)
{
	static const int fetch_delay_min_msec = 1;
	static const int fetch_delay_max_msec = 10000;
	static const int fetch_delay_nbuckets = 10;

	cras_metrics_log_histogram(kStreamTimeoutMilliSeconds,
				   delay_msec,
				   fetch_delay_min_msec,
				   fetch_delay_max_msec,
				   fetch_delay_nbuckets);
}

static void handle_metrics_message(struct cras_main_message *msg, void *arg)
{
	struct cras_server_metrics_message *metrics_msg =
			(struct cras_server_metrics_message *)msg;
	switch (metrics_msg->metrics_type) {
	case LONGEST_FETCH_DELAY:
		metrics_longest_fetch_delay(metrics_msg->data);
		break;
	default:
		syslog(LOG_ERR, "Unknown metrics type %u",
		       metrics_msg->metrics_type);
		break;
	}

}

int cras_server_metrics_init() {
	cras_main_message_add_handler(CRAS_MAIN_METRICS,
				      handle_metrics_message, NULL);
	return 0;
}
