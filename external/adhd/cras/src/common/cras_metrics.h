/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_METRICS_H_
#define CRAS_METRICS_H_

/* Logs the specified event. */
void cras_metrics_log_event(const char *event);

/* Sends histogram data. */
void cras_metrics_log_histogram(const char *name, int sample, int min,
				int max, int nbuckets);

#endif /* CRAS_METRICS_H_ */
