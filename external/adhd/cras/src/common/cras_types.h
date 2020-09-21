/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Types commonly used in the client and server are defined here.
 */
#ifndef CRAS_TYPES_H_
#define CRAS_TYPES_H_

#include <stdint.h>
#include <stdlib.h>

#include "cras_audio_format.h"
#include "cras_iodev_info.h"

/* Architecture independent timespec */
struct __attribute__ ((__packed__)) cras_timespec {
	int64_t tv_sec;
	int64_t tv_nsec;
};

/* Some special device index values. */
enum CRAS_SPECIAL_DEVICE {
	NO_DEVICE,
	SILENT_RECORD_DEVICE,
	SILENT_PLAYBACK_DEVICE,
	MAX_SPECIAL_DEVICE_IDX
};

/*
 * Types of test iodevs supported.
 */
enum TEST_IODEV_TYPE {
	TEST_IODEV_HOTWORD,
};


/* Commands for test iodevs. */
enum CRAS_TEST_IODEV_CMD {
	TEST_IODEV_CMD_HOTWORD_TRIGGER,
};

/* Directions of audio streams.
 * Input, Output, or loopback.
 *
 * Note that we use enum CRAS_STREAM_DIRECTION to access the elements in
 * num_active_streams in cras_server_state. For example,
 * num_active_streams[CRAS_STREAM_OUTPUT] is the number of active
 * streams with direction CRAS_STREAM_OUTPUT.
 */
enum CRAS_STREAM_DIRECTION {
	CRAS_STREAM_OUTPUT,
	CRAS_STREAM_INPUT,
	CRAS_STREAM_UNDEFINED,
	CRAS_STREAM_POST_MIX_PRE_DSP,
	CRAS_NUM_DIRECTIONS
};

/*
 * Flags for stream types.
 *  BULK_AUDIO_OK - This stream is OK with receiving up to a full shm of samples
 *      in a single callback.
 *  USE_DEV_TIMING - Don't wake up based on stream timing.  Only wake when the
 *      device is ready. Input streams only.
 *  HOTWORD_STREAM - This stream is used only to listen for hotwords such as "OK
 *      Google".  Hardware will wake the device when this phrase is heard.
 */
enum CRAS_INPUT_STREAM_FLAG {
	BULK_AUDIO_OK = 0x01,
	USE_DEV_TIMING = 0x02,
	HOTWORD_STREAM = BULK_AUDIO_OK | USE_DEV_TIMING,
};

/*
 * Types of Loopback stream.
 */
enum CRAS_LOOPBACK_TYPE {
	LOOPBACK_POST_MIX_PRE_DSP,
	LOOPBACK_POST_DSP,
	LOOPBACK_NUM_TYPES,
};

static inline int cras_stream_uses_output_hw(enum CRAS_STREAM_DIRECTION dir)
{
	return dir == CRAS_STREAM_OUTPUT;
}

static inline int cras_stream_uses_input_hw(enum CRAS_STREAM_DIRECTION dir)
{
	return dir == CRAS_STREAM_INPUT;
}

static inline int cras_stream_has_input(enum CRAS_STREAM_DIRECTION dir)
{
	return dir != CRAS_STREAM_OUTPUT;
}

static inline int cras_stream_is_loopback(enum CRAS_STREAM_DIRECTION dir)
{
	return dir == CRAS_STREAM_POST_MIX_PRE_DSP;
}

/* Types of audio streams. */
enum CRAS_STREAM_TYPE {
	CRAS_STREAM_TYPE_DEFAULT,
	CRAS_STREAM_TYPE_MULTIMEDIA,
	CRAS_STREAM_TYPE_VOICE_COMMUNICATION,
	CRAS_STREAM_TYPE_SPEECH_RECOGNITION,
	CRAS_STREAM_TYPE_PRO_AUDIO,
	CRAS_STREAM_NUM_TYPES,
};

#define ENUM_STR(x) case x: return #x;

static inline const char *cras_stream_type_str(
		enum CRAS_STREAM_TYPE stream_type)
{
	switch(stream_type) {
	ENUM_STR(CRAS_STREAM_TYPE_DEFAULT)
	ENUM_STR(CRAS_STREAM_TYPE_MULTIMEDIA)
	ENUM_STR(CRAS_STREAM_TYPE_VOICE_COMMUNICATION)
	ENUM_STR(CRAS_STREAM_TYPE_SPEECH_RECOGNITION)
	ENUM_STR(CRAS_STREAM_TYPE_PRO_AUDIO)
	default:
		return "INVALID_STREAM_TYPE";
	}
}

/* Information about a client attached to the server. */
struct __attribute__ ((__packed__)) cras_attached_client_info {
	uint32_t id;
	int32_t pid;
	uint32_t uid;
	uint32_t gid;
};

/* Each ionode has a unique id. The top 32 bits are the device index, lower 32
 * are the node index. */
typedef uint64_t cras_node_id_t;

static inline cras_node_id_t cras_make_node_id(uint32_t dev_index,
					       uint32_t node_index)
{
	cras_node_id_t id = dev_index;
	return (id << 32) | node_index;
}

static inline uint32_t dev_index_of(cras_node_id_t id)
{
	return (uint32_t) (id >> 32);
}

static inline uint32_t node_index_of(cras_node_id_t id)
{
	return (uint32_t) id;
}

#define CRAS_MAX_IODEVS 20
#define CRAS_MAX_IONODES 20
#define CRAS_MAX_ATTACHED_CLIENTS 20
#define CRAS_HOTWORD_STRING_SIZE 256
#define MAX_DEBUG_DEVS 4
#define MAX_DEBUG_STREAMS 8
#define AUDIO_THREAD_EVENT_LOG_SIZE (1024*6)

/* There are 8 bits of space for events. */
enum AUDIO_THREAD_LOG_EVENTS {
	AUDIO_THREAD_WAKE,
	AUDIO_THREAD_SLEEP,
	AUDIO_THREAD_READ_AUDIO,
	AUDIO_THREAD_READ_AUDIO_TSTAMP,
	AUDIO_THREAD_READ_AUDIO_DONE,
	AUDIO_THREAD_READ_OVERRUN,
	AUDIO_THREAD_FILL_AUDIO,
	AUDIO_THREAD_FILL_AUDIO_TSTAMP,
	AUDIO_THREAD_FILL_AUDIO_DONE,
	AUDIO_THREAD_WRITE_STREAMS_WAIT,
	AUDIO_THREAD_WRITE_STREAMS_WAIT_TO,
	AUDIO_THREAD_WRITE_STREAMS_MIX,
	AUDIO_THREAD_WRITE_STREAMS_MIXED,
	AUDIO_THREAD_WRITE_STREAMS_STREAM,
	AUDIO_THREAD_FETCH_STREAM,
	AUDIO_THREAD_STREAM_ADDED,
	AUDIO_THREAD_STREAM_REMOVED,
	AUDIO_THREAD_A2DP_ENCODE,
	AUDIO_THREAD_A2DP_WRITE,
	AUDIO_THREAD_DEV_STREAM_MIX,
	AUDIO_THREAD_CAPTURE_POST,
	AUDIO_THREAD_CAPTURE_WRITE,
	AUDIO_THREAD_CONV_COPY,
	AUDIO_THREAD_STREAM_SLEEP_TIME,
	AUDIO_THREAD_STREAM_SLEEP_ADJUST,
	AUDIO_THREAD_STREAM_SKIP_CB,
	AUDIO_THREAD_DEV_SLEEP_TIME,
	AUDIO_THREAD_SET_DEV_WAKE,
	AUDIO_THREAD_DEV_ADDED,
	AUDIO_THREAD_DEV_REMOVED,
	AUDIO_THREAD_IODEV_CB,
	AUDIO_THREAD_PB_MSG,
	AUDIO_THREAD_ODEV_NO_STREAMS,
	AUDIO_THREAD_ODEV_START,
	AUDIO_THREAD_ODEV_LEAVE_NO_STREAMS,
	AUDIO_THREAD_ODEV_DEFAULT_NO_STREAMS,
	AUDIO_THREAD_FILL_ODEV_ZEROS,
	AUDIO_THREAD_SEVERE_UNDERRUN,
};

struct __attribute__ ((__packed__)) audio_thread_event {
	uint32_t tag_sec;
	uint32_t nsec;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
};

/* Ring buffer of log events from the audio thread. */
struct __attribute__ ((__packed__)) audio_thread_event_log {
	uint32_t write_pos;
	uint32_t len;
	struct audio_thread_event log[AUDIO_THREAD_EVENT_LOG_SIZE];
};

struct __attribute__ ((__packed__)) audio_dev_debug_info {
	char dev_name[CRAS_NODE_NAME_BUFFER_SIZE];
	uint32_t buffer_size;
	uint32_t min_buffer_level;
	uint32_t min_cb_level;
	uint32_t max_cb_level;
	uint32_t frame_rate;
	uint32_t num_channels;
	double est_rate_ratio;
	uint8_t direction;
	uint32_t num_underruns;
	uint32_t num_severe_underruns;
};

struct __attribute__ ((__packed__)) audio_stream_debug_info {
	uint64_t stream_id;
	uint32_t dev_idx;
	uint32_t direction;
	uint32_t stream_type;
	uint32_t buffer_frames;
	uint32_t cb_threshold;
	uint32_t flags;
	uint32_t frame_rate;
	uint32_t num_channels;
	uint32_t longest_fetch_sec;
	uint32_t longest_fetch_nsec;
	uint32_t num_overruns;
	int8_t channel_layout[CRAS_CH_MAX];
};

/* Debug info shared from server to client. */
struct __attribute__ ((__packed__)) audio_debug_info {
	uint32_t num_streams;
	uint32_t num_devs;
	struct audio_dev_debug_info devs[MAX_DEBUG_DEVS];
	struct audio_stream_debug_info streams[MAX_DEBUG_STREAMS];
	struct audio_thread_event_log log;
};


/* The server state that is shared with clients.
 *    state_version - Version of this structure.
 *    volume - index from 0-100.
 *    min_volume_dBFS - volume in dB * 100 when volume = 1.
 *    max_volume_dBFS - volume in dB * 100 when volume = max.
 *    mute - 0 = unmuted, 1 = muted by system (device switch, suspend, etc).
 *    user_mute - 0 = unmuted, 1 = muted by user.
 *    mute_locked - 0 = unlocked, 1 = locked.
 *    suspended - 1 = suspended, 0 = resumed.
 *    capture_gain - Capture gain in dBFS * 100.
 *    capture_gain_target - Target capture gain in dBFS * 100. The actual
 *                          capture gain will be subjected to current
 *                          supported range. When active device/node changes,
 *                          supported range changes accordingly. System state
 *                          should try to re-apply target gain subjected to new
 *                          range.
 *    capture_mute - 0 = unmuted, 1 = muted.
 *    capture_mute_locked - 0 = unlocked, 1 = locked.
 *    min_capture_gain - Min allowed capture gain in dBFS * 100.
 *    max_capture_gain - Max allowed capture gain in dBFS * 100.
 *    num_streams_attached - Total number of streams since server started.
 *    num_output_devs - Number of available output devices.
 *    num_input_devs - Number of available input devices.
 *    output_devs - Output audio devices currently attached.
 *    input_devs - Input audio devices currently attached.
 *    num_output_nodes - Number of available output nodes.
 *    num_input_nodes - Number of available input nodes.
 *    output_nodes - Output nodes currently attached.
 *    input_nodes - Input nodes currently attached.
 *    num_attached_clients - Number of clients attached to server.
 *    client_info - List of first 20 attached clients.
 *    update_count - Incremented twice each time the struct is updated.  Odd
 *        during updates.
 *    num_active_streams - An array containing numbers or active
 *        streams of different directions.
 *    last_active_stream_time - Time the last stream was removed.  Can be used
 *        to determine how long audio has been idle.
 *    audio_debug_info - Debug data filled in when a client requests it. This
 *        isn't protected against concurrent updating, only one client should
 *        use it.
 */
#define CRAS_SERVER_STATE_VERSION 2
struct __attribute__ ((packed, aligned(4))) cras_server_state {
	uint32_t state_version;
	uint32_t volume;
	int32_t min_volume_dBFS;
	int32_t max_volume_dBFS;
	int32_t mute;
	int32_t user_mute;
	int32_t mute_locked;
	int32_t suspended;
	int32_t capture_gain;
	int32_t capture_gain_target;
	int32_t capture_mute;
	int32_t capture_mute_locked;
	int32_t min_capture_gain;
	int32_t max_capture_gain;
	uint32_t num_streams_attached;
	uint32_t num_output_devs;
	uint32_t num_input_devs;
	struct cras_iodev_info output_devs[CRAS_MAX_IODEVS];
	struct cras_iodev_info input_devs[CRAS_MAX_IODEVS];
	uint32_t num_output_nodes;
	uint32_t num_input_nodes;
	struct cras_ionode_info output_nodes[CRAS_MAX_IONODES];
	struct cras_ionode_info input_nodes[CRAS_MAX_IONODES];
	uint32_t num_attached_clients;
	struct cras_attached_client_info client_info[CRAS_MAX_ATTACHED_CLIENTS];
	uint32_t update_count;
	uint32_t num_active_streams[CRAS_NUM_DIRECTIONS];
	struct cras_timespec last_active_stream_time;
	struct audio_debug_info audio_debug_info;
};

/* Actions for card add/remove/change. */
enum cras_notify_device_action { /* Must match gavd action definitions.  */
	CRAS_DEVICE_ACTION_ADD    = 0,
	CRAS_DEVICE_ACTION_REMOVE = 1,
	CRAS_DEVICE_ACTION_CHANGE = 2,
};

/* Information about an ALSA card to be added to the system.
 *    card_type - Either internal card or a USB sound card.
 *    card_index - Index ALSA uses to refer to the card.  The X in "hw:X".
 *    priority - Base priority to give devices found on this card. Zero is the
 *      lowest priority.  Non-primary devices on the card will be given a
 *      lowered priority.
 *    usb_vendor_id - vendor ID if the device is on the USB bus.
 *    usb_product_id - product ID if the device is on the USB bus.
 *    usb_serial_number - serial number if the device is on the USB bus.
 *    usb_desc_checksum - the checksum of the USB descriptors if the device
 *      is on the USB bus.
 */
enum CRAS_ALSA_CARD_TYPE {
	ALSA_CARD_TYPE_INTERNAL,
	ALSA_CARD_TYPE_USB,
};
#define USB_SERIAL_NUMBER_BUFFER_SIZE 64
struct __attribute__ ((__packed__)) cras_alsa_card_info {
	enum CRAS_ALSA_CARD_TYPE card_type;
	uint32_t card_index;
	uint32_t usb_vendor_id;
	uint32_t usb_product_id;
	char usb_serial_number[USB_SERIAL_NUMBER_BUFFER_SIZE];
	uint32_t usb_desc_checksum;
};

/* Unique identifier for each active stream.
 * The top 16 bits are the client number, lower 16 are the stream number.
 */
typedef uint32_t cras_stream_id_t;
/* Generates a stream id for client stream. */
static inline cras_stream_id_t cras_get_stream_id(uint16_t client_id,
						  uint16_t stream_id)
{
	return (cras_stream_id_t)(((client_id & 0x0000ffff) << 16) |
				  (stream_id & 0x0000ffff));
}

enum CRAS_NODE_TYPE {
	/* These value can be used for output nodes. */
	CRAS_NODE_TYPE_INTERNAL_SPEAKER,
	CRAS_NODE_TYPE_HEADPHONE,
	CRAS_NODE_TYPE_HDMI,
	CRAS_NODE_TYPE_HAPTIC,
	CRAS_NODE_TYPE_LINEOUT,
	/* These value can be used for input nodes. */
	CRAS_NODE_TYPE_MIC,
	CRAS_NODE_TYPE_HOTWORD,
	CRAS_NODE_TYPE_POST_MIX_PRE_DSP,
	CRAS_NODE_TYPE_POST_DSP,
	/* These value can be used for both output and input nodes. */
	CRAS_NODE_TYPE_USB,
	CRAS_NODE_TYPE_BLUETOOTH,
	CRAS_NODE_TYPE_UNKNOWN,
};

/* Position values to described where a node locates on the system.
 * NODE_POSITION_EXTERNAL - The node works only when peripheral
 *     is plugged.
 * NODE_POSITION_INTERNAL - The node lives on the system and doesn't
 *     have specific direction.
 * NODE_POSITION_FRONT - The node locates on the side of system that
 *     faces user.
 * NODE_POSITION_REAR - The node locates on the opposite side of
 *     the system that faces user.
 * NODE_POSITION_KEYBOARD - The node locates under the keyboard.
 */
enum CRAS_NODE_POSITION {
	NODE_POSITION_EXTERNAL,
	NODE_POSITION_INTERNAL,
	NODE_POSITION_FRONT,
	NODE_POSITION_REAR,
	NODE_POSITION_KEYBOARD,
};

#endif /* CRAS_TYPES_H_ */
