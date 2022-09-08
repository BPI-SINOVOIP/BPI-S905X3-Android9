/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "amlaudioMixer"
//#define LOG_NDEBUG 0
#define DEBUG_DUMP 0

#define __USE_GNU
#include <cutils/log.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <system/audio.h>
#include <aml_volume_utils.h>

#include "amlAudioMixer.h"
#include "audio_hw_utils.h"
#include "hw_avsync.h"
#include "audio_hwsync.h"
#include "audio_data_process.h"
#include "audio_virtual_buf.h"

#include "audio_hw.h"
#include "audio_a2dp_hw.h"

#define MIXER_FRAME_COUNT                   (384)
#define MIXER_OUT_FRAME_SIZE                (8)
#define MIXER_WRITE_PERIOD_TIME_NANO        (MIXER_FRAME_COUNT * 1000000000LL / 48000)

struct ring_buf_desc {
    struct ring_buffer *buffer;
    struct pcm_config cfg;
    int period_time;
    int valid;
};

enum mixer_state {
    MIXER_IDLE,             // no active tracks
    MIXER_INPORTS_ENABLED,  // at least one active track, but no track has any data ready
    MIXER_INPORTS_READY,    // at least one active track, and at least one track has data
    MIXER_DRAIN_TRACK,      // drain currently playing track
    MIXER_DRAIN_ALL,        // fully drain the hardware
};
typedef enum AML_AUDIO_MIXER_RUN_STATE_TYPE {
    AML_AUDIO_MIXER_RUN_STATE_EXITED            = 0,
    AML_AUDIO_MIXER_RUN_STATE_REQ_EXIT          = 1,
    AML_AUDIO_MIXER_RUN_STATE_RUNNING           = 2,
    AML_AUDIO_MIXER_RUN_STATE_REQ_RUN           = 3,
    AML_AUDIO_MIXER_RUN_STATE_SLEEP             = 4,
    AML_AUDIO_MIXER_RUN_STATE_REQ_SLEEP         = 5,

    AML_AUDIO_MIXER_RUN_STATE_BUTT              = 6,
} aml_audio_mixer_run_state_type_e;


//simple mixer support: 2 in , 1 out
struct amlAudioMixer {
    struct input_port *in_ports[AML_MIXER_INPUT_PORT_BUTT];
    struct output_port *out_ports[MIXER_OUTPUT_PORT_NUM];
    pthread_mutex_t inport_lock;
    ssize_t (*write)(struct amlAudioMixer *mixer, void *buffer, int bytes);
    //int period_time;
    void *tmp_buffer;
    int idle_sleep_time_us;
    size_t frame_size_tmp;
    uint32_t hwsync_frame_size;
    pthread_t out_mixer_tid;
    pthread_mutex_t lock;
    int exit_thread : 1;
    int mixing_enable : 1;
    enum mixer_state state;
    struct timespec tval_last_write;
    struct timespec tval_last_run;
    void *adev_data;
    struct aml_audio_device *adev;
    bool continuous_output;
    //int init_ok : 1;
    bool submix_standby;
    aml_audio_mixer_run_state_type_e run_state;
};

int mixer_set_state(struct amlAudioMixer *audio_mixer, enum mixer_state state)
{
    audio_mixer->state = state;
    return 0;
}

int mixer_set_continuous_output(struct amlAudioMixer *audio_mixer,
        bool continuous_output)
{
    audio_mixer->continuous_output = continuous_output;
    return 0;
}

bool mixer_is_continuous_enabled(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->continuous_output;
}

enum mixer_state mixer_get_state(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->state;
}

int init_mixer_input_port(struct amlAudioMixer *audio_mixer,
        struct audio_config *config,
        audio_output_flags_t flags,
        int (*on_notify_cbk)(void *data),
        void *notify_data,
        int (*on_input_avail_cbk)(void *data),
        void *input_avail_data,
        meta_data_cbk_t on_meta_data_cbk,
        void *meta_data,
        float volume)
{
    struct input_port *port = NULL;
    aml_mixer_input_port_type_e port_index;
    struct aml_stream_out *aml_out = notify_data;
    bool direct_on = false;

    if (audio_mixer == NULL || config == NULL || notify_data == NULL) {
        ALOGE("[%s:%d] NULL pointer", __func__, __LINE__);
        return -EINVAL;
    }

    /* if direct on, ie. the ALSA buffer is full, no need padding data anymore  */
    direct_on = (audio_mixer->in_ports[AML_MIXER_INPUT_PORT_PCM_DIRECT] != NULL);
    port = new_input_port(MIXER_FRAME_COUNT, config, flags, volume, direct_on);
    port_index = port->enInPortType;
    if (audio_mixer->in_ports[port_index] != NULL) {
        ALOGW("[%s:%d] inport index:%s already exists! recreate", __func__, __LINE__, inportType2Str(port_index));
        free_input_port(audio_mixer->in_ports[port_index]);
    }
    ALOGI("[%s:%d] input port:%s, size %d frames, frame_write_sum:%lld", __func__, __LINE__,
        inportType2Str(port_index), MIXER_FRAME_COUNT, aml_out->frame_write_sum);
    audio_mixer->in_ports[port_index] = port;

    set_port_notify_cbk(port, on_notify_cbk, notify_data);
    set_port_input_avail_cbk(port, on_input_avail_cbk, input_avail_data);
    if (on_meta_data_cbk && meta_data) {
        port->is_hwsync = true;
        set_port_meta_data_cbk(port, on_meta_data_cbk, meta_data);
    }
    port->initial_frames = aml_out->frame_write_sum;
    return 0;
}

int delete_mixer_input_port(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index)
{
    ALOGI("[%s:%d] input port:%s", __func__, __LINE__, inportType2Str(port_index));
    if (port_index <= AML_MIXER_INPUT_PORT_INVAL || port_index >= AML_MIXER_INPUT_PORT_BUTT) {
        ALOGW("[%s:%d] input port:%s invalid!!!", __func__, __LINE__, inportType2Str(port_index));
        return -1;
    }
    if (audio_mixer->in_ports[port_index]) {
        free_input_port(audio_mixer->in_ports[port_index]);
        audio_mixer->in_ports[port_index] = NULL;
    }
    return 0;
}

int send_mixer_inport_message(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index , enum PORT_MSG msg)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (port == NULL) {
        ALOGE("%s(), port index %d, inval", __func__, port_index);
        return -EINVAL;
    }

    return send_inport_message(port, msg);
}

void set_mixer_hwsync_frame_size(struct amlAudioMixer *audio_mixer,
        uint32_t frame_size)
{
    aml_mixer_input_port_type_e port_index = AML_MIXER_INPUT_PORT_PCM_SYSTEM;
    enum MIXER_OUTPUT_PORT out_port_index = MIXER_OUTPUT_PORT_PCM;
    struct input_port *in_port = NULL;
    struct output_port *out_port = audio_mixer->out_ports[out_port_index];
    int port_cnt = 0;
    for (; port_index < AML_MIXER_INPUT_PORT_BUTT; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port) {
            //resize_input_port_buffer(in_port, MIXER_IN_BUFFER_SIZE);
        }
    }

    //resize_output_port_buffer(out_port, MIXER_IN_BUFFER_SIZE);
    ALOGI("%s framesize %d", __func__, frame_size);
    audio_mixer->hwsync_frame_size = frame_size;
}

uint32_t get_mixer_hwsync_frame_size(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->hwsync_frame_size;
}

uint32_t get_mixer_inport_consumed_frames(
        struct amlAudioMixer *audio_mixer, aml_mixer_input_port_type_e port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }
    return get_inport_consumed_size(port) / port->cfg.frame_size;
}

int set_mixer_inport_volume(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index, float vol)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    if (vol > 1.0 || vol < 0) {
        ALOGE("%s(), invalid vol %f", __func__, vol);
        return -EINVAL;
    }
    set_inport_volume(port, vol);
    return 0;
}

float get_mixer_inport_volume(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return 0;
    }
    return get_inport_volume(port);
}

int mixer_write_inport(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index, const void *buffer, int bytes)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    int written = 0;

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    written = port->write(port, buffer, bytes);
    if (get_inport_state(port) != ACTIVE) {
        ALOGI("[%s:%d] input port:%s is active now", __func__, __LINE__, inportType2Str(port_index));
        set_inport_state(port, ACTIVE);
    }
    ALOGV("%s(), signal line %d portIndex %d", __func__, __LINE__, port_index);
    return written;
}

int mixer_read_inport(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index, void *buffer, int bytes)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    return port->read(port, buffer, bytes);
}

int mixer_set_inport_state(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index, enum port_state state)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    return set_inport_state(port, state);
}

enum port_state mixer_get_inport_state(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    return get_inport_state(port);
}
//TODO: handle message queue
static void mixer_procs_msg_queue(struct amlAudioMixer *audio_mixer __unused)
{
    ALOGV("++%s start", __func__);
    return;
}

static struct output_port *get_outport(struct amlAudioMixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    return audio_mixer->out_ports[port_index];
}

size_t get_outport_data_avail(struct output_port *outport)
{
    return outport->bytes_avail;
}

int set_outport_data_avail(struct output_port *outport, size_t avail)
{
    if (avail > outport->data_buf_len) {
        ALOGE("%s(), invalid avail %u", __func__, avail);
        return -EINVAL;
    }
    outport->bytes_avail = avail;
    return 0;
}

static bool is_output_data_avail(struct amlAudioMixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    struct output_port *outport = NULL;

    ALOGV("++%s start", __func__);
    /* init check */
    //if(amlAudioMixer_check_status(audio_mixer))
    //    return false;

    outport = get_outport(audio_mixer, port_index);
    return !!get_outport_data_avail(outport);
    //return true;
}

int init_mixer_output_port(struct amlAudioMixer *audio_mixer,
        struct audioCfg cfg,
        size_t buf_frames)
{
    struct output_port *port = new_output_port(MIXER_OUTPUT_PORT_PCM,
            cfg, buf_frames);
    enum MIXER_OUTPUT_PORT port_index = port->enOutPortType;

    audio_mixer->out_ports[port_index] = port;
    audio_mixer->idle_sleep_time_us = (((MIXER_FRAME_COUNT * 1000) / cfg.sampleRate) * 1000) / 2;
    return 0;
}

int delete_mixer_output_port(struct amlAudioMixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    free_output_port(audio_mixer->out_ports[port_index]);
    audio_mixer->out_ports[port_index] = NULL;
    return 0;
}

static int mixer_output_startup(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_OUTPUT_PORT port_index = 0;
    struct output_port *out_port = audio_mixer->out_ports[port_index];

    ALOGI("++%s start open", __func__);
    out_port->start(out_port);
    audio_mixer->submix_standby = 0;

    return 0;
}

struct pcm * get_mixer_output_pcm_handle(struct amlAudioMixer *audio_mixer, enum MIXER_OUTPUT_PORT enOutputIndex)
{
    if (enOutputIndex <= MIXER_OUTPUT_PORT_INVAL || enOutputIndex >= MIXER_OUTPUT_PORT_NUM) {
        ALOGW("[%s:%d] enOutputIndex:%d, is invalid", __func__, __LINE__, enOutputIndex);
        return NULL;
    }
    struct output_port *out_port = audio_mixer->out_ports[enOutputIndex];
    if (out_port == NULL) {
        ALOGW("[%s:%d] sub mixer outport:% is null", __func__, __LINE__, enOutputIndex);
        return NULL;
    }
    return out_port->pcm_handle;
}

int mixer_output_standby(struct amlAudioMixer *audio_mixer)
{
    ALOGI("[%s:%d] request sleep thread", __func__, __LINE__);
    int timeoutMs = 200;
    audio_mixer->run_state = AML_AUDIO_MIXER_RUN_STATE_REQ_SLEEP;
    return 0;
}

static int mixer_thread_sleep(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];

    if (false == audio_mixer->submix_standby) {
        ALOGI("[%s:%d] start going to standby", __func__, __LINE__);
        out_port->standby(out_port);
        audio_mixer->submix_standby = true;
    }
    pthread_mutex_lock(&audio_mixer->lock);
    ALOGI("[%s:%d] the thread is sleeping", __func__, __LINE__);
    audio_mixer->run_state = AML_AUDIO_MIXER_RUN_STATE_SLEEP;
//    pthread_cond_wait(&audio_mixer->cond, &audio_mixer->lock);
    ALOGI("[%s:%d] the thread is awakened", __func__, __LINE__);
    pthread_mutex_unlock(&audio_mixer->lock);
    return 0;
}

int mixer_output_dummy(struct amlAudioMixer *audio_mixer, bool en)
{
    enum MIXER_OUTPUT_PORT port_index = 0;
    struct output_port *out_port = audio_mixer->out_ports[port_index];

    ALOGI("++%s(), en = %d", __func__, en);
    outport_set_dummy(out_port, en);

    return 0;
}

static int mixer_output_write(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_OUTPUT_PORT port_index = 0;
    struct input_port *in_port_direct = audio_mixer->in_ports[AML_MIXER_INPUT_PORT_PCM_DIRECT];
    struct input_port *in_port_system = audio_mixer->in_ports[AML_MIXER_INPUT_PORT_PCM_SYSTEM];
    struct aml_stream_out *out = NULL;
    struct output_port *out_port = audio_mixer->out_ports[port_index];
    out_port->sound_track_mode = audio_mixer->adev->sound_track_mode;
    if (true == audio_mixer->submix_standby) {
        mixer_output_startup(audio_mixer);
    }

    if (in_port_direct && in_port_direct->notify_cbk_data) {
        out = (struct aml_stream_out *)in_port_direct->notify_cbk_data;
    } else if (in_port_system && in_port_system->notify_cbk_data) {
        out = (struct aml_stream_out *)in_port_system->notify_cbk_data;
    }

    while (is_output_data_avail(audio_mixer, port_index)) {
        // out_write_callbacks();
        if (out && (out->out_device & AUDIO_DEVICE_OUT_ALL_A2DP))
            a2dp_out_write(&out->stream, out_port->data_buf, out_port->bytes_avail);
        else
            out_port->write(out_port, out_port->data_buf, out_port->bytes_avail);
        set_outport_data_avail(out_port, 0);
    };
    return 0;
}

#define DEFAULT_KERNEL_FRAMES (DEFAULT_PLAYBACK_PERIOD_SIZE*DEFAULT_PLAYBACK_PERIOD_CNT)

static int mixer_update_tstamp(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    struct input_port *in_port = audio_mixer->in_ports[AML_MIXER_INPUT_PORT_PCM_SYSTEM];
    unsigned int avail;
    //struct timespec *timestamp;

    /*only deal with system audio */
    if (in_port == NULL || out_port == NULL)
        return 0;

    if (out_port->pcm_handle == NULL)
        return 0;

    if (pcm_get_htimestamp(out_port->pcm_handle, &avail, &in_port->timestamp) == 0) {
        size_t kernel_buf_size = DEFAULT_KERNEL_FRAMES;
        int64_t signed_frames = in_port->mix_consumed_frames - kernel_buf_size + avail;
        if (signed_frames < 0) {
            signed_frames = 0;
        }
        in_port->presentation_frames = in_port->initial_frames + signed_frames;
        ALOGV("%s() present frames:%lld, initial %lld, consumed %lld, sec:%ld, nanosec:%ld",
                __func__,
                in_port->presentation_frames,
                in_port->initial_frames,
                in_port->mix_consumed_frames,
                in_port->timestamp.tv_sec,
                in_port->timestamp.tv_nsec);
    }

    return 0;
}

static bool is_mixer_inports_ready(struct amlAudioMixer *audio_mixer)
{
    aml_mixer_input_port_type_e port_index = 0;
    int port_cnt = 0, ready = 0;
    for (port_index = 0; port_index < AML_MIXER_INPUT_PORT_BUTT; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        ALOGV("%s() port index %d, port ptr %p", __func__, port_index, in_port);
        if (in_port) {
            port_cnt++;
            if (in_port->rbuf_ready(in_port)) {
                ALOGV("port %d data ready", port_index);
                ready++;
            } else {
                ALOGV("port %d data not ready", port_index);
            }
        }
    }

    return (port_cnt == ready);
}

inline float get_fade_step_by_size(int fade_size, int frame_size)
{
    return 1.0/(fade_size/frame_size);
}

int init_fade(struct fade_out *fade_out, int fade_size,
        int sample_size, int channel_cnt)
{
    fade_out->vol = 1.0;
    fade_out->target_vol = 0;
    fade_out->fade_size = fade_size;
    fade_out->sample_size = sample_size;
    fade_out->channel_cnt = channel_cnt;
    fade_out->stride = get_fade_step_by_size(fade_size, sample_size * channel_cnt);
    ALOGI("%s() size %d, stride %f", __func__, fade_size, fade_out->stride);
    return 0;
}

int process_fade_out(void *buf, int bytes, struct fade_out *fout)
{
    int i = 0;
    int frame_cnt = bytes / fout->sample_size / fout->channel_cnt;
    int16_t *sample = (int16_t *)buf;

    if (fout->channel_cnt != 2 || fout->sample_size != 2)
        ALOGE("%s(), not support yet", __func__);
    ALOGI("++++fade out vol %f, size %d", fout->vol, fout->fade_size);
    for (i = 0; i < frame_cnt; i++) {
        sample[i] = sample[i]*fout->vol;
        sample[i+1] = sample[i+1]*fout->vol;
        fout->vol -= fout->stride;
        if (fout->vol < 0)
            fout->vol = 0;
    }
    fout->fade_size -= bytes;
    ALOGI("----fade out vol %f, size %d", fout->vol, fout->fade_size);

    return 0;
}

static int update_inport_avail(struct input_port *in_port)
{
    // first throw away the padding frames
    if (in_port->padding_frames > 0) {
        in_port->padding_frames -= in_port->data_buf_frame_cnt;
        set_inport_pts_valid(in_port, false);
    } else {
        in_port->mix_consumed_frames += in_port->data_buf_frame_cnt;
        set_inport_pts_valid(in_port, true);
    }
    in_port->data_valid = 1;
    return 0;
}

static void process_port_msg(struct input_port *port)
{
    struct port_message *msg = get_inport_message(port);
    if (msg) {
        ALOGI("%s(), msg: %s", __func__, port_msg_to_str(msg->msg_what));
        switch (msg->msg_what) {
        case MSG_PAUSE:
            set_inport_state(port, PAUSING);
            break;
        case MSG_FLUSH:
            set_inport_state(port, FLUSHING);
            break;
        case MSG_RESUME:
            set_inport_state(port, RESUMING);
            break;
        default:
            ALOGE("%s(), not support", __func__);
        }

        remove_inport_message(port, msg);
    }
}

int mixer_flush_inport(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index)
{
    struct input_port *in_port = audio_mixer->in_ports[port_index];

    if (!in_port) {
        return -EINVAL;
    }

    return reset_input_port(in_port);
}

static int mixer_inports_read(struct amlAudioMixer *audio_mixer)
{
    aml_mixer_input_port_type_e port_index = AML_MIXER_INPUT_PORT_PCM_SYSTEM;
    struct aml_audio_device     *adev = audio_mixer->adev;

    ALOGV("++%s(), line %d", __func__, __LINE__);
    for (; port_index < AML_MIXER_INPUT_PORT_BUTT; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];

        if (in_port) {
            int ret = 0, fade_out = 0, fade_in = 0;

            process_port_msg(in_port);
            enum port_state state = get_inport_state(in_port);

            if (port_index == AML_MIXER_INPUT_PORT_PCM_DIRECT) {
                //if in pausing states, don't retrieve data
                if (state == PAUSING) {
                    fade_out = 1;
                    ALOGI("%s(), tsync pause audio", __func__);
                    aml_hwsync_set_tsync_pause();
                } else if (state == RESUMING) {
                    fade_in = 1;
                    ALOGI("[%s:%d] input port:%s tsync resume", __func__, __LINE__, inportType2Str(port_index));
                    aml_hwsync_set_tsync_resume();
                    set_inport_state(in_port, ACTIVE);
                } else if (state == STOPPED || state == PAUSED || state == FLUSHED) {
                    ALOGV("[%s:%d] input port:%s stopped, paused or flushed", __func__, __LINE__, inportType2Str(port_index));
                    continue;
                } else if (state == FLUSHING) {
                    mixer_flush_inport(audio_mixer, port_index);
                    ALOGI("[%s:%d] input port:%s flushing->flushed", __func__, __LINE__, inportType2Str(port_index));
                    set_inport_state(in_port, FLUSHED);
                    continue;
                }
                if (get_inport_state(in_port) == ACTIVE && in_port->data_valid) {
                    ALOGI("[%s:%d] input port:%s data already valid", __func__, __LINE__, inportType2Str(port_index));
                    continue;
                }
            } else {
                if (in_port->data_valid) {
                    ALOGI("[%s:%d] input port:%s data already valid", __func__, __LINE__, inportType2Str(port_index));
                    continue;
                }
            }

            if (in_port->rbuf_ready(in_port)) {
                ret = mixer_read_inport(audio_mixer, port_index, in_port->data, in_port->data_len_bytes);
                if (ret == (int)in_port->data_len_bytes) {
                    if (fade_out) {
                        ALOGI("[%s:%d] output port:%s fade out, pausing->pausing_1, tsync pause audio",
                            __func__, __LINE__, inportType2Str(port_index));
                        aml_hwsync_set_tsync_pause();
                        audio_fade_func(in_port->data, ret, 0);
                        set_inport_state(in_port, PAUSED);
                    } else if (fade_in) {
                        ALOGI("[%s:%d] input port:%s fade in", __func__, __LINE__, inportType2Str(port_index));
                        audio_fade_func(in_port->data, ret, 1);
                        set_inport_state(in_port, ACTIVE);
                    }
                    update_inport_avail(in_port);
                    if (getprop_bool("media.audiohal.inport") &&
                            (in_port->enInPortType == AML_MIXER_INPUT_PORT_PCM_DIRECT)) {
                            aml_audio_dump_audio_bitstreams("/data/audio/inportDirectFade.raw",
                                    in_port->data, in_port->data_len_bytes);
                    }
                } else {
                    ALOGW("[%s:%d] port:%s read fail, have read:%d Byte, need %d Byte", __func__, __LINE__,
                        inportType2Str(port_index), ret, in_port->data_len_bytes);
                }
            } else {
                if (adev->debug_flag) {
                    ALOGI("[%s:%d] port:%s ring buffer data is not enough", __func__, __LINE__, inportType2Str(port_index));
                }
            }
        }
    }

    return 0;
}

int check_mixer_state(struct amlAudioMixer *audio_mixer)
{
    aml_mixer_input_port_type_e port_index = 0;
    int inport_avail = 0, inport_ready = 0;

    ALOGV("++%s(), line %d", __func__, __LINE__);
    for (port_index = 0; port_index < AML_MIXER_INPUT_PORT_BUTT; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port) {
            inport_avail = 1;

            // only when one or more inport is active, mixer is ready
            if (get_inport_state(in_port) == ACTIVE || get_inport_state(in_port) == PAUSING
                    || get_inport_state(in_port) == PAUSING_1)
                inport_ready = 1;
        }
    }

    if (inport_ready)
        audio_mixer->state = MIXER_INPORTS_READY;
    else if (inport_avail)
        audio_mixer->state = MIXER_INPORTS_ENABLED;
    else
        audio_mixer->state = MIXER_IDLE;

    return 0;
}

int mixer_need_wait_forever(struct amlAudioMixer *audio_mixer)
{
    return mixer_get_state(audio_mixer) != MIXER_INPORTS_READY;
}

static inline int16_t CLIP16(int r)
{
    return (r >  0x7fff) ? 0x7fff :
           (r < -0x8000) ? 0x8000 :
           r;
}

static uint32_t hwsync_align_to_frame(uint32_t consumed_size, uint32_t frame_size)
{
    return consumed_size - (consumed_size % frame_size);
}

static int retrieve_hwsync_header(struct amlAudioMixer *audio_mixer,
        struct input_port *in_port, struct output_port *out_port)
{
    uint32_t frame_size = get_mixer_hwsync_frame_size(audio_mixer);
    uint32_t port_consumed_size = get_inport_consumed_size(in_port);
    uint32_t aligned_offset = 0;
    int diff_ms = 0;
    struct hw_avsync_header header;
    int ret = 0;

    if (frame_size == 0) {
        ALOGV("%s(), invalid frame size 0", __func__);
        return -EINVAL;
    }

    if (!in_port->is_hwsync) {
        ALOGE("%s(), not hwsync port", __func__);
        return -EINVAL;
    }

    aligned_offset = hwsync_align_to_frame(port_consumed_size, frame_size);
    memset(&header, 0, sizeof(struct hw_avsync_header));
    ALOGV("direct out port bytes before cbk %d", get_outport_data_avail(out_port));
    if (!in_port->meta_data_cbk) {
        ALOGE("no meta_data_cbk set!!");
        return -EINVAL;
    }
    ALOGV("%s(), port %p, data %p", __func__, in_port, in_port->meta_data_cbk_data);
    ret = in_port->meta_data_cbk(in_port->meta_data_cbk_data,
                aligned_offset, &header, &diff_ms);
    if (ret < 0) {
        if (ret != -EAGAIN)
            ALOGE("meta_data_cbk fail err = %d!!", ret);
        return ret;
    }
    ALOGV("%s(), meta data cbk, diffms = %d", __func__, diff_ms);
    if (diff_ms > 0) {
        in_port->bytes_to_insert = diff_ms * 48 * 4;
    } else if (diff_ms < 0) {
        in_port->bytes_to_skip = -diff_ms * 48 * 4;
    }

    return 0;
}

static int mixer_do_mixing_32bit(struct amlAudioMixer *audio_mixer)
{
    struct input_port *in_port_sys = audio_mixer->in_ports[AML_MIXER_INPUT_PORT_PCM_SYSTEM];
    struct input_port *in_port_drct = audio_mixer->in_ports[AML_MIXER_INPUT_PORT_PCM_DIRECT];
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    struct aml_audio_device *adev = audio_mixer->adev;
    int16_t *data_sys, *data_drct, *data_mixed;
    int mixing = 0, sys_only = 0, direct_only = 0;
    int dirct_okay = 0, sys_okay = 0;
    float dirct_vol = 1.0, sys_vol = 1.0;
    int mixed_32 = 0;
    size_t i = 0, mixing_len_bytes = 0;
    size_t frames = 0;
    size_t frames_written = 0;
    float gain_speaker = adev->sink_gain[OUTPORT_SPEAKER];

    if (!out_port) {
        ALOGE("%s(), out null !!!", __func__);
        return 0;
    }
    if (!in_port_sys && !in_port_drct) {
        ALOGE("%s(), sys or direct pcm must exist!!!", __func__);
        return 0;
    }

    if (in_port_sys && in_port_sys->data_valid) {
        sys_okay = 1;
    }
    if (in_port_drct && in_port_drct->data_valid) {
        dirct_okay = 1;
    }
    if (sys_okay && dirct_okay) {
        mixing = 1;
    } else if (dirct_okay) {
        ALOGV("only direct okay");
        direct_only = 1;
    } else if (sys_okay) {
        sys_only = 1;
    } else {
        ALOGV("%s(), sys direct both not ready!", __func__);
        return -EINVAL;
    }

    data_mixed = (int16_t *)out_port->data_buf;
    memset(audio_mixer->tmp_buffer, 0 , MIXER_FRAME_COUNT * MIXER_OUT_FRAME_SIZE);
    if (mixing) {
        ALOGV("%s() mixing", __func__);
        data_sys = (int16_t *)in_port_sys->data;
        data_drct = (int16_t *)in_port_drct->data;
        mixing_len_bytes = in_port_drct->data_len_bytes;
        //TODO: check if the two stream's frames are equal
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, in_port_drct->data_len_bytes);
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, in_port_sys->data_len_bytes);
        }
        if (in_port_drct->is_hwsync && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        // insert data for direct hwsync case, only send system sound
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
            ALOGD("%s() insert mixing data, need %zu, insert length %zu",
                    __func__, in_port_drct->bytes_to_insert, mixing_len_bytes);
            //memcpy(data_mixed, data_sys, mixing_len_bytes);
            //memcpy(audio_mixer->tmp_buffer, data_sys, mixing_len_bytes);
            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/systbeforemix.raw",
                        data_sys, in_port_sys->data_len_bytes);
            }
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);
            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/sysAftermix.raw",
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
            }
            if (adev->is_TV) {
                apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
            }
#ifdef IS_ATOM_PROJECT
            if (adev->has_dsp_lib) {
                dsp_process_output(audio_mixer->adev,
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
                extend_channel_5_8(data_mixed,
                        audio_mixer->adev->effect_buf, frames, 5, 8);
            } else
#endif
                extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                        frames, 2, 8);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/dataInsertMixed.raw",
                        data_mixed, frames * out_port->cfg.frame_size);
            }
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
            in_port_sys->data_valid = 0;
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        } else {
            frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_drct,
                frames, in_port_drct->cfg, out_port->cfg);
            if (DEBUG_DUMP)
                aml_audio_dump_audio_bitstreams("/data/audio/tmpMixed0.raw",
                    audio_mixer->tmp_buffer, frames * audio_mixer->frame_size_tmp);
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);
            if (DEBUG_DUMP)
                aml_audio_dump_audio_bitstreams("/data/audio/tmpMixed1.raw",
                    audio_mixer->tmp_buffer, frames * audio_mixer->frame_size_tmp);
            if (adev->is_TV) {
                apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
            }
#ifdef IS_ATOM_PROJECT
            if (adev->has_dsp_lib) {
                dsp_process_output(audio_mixer->adev,
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
                extend_channel_5_8(data_mixed,
                        audio_mixer->adev->effect_buf, frames, 5, 8);
            } else
#endif
                extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                        frames, 2, 8);

            in_port_drct->data_valid = 0;
            in_port_sys->data_valid = 0;
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        }
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/data_mixed.raw",
                out_port->data_buf, frames * out_port->cfg.frame_size);
        }
    }

    if (sys_only) {
        frames = in_port_sys->data_buf_frame_cnt;
        ALOGV("%s() sys_only, frames %d", __func__, frames);
        mixing_len_bytes = in_port_sys->data_len_bytes;
        data_sys = (int16_t *)in_port_sys->data;
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, mixing_len_bytes);
        }
        // processing data and make convertion according to cfg
        // processing_and_convert(data_mixed, data_sys, frames, in_port_sys->cfg, out_port->cfg);
        frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/sysTmp.raw",
                    audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
        }
        if (adev->is_TV) {
            apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
        }
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/sysvol.raw",
                    audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
        }
#ifdef IS_ATOM_PROJECT
        if (adev->has_dsp_lib) {
            dsp_process_output(audio_mixer->adev,
                    audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
            extend_channel_5_8(data_mixed,
                    audio_mixer->adev->effect_buf, frames, 5, 8);
        } else
#endif
            extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer, frames, 2, 8);

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/extandsys.raw",
                    data_mixed, frames * out_port->cfg.frame_size);
        }
        in_port_sys->data_valid = 0;
        set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
    }

    if (direct_only) {
        ALOGV("%s() direct_only", __func__);
        //dirct_vol = get_inport_volume(in_port_drct);
        mixing_len_bytes = in_port_drct->data_len_bytes;
        data_drct = (int16_t *)in_port_drct->data;
        ALOGV("%s() direct_only, inport consumed %d",
                __func__, get_inport_consumed_size(in_port_drct));

        if (in_port_drct->is_hwsync && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
        }
        // insert 0 data to delay audio
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
            ALOGD("%s() inserting direct_only, need %zu, insert length %zu",
                    __func__, in_port_drct->bytes_to_insert, mixing_len_bytes);
            memset(data_mixed, 0, mixing_len_bytes);
            extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                    frames, 2, 8);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        } else {
            ALOGV("%s() direct_only, vol %f", __func__, dirct_vol);
            frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
            //cpy_16bit_data_with_gain(data_mixed, data_drct,
            //        in_port_drct->data_len_bytes, dirct_vol);
            ALOGV("%s() direct_only, frames %d, bytes %d", __func__, frames, mixing_len_bytes);

            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_drct,
                frames, in_port_drct->cfg, out_port->cfg);
            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/dirctTmp.raw",
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
            }
            if (adev->is_TV) {
                apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
            }
#ifdef IS_ATOM_PROJECT
            if (adev->has_dsp_lib) {
                dsp_process_output(audio_mixer->adev,
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
                extend_channel_5_8(data_mixed,
                        audio_mixer->adev->effect_buf, frames, 5, 8);
            } else
#endif
                extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                        frames, 2, 8);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/exDrct.raw",
                        data_mixed, frames * out_port->cfg.frame_size);
            }
            in_port_drct->data_valid = 0;
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        }
    }

    if (0) {
        aml_audio_dump_audio_bitstreams("/data/audio/data_mixed.raw",
                out_port->data_buf, mixing_len_bytes);
    }
    return 0;
}

static int mixer_add_mixing_data(void *pMixedBuf, struct input_port *pInputPort, struct output_port *pOutputPort)
{
    if (pInputPort->data_buf_frame_cnt < MIXER_FRAME_COUNT) {
        ALOGE("[%s:%d] input port type:%s buf frames:%d too small", __func__, __LINE__,
            inportType2Str(pInputPort->enInPortType), pInputPort->data_buf_frame_cnt);
        return -EINVAL;
    }
    do_mixing_2ch(pMixedBuf, pInputPort->data, MIXER_FRAME_COUNT, pInputPort->cfg, pOutputPort->cfg);
    pInputPort->data_valid = 0;
    return 0;
}

static int mixer_do_mixing_16bit(struct amlAudioMixer *audio_mixer)
{
    struct input_port           *pstInputPort = NULL;
    struct output_port          *pstOutPort = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    struct aml_audio_device     *adev = audio_mixer->adev;

    if (NULL == pstOutPort) {
        ALOGE("[%s:%d] outport is null", __func__, __LINE__);
        return 0;
    }

    memset(audio_mixer->tmp_buffer, 0, MIXER_FRAME_COUNT * MIXER_OUT_FRAME_SIZE);
    for (int i = AML_MIXER_INPUT_PORT_PCM_SYSTEM; i < AML_MIXER_INPUT_PORT_BUTT; i++) {
        pstInputPort = audio_mixer->in_ports[i];
        if (NULL == pstInputPort) {
            continue;
        }
        if (0 == pstInputPort->data_valid) {
            if (adev->debug_flag) {
                ALOGI("[%s:%d] inport:%s, but no valid data, maybe underrun", __func__, __LINE__, inportType2Str(i));
            }
            continue;
        }
        if (getprop_bool("media.audiohal.indump")) {
            char acFilePathStr[ENUM_TYPE_STR_MAX_LEN];
            sprintf(acFilePathStr, "/data/audio/%s", inportType2Str(i));
            aml_audio_dump_audio_bitstreams(acFilePathStr, pstInputPort->data, pstInputPort->data_len_bytes);
        }
        if (AML_MIXER_INPUT_PORT_PCM_DIRECT == i) {
            if (pstInputPort->is_hwsync && pstInputPort->bytes_to_insert < pstInputPort->data_len_bytes) {
                retrieve_hwsync_header(audio_mixer, pstInputPort, pstOutPort);
            }
            if (pstInputPort->bytes_to_insert >= pstInputPort->data_len_bytes) {
                pstInputPort->bytes_to_insert -= pstInputPort->data_len_bytes;
                ALOGD("[%s:%d] PCM_DIRECT inport insert mute data, still need %zu, inserted length %zu", __func__, __LINE__,
                        pstInputPort->bytes_to_insert, pstInputPort->data_len_bytes);
                continue;
            }
        }
        mixer_add_mixing_data(audio_mixer->tmp_buffer, pstInputPort, pstOutPort);
    }
    if (adev->is_TV) {
        apply_volume(adev->sink_gain[OUTPORT_SPEAKER], audio_mixer->tmp_buffer, sizeof(uint16_t),
            MIXER_FRAME_COUNT * pstOutPort->cfg.frame_size);
    }
    memcpy(pstOutPort->data_buf, audio_mixer->tmp_buffer, MIXER_FRAME_COUNT * pstOutPort->cfg.frame_size);
    if (getprop_bool("media.audiohal.outdump")) {
        aml_audio_dump_audio_bitstreams("/data/audio/audio_mixed", pstOutPort->data_buf,
            MIXER_FRAME_COUNT * pstOutPort->cfg.frame_size);
    }
    set_outport_data_avail(pstOutPort, MIXER_FRAME_COUNT * pstOutPort->cfg.frame_size);
    return 0;
}

int notify_mixer_input_avail(struct amlAudioMixer *audio_mixer)
{
    aml_mixer_input_port_type_e port_index = 0;
    for (port_index = 0; port_index < AML_MIXER_INPUT_PORT_BUTT; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port && in_port->on_notify_cbk)
            in_port->on_input_avail_cbk(in_port->input_avail_cbk_data);
    }

    return 0;
}

int notify_mixer_exit(struct amlAudioMixer *audio_mixer)
{
    aml_mixer_input_port_type_e port_index = 0;
    for (port_index = 0; port_index < AML_MIXER_INPUT_PORT_BUTT; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port && in_port->on_notify_cbk)
            in_port->on_notify_cbk(in_port->notify_cbk_data);
    }

    return 0;
}

static int set_thread_affinity(void)
{
    cpu_set_t cpuSet;
    int sastat = 0;

    CPU_ZERO(&cpuSet);
    CPU_SET(2, &cpuSet);
    CPU_SET(3, &cpuSet);
    sastat = sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
    if (sastat) {
        ALOGW("%s(), failed to set cpu affinity", __FUNCTION__);
        return sastat;
    }

    return 0;
}

#define THROTTLE_TIME_US 5000
static void *mixer_32b_threadloop(void *data)
{
    struct amlAudioMixer *audio_mixer = data;
    enum MIXER_OUTPUT_PORT port_index = MIXER_OUTPUT_PORT_PCM;
    aml_mixer_input_port_type_e in_index = AML_MIXER_INPUT_PORT_PCM_SYSTEM;
    int ret = 0;

    ALOGI("++%s start", __func__);

    audio_mixer->exit_thread = 0;
    prctl(PR_SET_NAME, "amlAudioMixer32");
    set_thread_affinity();
    while (!audio_mixer->exit_thread) {
        //pthread_mutex_lock(&audio_mixer->lock);
        //mixer_procs_msg_queue(audio_mixer);
        // processing throttle
        struct timespec tval_new;
        clock_gettime(CLOCK_MONOTONIC, &tval_new);
        const uint32_t delta_us = tspec_diff_to_us(audio_mixer->tval_last_write, tval_new);
        ret = mixer_inports_read(audio_mixer);
        if (ret < 0) {
            //usleep(5000);
            ALOGV("%s %d data not enough, next turn", __func__, __LINE__);
            notify_mixer_input_avail(audio_mixer);
            continue;
            //notify_mixer_input_avail(audio_mixer);
            //continue;
        }
        notify_mixer_input_avail(audio_mixer);
        ALOGV("%s %d do mixing", __func__, __LINE__);
        mixer_do_mixing_32bit(audio_mixer);
        uint64_t tpast_us = 0;
        clock_gettime(CLOCK_MONOTONIC, &tval_new);
        tpast_us = tspec_diff_to_us(audio_mixer->tval_last_write, tval_new);
        // audio patching should not in this write
        // TODO: fix me, make compatible with source output
        if (!audio_mixer->adev->audio_patching) {
            mixer_output_write(audio_mixer);
            mixer_update_tstamp(audio_mixer);
        }
    }

    ALOGI("--%s", __func__);
    return NULL;
}

static int mixer_do_continous_output(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    int16_t *data_mixed = (int16_t *)out_port->data_buf;
    size_t frames = 4;
    size_t bytes = frames * out_port->cfg.frame_size;

    memset(data_mixed, 0 , bytes);
    set_outport_data_avail(out_port, bytes);
    mixer_output_write(audio_mixer);
    return 0;
}

static uint32_t get_mixer_inport_count(struct amlAudioMixer *audio_mixer)
{
    aml_mixer_input_port_type_e enPortIndex = AML_MIXER_INPUT_PORT_PCM_SYSTEM;
    uint32_t                    u32PortCnt = 0;
    for (; enPortIndex < AML_MIXER_INPUT_PORT_BUTT; enPortIndex++) {
        if (audio_mixer->in_ports[enPortIndex]) {
            u32PortCnt++;
        }
    }
    return u32PortCnt;
}

int mixerIdleSleepTimeUs(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->idle_sleep_time_us;
}


static void *mixer_16b_threadloop(void *data)
{
    struct amlAudioMixer        *audio_mixer = data;
    struct audio_virtual_buf    *pstVirtualBuffer = NULL;

    ALOGI("[%s:%d] begin create thread", __func__, __LINE__);
    if (audio_mixer->mixing_enable == 0) {
        pthread_exit(0);
        ALOGI("[%s:%d] mixing_enable is 0 exit thread", __func__, __LINE__);
        return NULL;
    }
    audio_mixer->exit_thread = 0;
    prctl(PR_SET_NAME, "amlAudioMixer16");
    set_thread_affinity();
    aml_set_thread_priority("amlAudioMixer16", audio_mixer->out_mixer_tid);
    while (!audio_mixer->exit_thread) {
        if (pstVirtualBuffer == NULL) {
            audio_virtual_buf_open((void **)&pstVirtualBuffer, "mixer_16bit_thread",
                    MIXER_WRITE_PERIOD_TIME_NANO * 4, MIXER_WRITE_PERIOD_TIME_NANO * 4, 0);
            audio_virtual_buf_process((void *)pstVirtualBuffer, MIXER_WRITE_PERIOD_TIME_NANO * 4);
        }
        mixer_inports_read(audio_mixer);
        audio_virtual_buf_process((void *)pstVirtualBuffer, MIXER_WRITE_PERIOD_TIME_NANO);
        notify_mixer_input_avail(audio_mixer);
        mixer_do_mixing_16bit(audio_mixer);
        if (!audio_mixer->adev->audio_patching) {
            mixer_output_write(audio_mixer);
            mixer_update_tstamp(audio_mixer);
        }
    }
    if (pstVirtualBuffer != NULL) {
        audio_virtual_buf_close((void **)&pstVirtualBuffer);
    }
    audio_mixer->run_state = AML_AUDIO_MIXER_RUN_STATE_EXITED;

    ALOGI("[%s:%d] exit thread", __func__, __LINE__);
    return NULL;
}

uint32_t mixer_get_inport_latency_frames(struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    int written = 0;

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return 0;
    }

    return port->get_latency_frames(port);
}

uint32_t mixer_get_outport_latency_frames(struct amlAudioMixer *audio_mixer)
{
    struct output_port *port = get_outport(audio_mixer, MIXER_OUTPUT_PORT_PCM);
    if (port == NULL) {
        ALOGE("%s(), port invalid", __func__);
    }
    return outport_get_latency_frames(port);
}

int pcm_mixer_thread_run(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_pcm_port = NULL;
    int ret = 0;

    ALOGI("++%s()", __func__);
    if (audio_mixer == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    out_pcm_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    if (out_pcm_port == NULL) {
        ALOGE("%s(), out port not initialized", __func__);
        return -EINVAL;
    }

    if (audio_mixer->out_mixer_tid > 0) {
        ALOGE("%s(), out mixer thread already running", __func__);
        return -EINVAL;
    }
    audio_mixer->mixing_enable = 1;
    ALOGI("++%s() audio_mixer->mixing_enable %d", __func__, audio_mixer->mixing_enable);
    switch (out_pcm_port->cfg.format) {
    case AUDIO_FORMAT_PCM_32_BIT:
        ret = pthread_create(&audio_mixer->out_mixer_tid,
                NULL, mixer_32b_threadloop, audio_mixer);
        if (ret < 0)
            ALOGE("%s() thread run failed.", __func__);
        break;
    case AUDIO_FORMAT_PCM_16_BIT:
        ret = pthread_create(&audio_mixer->out_mixer_tid,
                NULL, mixer_16b_threadloop, audio_mixer);
        if (ret < 0)
            ALOGE("%s() thread run failed.", __func__);
        break;
    default:
        ALOGE("%s(), format not supported", __func__);
        break;
    }
    ALOGI("++%s() audio_mixer->mixing_enable %d, pthread_create ret %d", __func__, audio_mixer->mixing_enable, ret);

    return ret;
}

int pcm_mixer_thread_exit(struct amlAudioMixer *audio_mixer)
{
    ALOGD("+%s()", __func__);
    audio_mixer->mixing_enable = 0;
    ALOGI("++%s() audio_mixer->mixing_enable %d", __func__, audio_mixer->mixing_enable);
    // block exit
    audio_mixer->exit_thread = 1;
    audio_mixer->run_state = AML_AUDIO_MIXER_RUN_STATE_REQ_EXIT;
    pthread_join(audio_mixer->out_mixer_tid, NULL);
    audio_mixer->out_mixer_tid = 0;

    notify_mixer_exit(audio_mixer);
    return 0;
}
struct amlAudioMixer *newAmlAudioMixer(
        struct audioCfg cfg,
        struct aml_audio_device *adev)
{
    struct amlAudioMixer *audio_mixer = NULL;
    int ret = 0;
    ALOGD("%s()", __func__);

    audio_mixer = calloc(1, sizeof(*audio_mixer));
    if (audio_mixer == NULL) {
        ALOGE("%s(), no memory", __func__);
        return NULL;
    }

    // 2 channel  32bit
    audio_mixer->tmp_buffer = calloc(1, MIXER_FRAME_COUNT * MIXER_OUT_FRAME_SIZE);
    if (audio_mixer->tmp_buffer == NULL) {
        ALOGE("%s(), no memory", __func__);
        goto err_tmp;
    }
    // 2 channel X sample bytes;
    audio_mixer->frame_size_tmp = 2 * audio_bytes_per_sample(cfg.format);
    audio_mixer->submix_standby = 1;
    mixer_set_state(audio_mixer, MIXER_IDLE);
    ret = init_mixer_output_port(audio_mixer,
            cfg, MIXER_FRAME_COUNT);
    if (ret < 0) {
        ALOGE("%s(), init mixer out port failed", __func__);
        goto err_state;
    }
    audio_mixer->adev = adev;
    pthread_mutex_init(&audio_mixer->lock, NULL);
    return audio_mixer;

err_state:
    free(audio_mixer->tmp_buffer);
    audio_mixer->tmp_buffer = NULL;
err_tmp:
    free(audio_mixer);
    audio_mixer = NULL;

    return audio_mixer;
}

void freeAmlAudioMixer(struct amlAudioMixer *audio_mixer)
{
    if (audio_mixer) {
        pthread_mutex_destroy(&audio_mixer->lock);
        free(audio_mixer);
    }
}

int64_t mixer_latency_frames(struct amlAudioMixer *audio_mixer)
{
    (void)audio_mixer;
    /* TODO: calc the mixer buf latency
    * Now using estimated buffer length
    */
    return MIXER_FRAME_COUNT;
}

int mixer_get_presentation_position(
        struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index,
        uint64_t *frames,
        struct timespec *timestamp)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGV("%s(), port not ready now", __func__);
        return -EINVAL;
    }

    *frames = port->presentation_frames;
    *timestamp = port->timestamp;
    if (!is_inport_pts_valid(port)) {
        ALOGV("%s(), not valid now", __func__);
        return -EINVAL;
    }
    return 0;
}

int mixer_set_padding_size(
        struct amlAudioMixer *audio_mixer,
        aml_mixer_input_port_type_e port_index,
        int padding_bytes)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }
    return set_inport_padding_size(port, padding_bytes);
}

int mixer_stop_outport_pcm(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_OUTPUT_PORT port_index = MIXER_OUTPUT_PORT_PCM;
    struct output_port *port = audio_mixer->out_ports[port_index];
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    return outport_stop_pcm(port);
}

void mixer_dump(int s32Fd, const struct aml_audio_device *pstAmlDev)
{
    if (NULL == pstAmlDev || NULL == pstAmlDev->sm) {
        dprintf(s32Fd, "[AML_HAL] [%s:%d] device or sub mixing is NULL !\n", __func__, __LINE__);
        return;
    }
    struct amlAudioMixer *pstAudioMixer = (struct amlAudioMixer *)pstAmlDev->sm->mixerData;
    if (NULL == pstAudioMixer) {
        dprintf(s32Fd, "[AML_HAL] [%s:%d] amlAudioMixer is NULL !\n", __func__, __LINE__);
        return;
    }
    dprintf(s32Fd, "[AML_HAL]---------------------input port description cnt: [%d]--------------\n",
        get_mixer_inport_count(pstAudioMixer));
    aml_mixer_input_port_type_e enInPort = AML_MIXER_INPUT_PORT_PCM_SYSTEM;
    for (; enInPort < AML_MIXER_INPUT_PORT_BUTT; enInPort++) {
        struct input_port *pstInputPort = pstAudioMixer->in_ports[enInPort];
        if (pstInputPort) {
            dprintf(s32Fd, "[AML_HAL]  input port type: %s\n", inportType2Str(pstInputPort->enInPortType));
            dprintf(s32Fd, "[AML_HAL]      Channel       : %10d     | Format    : %#10x\n",
                pstInputPort->cfg.channelCnt, pstInputPort->cfg.format);
            dprintf(s32Fd, "[AML_HAL]      FrameCnt      : %10d     | data size : %10d Byte\n",
                pstInputPort->data_buf_frame_cnt, pstInputPort->data_len_bytes);
            dprintf(s32Fd, "[AML_HAL]      is_hwsync     : %10d     | rbuf size : %10d Byte\n",
                pstInputPort->is_hwsync, pstInputPort->r_buf->size);
        }
    }
    dprintf(s32Fd, "[AML_HAL]---------------------output port description----------------------\n");
    struct output_port *pstOutPort = pstAudioMixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    if (pstOutPort) {
        dprintf(s32Fd, "[AML_HAL]      Channel       : %10d     | Format    : %#10x\n", pstOutPort->cfg.channelCnt, pstOutPort->cfg.format);
        dprintf(s32Fd, "[AML_HAL]      FrameCnt      : %10d     | data size : %10d Byte\n", pstOutPort->data_buf_frame_cnt, pstOutPort->data_buf_len);
    } else {
        dprintf(s32Fd, "[AML_HAL] not find output port description!!!\n");
    }
}
