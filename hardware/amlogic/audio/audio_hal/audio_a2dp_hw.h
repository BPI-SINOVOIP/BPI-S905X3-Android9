/******************************************************************************
 *
 *  Copyright 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/*****************************************************************************
 *
 *  Filename:      audio_a2dp_hw.h
 *
 *  Description:
 *
 *****************************************************************************/

#ifndef AUDIO_A2DP_HW_H
#define AUDIO_A2DP_HW_H

#include <stdint.h>

/*****************************************************************************
 *  Constants & Macros
 *****************************************************************************/

#define A2DP_AUDIO_HARDWARE_INTERFACE "audio.a2dp"
#define A2DP_CTRL_PATH "/data/misc/bluedroid/.a2dp_ctrl"
#define A2DP_DATA_PATH "/data/misc/bluedroid/.a2dp_data"

// AUDIO_STREAM_OUTPUT_BUFFER_SZ controls the size of the audio socket buffer.
// If one assumes the write buffer is always full during normal BT playback,
// then increasing this value increases our playback latency.
//
// FIXME: The BT HAL should consume data at a constant rate.
// AudioFlinger assumes that the HAL draws data at a constant rate, which is
// true for most audio devices; however, the BT engine reads data at a variable
// rate (over the short term), which confuses both AudioFlinger as well as
// applications which deliver data at a (generally) fixed rate.
//
// 20 * 512 is not sufficient to smooth the variability for some BT devices,
// resulting in mixer sleep and throttling. We increase this to 28 * 512 to help
// reduce the effect of variable data consumption.
#define AUDIO_STREAM_OUTPUT_BUFFER_SZ (28 * 512)
#define AUDIO_STREAM_CONTROL_OUTPUT_BUFFER_SZ 256

// AUDIO_STREAM_OUTPUT_BUFFER_PERIODS controls how the socket buffer is divided
// for AudioFlinger data delivery. The AudioFlinger mixer delivers data in
// chunks of AUDIO_STREAM_OUTPUT_BUFFER_SZ / AUDIO_STREAM_OUTPUT_BUFFER_PERIODS.
// If the number of periods is 2, the socket buffer represents "double
// buffering" of the AudioFlinger mixer buffer.
//
// In general, AUDIO_STREAM_OUTPUT_BUFFER_PERIODS * 16 * 4 should be a divisor
// of AUDIO_STREAM_OUTPUT_BUFFER_SZ.
//
// These values should be chosen such that
//
// AUDIO_STREAM_BUFFER_SIZE * 1000 / (AUDIO_STREAM_OUTPUT_BUFFER_PERIODS
//         * AUDIO_STREAM_DEFAULT_RATE * 4) > 20 (ms)
//
// to avoid introducing the FastMixer in AudioFlinger. Using the FastMixer
// results in unnecessary latency and CPU overhead for Bluetooth.
#define AUDIO_STREAM_OUTPUT_BUFFER_PERIODS 2

#define AUDIO_SKT_DISCONNECTED (-1)

typedef enum {
    A2DP_CTRL_CMD_NONE,
    A2DP_CTRL_CMD_CHECK_READY,
    A2DP_CTRL_CMD_START,
    A2DP_CTRL_CMD_STOP,
    A2DP_CTRL_CMD_SUSPEND,
    A2DP_CTRL_GET_INPUT_AUDIO_CONFIG,
    A2DP_CTRL_GET_OUTPUT_AUDIO_CONFIG,
    A2DP_CTRL_SET_OUTPUT_AUDIO_CONFIG,
    A2DP_CTRL_CMD_OFFLOAD_START,
    A2DP_CTRL_GET_PRESENTATION_POSITION,
} tA2DP_CTRL_CMD;

typedef enum {
    A2DP_CTRL_ACK_SUCCESS,
    A2DP_CTRL_ACK_FAILURE,
    A2DP_CTRL_ACK_INCALL_FAILURE, /* Failure when in Call*/
    A2DP_CTRL_ACK_UNSUPPORTED,
    A2DP_CTRL_ACK_PENDING,
    A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS,
} tA2DP_CTRL_ACK;

typedef enum {
    BTAV_A2DP_CODEC_SAMPLE_RATE_NONE = 0x0,
    BTAV_A2DP_CODEC_SAMPLE_RATE_44100 = 0x1 << 0,
    BTAV_A2DP_CODEC_SAMPLE_RATE_48000 = 0x1 << 1,
    BTAV_A2DP_CODEC_SAMPLE_RATE_88200 = 0x1 << 2,
    BTAV_A2DP_CODEC_SAMPLE_RATE_96000 = 0x1 << 3,
    BTAV_A2DP_CODEC_SAMPLE_RATE_176400 = 0x1 << 4,
    BTAV_A2DP_CODEC_SAMPLE_RATE_192000 = 0x1 << 5,
    BTAV_A2DP_CODEC_SAMPLE_RATE_16000 = 0x1 << 6,
    BTAV_A2DP_CODEC_SAMPLE_RATE_24000 = 0x1 << 7
} btav_a2dp_codec_sample_rate_t;

typedef enum {
    BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE = 0x0,
    BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16 = 0x1 << 0,
    BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24 = 0x1 << 1,
    BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32 = 0x1 << 2
} btav_a2dp_codec_bits_per_sample_t;

typedef enum {
    BTAV_A2DP_CODEC_CHANNEL_MODE_NONE = 0x0,
    BTAV_A2DP_CODEC_CHANNEL_MODE_MONO = 0x1 << 0,
    BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO = 0x1 << 1
} btav_a2dp_codec_channel_mode_t;

typedef struct btav_a2dp_codec_config {
    btav_a2dp_codec_sample_rate_t sample_rate;
    btav_a2dp_codec_bits_per_sample_t bits_per_sample;
    btav_a2dp_codec_channel_mode_t channel_mode;
} btav_a2dp_codec_config_t;

typedef enum {
    AUDIO_A2DP_STATE_STARTING,
    AUDIO_A2DP_STATE_STARTED,
    AUDIO_A2DP_STATE_STOPPING,
    AUDIO_A2DP_STATE_STOPPED,
    /* need explicit set param call to resume (suspend=false) */
    AUDIO_A2DP_STATE_SUSPENDED,
    AUDIO_A2DP_STATE_STANDBY /* allows write to autoresume */
} a2dp_state_t;

struct a2dp_stream_out {
    //struct aml_stream_out aml_out;
    pthread_mutex_t mutex;
    int ctrl_fd;
    int audio_fd;
    size_t buffer_sz;
    bool is_stereo_to_mono;  // True if fetching Stereo and mixing into Mono
    bool enable_delay_reporting;
    a2dp_state_t state;
    uint64_t frames_presented;  // frames written, never reset
    uint64_t frames_rendered;   // frames written, reset on standby
    aml_audio_resample_t *pstResampler;
    uint32_t rate;
    void * vir_buf_handle;

#if defined(AUDIO_EFFECT_EXTERN_DEVICE)
    float bt_gain;
    float right_gain;
    float left_gain;
    int bt_unmute;
#endif
};


/*****************************************************************************
 *  Functions
 *****************************************************************************/

int a2dp_output_enable(struct audio_stream_out* stream);
void a2dp_output_disable(struct audio_stream_out* stream);
uint32_t a2dp_out_get_latency(const struct audio_stream_out* stream);
ssize_t a2dp_out_write(struct audio_stream_out* stream, const void* buffer, size_t bytes);
int a2dp_out_standby(struct audio_stream* stream);
int a2dp_out_get_presentation_position(const struct audio_stream_out* stream,
        uint64_t* frames, struct timespec* timestamp);
int a2dp_out_set_parameters(struct audio_stream* stream, const char* kvpairs);

#endif /* A2DP_AUDIO_HW_H */
