/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_UTIL_H_
#define CRAS_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <poll.h>
#include <time.h>

#include "cras_types.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define assert_on_compile(e) ((void)sizeof(char[1 - 2 * !(e)]))
#define assert_on_compile_is_power_of_2(n) \
	assert_on_compile((n) != 0 && (((n) & ((n) - 1)) == 0))

/* Enables real time scheduling. */
int cras_set_rt_scheduling(int rt_lim);
/* Sets the priority. */
int cras_set_thread_priority(int priority);
/* Sets the niceness level of the current thread. */
int cras_set_nice_level(int nice);

/* Converts a buffer level from one sample rate to another. */
static inline size_t cras_frames_at_rate(size_t orig_rate, size_t orig_frames,
					 size_t act_rate)
{
	return (orig_frames * act_rate + orig_rate - 1) / orig_rate;
}

/* Converts a number of frames to a time in a timespec. */
static inline void cras_frames_to_time(unsigned int frames,
				       unsigned int rate,
				       struct timespec *t)
{
	t->tv_sec = frames / rate;
	frames = frames % rate;
	t->tv_nsec = (uint64_t)frames * 1000000000 / rate;
}

/* Converts a number of frames to a time in a timespec. */
static inline void cras_frames_to_time_precise(unsigned int frames,
					       double rate,
					       struct timespec *t)
{
	double seconds = frames / rate;
	t->tv_sec = (unsigned int)seconds;
	seconds -= t->tv_sec;
	t->tv_nsec = (unsigned int)(seconds * 1000000000);
}

/* Converts a timespec duration to a frame count. */
static inline unsigned int cras_time_to_frames(const struct timespec *t,
					       unsigned int rate)
{
	return t->tv_nsec * (uint64_t)rate / 1000000000 + rate * t->tv_sec;
}

/* Converts a number of frames to a duration in ms. */
static inline unsigned int cras_frames_to_ms(unsigned int frames,
					     unsigned int rate)
{
	return 1000 * frames / rate;
}

/* Makes a file descriptor non blocking. */
int cras_make_fd_nonblocking(int fd);

/* Makes a file descriptor blocking. */
int cras_make_fd_blocking(int fd);

/* Send data in buf to the socket attach the fds. */
int cras_send_with_fds(int sockfd, const void *buf, size_t len, int *fd,
		       unsigned int num_fds);

/* Receive data in buf from the socket. If file descriptors are received, put
 * them in *fd, otherwise set *fd to -1. */
int cras_recv_with_fds(int sockfd, void *buf, size_t len, int *fd,
		       unsigned int *num_fds);

/* This must be written a million times... */
static inline void subtract_timespecs(const struct timespec *end,
				      const struct timespec *beg,
				      struct timespec *diff)
{
	diff->tv_sec = end->tv_sec - beg->tv_sec;
	diff->tv_nsec = end->tv_nsec - beg->tv_nsec;

	/* Adjust tv_sec and tv_nsec to the same sign. */
	if (diff->tv_sec > 0 && diff->tv_nsec < 0) {
		diff->tv_sec--;
		diff->tv_nsec += 1000000000L;
	} else if (diff->tv_sec < 0 && diff->tv_nsec > 0) {
		diff->tv_sec++;
		diff->tv_nsec -= 1000000000L;
	}
}

static inline void add_timespecs(struct timespec *a,
				 const struct timespec *b)
{
	a->tv_sec += b->tv_sec;
	a->tv_nsec += b->tv_nsec;

	while (a->tv_nsec >= 1000000000L) {
		a->tv_sec++;
		a->tv_nsec -= 1000000000L;
	}
}

/* Converts a fixed-size cras_timespec to a native timespec */
static inline void cras_timespec_to_timespec(struct timespec *dest,
					     const struct cras_timespec *src)
{
	dest->tv_sec = src->tv_sec;
	dest->tv_nsec = src->tv_nsec;
}

/* Fills a fixed-size cras_timespec with the current system time */
static inline int cras_clock_gettime(clockid_t clk_id,
				     struct cras_timespec *ctp)
{
	struct timespec tp;
	int ret = clock_gettime(clk_id, &tp);
	ctp->tv_sec = tp.tv_sec;
	ctp->tv_nsec = tp.tv_nsec;
	return ret;
}

/* Returns true if timeval a is after timeval b */
static inline int timeval_after(const struct timeval *a,
				const struct timeval *b)
{
	return (a->tv_sec > b->tv_sec) ||
		(a->tv_sec == b->tv_sec && a->tv_usec > b->tv_usec);
}

/* Returns true if timespec a is after timespec b */
static inline int timespec_after(const struct timespec *a,
				 const struct timespec *b)
{
	return (a->tv_sec > b->tv_sec) ||
		(a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec);
}

/* Retruns the equivalent number of milliseconds for a given timespec.
 * The result is rounded up to the next millisecond. */
static inline unsigned int timespec_to_ms(const struct timespec *ts)
{
	return ts->tv_sec * 1000 + (ts->tv_nsec + 999999) / 1000000;
}

/* Convert milliseconds to timespec. */
static inline void ms_to_timespec(time_t milliseconds, struct timespec *ts)
{
	ts->tv_sec = milliseconds / 1000;
	ts->tv_nsec = (milliseconds % 1000) * 1000000;
}

/* Returns non-zero if the given timespec is non-zero. */
static inline int timespec_is_nonzero(const struct timespec *ts) {
	return ts && (ts->tv_sec != 0 ||
		      (ts->tv_sec == 0 && ts->tv_nsec != 0));
}

/* Calculates frames since time beg. */
static inline unsigned int cras_frames_since_time(const struct timespec *beg,
						  unsigned int rate)
{
	struct timespec now, time_since;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	if (!timespec_after(&now, beg))
		return 0;

	subtract_timespecs(&now, beg, &time_since);
	return cras_time_to_frames(&time_since, rate);
}

/* Poll on the given file descriptors.
 *
 * See ppoll(). This implementation changes the value of timeout to the
 * remaining time, and returns negative error codes on error.
 *
 * Args:
 *    fds - Array of pollfd structures.
 *    nfds - Number of pollfd structures.
 *    timeout - Timeout time updated upon return with remaining time. The
 *              timeout value may be updated to become invalid (negative
 *              tv_nsec or negative tv_sec). In that case, -tv_nsec is the
 *              number of nanoseconds by which the polling exceeded the
 *              supplied timeout. The function immediately returns with
 *              -ETIMEOUT if tv_nsec is negative, simplifying loops that
 *              rely on the returned remaining timeout.
 *    sigmask - Signal mask while in the poll.
 *
 * Returns:
 *    Positive when file decriptors are ready.
 *    Zero if no file descriptors are ready and timeout is NULL.
 *    -ETIMEDOUT when no file descriptors are ready and a timeout specified.
 *    Other negative error codes specified in the ppoll() man page.
 */
int cras_poll(struct pollfd *fds, nfds_t nfds, struct timespec *timeout,
              const sigset_t *sigmask);

/* Wait for /dev/input/event* files to become accessible.
 *
 * Returns:
 *   Zero on success. Otherwise a negative error code.
 */
int wait_for_dev_input_access();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CRAS_UTIL_H_ */
