/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * The blow logging funcitons must only be called from the audio thread.
 */

#ifndef AUDIO_THREAD_LOG_H_
#define AUDIO_THREAD_LOG_H_

#include <pthread.h>
#include <stdint.h>

#include "cras_types.h"

#define AUDIO_THREAD_LOGGING	1

#if (AUDIO_THREAD_LOGGING)
#define ATLOG(log,event,data1,data2,data3) \
	audio_thread_event_log_data(log,event,data1,data2,data3);
#else
#define ATLOG(log,event,data1,data2,data3)
#endif

extern struct audio_thread_event_log *atlog;

static inline
struct audio_thread_event_log *audio_thread_event_log_init()
{
	struct audio_thread_event_log *log;
	log = (struct audio_thread_event_log *)
			calloc(1, sizeof(struct audio_thread_event_log));
	log->len = AUDIO_THREAD_EVENT_LOG_SIZE;

	return log;
}

static inline
void audio_thread_event_log_deinit(struct audio_thread_event_log *log)
{
	free(log);
}

/* Log a tag and the current time, Uses two words, the first is split
 * 8 bits for tag and 24 for seconds, second word is micro seconds.
 */
static inline void audio_thread_event_log_data(
		struct audio_thread_event_log *log,
		enum AUDIO_THREAD_LOG_EVENTS event,
		uint32_t data1,
		uint32_t data2,
		uint32_t data3)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	log->log[log->write_pos].tag_sec =
			(event << 24) | (now.tv_sec & 0x00ffffff);
	log->log[log->write_pos].nsec = now.tv_nsec;
	log->log[log->write_pos].data1 = data1;
	log->log[log->write_pos].data2 = data2;
	log->log[log->write_pos].data3 = data3;

	log->write_pos++;
	log->write_pos %= AUDIO_THREAD_EVENT_LOG_SIZE;
}

#endif /* AUDIO_THREAD_LOG_H_ */
