/**************************************************************************
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
 **************************************************************************/

/**************************************************************************
 *
 *  Filename:      audio_a2dp_hw.c
 *
 *  Description:   Implements hal for bluedroid a2dp audio device
 *
 **************************************************************************/

#define LOG_TAG "audio_a2dp_hw"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#if defined(AUDIO_EFFECT_EXTERN_DEVICE)
#include <cutils/str_parms.h>
#endif
#include <cutils/properties.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>

#include "audio_hw.h"
#include "audio_hw_utils.h"
#include "audio_a2dp_hw.h"
//#include "alsa_manager.h"
#include <aml_android_utils.h>
#include "audio_virtual_buf.h"
#include "dolby_lib_api.h"
#include "aml_audio_timer.h"

/*************************************************************************
 *  Constants & Macros
 *************************************************************************/

#define CTRL_CHAN_RETRY_COUNT 3
#define USEC_PER_SEC 1000000L
#define SOCK_SEND_TIMEOUT_MS 2000 /* Timeout for sending */
#define SOCK_RECV_TIMEOUT_MS 5000 /* Timeout for receiving */
#define SEC_TO_MS 1000
#define SEC_TO_NS 1000000000
#define MS_TO_NS 1000000
#define MIN_DELAY_NS (100*MS_TO_NS)
#define MAX_DELAY_NS (1000*MS_TO_NS)
#define DELAY_TO_NS 100000
#define VIR_BUFF_NS (80*MS_TO_NS)

/* set WRITE_POLL_MS to 0 for blocking sockets,
 * nonzero for polled non-blocking sockets
 */
#define WRITE_POLL_MS 3

#define UNUSED_ATTR __attribute__((unused))

// Re-run |fn| system call until the system call doesn't cause EINTR.
#define OSI_NO_INTR(fn) \
  do {                  \
  } while ((fn) == -1 && errno == EINTR)

uint64_t total_input_ns = 0;

static void a2dp_open_ctrl_path(struct a2dp_stream_out* out);

#define CASE_RETURN_STR(const) \
    case const:                  \
        return #const;

const char* audio_a2dp_hw_dump_ctrl_event(tA2DP_CTRL_CMD event) {
    switch (event) {
        CASE_RETURN_STR(A2DP_CTRL_CMD_NONE)
        CASE_RETURN_STR(A2DP_CTRL_CMD_CHECK_READY)
        CASE_RETURN_STR(A2DP_CTRL_CMD_START)
        CASE_RETURN_STR(A2DP_CTRL_CMD_STOP)
        CASE_RETURN_STR(A2DP_CTRL_CMD_SUSPEND)
        CASE_RETURN_STR(A2DP_CTRL_GET_INPUT_AUDIO_CONFIG)
        CASE_RETURN_STR(A2DP_CTRL_GET_OUTPUT_AUDIO_CONFIG)
        CASE_RETURN_STR(A2DP_CTRL_SET_OUTPUT_AUDIO_CONFIG)
        CASE_RETURN_STR(A2DP_CTRL_CMD_OFFLOAD_START)
        CASE_RETURN_STR(A2DP_CTRL_GET_PRESENTATION_POSITION)
    }
    return "UNKNOWN A2DP_CTRL_CMD";
}

static int calc_audiotime_usec(int bytes, unsigned int rate,
        audio_channel_mask_t channel_mask, audio_format_t format) {
    int chan_count = audio_channel_count_from_out_mask(channel_mask);
    int bytes_per_sample;

    switch (format) {
        case AUDIO_FORMAT_PCM_8_BIT:
            bytes_per_sample = 1;
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
            bytes_per_sample = 2;
            break;
        case AUDIO_FORMAT_PCM_24_BIT_PACKED:
            bytes_per_sample = 3;
            break;
        case AUDIO_FORMAT_PCM_8_24_BIT:
            bytes_per_sample = 4;
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            bytes_per_sample = 4;
            break;
        default:
            ALOGE("unsupported sample format %d", format);
            bytes_per_sample = 2;
            break;
        }
    return (int)(((int64_t)bytes * (USEC_PER_SEC / (chan_count * bytes_per_sample))) / rate);
}


/**
 * connect to peer named "name" on fd
 * returns same fd or -1 on error.
 * fd is not closed on error. that's your job.
 *
 * Used by AndroidSocketImpl
 */
int osi_socket_local_client_connect(int fd, const char* name) {
    struct sockaddr_un addr;
    socklen_t alen;
    int err;
    size_t namelen = strlen(name);

    memset(&addr, 0, sizeof(struct sockaddr_un));
    if ((namelen + 1) > sizeof(addr.sun_path)) {
        return -1;
    }
    addr.sun_path[0] = 0;
    memcpy(addr.sun_path + 1, name, namelen);
    addr.sun_family = AF_LOCAL;
    alen = namelen + offsetof(struct sockaddr_un, sun_path) + 1;

    OSI_NO_INTR(err = connect(fd, (struct sockaddr*)&addr, alen));
    if (err < 0) {
        return -1;
    }
    return fd;
}

/*****************************************************************************
 *
 *   bluedroid stack adaptation
 *
 ****************************************************************************/
static int skt_connect(const char* path, size_t buffer_sz) {
    int ret;
    int skt_fd;
    int len;

    ALOGD("connect to %s (sz %zu)", path, buffer_sz);
    skt_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (osi_socket_local_client_connect(skt_fd, path) < 0) {
        ALOGE("failed to connect (%s)", strerror(errno));
        close(skt_fd);
        return -1;
    }

    len = buffer_sz;
    ret = setsockopt(skt_fd, SOL_SOCKET, SO_SNDBUF, (char*)&len, (int)sizeof(len));
    if (ret < 0)
        ALOGE("setsockopt failed (%s)", strerror(errno));

    ret = setsockopt(skt_fd, SOL_SOCKET, SO_RCVBUF, (char*)&len, (int)sizeof(len));
    if (ret < 0)
        ALOGE("setsockopt failed (%s)", strerror(errno));

    /* Socket send/receive timeout value */
    struct timeval tv;
    tv.tv_sec = SOCK_SEND_TIMEOUT_MS / 1000;
    tv.tv_usec = (SOCK_SEND_TIMEOUT_MS % 1000) * 1000;
    ret = setsockopt(skt_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (ret < 0)
        ALOGE("setsockopt failed (%s)", strerror(errno));

    tv.tv_sec = SOCK_RECV_TIMEOUT_MS / 1000;
    tv.tv_usec = (SOCK_RECV_TIMEOUT_MS % 1000) * 1000;
    ret = setsockopt(skt_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret < 0)
        ALOGE("setsockopt failed (%s)", strerror(errno));

    ALOGD("connected to stack fd = %d", skt_fd);
    return skt_fd;
}

static int skt_disconnect(int fd) {
    ALOGD("skt_disconnect fd %d", fd);
    if (fd != AUDIO_SKT_DISCONNECTED) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    return 0;
}

static int skt_write(struct audio_stream_out* stream, int fd, const void* p, size_t len) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct aml_audio_device *aml_dev = aml_out->dev;
    ssize_t sent;

    if (WRITE_POLL_MS == 0) {
        // do not poll, use blocking send
        OSI_NO_INTR(sent = send(fd, p, len, MSG_NOSIGNAL));
        if (sent == -1)
            ALOGE("write failed with error(%s)", strerror(errno));
        return (int)sent;
    }

    // use non-blocking send, poll
    int ms_timeout = SOCK_SEND_TIMEOUT_MS;
    size_t count = 0;
    while (count < len) {
        OSI_NO_INTR(sent = send(fd, p, len - count, MSG_NOSIGNAL | MSG_DONTWAIT));
        if (sent == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                ALOGE("write failed with error(%s)", strerror(errno));
                return -1;
            }
            if (ms_timeout >= WRITE_POLL_MS) {
                usleep(WRITE_POLL_MS * 1000);
                ms_timeout -= WRITE_POLL_MS;
                continue;
            }
            ALOGW("write timeout exceeded, sent %zu bytes", count);
            return -1;
        }
        count += sent;
        p = (const uint8_t*)p + sent;
    }
    if ((aml_dev->is_TV) && (count != 0)) {
        uint64_t input_ns = 0;
        size_t frame_size;
        static uint64_t begin_ns = 0;
        uint64_t process_ns = 0;
        int mutex_lock_success = 0;

        if (pthread_mutex_unlock(&aml_dev->alsa_pcm_lock) == 0) {
            mutex_lock_success = 1;
        }
        if (out->is_stereo_to_mono)
            frame_size = 2; // mono pcm 16bit
        else
            frame_size = 4; // stereo pcm 16bit
        input_ns = (uint64_t)(count) * 1000000000LL / frame_size / out->rate;
        if (aml_dev->debug_flag) {
            if (total_input_ns == 0)
                begin_ns = aml_audio_get_systime_ns();
            process_ns = aml_audio_get_systime_ns() - begin_ns;
            ALOGD("skt_write: process_ns %lld input_ns %lld, diff: %lldms (%lld), cur write=%llu",
                process_ns, total_input_ns, (((int64_t)total_input_ns- process_ns)/1000000),
                ((int64_t)total_input_ns- process_ns), input_ns);
            total_input_ns += input_ns;
        }

        if (out->vir_buf_handle == NULL) {
            audio_virtual_buf_open(&out->vir_buf_handle, "a2dp", VIR_BUFF_NS, VIR_BUFF_NS, 0);
            audio_virtual_buf_process(out->vir_buf_handle, VIR_BUFF_NS - input_ns/2);
        }
        audio_virtual_buf_process(out->vir_buf_handle, input_ns);
        if (mutex_lock_success) {
            pthread_mutex_lock(&aml_dev->alsa_pcm_lock);
        }
    }

    return (int)count;
}

static int a2dp_ctrl_receive(struct a2dp_stream_out* out, void* buffer, size_t length) {
    ssize_t ret;
    int i;

    for (i = 0;; i++) {
        OSI_NO_INTR(ret = recv(out->ctrl_fd, buffer, length, MSG_NOSIGNAL));
        if (ret > 0)
            break;
        if (ret == 0) {
            ALOGE("receive control data failed: peer closed");
            break;
        }
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            ALOGE("receive control data failed: error(%s)", strerror(errno));
            break;
        }
        if (i == (CTRL_CHAN_RETRY_COUNT - 1)) {
            ALOGE("receive control data failed: max retry count");
            break;
        }
        ALOGD("receive control data failed (%s), retrying", strerror(errno));
    }
    if (ret <= 0) {
        skt_disconnect(out->ctrl_fd);
        out->ctrl_fd = AUDIO_SKT_DISCONNECTED;
    }
    return ret;
}

/* Sends control info for stream |out|. The data to send is stored in
 * |buffer| and has size |length|.
 * On success, returns the number of octets sent, otherwise -1.
 */
static int a2dp_ctrl_send(struct a2dp_stream_out* out, const void* buffer, size_t length) {
    ssize_t sent;
    size_t remaining = length;
    int i;

    if (length == 0)
        return 0;  // Nothing to do

    for (i = 0;; i++) {
        OSI_NO_INTR(sent = send(out->ctrl_fd, buffer, remaining, MSG_NOSIGNAL));
        if (sent == (ssize_t)(remaining)) {
            remaining = 0;
            break;
        }
        if (sent > 0) {
            buffer = ((const char*)(buffer) + sent);
            remaining -= sent;
            continue;
        }
        if (sent < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                ALOGE("send control data failed: error(%s)", strerror(errno));
                break;
            }
            ALOGD("send control data failed (%s), retrying", strerror(errno));
        }
        if (i >= (CTRL_CHAN_RETRY_COUNT - 1)) {
            ALOGE("send control data failed: max retry count");
            break;
        }
    }
    if (remaining > 0) {
        skt_disconnect(out->ctrl_fd);
        out->ctrl_fd = AUDIO_SKT_DISCONNECTED;
        return -1;
    }
    return length;
}

static int a2dp_command(struct a2dp_stream_out* out, tA2DP_CTRL_CMD cmd) {
    char ack;

    if (out->ctrl_fd == AUDIO_SKT_DISCONNECTED) {
        ALOGD("starting up or recovering from previous error: command=%s",
                audio_a2dp_hw_dump_ctrl_event(cmd));
        a2dp_open_ctrl_path(out);
        if (out->ctrl_fd == AUDIO_SKT_DISCONNECTED) {
            ALOGE("failure to open ctrl path: command=%s",
            audio_a2dp_hw_dump_ctrl_event(cmd));
            return -1;
        }
    }

    /* send command */
    ssize_t sent;
    OSI_NO_INTR(sent = send(out->ctrl_fd, &cmd, 1, MSG_NOSIGNAL));
    if (sent == -1) {
        ALOGE("cmd failed (%s): command=%s", strerror(errno),
                audio_a2dp_hw_dump_ctrl_event(cmd));
        skt_disconnect(out->ctrl_fd);
        out->ctrl_fd = AUDIO_SKT_DISCONNECTED;
        return -1;
    }

    /* wait for ack byte */
    if (a2dp_ctrl_receive(out, &ack, 1) < 0) {
        ALOGE("A2DP COMMAND %s: no ACK", audio_a2dp_hw_dump_ctrl_event(cmd));
        return -1;
    }

    if (ack == A2DP_CTRL_ACK_INCALL_FAILURE) {
        ALOGE("A2DP COMMAND %s error %d", audio_a2dp_hw_dump_ctrl_event(cmd), ack);
        return ack;
    }
    if (ack != A2DP_CTRL_ACK_SUCCESS) {
        ALOGE("A2DP COMMAND %s error %d", audio_a2dp_hw_dump_ctrl_event(cmd), ack);
        return -1;
    }
    return A2DP_CTRL_ACK_SUCCESS;
}

static int check_a2dp_ready(struct a2dp_stream_out* out) {
    if (a2dp_command(out, A2DP_CTRL_CMD_CHECK_READY) < 0) {
        ALOGE("check a2dp ready failed");
        return -1;
    }
    return 0;
}

static void a2dp_open_ctrl_path(struct a2dp_stream_out* out) {
    int i;

    if (out->ctrl_fd != AUDIO_SKT_DISCONNECTED)
        return;  // already connected

    /* retry logic to catch any timing variations on control channel */
    for (i = 0; i < CTRL_CHAN_RETRY_COUNT; i++) {
        /* connect control channel if not already connected */
        if ((out->ctrl_fd = skt_connect(
                A2DP_CTRL_PATH, AUDIO_STREAM_CONTROL_OUTPUT_BUFFER_SZ)) >= 0) {
            /* success, now check if stack is ready */
            if (check_a2dp_ready(out) == 0)
                break;
            ALOGE("error : a2dp not ready, wait 250 ms and retry");
            usleep(250000);
            skt_disconnect(out->ctrl_fd);
            out->ctrl_fd = AUDIO_SKT_DISCONNECTED;
        }
        /* ctrl channel not ready, wait a bit */
        usleep(250000);
    }
}

size_t a2dp_hw_buffer_size(btav_a2dp_codec_config_t * config) {
    size_t buffer_sz = AUDIO_STREAM_OUTPUT_BUFFER_SZ;  // Default value
    const uint64_t time_period_ms = 40;                // Conservative 20ms
    uint32_t sample_rate;
    uint32_t bytes_per_sample;
    uint32_t number_of_channels;

    // Check the codec config sample rate
    switch (config->sample_rate) {
        case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
            sample_rate = 44100;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
          sample_rate = 48000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_88200:
            sample_rate = 88200;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_96000:
            sample_rate = 96000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_176400:
            sample_rate = 176400;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_192000:
            sample_rate = 192000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_NONE:
        default:
            ALOGE("Invalid sample rate: 0x%x", config->sample_rate);
            return buffer_sz;
    }

    // Check the codec config bits per sample
    switch (config->bits_per_sample) {
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
            bytes_per_sample = 2;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
            bytes_per_sample = 3;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
            bytes_per_sample = 4;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE:
        default:
            ALOGE("Invalid bits per sample: 0x%x", config->bits_per_sample);
            return buffer_sz;
    }

    // Check the codec config channel mode
    switch (config->channel_mode) {
        case BTAV_A2DP_CODEC_CHANNEL_MODE_MONO:
            number_of_channels = 1;
            break;
        case BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO:
            number_of_channels = 2;
            break;
        case BTAV_A2DP_CODEC_CHANNEL_MODE_NONE:
        default:
            ALOGE("Invalid channel mode: 0x%x", config->channel_mode);
            return buffer_sz;
    }

    /*
    * The buffer size is computed by using the following formula:
    *
    * AUDIO_STREAM_OUTPUT_BUFFER_SIZE =
    *    (TIME_PERIOD_MS * AUDIO_STREAM_OUTPUT_BUFFER_PERIODS *
    *     SAMPLE_RATE_HZ * NUMBER_OF_CHANNELS * (BITS_PER_SAMPLE / 8)) / 1000
    *
    * AUDIO_STREAM_OUTPUT_BUFFER_PERIODS controls how the socket buffer is
    * divided for AudioFlinger data delivery. The AudioFlinger mixer delivers
    * data in chunks of
    * (AUDIO_STREAM_OUTPUT_BUFFER_SIZE / AUDIO_STREAM_OUTPUT_BUFFER_PERIODS) .
    * If the number of periods is 2, the socket buffer represents "double
    * buffering" of the AudioFlinger mixer buffer.
    *
    * Furthermore, the AudioFlinger expects the buffer size to be a multiple
    * of 16 frames.
    */
    const size_t divisor = (AUDIO_STREAM_OUTPUT_BUFFER_PERIODS * 16 *
                number_of_channels * bytes_per_sample);
    buffer_sz = (time_period_ms * AUDIO_STREAM_OUTPUT_BUFFER_PERIODS *
            sample_rate * number_of_channels * bytes_per_sample) / 1000;
    // Adjust the buffer size so it can be divided by the divisor
    const size_t remainder = buffer_sz % divisor;
    if (remainder != 0) {
        buffer_sz += divisor - remainder;
    }
    return buffer_sz;
}

static int a2dp_get_output_audio_config(
        struct a2dp_stream_out* out,
        btav_a2dp_codec_config_t* codec_config,
        btav_a2dp_codec_config_t* codec_capability) {
    if (a2dp_command(out, A2DP_CTRL_GET_OUTPUT_AUDIO_CONFIG) < 0) {
        ALOGE("get a2dp output audio config failed");
        return -1;
    }

    // Receive the current codec config
    if (a2dp_ctrl_receive(out, &codec_config->sample_rate,
            sizeof(btav_a2dp_codec_sample_rate_t)) < 0) {
        return -1;
    }
    if (a2dp_ctrl_receive(out, &codec_config->bits_per_sample,
            sizeof(btav_a2dp_codec_bits_per_sample_t)) < 0) {
        return -1;
    }
    if (a2dp_ctrl_receive(out, &codec_config->channel_mode,
            sizeof(btav_a2dp_codec_channel_mode_t)) < 0) {
        return -1;
    }

    // Receive the current codec capability
    if (a2dp_ctrl_receive(out, &codec_capability->sample_rate,
            sizeof(btav_a2dp_codec_sample_rate_t)) < 0) {
        return -1;
    }
    if (a2dp_ctrl_receive(out, &codec_capability->bits_per_sample,
            sizeof(btav_a2dp_codec_bits_per_sample_t)) < 0) {
        return -1;
    }
    if (a2dp_ctrl_receive(out, &codec_capability->channel_mode,
            sizeof(btav_a2dp_codec_channel_mode_t)) < 0) {
        return -1;
    }
    return 0;
}

static int a2dp_read_output_audio_config(struct audio_stream_out* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    btav_a2dp_codec_config_t codec_config;
    btav_a2dp_codec_config_t codec_capability;
    bool is_stereo_to_mono;
    unsigned int rate;
    audio_format_t format;
    audio_channel_mask_t channel_mask;

    if (a2dp_get_output_audio_config(out, &codec_config, &codec_capability) < 0)
        return -1;
    // Check the codec config sample rate
    switch (codec_config.sample_rate) {
        case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
            rate = 44100;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
            rate = 48000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_88200:
            rate = 88200;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_96000:
            rate = 96000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_176400:
            rate = 176400;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_192000:
            rate = 192000;
            break;
        case BTAV_A2DP_CODEC_SAMPLE_RATE_NONE:
        default:
            ALOGE("Invalid sample rate: 0x%x", codec_config.sample_rate);
            return -1;
    }

    // Check the codec config bits per sample
    switch (codec_config.bits_per_sample) {
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
            format = AUDIO_FORMAT_PCM_16_BIT;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
            format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
            format = AUDIO_FORMAT_PCM_32_BIT;
            break;
        case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE:
        default:
            ALOGE("Invalid bits per sample: 0x%x", codec_config.bits_per_sample);
            return -1;
    }

    // Check the codec config channel mode
    switch (codec_config.channel_mode) {
        case BTAV_A2DP_CODEC_CHANNEL_MODE_MONO:
            channel_mask = AUDIO_CHANNEL_OUT_MONO;
            is_stereo_to_mono = true;
            break;
        case BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO:
            channel_mask = AUDIO_CHANNEL_OUT_STEREO;
            is_stereo_to_mono = false;
            break;
        case BTAV_A2DP_CODEC_CHANNEL_MODE_NONE:
        default:
            ALOGE("Invalid channel mode: 0x%x", codec_config.channel_mode);
            return -1;
    }
    if (is_stereo_to_mono) {
        channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    }

    out->rate = rate;
    out->is_stereo_to_mono = is_stereo_to_mono;
    out->buffer_sz = a2dp_hw_buffer_size(&codec_config);
    if (is_stereo_to_mono) {
        // We need to fetch twice as much data from the Audio framework
        out->buffer_sz *= 2;
    }

    ALOGD("got output codec config: sample_rate=0x%x bits_per_sample=0x%x channel_mode=0x%x",
        codec_config.sample_rate, codec_config.bits_per_sample, codec_config.channel_mode);
    ALOGD("got output codec capability: sample_rate=0x%x bits_per_sample=0x%x channel_mode=0x%x",
        codec_capability.sample_rate, codec_capability.bits_per_sample, codec_capability.channel_mode);
    return 0;
}

static int a2dp_write_output_audio_config(struct audio_stream_out* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    btav_a2dp_codec_config_t config;

    if (a2dp_command(out, A2DP_CTRL_SET_OUTPUT_AUDIO_CONFIG) < 0) {
        ALOGE("set a2dp output audio config failed");
        return -1;
    }

    switch (aml_out->hal_rate) {
        case 44100:
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
            break;
        case 48000:
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
            break;
        case 88200:
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_88200;
            break;
        case 96000:
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
            break;
        case 176400:
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_176400;
            break;
        case 192000:
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
            break;
        default:
            ALOGE("Invalid sample rate: %" PRIu32, aml_out->hal_rate);
            config.sample_rate = BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
            //return -1;
    }

    switch (aml_out->hal_format) {
        case AUDIO_FORMAT_PCM_16_BIT:
            config.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            break;
        case AUDIO_FORMAT_PCM_24_BIT_PACKED:
            config.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            config.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32;
            break;
        case AUDIO_FORMAT_PCM_8_24_BIT:
        // FALLTHROUGH
        // All 24-bit audio is expected in AUDIO_FORMAT_PCM_24_BIT_PACKED format
        default:
            ALOGE("Invalid audio format: 0x%x", aml_out->hal_format);
            config.bits_per_sample = BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
            //return -1;
    }

    switch (aml_out->hal_channel_mask) {
        case AUDIO_CHANNEL_OUT_MONO:
            config.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_MONO;
            break;
        case AUDIO_CHANNEL_OUT_STEREO:
            if (out->is_stereo_to_mono) {
                config.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_MONO;
            } else {
                config.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
            }
            break;
        default:
            ALOGE("Invalid channel mask: 0x%x", aml_out->hal_channel_mask);
            config.channel_mode = BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
            //return -1;
    }

    // Send the current codec config that has been selected by us
    if (a2dp_ctrl_send(out, &config.sample_rate,
            sizeof(btav_a2dp_codec_sample_rate_t)) < 0)
        return -1;
    if (a2dp_ctrl_send(out, &config.bits_per_sample,
            sizeof(btav_a2dp_codec_bits_per_sample_t)) < 0)
        return -1;
    if (a2dp_ctrl_send(out, &config.channel_mode,
            sizeof(btav_a2dp_codec_channel_mode_t)) < 0)
        return -1;

    ALOGD("sent output codec config: sample_rate=0x%x "
        "bits_per_sample=0x%x channel_mode=0x%x",
        config.sample_rate, config.bits_per_sample,
        config.channel_mode);
    return 0;
}

static int a2dp_get_presentation_position_cmd(struct a2dp_stream_out* out,
        uint64_t* bytes, uint16_t* delay, struct timespec* timestamp) {
    if ((out->ctrl_fd == AUDIO_SKT_DISCONNECTED) ||
            (out->state != AUDIO_A2DP_STATE_STARTED))  // Audio is not streaming
        return -1;
    if (a2dp_command(out, A2DP_CTRL_GET_PRESENTATION_POSITION) < 0)
        return -1;
    if (a2dp_ctrl_receive(out, bytes, sizeof(*bytes)) < 0)
        return -1;
    if (a2dp_ctrl_receive(out, delay, sizeof(*delay)) < 0)
        return -1;

    uint32_t seconds;
    if (a2dp_ctrl_receive(out, &seconds, sizeof(seconds)) < 0)
        return -1;

    uint32_t nsec;
    if (a2dp_ctrl_receive(out, &nsec, sizeof(nsec)) < 0)
        return -1;

    timestamp->tv_sec = seconds;
    timestamp->tv_nsec = nsec;
    return 0;
}

static void a2dp_stream_common_init(struct a2dp_stream_out* out) {
    out->ctrl_fd = AUDIO_SKT_DISCONNECTED;
    out->audio_fd = AUDIO_SKT_DISCONNECTED;
    out->state = AUDIO_A2DP_STATE_STOPPED;
    /* manages max capacity of socket pipe */
    out->buffer_sz = AUDIO_STREAM_OUTPUT_BUFFER_SZ;
    out->rate = 44100;
    out->vir_buf_handle = NULL;
}

static void a2dp_stream_common_destroy(struct a2dp_stream_out* out) {
    if (out->ctrl_fd != AUDIO_SKT_DISCONNECTED) {
        skt_disconnect(out->ctrl_fd);
        out->ctrl_fd = AUDIO_SKT_DISCONNECTED;
    }
    if (out->audio_fd != AUDIO_SKT_DISCONNECTED) {
        skt_disconnect(out->audio_fd);
        out->audio_fd = AUDIO_SKT_DISCONNECTED;
    }
}

static int start_audio_datapath(struct audio_stream_out* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct aml_audio_device *adev = aml_out->dev;
    int oldstate = out->state;

    ALOGD("start_audio_datapath state %d (%p), flags=%x", out->state, aml_out, aml_out->flags);
    out->state = AUDIO_A2DP_STATE_STARTING;
    adev->debug_flag = aml_audio_get_debug_flag();

    int a2dp_status = a2dp_command(out, A2DP_CTRL_CMD_START);
    if (a2dp_status < 0) {
        ALOGE("Audiopath start failed (status %d)", a2dp_status);
        a2dp_command(out, A2DP_CTRL_CMD_SUSPEND);
        goto error;
    } else if (a2dp_status == A2DP_CTRL_ACK_INCALL_FAILURE) {
        ALOGE("Audiopath start failed - in call, move to suspended");
        goto error;
    }

    /* connect socket if not yet connected */
    if (out->audio_fd == AUDIO_SKT_DISCONNECTED) {
        out->audio_fd = skt_connect(A2DP_DATA_PATH, out->buffer_sz);
        if (out->audio_fd < 0) {
            ALOGE("Audiopath start failed - error opening data socket");
            goto error;
        }
    }
    out->state = (a2dp_state_t)AUDIO_A2DP_STATE_STARTED;
    /* check to see if delay reporting is enabled */
    out->enable_delay_reporting = !property_get_bool("persist.bluetooth.disabledelayreports", false);
    if (aml_out->hal_rate != out->rate) {
        audio_resample_config_t stResamplerConfig;
        stResamplerConfig.aformat   = AUDIO_FORMAT_PCM_16_BIT;
        stResamplerConfig.channels  = 2;
        stResamplerConfig.input_sr  = aml_out->hal_rate;
        stResamplerConfig.output_sr = aml_out->a2dp_out->rate;
        int ret = aml_audio_resample_init(&out->pstResampler, AML_AUDIO_SIMPLE_RESAMPLE, &stResamplerConfig);
        if (ret < 0) {
            ALOGE("[%s:%d] Resampler is failed initialization !!!", __func__, __LINE__);
            return -1;
        }
    }
    total_input_ns = 0;
    return 0;

error:
    out->state = (a2dp_state_t)oldstate;
    return -1;
}

static int stop_audio_datapath(struct audio_stream* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct aml_audio_device *adev = aml_out->dev;
    int oldstate = out->state;

    ALOGD("stop_audio_datapath state %d (%p)", out->state, out);

    /* prevent any stray output writes from autostarting the stream
    * while stopping audiopath
    */
    out->state = AUDIO_A2DP_STATE_STOPPING;

    if (adev->a2dp_connected && (a2dp_command(out, A2DP_CTRL_CMD_STOP) < 0)) {
        ALOGE("audiopath stop failed");
        out->state = (a2dp_state_t)oldstate;
        return -1;
    }

    out->state = (a2dp_state_t)AUDIO_A2DP_STATE_STOPPED;

    /* disconnect audio path */
    skt_disconnect(out->audio_fd);
    out->audio_fd = AUDIO_SKT_DISCONNECTED;

    if (out->pstResampler) {
        aml_audio_resample_close(out->pstResampler);
        out->pstResampler = NULL;
    }
    return 0;
}

int suspend_audio_datapath(struct audio_stream* stream, bool standby) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct aml_audio_device *adev = aml_out->dev;

    ALOGD("suspend_audio_datapath state %d", out->state);
    if (out->state == AUDIO_A2DP_STATE_STOPPING)
        return -1;
    if (adev->a2dp_connected && (a2dp_command(out, A2DP_CTRL_CMD_SUSPEND) < 0))
        return -1;
    if (standby)
        out->state = AUDIO_A2DP_STATE_STANDBY;
    else
        out->state = AUDIO_A2DP_STATE_SUSPENDED;

    /* disconnect audio path */
    skt_disconnect(out->audio_fd);
    out->audio_fd = AUDIO_SKT_DISCONNECTED;
    return 0;
}

static size_t a2dp_out_get_buffer_size(const struct audio_stream* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    // the AudioFlinger mixer buffer size.
    return out->buffer_sz / AUDIO_STREAM_OUTPUT_BUFFER_PERIODS;
}

int a2dp_out_set_parameters(struct audio_stream* stream, const char* kvpairs) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct str_parms *parms;
    char value[32];
    int ret;

    ALOGI("a2dp_out_set_parameters %s (%p)\n", kvpairs, aml_out);
    parms = str_parms_create_str (kvpairs);
    ret = str_parms_get_str (parms, "closing", value, sizeof (value) );
    if (ret >= 0) {
        pthread_mutex_lock(&out->mutex);
        if (strncmp(value, "true", 4) == 0)
            out->state = AUDIO_A2DP_STATE_STOPPING;
        pthread_mutex_unlock(&out->mutex);
    }
    ret = str_parms_get_str (parms, "A2dpSuspended", value, sizeof (value) );
    if (ret >= 0) {
        pthread_mutex_lock(&out->mutex);
        if (strncmp(value, "true", 4) == 0) {
            if (out->state == AUDIO_A2DP_STATE_STARTED)
                suspend_audio_datapath(stream, false);
        } else {
            if (out->state == AUDIO_A2DP_STATE_SUSPENDED)
                out->state = AUDIO_A2DP_STATE_STANDBY;
        }
        pthread_mutex_unlock(&out->mutex);
    }
    return 0;
}
static char* a2dp_out_get_parameters(const struct audio_stream* stream, const char* keys) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    char cap[1024];
    int size = 0;
    btav_a2dp_codec_config_t codec_config;
    btav_a2dp_codec_config_t codec_capability;

    ALOGI("a2dp_out_get_parameters %s,out %p\n", keys, aml_out);
    if (a2dp_get_output_audio_config(out, &codec_config, &codec_capability) < 0) {
        ALOGE("a2dp_out_get_parameters: keys=%s, a2dp_get_output_audio_config error", keys);
        return strdup("");
    }
    memset(cap, 0, 1024);
    if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
        bool first = true;
        size += sprintf(cap + size, "%s=", AUDIO_PARAMETER_STREAM_SUP_FORMATS);
        if (codec_capability.bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "AUDIO_FORMAT_PCM_16_BIT");
        }
        if (codec_capability.bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "AUDIO_FORMAT_PCM_24_BIT_PACKED");
        }
        if (codec_capability.bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "AUDIO_FORMAT_PCM_32_BIT");
        }
    }else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
        bool first = true;
        size += sprintf(cap + size, "%s=", AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES);
        if (codec_capability.sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_44100) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "44100");
        }
        if (codec_capability.sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_48000) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "48000");
        }
        if (codec_capability.sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_88200) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "88200");
        }
        if (codec_capability.sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_96000) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "96000");
        }
        if (codec_capability.sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_176400) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "176400");
        }
        if (codec_capability.sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_192000) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "192000");
        }
    } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
        bool first = true;
        size += sprintf(cap + size, "%s=", AUDIO_PARAMETER_STREAM_SUP_CHANNELS);
        if (codec_capability.channel_mode & BTAV_A2DP_CODEC_CHANNEL_MODE_MONO) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "AUDIO_CHANNEL_OUT_MONO");
        }
        if (codec_capability.channel_mode & BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO) {
            if (!first)
                size += sprintf(cap + size, "%s", "|");
            else
                first = false;
            size += sprintf(cap + size, "%s", "AUDIO_CHANNEL_OUT_STEREO");
        }
    }
    return strdup(cap);
}

uint32_t a2dp_out_get_latency(const struct audio_stream_out* stream) {
    (void *)stream;
    return 200;
}


int a2dp_out_get_presentation_position(const struct audio_stream_out* stream,
        uint64_t* frames, struct timespec* timestamp) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;

    if (stream == NULL || frames == NULL || timestamp == NULL)
        return -EINVAL;

    // bytes is the total number of bytes sent by the Bluetooth stack to a
    // remote headset
    uint64_t bytes = 0;
    // delay_report is the audio delay from the remote headset receiving data to
    // the headset playing sound in units of 1/10ms
    uint16_t delay_report = 0;

    // If for some reason getting a delay fails or delay reports are disabled,
    // default to old delay
    pthread_mutex_lock(&out->mutex);
    if (out->enable_delay_reporting &&
        a2dp_get_presentation_position_cmd(out, &bytes, &delay_report, timestamp) == 0) {
        uint64_t delay_ns = delay_report * DELAY_TO_NS;
        if (delay_ns > MIN_DELAY_NS && delay_ns < MAX_DELAY_NS) {
            *frames = bytes / audio_stream_out_frame_size(stream);
            timestamp->tv_nsec += delay_ns;
            if (timestamp->tv_nsec > 1 * SEC_TO_NS) {
                timestamp->tv_sec++;
                timestamp->tv_nsec -= SEC_TO_NS;
            }
            pthread_mutex_unlock(&out->mutex);
            return 0;
        }
    }

    uint64_t latency_frames = (uint64_t)a2dp_out_get_latency(stream) * aml_out->hal_rate / SEC_TO_MS;
    if (out->frames_presented >= latency_frames) {
        clock_gettime(CLOCK_MONOTONIC, timestamp);
        *frames = out->frames_presented - latency_frames;
        pthread_mutex_unlock(&out->mutex);
        return 0;
    }
    pthread_mutex_unlock(&out->mutex);
    return -EWOULDBLOCK;
}

static int a2dp_out_get_render_position(const struct audio_stream_out* stream,
                                   uint32_t* dsp_frames) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;

    if (stream == NULL || dsp_frames == NULL)
        return -EINVAL;

    pthread_mutex_lock(&out->mutex);
    uint64_t latency_frames = (uint64_t)a2dp_out_get_latency(stream) * aml_out->hal_rate / SEC_TO_MS;
    if (out->frames_rendered >= latency_frames) {
        *dsp_frames = (uint32_t)(out->frames_rendered - latency_frames);
    } else {
        *dsp_frames = 0;
    }
    pthread_mutex_unlock(&out->mutex);
    return 0;
}
#if 0
static int a2dp_write_hwsync(struct audio_stream_out* stream, const void* buffer, size_t bytes,
            const void* out_buffer, size_t* out_size) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct aml_audio_device *adev = aml_out->dev;
    audio_hwsync_t *hw_sync = aml_out->hwsync;
    int return_bytes = bytes;
    uint64_t cur_pts = 0xffffffff;
    int outsize = 0;

    if (aml_out->hw_sync_mode == 0) {
        out_buffer = buffer;
        *out_size = bytes;
        return bytes;
    }

    if (adev->debug_flag)
        ALOGD("a2dp_write_hwsync bytes=%zu\n", bytes);
    pthread_mutex_lock (&adev->lock);
    pthread_mutex_lock (&out->mutex);
    return_bytes = aml_audio_hwsync_find_frame(hw_sync, (char *)buffer, bytes, &cur_pts, &outsize);
    if (cur_pts > 0xffffffff)
            ALOGE ("APTS exeed the max 32bit value");
    if (adev->debug_flag)
        ALOGI ("after aml_audio_hwsync_find_frame bytes remain %zu,cost %zu,outsize %d,pts %"PRId64"ms\n",
                bytes - return_bytes, return_bytes, outsize, cur_pts / 90);

    if (cur_pts != 0xffffffff && outsize > 0) {
        //TODO,skip 3 frames after flush, to tmp fix seek pts discontinue issue.need dig more
        // to find out why seek ppint pts frame is remained after flush.WTF.
        if (aml_out->skip_frame > 0) {
            aml_out->skip_frame--;
            ALOGI ("skip pts@%"PRIx64",cur frame size %d,cost size %zu\n", cur_pts, outsize, return_bytes);
            pthread_mutex_unlock (&out->mutex);
            pthread_mutex_unlock (&adev->lock);
            return return_bytes;
        }
        *out_size = outsize;
        out_buffer = hw_sync->hw_sync_body_buf;

        // if we got the frame body,which means we get a complete frame.
        //we take this frame pts as the first apts.
        //this can fix the seek discontinue,we got a fake frame,which maybe cached before the seek
        if (hw_sync->first_apts_flag == false) {
            aml_audio_hwsync_set_first_pts(hw_sync, cur_pts);
        } else {
            uint64_t apts;
            uint32_t apts32;
            uint pcr = 0;
            uint apts_gap = 0;
            uint64_t latency = a2dp_out_get_latency(stream) * 90;
            // check PTS discontinue, which may happen when audio track switching
            // discontinue means PTS calculated based on first_apts and frame_write_sum
            // does not match the timestamp of next audio samples
            if (cur_pts > latency)
                apts = cur_pts - latency;
            else
                apts = 0;
            apts32 = apts & 0xffffffff;
            if (get_sysfs_uint (TSYNC_PCRSCR, &pcr) == 0) {
                enum hwsync_status sync_status = CONTINUATION;
                apts_gap = get_pts_gap (pcr, apts32);
                sync_status = check_hwsync_status (apts_gap);
                ALOGD("%s()audio pts %dms, pcr %dms, latency %lldms, diff %dms, sync_status=%d",
                        __func__, apts32/90, pcr/90, latency/90,
                        (apts32 > pcr) ? (apts32 - pcr)/90 : (pcr - apts32)/90, sync_status);
                // limit the gap handle to 0.5~5 s.
                if (sync_status == ADJUSTMENT) {
                    // two cases: apts leading or pcr leading
                    // apts leading needs inserting frame and pcr leading neads discarding frame
                    if (apts32 > pcr) {
                        size_t once_write_size = 0;
                        int sent = -1;
                        char insert_buf[1024];
                        int insert_size = apts_gap / 90 * 44 * 4;
                        insert_size = insert_size & (~63);
                        ALOGI ("audio gap 0x%"PRIx32" ms ,need insert data %d\n", apts_gap / 90, insert_size);
                        memset(&insert_buf, 0, 1024);
                        while (insert_size > 0) {
                            once_write_size = insert_size > 1024 ? 1024 : insert_size;
                            sent = skt_write(out->audio_fd, &insert_buf, once_write_size);
                            insert_size -= once_write_size;
                            if (sent < 0) {
                                pthread_mutex_unlock (&out->mutex);
                                pthread_mutex_unlock (&adev->lock);
                                return -1;
                            }
                        }
                    } else {
                        //audio pts smaller than pcr,need skip frame.
                        ALOGI ("audio slow 0x%x,skip frame @pts 0x%"PRIx64",pcr 0x%x,cur apts 0x%x\n",
                                apts_gap, cur_pts, pcr, apts32);
                        aml_out->frame_skip_sum += 1764; // 40ms*44100/SEC_TO_MS
                        pthread_mutex_unlock (&out->mutex);
                        pthread_mutex_unlock (&adev->lock);
                        return return_bytes;
                    }
                } else if (sync_status == RESYNC) {
                    char tempbuf[32];
                    sprintf (tempbuf, "0x%x", apts32);
                    ALOGI ("tsync -> reset pcrscr 0x%x -> 0x%x, %s big,diff %"PRIx64" ms",
                            pcr, apts32, apts32 > pcr ? "apts" : "pcr", get_pts_gap (apts, pcr) / 90);
                    int ret_val = sysfs_set_sysfs_str (TSYNC_APTS, tempbuf);
                    if (ret_val == -1) {
                        ALOGE ("unable to open file %s,err: %s", TSYNC_APTS, strerror (errno) );
                    }
                }
            }
        }
    }
    pthread_mutex_unlock (&out->mutex);
    pthread_mutex_unlock (&adev->lock);

    return return_bytes;
}
#endif
ssize_t a2dp_out_write(struct audio_stream_out* stream, const void* buffer, size_t bytes) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    struct aml_audio_device *adev = aml_out->dev;
    struct aml_audio_patch *patch = adev->audio_patch;
    char* data_buffer = (char *)buffer;
    int sent = -1;
    const void *out_buffer;
    size_t out_size = 0;
    int frame_size = 4;
    size_t in_frames = bytes / frame_size;
    int i;
#if defined(AUDIO_EFFECT_EXTERN_DEVICE)
    int16_t *outbuff;
    int32_t *outbuff1;
    float tmp = 0;
    int chan_num = audio_channel_count_from_out_mask(aml_out->hal_channel_mask);
#endif

    if (out == NULL)
        return bytes;
    if (aml_out->pause_status)
        return bytes;
    if (!adev->a2dp_connected)
        return bytes;
    if (adev->audio_patch && out->state == AUDIO_A2DP_STATE_STARTED) {
        uint64_t cur_time = aml_audio_get_systime();
        if (cur_time - out->last_write_time > 64000) {
            ALOGD("%s:%d, for DTV/HDMIIN, input may be has gap: %lld", __func__, __LINE__, cur_time - out->last_write_time);
            a2dp_out_standby(&stream->common);
            return bytes;
         }
    }

    pthread_mutex_lock(&out->mutex);
    if (aml_out->is_tv_platform == 1) {
        in_frames = bytes/32; // 8ch pcm32
    }

    if (out->state == AUDIO_A2DP_STATE_SUSPENDED || out->state == AUDIO_A2DP_STATE_STOPPING) {
        ALOGD("stream %p suspended or closing", aml_out);
        goto finish;
    }

    /* only allow autostarting if we are in stopped or standby */
    if (out->state == AUDIO_A2DP_STATE_STOPPED || out->state == AUDIO_A2DP_STATE_STANDBY) {
        if (start_audio_datapath(stream) < 0) {
            ALOGE("stream %p start_audio_datapath fail", aml_out);
            goto finish;
        }
    } else if (out->state != AUDIO_A2DP_STATE_STARTED) {
        ALOGE("stream %p not in stopped or standby", aml_out);
        goto finish;
    }

    if (adev->debug_flag)
        ALOGD("a2dp_out_write: out(%p), write %zu bytes flags=%x, hwsync=%d",
                aml_out, bytes, aml_out->flags, aml_out->hw_sync_mode);

    /*dtv add drop ac3 pcm function*/
    if (adev->patch_src ==  SRC_DTV && aml_out->need_drop_size > 0) {
        if (aml_out->need_drop_size >= (int)bytes) {
            aml_out->need_drop_size -= bytes;
            ALOGI("a2dp av sync drop %d pcm, need drop:%d more,apts:0x%x,pcr:0x%x\n",
                (int)bytes, aml_out->need_drop_size, patch->last_apts, patch->last_pcrpts);
            if (patch->last_apts >= patch->last_pcrpts) {
                ALOGI("a2dp pts already ok, drop finish\n");
                aml_out->need_drop_size = 0;
            } else {
                sent = 0;
                in_frames = 0;
                goto finish;
            }
        } else {
            data_buffer += aml_out->need_drop_size;
            bytes -= aml_out->need_drop_size;
            aml_out->need_drop_size = 0;
            if (aml_out->is_tv_platform == 1) {
                in_frames = bytes/32; // 8ch pcm32
            } else {
                in_frames = bytes/frame_size;
            }
            ALOGI("a2dp drop finish bytes:%d, need_drop_size=%d\n", bytes, aml_out->need_drop_size);
        }
    }

    if (aml_out->is_tv_platform == 1) {
        int16_t *tmp_buffer = (int16_t *)data_buffer;
        int32_t *tmp_buffer_8ch = (int32_t *)data_buffer;
        for (int i=0; i<(int)in_frames; i++) {
            tmp_buffer[2*i] = (tmp_buffer_8ch[8*i]>>16);
            tmp_buffer[2*i+1] = (tmp_buffer_8ch[8*i+1]>>16);
        }
    }

    if (aml_out->hal_rate != out->rate) {
        if (out->pstResampler == NULL) {
            ALOGE("[%s:%d] Resampler is uninitialized !!!", __func__, __LINE__);
            return -1;
        }
        aml_audio_resample_process(out->pstResampler, data_buffer, in_frames * frame_size);
        memcpy(data_buffer, out->pstResampler->resample_buffer, out->pstResampler->resample_size);
        out_size = out->pstResampler->resample_size;
        in_frames = out->pstResampler->resample_size / frame_size;
    } else {
        out_size = in_frames*frame_size;
    }
    out_buffer = data_buffer;

    // Mix the stereo into mono if necessary
    if (out->is_stereo_to_mono) {
        int16_t* src = (int16_t*)out_buffer;
        int16_t* dst = (int16_t*)out_buffer;
        for (i = 0; i < (int)in_frames; i++, dst++, src += 2) {
            *dst = (int16_t)(((int32_t)src[0] + (int32_t)src[1]) >> 1);
        }
        out_size /= 2;
    }
    pthread_mutex_unlock(&out->mutex);
#if defined(AUDIO_EFFECT_EXTERN_DEVICE)
    if (aml_out->hal_format == AUDIO_FORMAT_PCM_16_BIT) {
        outbuff = (int16_t *)out_buffer;
        outbuff1 = NULL;
        if (chan_num == 2) {
            for (i = 0; i < out_size/2; i++) {
                if (i % 2 == 0) {
                    tmp = (float)(outbuff[i] * out->bt_gain * out->bt_unmute * out->left_gain);
                    outbuff[i] = (int16_t)tmp;
                } else {
                    tmp = (float)(outbuff[i] * out->bt_gain * out->bt_unmute * out->right_gain);
                    outbuff[i] = (int16_t)tmp;
                }
            }
        } else {
            for (i = 0; i < out_size/2; i++) {
                tmp = (float)(outbuff[i] * out->bt_gain * out->bt_unmute);
                outbuff[i] = (int16_t)tmp;
            }
        }
    }

    if (aml_out->hal_format == AUDIO_FORMAT_PCM_32_BIT) {
        outbuff1 = (int32_t *)out_buffer;
        outbuff = NULL;
        if (chan_num == 2) {
            for (i = 0; i < out_size/4; i++) {
                if (i % 2 == 0) {
                    tmp = (float)(outbuff1[i] * out->bt_gain * out->bt_unmute * out->left_gain);
                    outbuff1[i] = (int32_t)tmp;
                } else {
                    tmp = (float)(outbuff1[i] * out->bt_gain * out->bt_unmute * out->right_gain);
                    outbuff[i] = (int32_t)tmp;
                }
            }
        } else {
            for (i = 0; i < out_size/4; i++) {
                tmp = (float)(outbuff1[i] * out->bt_gain * out->bt_unmute);
                outbuff1[i] = (int32_t)tmp;
            }
        }
    }
#endif

    if (adev->patch_src == SRC_DTV && adev->parental_control_av_mute) {
        memset((void*)out_buffer,0x0,out_size);
    }

    //sent = skt_write(out->audio_fd, out_buffer, out_size);
    if (aml_getprop_bool("media.audiohal.a2dp"))
    {
        FILE *fp1 = fopen("/data/audio_out/a2dp.pcm", "a+");
        if (fp1) {
            int flen = fwrite((char *)out_buffer, 1, out_size, fp1);
            fclose(fp1);
        }
    }

    int write_bytes = 0;
    while (write_bytes < (int)out_size) {
        sent = ((out_size-write_bytes) > out->buffer_sz)?out->buffer_sz:(out_size-write_bytes);
        sent = skt_write(stream, out->audio_fd, (void*)((char*)out_buffer+write_bytes), sent);
        if (sent < 0)
            break;
        write_bytes += sent;
    }
    pthread_mutex_lock(&out->mutex);
    if (sent == -1) {
        skt_disconnect(out->audio_fd);
        out->audio_fd = AUDIO_SKT_DISCONNECTED;
        if (out->state != AUDIO_A2DP_STATE_SUSPENDED && out->state != AUDIO_A2DP_STATE_STOPPING)
            out->state = AUDIO_A2DP_STATE_STOPPED;
        else
            ALOGE("write failed : stream suspended, avoid resetting state");
        goto finish;
    }

finish:
    out->last_write_time = aml_audio_get_systime();
    out->frames_rendered += in_frames;
    out->frames_presented += in_frames;
    pthread_mutex_unlock(&out->mutex);

    // If send didn't work out, sleep to emulate write delay.
    if (sent == -1) {
        const int us_delay = in_frames * USEC_PER_SEC / aml_out->hal_rate;
        ALOGD("stream %p emulate a2dp write delay (%d us)", aml_out, us_delay);
        usleep(us_delay);
    }
    return bytes;
}

int a2dp_out_standby(struct audio_stream* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    int retVal = 0;

    ALOGD("a2dp_out_standby: %p", aml_out);
    pthread_mutex_lock(&out->mutex);
    // Do nothing in SUSPENDED state.
    if (out->state != AUDIO_A2DP_STATE_SUSPENDED)
        retVal = suspend_audio_datapath(stream, true);
    out->frames_rendered = 0;  // rendered is reset, presented is not
    if (out->vir_buf_handle != NULL)
        audio_virtual_buf_close(&out->vir_buf_handle);
    pthread_mutex_unlock(&out->mutex);
    return retVal;
}

int a2dp_output_enable(struct audio_stream_out* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct a2dp_stream_out* out = NULL;
    struct audio_config* config = &aml_out->out_cfg;
    int ret = 0;

    ALOGD("a2dp_output_enable");
    if (aml_out->a2dp_out != NULL) {
        ALOGD("a2dp_output_enable already exist");
        return 0;
    }
    out = (struct a2dp_stream_out*)malloc(sizeof(struct a2dp_stream_out));
    if (!out) {
        ALOGE("a2dp_output_enable a2dp_stream_out realloc error");
        return -ENOMEM;
    }
    memset(out, 0, sizeof(struct a2dp_stream_out));
    aml_out->a2dp_out = out;
    //aml_alsa_output_close(stream);
    pthread_mutex_init(&out->mutex, NULL);
    pthread_mutex_lock(&out->mutex);

    total_input_ns = 0;
    /* initialize a2dp specifics */
    a2dp_stream_common_init(out);

    // Make sure we always have the feeding parameters configured
    if (a2dp_read_output_audio_config(stream) < 0) {
        ALOGE("a2dp_read_output_audio_config failed");
        ret = -1;
        //goto err_open;
    }

    a2dp_write_output_audio_config(stream);
    a2dp_read_output_audio_config(stream);

    ALOGD("Output stream config: format=0x%x sample_rate=%d channel_mask=0x%x buffer_sz=%zu",
            config->format, config->sample_rate, config->channel_mask, out->buffer_sz);

    //aml_out->stream.common.set_parameters = a2dp_out_set_parameters;
    //aml_out->stream.common.get_parameters = a2dp_out_get_parameters;
    //aml_out->stream.common.get_buffer_size = a2dp_out_get_buffer_size;
    //aml_out->stream.get_render_position = a2dp_out_get_render_position;
    //aml_out->stream.get_presentation_position = a2dp_out_get_presentation_position;
    aml_dev->sink_format = AUDIO_FORMAT_PCM_16_BIT;
    aml_dev->optical_format = AUDIO_FORMAT_PCM_16_BIT;

#if defined(AUDIO_EFFECT_EXTERN_DEVICE)
    out->bt_gain = 1;
    out->bt_unmute = 1;
    out->left_gain = 1;
    out->right_gain = 1;
#endif
    pthread_mutex_unlock(&out->mutex);
    ALOGD("a2dp_output_enable success");
    /* Delay to ensure Headset is in proper state when START is initiated from
    * DUT immediately after the connection due to ongoing music playback.
    */
    //usleep(250000);
    return 0;

err_open:
    pthread_mutex_unlock(&out->mutex);
    aml_out->a2dp_out = NULL;
    ALOGE("a2dp_output_enable failed");
    return ret;
}
void a2dp_output_disable(struct audio_stream_out* stream) {
    struct aml_stream_out* aml_out = (struct aml_stream_out*)stream;
    struct a2dp_stream_out* out = aml_out->a2dp_out;
    /*
    aml_out->stream.common.set_parameters = out_set_parameters;
    aml_out->stream.common.get_parameters = out_get_parameters;
    //aml_out->stream.common.get_buffer_size = out_get_buffer_size;
    aml_out->stream.get_render_position = out_get_render_position;
    aml_out->stream.get_presentation_position = out_get_presentation_position;
    */
    if (out == NULL)
        return;
    pthread_mutex_lock(&out->mutex);
    total_input_ns = 0;
    ALOGD("a2dp_output_disable  (state %d)", (int)out->state);
    if ((out->state == AUDIO_A2DP_STATE_STARTED) || (out->state == AUDIO_A2DP_STATE_STOPPING)) {
        stop_audio_datapath(&stream->common);
    }
    if (out->vir_buf_handle != NULL)
        audio_virtual_buf_close(&out->vir_buf_handle);

    a2dp_stream_common_destroy(out);
    pthread_mutex_unlock(&out->mutex);
    free(out);
    aml_out->a2dp_out = NULL;
    ALOGD("a2dp_output_disable done");
}

