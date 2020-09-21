/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cras_types.h"
#include "cras_util.h"
#include "utlist.h"

#include <time.h>

/* Represents an armed timer.
 * Members:
 *    ts - timespec at which the timer should fire.
 *    cb - Callback to call when the timer expires.
 *    cb_data - Data passed to the callback.
 */
struct cras_timer {
	struct timespec ts;
	void (*cb)(struct cras_timer *t, void *data);
	void *cb_data;
	struct cras_timer *next, *prev;
};

/* Timer Manager, keeps a list of active timers. */
struct cras_tm {
	struct cras_timer *timers;
};

/* Local Functions. */

/* Adds ms milliseconds to ts. */
static inline void add_ms_ts(struct timespec *ts, unsigned int ms)
{
	if (ms >= 1000) {
		ts->tv_sec += ms / 1000;
		ms %= 1000;
	}
	ts->tv_nsec += ms * 1000000L;
	if (ts->tv_nsec >= 1000000000L) {
		ts->tv_sec += ts->tv_nsec / 1000000000L;
		ts->tv_nsec %= 1000000000L;
	}
}

/* Checks if timespec a is less than b. */
static inline int timespec_sooner(const struct timespec *a,
				  const struct timespec *b)
{
	return (a->tv_sec < b->tv_sec ||
		(a->tv_sec == b->tv_sec && a->tv_nsec <= b->tv_nsec));
}

/* Exported Interface. */

struct cras_timer *cras_tm_create_timer(
		struct cras_tm *tm,
		unsigned int ms,
		void (*cb)(struct cras_timer *t, void *data),
		void *cb_data)
{
	struct cras_timer *t;

	t = calloc(1, sizeof(*t));
	if (!t)
		return NULL;

	t->cb = cb;
	t->cb_data = cb_data;

	clock_gettime(CLOCK_MONOTONIC_RAW, &t->ts);
	add_ms_ts(&t->ts, ms);

	DL_APPEND(tm->timers, t);

	return t;
}

void cras_tm_cancel_timer(struct cras_tm *tm, struct cras_timer *t)
{
	DL_DELETE(tm->timers, t);
	free(t);
}

struct cras_tm *cras_tm_init()
{
	return calloc(1, sizeof(struct cras_tm));
}

void cras_tm_deinit(struct cras_tm *tm)
{
	struct cras_timer *t;

	DL_FOREACH(tm->timers, t) {
		DL_DELETE(tm->timers, t);
		free(t);
	}
	free(tm);
}

int cras_tm_get_next_timeout(const struct cras_tm *tm, struct timespec *ts)
{
	struct cras_timer *t;
	struct timespec now;
	struct timespec *min;

	if (!tm->timers)
		return 0;

	min = &tm->timers->ts;
	DL_FOREACH(tm->timers, t)
		if (timespec_sooner(&t->ts, min))
			min = &t->ts;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	if (timespec_sooner(min, &now)) {
		/* Timer already expired. */
		ts->tv_sec = ts->tv_nsec = 0;
		return 1;
	}

	subtract_timespecs(min, &now, ts);
	return 1;
}

void cras_tm_call_callbacks(struct cras_tm *tm)
{
	struct timespec now;
	struct cras_timer *t, *next;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	/* Don't use DL_FOREACH to iterate timers because in each loop the
	 * next timer pointer is stored for later access but it could be
	 * cancelled and freed in current timer's callback causing invalid
	 * memory access. */
	t = tm->timers;
	while (t) {
		next = t->next;
		if (timespec_sooner(&t->ts, &now)) {
			t->cb(t, t->cb_data);
			/* Update next timer because it could have been modified
			 * in t->cb(). */
			next = t->next;
			cras_tm_cancel_timer(tm, t);
		}
		t = next;
	}
}
