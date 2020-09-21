/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SERVER_METRICS_H_
#define CRAS_SERVER_METRICS_H_

extern const char kNoCodecsFoundMetric[];
extern const char kStreamTimeoutMilliSeconds[];

/* Logs the longest fetch delay of a stream in millisecond. */
int cras_server_metrics_longest_fetch_delay(int delay_msec);

/* Initialize metrics logging stuff. */
int cras_server_metrics_init();

#endif /* CRAS_SERVER_METRICS_H_ */

