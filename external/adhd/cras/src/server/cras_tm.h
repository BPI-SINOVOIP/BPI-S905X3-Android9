/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_TM_H_
#define CRAS_TM_H_

/* cras_timer provides an interface to register a function to be called at a
 * later time.  This interface should be used from the main thread only, it is
 * not thread safe.
 */

struct cras_tm; /* timer manager */
struct cras_timer;

/* Creates a timer.  Must later be removed with cras_rm_cancel_timer.
 * Args:
 *    tm - Timer manager.
 *    ms - Call 'cb' in ms milliseconds.
 *    cb - The callback to call at timeout.
 *    cb_data - Passed to the callback when it is run.
 * Returns:
 *    Pointer to a newly allocated timer, passed timer to cras_tm_cancel_timer
 *    to cancel before it fires.
 */
struct cras_timer *cras_tm_create_timer(
		struct cras_tm *tm,
		unsigned int ms,
		void (*cb)(struct cras_timer *t, void *data),
		void *cb_data);

/* Deletes a timer returned from cras_tm_create_timer. */
void cras_tm_cancel_timer(struct cras_tm *tm, struct cras_timer *t);

/* Interface for system to create the timer manager. */
struct cras_tm *cras_tm_init();

/* Interface for system to destroy the timer manager. */
void cras_tm_deinit(struct cras_tm *tm);

/* Get the amount of time before the next timer expires. ts is set to an
 * the amount of time before the next timer expires (0 if already past due).
 * Args:
 *    tm - Timer manager.
 *    ts - Filled with time before next event.
 * Returns:
 *    0 if no timers are active, 1 if they are.
 */
int cras_tm_get_next_timeout(const struct cras_tm *tm, struct timespec *ts);

/* Calls any expired timers. */
void cras_tm_call_callbacks(struct cras_tm *tm);

#endif /* CRAS_TM_H_ */
