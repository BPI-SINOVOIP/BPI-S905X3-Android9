/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

#define METRICS_CLIENT "metrics_client"

void cras_metrics_log_event(const char *event)
{
	syslog(LOG_DEBUG, "Log event: %s", event);
	if (!fork()) {
		const char *argv[] = {METRICS_CLIENT, "-v", event, NULL} ;
		execvp(argv[0], (char * const *)argv);
		_exit(1);
	}
}

void cras_metrics_log_histogram(const char *name, int sample, int min,
				int max, int nbuckets)
{
	if (!fork()) {
		char tmp[4][16];
		snprintf(tmp[0], 16, "%d", sample);
		snprintf(tmp[1], 16, "%d", min);
		snprintf(tmp[2], 16, "%d", max);
		snprintf(tmp[3], 16, "%d", nbuckets);
		const char *argv[] = {METRICS_CLIENT, name, tmp[0], tmp[1],
				      tmp[2], tmp[3], NULL};
		execvp(argv[0], (char * const *)argv);
		_exit(1);
	}
}
