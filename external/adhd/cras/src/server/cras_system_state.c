/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <syslog.h>

#include "cras_alsa_card.h"
#include "cras_config.h"
#include "cras_device_blacklist.h"
#include "cras_observer.h"
#include "cras_shm.h"
#include "cras_system_state.h"
#include "cras_tm.h"
#include "cras_types.h"
#include "cras_util.h"
#include "utlist.h"

struct card_list {
	struct cras_alsa_card *card;
	struct card_list *prev, *next;
};

/* The system state.
 * Members:
 *    exp_state - The exported system state shared with clients.
 *    shm_name - Name of posix shm region for exported state.
 *    shm_fd - fd for shm area of system_state struct.
 *    shm_fd_ro - fd for shm area of system_state struct, opened read-only.
 *    shm_size - Size of the shm area.
 *    device_config_dir - Directory of device configs where volume curves live.
 *    internal_ucm_suffix - The suffix to append to internal card name to
 *        control which ucm config file to load.
 *    device_blacklist - Blacklist of device the server will ignore.
 *    cards - A list of active sound cards in the system.
 *    update_lock - Protects the update_count, as audio threads can update the
 *      stream count.
 *    tm - The system-wide timer manager.
 */
static struct {
	struct cras_server_state *exp_state;
	char shm_name[NAME_MAX];
	int shm_fd;
	int shm_fd_ro;
	size_t shm_size;
	const char *device_config_dir;
	const char *internal_ucm_suffix;
	struct cras_device_blacklist *device_blacklist;
	struct card_list *cards;
	pthread_mutex_t update_lock;
	struct cras_tm *tm;
	/* Select loop callback registration. */
	int (*fd_add)(int fd, void (*cb)(void *data),
		      void *cb_data, void *select_data);
	void (*fd_rm)(int fd, void *select_data);
	void *select_data;
} state;

/*
 * Exported Interface.
 */

void cras_system_state_init(const char *device_config_dir)
{
	struct cras_server_state *exp_state;
	int rc;

	state.shm_size = sizeof(*exp_state);

	snprintf(state.shm_name, sizeof(state.shm_name), "/cras-%d", getpid());
	state.shm_fd = cras_shm_open_rw(state.shm_name, state.shm_size);
	if (state.shm_fd < 0)
		exit(state.shm_fd);

	/* mmap shm. */
	exp_state = mmap(NULL, state.shm_size,
			 PROT_READ | PROT_WRITE, MAP_SHARED,
			 state.shm_fd, 0);
	if (exp_state == (struct cras_server_state *)-1)
		exit(-ENOMEM);

	/* Open a read-only copy to dup and pass to clients. */
	state.shm_fd_ro = cras_shm_reopen_ro(state.shm_name, state.shm_fd);
	if (state.shm_fd_ro < 0)
		exit(state.shm_fd_ro);

	/* Initial system state. */
	exp_state->state_version = CRAS_SERVER_STATE_VERSION;
	exp_state->volume = CRAS_MAX_SYSTEM_VOLUME;
	exp_state->mute = 0;
	exp_state->mute_locked = 0;
	exp_state->suspended = 0;
	exp_state->capture_gain = DEFAULT_CAPTURE_GAIN;
	exp_state->capture_gain_target = DEFAULT_CAPTURE_GAIN;
	exp_state->capture_mute = 0;
	exp_state->capture_mute_locked = 0;
	exp_state->min_volume_dBFS = DEFAULT_MIN_VOLUME_DBFS;
	exp_state->max_volume_dBFS = DEFAULT_MAX_VOLUME_DBFS;
	exp_state->min_capture_gain = DEFAULT_MIN_CAPTURE_GAIN;
	exp_state->max_capture_gain = DEFAULT_MAX_CAPTURE_GAIN;
	exp_state->num_streams_attached = 0;

	if ((rc = pthread_mutex_init(&state.update_lock, 0) != 0)) {
		syslog(LOG_ERR, "Fatal: system state mutex init");
		exit(rc);
	}

	state.exp_state = exp_state;

	/* Directory for volume curve configs.
	 * Note that device_config_dir does not affect device blacklist.
	 * Device blacklist is common to all boards so we do not need
	 * to change device blacklist at run time. */
	state.device_config_dir = device_config_dir;
	state.internal_ucm_suffix = NULL;

	state.tm = cras_tm_init();
	if (!state.tm) {
		syslog(LOG_ERR, "Fatal: system state timer init");
		exit(-ENOMEM);
	}

	/* Read config file for blacklisted devices. */
	state.device_blacklist =
		cras_device_blacklist_create(CRAS_CONFIG_FILE_DIR);
}

void cras_system_state_set_internal_ucm_suffix(const char *internal_ucm_suffix)
{
	state.internal_ucm_suffix = internal_ucm_suffix;
}

void cras_system_state_deinit()
{
	/* Free any resources used.  This prevents unit tests from leaking. */

	cras_device_blacklist_destroy(state.device_blacklist);

	cras_tm_deinit(state.tm);

	if (state.exp_state) {
		munmap(state.exp_state, state.shm_size);
		cras_shm_close_unlink(state.shm_name, state.shm_fd);
		if (state.shm_fd_ro != state.shm_fd)
			close(state.shm_fd_ro);
	}

	pthread_mutex_destroy(&state.update_lock);
}

void cras_system_set_volume(size_t volume)
{
	if (volume > CRAS_MAX_SYSTEM_VOLUME)
		syslog(LOG_DEBUG, "system volume set out of range %zu", volume);

	state.exp_state->volume = MIN(volume, CRAS_MAX_SYSTEM_VOLUME);
	cras_observer_notify_output_volume(state.exp_state->volume);
}

size_t cras_system_get_volume()
{
	return state.exp_state->volume;
}

void cras_system_set_capture_gain(long gain)
{
	/* Adjust targeted gain to be in supported range. */
	state.exp_state->capture_gain_target = gain;
	gain = MAX(gain, state.exp_state->min_capture_gain);
	gain = MIN(gain, state.exp_state->max_capture_gain);
	state.exp_state->capture_gain = gain;
	cras_observer_notify_capture_gain(state.exp_state->capture_gain);
}

long cras_system_get_capture_gain()
{
	return state.exp_state->capture_gain;
}

void cras_system_notify_mute(void)
{
	cras_observer_notify_output_mute(state.exp_state->mute,
					 state.exp_state->user_mute,
					 state.exp_state->mute_locked);
}

void cras_system_set_user_mute(int mute)
{
	if (state.exp_state->user_mute == !!mute)
		return;

	state.exp_state->user_mute = !!mute;
	cras_system_notify_mute();
}

void cras_system_set_mute(int mute)
{
	if (state.exp_state->mute_locked)
		return;

	if (state.exp_state->mute == !!mute)
		return;

	state.exp_state->mute = !!mute;
	cras_system_notify_mute();
}

void cras_system_set_mute_locked(int locked)
{
	if (state.exp_state->mute_locked == !!locked)
		return;

	state.exp_state->mute_locked = !!locked;
	cras_system_notify_mute();
}

int cras_system_get_mute()
{
	return state.exp_state->mute || state.exp_state->user_mute;
}

int cras_system_get_user_mute()
{
	return state.exp_state->user_mute;
}

int cras_system_get_system_mute()
{
	return state.exp_state->mute;
}

int cras_system_get_mute_locked()
{
	return state.exp_state->mute_locked;
}

void cras_system_notify_capture_mute(void)
{
	cras_observer_notify_capture_mute(state.exp_state->capture_mute,
					  state.exp_state->capture_mute_locked);
}

void cras_system_set_capture_mute(int mute)
{
	if (state.exp_state->capture_mute_locked)
		return;

	state.exp_state->capture_mute = !!mute;
	cras_system_notify_capture_mute();
}

void cras_system_set_capture_mute_locked(int locked)
{
	state.exp_state->capture_mute_locked = !!locked;
	cras_system_notify_capture_mute();
}

int cras_system_get_capture_mute()
{
	return state.exp_state->capture_mute;
}

int cras_system_get_capture_mute_locked()
{
	return state.exp_state->capture_mute_locked;
}

int cras_system_get_suspended()
{
	return state.exp_state->suspended;
}

void cras_system_set_suspended(int suspended)
{
	state.exp_state->suspended = suspended;
	cras_observer_notify_suspend_changed(suspended);
}

void cras_system_set_volume_limits(long min, long max)
{
	state.exp_state->min_volume_dBFS = min;
	state.exp_state->max_volume_dBFS = max;
}

long cras_system_get_min_volume()
{
	return state.exp_state->min_volume_dBFS;
}

long cras_system_get_max_volume()
{
	return state.exp_state->max_volume_dBFS;
}

void cras_system_set_capture_gain_limits(long min, long max)
{
	state.exp_state->min_capture_gain = MAX(min, DEFAULT_MIN_CAPTURE_GAIN);
	state.exp_state->max_capture_gain = max;
	/* Re-apply target gain subjected to the new supported range. */
	cras_system_set_capture_gain(state.exp_state->capture_gain_target);
}

long cras_system_get_min_capture_gain()
{
	return state.exp_state->min_capture_gain;
}

long cras_system_get_max_capture_gain()
{
	return state.exp_state->max_capture_gain;
}

int cras_system_add_alsa_card(struct cras_alsa_card_info *alsa_card_info)
{
	struct card_list *card;
	struct cras_alsa_card *alsa_card;
	unsigned card_index;

	if (alsa_card_info == NULL)
		return -EINVAL;

	card_index = alsa_card_info->card_index;

	DL_FOREACH(state.cards, card) {
		if (card_index == cras_alsa_card_get_index(card->card))
			return -EINVAL;
	}
	alsa_card = cras_alsa_card_create(
			alsa_card_info,
			state.device_config_dir,
			state.device_blacklist,
			(alsa_card_info->card_type == ALSA_CARD_TYPE_INTERNAL)
				? state.internal_ucm_suffix
				: NULL);
	if (alsa_card == NULL)
		return -ENOMEM;
	card = calloc(1, sizeof(*card));
	if (card == NULL)
		return -ENOMEM;
	card->card = alsa_card;
	DL_APPEND(state.cards, card);
	return 0;
}

int cras_system_remove_alsa_card(size_t alsa_card_index)
{
	struct card_list *card;

	DL_FOREACH(state.cards, card) {
		if (alsa_card_index == cras_alsa_card_get_index(card->card))
			break;
	}
	if (card == NULL)
		return -EINVAL;
	DL_DELETE(state.cards, card);
	cras_alsa_card_destroy(card->card);
	free(card);
	return 0;
}

int cras_system_alsa_card_exists(unsigned alsa_card_index)
{
	struct card_list *card;

	DL_FOREACH(state.cards, card)
		if (alsa_card_index == cras_alsa_card_get_index(card->card))
			return 1;
	return 0;
}

int cras_system_set_select_handler(int (*add)(int fd,
					      void (*callback)(void *data),
					      void *callback_data,
					      void *select_data),
				   void (*rm)(int fd, void *select_data),
				   void *select_data)
{
	if (state.fd_add != NULL || state.fd_rm != NULL)
		return -EEXIST;
	state.fd_add = add;
	state.fd_rm = rm;
	state.select_data = select_data;
	return 0;
}

int cras_system_add_select_fd(int fd,
			      void (*callback)(void *data),
			      void *callback_data)
{
	if (state.fd_add == NULL)
		return -EINVAL;
	return state.fd_add(fd, callback, callback_data,
			    state.select_data);
}

void cras_system_rm_select_fd(int fd)
{
	if (state.fd_rm != NULL)
		state.fd_rm(fd, state.select_data);
}

void cras_system_state_stream_added(enum CRAS_STREAM_DIRECTION direction)
{
	struct cras_server_state *s;

	s = cras_system_state_update_begin();
	if (!s)
		return;

	s->num_active_streams[direction]++;
	s->num_streams_attached++;

	cras_system_state_update_complete();
	cras_observer_notify_num_active_streams(
		direction, s->num_active_streams[direction]);
}

void cras_system_state_stream_removed(enum CRAS_STREAM_DIRECTION direction)
{
	struct cras_server_state *s;
	unsigned i, sum;


	s = cras_system_state_update_begin();
	if (!s)
		return;

	sum = 0;
	for (i=0; i < CRAS_NUM_DIRECTIONS; i++)
		sum += s->num_active_streams[i];

	/* Set the last active time when removing the final stream. */
	if (sum == 1)
		cras_clock_gettime(CLOCK_MONOTONIC_RAW,
				   &s->last_active_stream_time);
	s->num_active_streams[direction]--;

	cras_system_state_update_complete();
	cras_observer_notify_num_active_streams(
		direction, s->num_active_streams[direction]);
}

unsigned cras_system_state_get_active_streams()
{
	unsigned i, sum;
	sum = 0;
	for (i=0; i < CRAS_NUM_DIRECTIONS; i++)
		sum += state.exp_state->num_active_streams[i];
	return sum;
}

unsigned cras_system_state_get_active_streams_by_direction(
	enum CRAS_STREAM_DIRECTION direction)
{
	return state.exp_state->num_active_streams[direction];
}

void cras_system_state_get_last_stream_active_time(struct cras_timespec *ts)
{
	*ts = state.exp_state->last_active_stream_time;
}

int cras_system_state_get_output_devs(const struct cras_iodev_info **devs)
{
	*devs = state.exp_state->output_devs;
	return state.exp_state->num_output_devs;
}

int cras_system_state_get_input_devs(const struct cras_iodev_info **devs)
{
	*devs = state.exp_state->input_devs;
	return state.exp_state->num_input_devs;
}

int cras_system_state_get_output_nodes(const struct cras_ionode_info **nodes)
{
	*nodes = state.exp_state->output_nodes;
	return state.exp_state->num_output_nodes;
}

int cras_system_state_get_input_nodes(const struct cras_ionode_info **nodes)
{
	*nodes = state.exp_state->input_nodes;
	return state.exp_state->num_input_nodes;
}

struct cras_server_state *cras_system_state_update_begin()
{
	if (pthread_mutex_lock(&state.update_lock)) {
		syslog(LOG_ERR, "Failed to lock stream mutex");
		return NULL;
	}

	__sync_fetch_and_add(&state.exp_state->update_count, 1);
	return state.exp_state;
}

void cras_system_state_update_complete()
{
	__sync_fetch_and_add(&state.exp_state->update_count, 1);
	pthread_mutex_unlock(&state.update_lock);
}

struct cras_server_state *cras_system_state_get_no_lock()
{
	return state.exp_state;
}

key_t cras_sys_state_shm_fd()
{
	return state.shm_fd_ro;
}

struct cras_tm *cras_system_state_get_tm()
{
	return state.tm;
}
