/*
 * Copyright (C) 2018 Amlogic Corporation.
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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <cutils/atomic.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include <errno.h>
#include <fcntl.h>
#include <hardware/hardware.h>
#include <inttypes.h>
#include <linux/ioctl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <system/audio.h>
#include <time.h>
#include <utils/Timers.h>

#if ANDROID_PLATFORM_SDK_VERSION >= 25 // 8.0
#include <system/audio-base.h>
#endif

#include <hardware/audio.h>

#include <aml_android_utils.h>
#include <aml_data_utils.h>


#include "aml_audio_stream.h"
#include "aml_data_utils.h"
#include "aml_dump_debug.h"
#include "audio_hw.h"
#include "audio_hw_dtv.h"
#include "audio_hw_profile.h"
#include "audio_hw_utils.h"
#include "dtv_patch_out.h"
#include "aml_audio_parser.h"
#include "aml_audio_resampler.h"
#include "audio_hw_ms12.h"
#include "dolby_lib_api.h"
#include "audio_dtv_ad.h"
#include "alsa_device_parser.h"

#define TSYNC_PCRSCR "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT "/sys/class/tsync/event"
#define TSYNC_APTS "/sys/class/tsync/pts_audio"
#define TSYNC_VPTS "/sys/class/tsync/pts_video"
#define TSYNC_DEMUX_APTS "/sys/class/stb/audio_pts"
#define TSYNC_DEMUX_VPTS "/sys/class/stb/video_pts"

#define TSYNC_FIRST_VPTS "/sys/class/tsync/firstvpts"
#define TSYNC_AUDIO_MODE "/sys/class/tsync_pcr/tsync_audio_mode"
#define TSYNC_AUDIO_LEVEL "/sys/class/tsync_pcr/tsync_audio_level"
#define TSYNC_VIDEO_DISCONT "/sys/class/tsync_pcr/tsync_vdiscontinue"
#define TSYNC_LAST_CHECKIN_APTS "/sys/class/tsync_pcr/tsync_last_discontinue_checkin_apts"
#define TSYNC_PCR_DEBUG "/sys/class/tsync_pcr/tsync_pcr_debug"
#define TSYNC_APTS_DIFF "/sys/class/tsync_pcr/tsync_pcr_apts_diff"
#define TSYNC_VPTS_ADJ "/sys/class/tsync_pcr/tsync_vpts_adjust"
#define TSYNC_PCR_MODE "/sys/class/tsync_pcr/tsync_pcr_mode"
#define TSYNC_FIRSTCHECKIN_APTS "/sys/class/tsync/checkin_firstapts"
#define TSYNC_DEMUX_PCR         "/sys/class/tsync/demux_pcr"
#define TSYNC_LASTCHECKIN_APTS "/sys/class/tsync/last_checkin_apts"
#define TSYNC_LASTCHECKIN_VPTS "/sys/class/tsync/checkin_firstvpts"
#define TSYNC_PCR_LANTCY        "/sys/class/tsync/pts_latency"
#define AMSTREAM_AUDIO_PORT_RESET   "/sys/class/amstream/reset_audio_port"
#define VIDEO_FIRST_FRAME_SHOW  "/sys/module/amvideo/parameters/first_frame_toggled"

#define PATCH_PERIOD_COUNT 4
#define DTV_PTS_CORRECTION_THRESHOLD (90000 * 30 / 1000)
#define AUDIO_PTS_DISCONTINUE_THRESHOLD (90000 * 5)
#define AC3_IEC61937_FRAME_SIZE 6144
#define EAC3_IEC61937_FRAME_SIZE 24576
#define DECODER_PTS_DEFAULT_LATENCY (200 * 90)
#define DECODER_PTS_MAX_LATENCY (320 * 90)
#define DEMUX_PCR_APTS_LATENCY (300 * 90)
#define DEFAULT_ARC_DELAY_MS (100)
#define PCM 0  /*AUDIO_FORMAT_PCM_16_BIT*/
#define DD 4   /*AUDIO_FORMAT_AC3*/
#define AUTO 5 /*choose by sink capability/source format/Digital format*/

#define DEFAULT_DTV_OUTPUT_CLOCK    (1000*1000)
#define DEFAULT_DTV_ADJUST_CLOCK    (1000)
#define DEFALUT_DTV_MIN_OUT_CLOCK   (1000*1000-100*1000)
#define DEFAULT_DTV_MAX_OUT_CLOCK   (1000*1000+100*1000)
#define DEFAULT_I2S_OUTPUT_CLOCK    (256*48000)
#define DEFAULT_CLOCK_MUL    (4)
#define DEFAULT_SPDIF_PLL_DDP_CLOCK    (256*48000*2)
#define DEFAULT_SPDIF_ADJUST_TIMES    (40)
#define DTV_DECODER_PTS_LOOKUP_PATH "/sys/class/tsync/apts_lookup"
#define DTV_DECODER_CHECKIN_FIRSTAPTS_PATH "/sys/class/tsync/checkin_firstapts"
#define DTV_DECODER_TSYNC_MODE      "/sys/class/tsync/mode"
#define PROPERTY_LOCAL_ARC_LATENCY   "media.amnuplayer.audio.delayus"
#define PROPERTY_LOCAL_PASSTHROUGH_LATENCY  "media.dtv.passthrough.latencyms"
#define AUDIO_EAC3_FRAME_SIZE 16
#define AUDIO_AC3_FRAME_SIZE 4
#define AUDIO_TV_PCM_FRAME_SIZE 32
#define AUDIO_DEFAULT_PCM_FRAME_SIZE 4

//pthread_mutex_t dtv_patch_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dtv_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
struct cmd_list {
    struct cmd_list *next;
    int cmd;
    int cmd_num;
    int used;
    int initd;
};

struct cmd_list dtv_cmd_list = {
    .next = NULL,
    .cmd = -1,
    .cmd_num = 0,
    .used = 1,
};

enum {
    AVSYNC_ACTION_NORMAL,
    AVSYNC_ACTION_DROP,
    AVSYNC_ACTION_HOLD,
};
enum {
    DIRECT_SPEED = 0, // DERIECT_SPEED
    DIRECT_SLOW,
    DIRECT_NORMAL,
};
enum tsync_mode_e {
    TSYNC_MODE_VMASTER,
    TSYNC_MODE_AMASTER,
    TSYNC_MODE_PCRMASTER,
};
static struct timespec start_time;

const unsigned int mute_dd_frame[] = {
    0x5d9c770b, 0xf0432014, 0xf3010713, 0x2020dc62, 0x4842020, 0x57100404, 0xf97c3e1f, 0x9fcfe7f3, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0x7c3e9fcf, 0xcfe7f3f9, 0xfb7c3e9f, 0xf97c75fe, 0x9fcfe7f3,
    0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0x7c3e9fcf, 0xcfe7f3f9, 0xfb7c3e9f, 0x3e5f9dff, 0xe7f3f97c, 0x7c3e9fcf, 0xcfe7f3f9, 0xf97c3e9f, 0x9fcfe7f3, 0xf3f97c3e, 0x3e9fcfe7, 0x48149ff2, 0x2091,
    0x361e0000, 0x78bc6ddb, 0xbbbbe3f1, 0xb8, 0x0, 0x0, 0x0, 0x77770700, 0x361e8f77, 0x359f6fdb, 0xd65a6bad, 0x5a6badb5, 0x6badb5d6, 0xa0b5d65a, 0x1e000000, 0xbc6ddb36,
    0xbbe3f178, 0xb8bb, 0x0, 0x0, 0x0, 0x77070000, 0x1e8f7777, 0x9f6fdb36, 0x5a6bad35, 0xa6b5d6, 0x0, 0xb66de301, 0x1e8fc7db, 0x80bbbb3b, 0x0, 0x0,
    0x0, 0x0, 0x78777777, 0xb66de3f1, 0xd65af3f9, 0x5a6badb5, 0x6badb5d6, 0xadb5d65a, 0x5a6b, 0x6de30100, 0x8fc7dbb6, 0xbbbb3b1e, 0x80, 0x0, 0x0, 0x0,
    0x77777700, 0x6de3f178, 0x5af3f9b6, 0x6badb5d6, 0x605a, 0x1e000000, 0xbc6ddb36, 0xbbe3f178, 0xb8bb, 0x0, 0x0, 0x0, 0x77070000, 0x1e8f7777, 0x9f6fdb36, 0x5a6bad35,
    0x6badb5d6, 0xadb5d65a, 0xb5d65a6b, 0xa0, 0x6ddb361e, 0xe3f178bc, 0xb8bbbb, 0x0, 0x0, 0x0, 0x7000000, 0x8f777777, 0x6fdb361e, 0x6bad359f, 0xa6b5d65a, 0x0,
    0x6de30100, 0x8fc7dbb6, 0xbbbb3b1e, 0x80, 0x0, 0x0, 0x0, 0x77777700, 0x6de3f178, 0x5af3f9b6, 0x6badb5d6, 0xadb5d65a, 0xb5d65a6b, 0x5a6bad, 0xe3010000, 0xc7dbb66d,
    0xbb3b1e8f, 0x80bb, 0x0, 0x0, 0x0, 0x77770000, 0xe3f17877, 0xf3f9b66d, 0xadb5d65a, 0x605a6b, 0x0, 0x6ddb361e, 0xe3f178bc, 0xb8bbbb, 0x0, 0x0,
    0x0, 0x7000000, 0x8f777777, 0x6fdb361e, 0x6bad359f, 0xadb5d65a, 0xb5d65a6b, 0xd65a6bad, 0xa0b5, 0xdb361e00, 0xf178bc6d, 0xb8bbbbe3, 0x0, 0x0, 0x0, 0x0,
    0x77777707, 0xdb361e8f, 0xad359f6f, 0xb5d65a6b, 0x10200a6, 0x0, 0xdbb6f100, 0x8fc7e36d, 0xc0dddd1d, 0x0, 0x0, 0x0, 0x0, 0xbcbbbb3b, 0xdbb6f178, 0x6badf97c,
    0xadb5d65a, 0xb5d65a6b, 0xd65a6bad, 0xadb5, 0xb6f10000, 0xc7e36ddb, 0xdddd1d8f, 0xc0, 0x0, 0x0, 0x0, 0xbbbb3b00, 0xb6f178bc, 0xadf97cdb, 0xb5d65a6b, 0x4deb00ad
};

const unsigned int mute_ddp_frame[] = {
    0x7f01770b, 0x20e06734, 0x2004, 0x8084500, 0x404046c, 0x1010104, 0xe7630001, 0x7c3e9fcf, 0xcfe7f3f9, 0xf97c3e9f, 0x9fcfe7f3, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0xce7f9fcf, 0x7c3e9faf,
    0xcfe7f3f9, 0xf97c3e9f, 0x9fcfe7f3, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0xf37f9fcf, 0x9fcfe7ab, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0x7c3e9fcf, 0xcfe7f3f9, 0xf97c3e9f, 0x53dee7f3, 0xf0e9,
    0x6d3c0000, 0xf178dbb6, 0x7777c7e3, 0x70, 0x0, 0x0, 0x0, 0xeeee0e00, 0x6d3c1eef, 0x6b3edfb6, 0xadb5d65a, 0xb5d65a6b, 0xd65a6bad, 0x406badb5, 0x3c000000, 0x78dbb66d,
    0x77c7e3f1, 0x7077, 0x0, 0x0, 0x0, 0xee0e0000, 0x3c1eefee, 0x3edfb66d, 0xb5d65a6b, 0x20606bad, 0x0, 0xdbb66d3c, 0xc7e3f178, 0x707777, 0x0, 0x0,
    0x0, 0xe000000, 0x1eefeeee, 0xdfb66d3c, 0xd65a6b3e, 0x5a6badb5, 0x6badb5d6, 0xadb5d65a, 0x406b, 0xb66d3c00, 0xe3f178db, 0x707777c7, 0x0, 0x0, 0x0, 0x0,
    0xefeeee0e, 0xb66d3c1e, 0x5a6b3edf, 0x6badb5d6, 0x2060, 0x6d3c0000, 0xf178dbb6, 0x7777c7e3, 0x70, 0x0, 0x0, 0x0, 0xeeee0e00, 0x6d3c1eef, 0x6b3edfb6, 0xadb5d65a,
    0xb5d65a6b, 0xd65a6bad, 0x406badb5, 0x3c000000, 0x78dbb66d, 0x77c7e3f1, 0x7077, 0x0, 0x0, 0x0, 0xee0e0000, 0x3c1eefee, 0x3edfb66d, 0xb5d65a6b, 0x20606bad, 0x0,
    0xdbb66d3c, 0xc7e3f178, 0x707777, 0x0, 0x0, 0x0, 0xe000000, 0x1eefeeee, 0xdfb66d3c, 0xd65a6b3e, 0x5a6badb5, 0x6badb5d6, 0xadb5d65a, 0x406b, 0xb66d3c00, 0xe3f178db,
    0x707777c7, 0x0, 0x0, 0x0, 0x0, 0xefeeee0e, 0xb66d3c1e, 0x5a6b3edf, 0x6badb5d6, 0x2060, 0x6d3c0000, 0xf178dbb6, 0x7777c7e3, 0x70, 0x0, 0x0,
    0x0, 0xeeee0e00, 0x6d3c1eef, 0x6b3edfb6, 0xadb5d65a, 0xb5d65a6b, 0xd65a6bad, 0x406badb5, 0x3c000000, 0x78dbb66d, 0x77c7e3f1, 0x7077, 0x0, 0x0, 0x0, 0xee0e0000,
    0x3c1eefee, 0x3edfb66d, 0xb5d65a6b, 0x20606bad, 0x0, 0xdbb66d3c, 0xc7e3f178, 0x707777, 0x0, 0x0, 0x0, 0xe000000, 0x1eefeeee, 0xdfb66d3c, 0xd65a6b3e, 0x5a6badb5,
    0x6badb5d6, 0xadb5d65a, 0x406b, 0xb66d3c00, 0xe3f178db, 0x707777c7, 0x0, 0x0, 0x0, 0x0, 0xefeeee0e, 0xb66d3c1e, 0x5a6b3edf, 0x6badb5d6, 0x40, 0x7f227c55,
};
static int dtv_get_tsync_mode(void);
static int create_dtv_output_stream_thread(struct aml_audio_patch *patch);
static int release_dtv_output_stream_thread(struct aml_audio_patch *patch);
extern int calc_time_interval_us(struct timespec *ts0, struct timespec *ts1);
extern size_t aml_alsa_output_write(struct audio_stream_out *stream, void *buffer, size_t bytes);
struct cmd_list cmd_array[16]; // max cache 16 cmd;

static int sysfs_get_systemfs_str(const char *path, char *val, int len)
{
    int fd;
    int bytes;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(val, 0, len);
        read(fd, val, len);
        close(fd);
        return 0;
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}

static unsigned long decoder_apts_lookup(unsigned int offset)
{
    unsigned int pts = 0;
    int ret;
    char buff[32];
    memset(buff, 0, 32);
    snprintf(buff, 32, "%d", offset);

    aml_sysfs_set_str(DTV_DECODER_PTS_LOOKUP_PATH, buff);
    ret = aml_sysfs_get_str(DTV_DECODER_PTS_LOOKUP_PATH, buff, sizeof(buff));

    if (ret > 0) {
        ret = sscanf(buff, "0x%x\n", &pts);
    }
    // ALOGI("decoder_apts_lookup get the pts is %x\n", pts);
    if (pts == (unsigned int) - 1) {
        pts = 0;
    }

    // adec_print("adec_apts_lookup get the pts is %lx\n", pts);

    return (unsigned long)pts;
}

static unsigned long decoder_checkin_firstapts(void)
{
    unsigned int firstapts = 0;
    int ret;
    char buff[32];

    memset(buff, 0, 32);
    ret = aml_sysfs_get_str(DTV_DECODER_CHECKIN_FIRSTAPTS_PATH, buff, sizeof(buff));

    if (ret > 0) {
        ret = sscanf(buff, "0x%x\n", &firstapts);
    }

    if (firstapts == (unsigned int) - 1) {
        firstapts = 0;
    }

    return (unsigned long)firstapts;
}

static void decoder_set_latency(unsigned int latency)
{
    char tempbuf[128];
    memset(tempbuf, 0, 128);
    sprintf(tempbuf, "%d", latency);
    if (aml_sysfs_set_str(TSYNC_PCR_LANTCY, tempbuf) == -1) {
        ALOGE("set pcr lantcy failed %s\n", tempbuf);
    }
    return;
}

static unsigned int decoder_get_latency(void)
{
    unsigned int latency = 0;
    int ret;
    char buff[64];
    memset(buff, 0, 64);
    ret = aml_sysfs_get_str(TSYNC_PCR_LANTCY, buff, sizeof(buff));
    if (ret > 0) {
        ret = sscanf(buff, "%u\n", &latency);
    }
    //ALOGI("get lantcy %d", latency);
    return (unsigned int)latency;
}

static void decoder_stop_audio(struct aml_audio_patch *patch)
{
    char buff[64];
    if (patch == NULL ||
        patch->tsync_mode == TSYNC_MODE_PCRMASTER) {
        return;
    }
    sprintf(buff, "AUDIO_STOP");
    if (sysfs_set_sysfs_str(TSYNC_EVENT, buff) == -1) {
        ALOGE("set AUDIO_STOP failed \n");
    }
}

static void decoder_set_pcrsrc(unsigned int pcrsrc)
{
    char tempbuf[128];
    memset(tempbuf, 0, 128);
    sprintf(tempbuf, "0x%x", pcrsrc);
    if (aml_sysfs_set_str(TSYNC_PCRSCR, tempbuf) == -1) {
        ALOGE("set pcr lantcy failed %s\n", tempbuf);
    }
    return;
}

static int get_dtv_audio_mode(void)
{
    int ret, mode = 0;
    char buff[64];
    ret = aml_sysfs_get_str(TSYNC_AUDIO_MODE, buff, sizeof(buff));
    if (ret > 0) {
        ret = sscanf(buff, "%d", &mode);
    }
    return mode;
}

static int get_dtv_pcr_sync_mode(void)
{
    int ret, mode = 0;
    char buff[64];
    ret = aml_sysfs_get_str(TSYNC_PCR_MODE, buff, sizeof(buff));
    if (ret > 0) {
        ret = sscanf(buff, "%d", &mode);
    }
    return mode;
}

static int get_dtv_apts_gap(void)
{
    int ret, diff = 0;
    char buff[32];
    ret = aml_sysfs_get_str(TSYNC_APTS_DIFF, buff, sizeof(buff));
    if (ret > 0) {
        ret = sscanf(buff, "%d\n", &diff);
    } else {
        diff = 0;
    }
    return diff;
}

static int get_tsync_pcr_debug(void)
{
    char tempbuf[128];
    int debug = 0, ret;
    ret = aml_sysfs_get_str(TSYNC_PCR_DEBUG, tempbuf, sizeof(tempbuf));
    if (ret > 0) {
        ret = sscanf(tempbuf, "%d\n", &debug);
    }
    if (ret > 0 && debug > 0) {
        return debug;
    } else {
        debug = 0;
    }
    return debug;
}

static int get_video_delay(void)
{
    char tempbuf[128];
    int vpts = 0, ret;
    ret = aml_sysfs_get_str(TSYNC_VPTS_ADJ, tempbuf, sizeof(tempbuf));
    if (ret > 0) {
        ret = sscanf(tempbuf, "%d\n", &vpts);
    }
    if (ret > 0) {
        return vpts;
    } else {
        vpts = 0;
    }
    return vpts;
}

static void set_video_delay(int delay_ms)
{
    char tempbuf[128];
    memset(tempbuf, 0, 128);
    if (delay_ms < -100 || delay_ms > 500) {
        ALOGE("set_video_delay out of range[-100 - 500] %d\n", delay_ms);
        return;
    }
    sprintf(tempbuf, "%d", delay_ms);
    if (aml_sysfs_set_str(TSYNC_VPTS_ADJ, tempbuf) == -1) {
        ALOGE("set_video_delay %s\n", tempbuf);
    }
    return;
}

static void clean_dtv_patch_pts(struct aml_audio_patch *patch)
{
    if (patch) {
        patch->last_apts = 0;
        patch->last_pcrpts = 0;
    }
}

static int get_video_discontinue(void)
{
    char tempbuf[128];
    int pcr_vdiscontinue = 0, ret;
    ret = aml_sysfs_get_str(TSYNC_VIDEO_DISCONT, tempbuf, sizeof(tempbuf));
    if (ret > 0) {
        ret = sscanf(tempbuf, "%d\n", &pcr_vdiscontinue);
    }
    if (ret > 0 && pcr_vdiscontinue > 0) {
        pcr_vdiscontinue = (pcr_vdiscontinue & 0xff);
    } else {
        pcr_vdiscontinue = 0;
    }
    return pcr_vdiscontinue;
}

static int get_audio_discontinue(struct aml_audio_patch *patch)
{
    char tempbuf[128];
    int a_discontinue = 0, ret;
    if (patch->tsync_mode != TSYNC_MODE_PCRMASTER) {
        return a_discontinue;
    }
    ret = aml_sysfs_get_str(TSYNC_AUDIO_LEVEL, tempbuf, sizeof(tempbuf));
    if (ret > 0) {
        ret = sscanf(tempbuf, "%d\n", &a_discontinue);
    }
    if (ret > 0 && a_discontinue > 0) {
        a_discontinue = (a_discontinue & 0xff);
    } else {
        a_discontinue = 0;
    }
    return a_discontinue;
}



static void init_cmd_list(void)
{
    dtv_cmd_list.next = NULL;
    dtv_cmd_list.cmd = -1;
    dtv_cmd_list.cmd_num = 0;
    dtv_cmd_list.used = 0;
    dtv_cmd_list.initd = 1;
    memset(cmd_array, 0, sizeof(cmd_array));
}

static void deinit_cmd_list(void)
{
    dtv_cmd_list.next = NULL;
    dtv_cmd_list.cmd = -1;
    dtv_cmd_list.cmd_num = 0;
    dtv_cmd_list.used = 0;
    dtv_cmd_list.initd = 0;
}

static struct cmd_list *cmd_array_get(void)
{
    int index = 0;

    pthread_mutex_lock(&dtv_cmd_mutex);
    for (index = 0; index < 16; index++) {
        if (cmd_array[index].used == 0) {
            break;
        }
    }

    if (index == 16) {
        pthread_mutex_unlock(&dtv_cmd_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&dtv_cmd_mutex);
    return &cmd_array[index];
}
static void cmd_array_put(struct cmd_list *list)
{
    pthread_mutex_lock(&dtv_cmd_mutex);
    list->used = 0;
    pthread_mutex_unlock(&dtv_cmd_mutex);
}

static void _add_cmd_to_tail(struct cmd_list *node)
{
    struct cmd_list *list = &dtv_cmd_list;
    pthread_mutex_lock(&dtv_cmd_mutex);
    while (list->next != NULL) {
        list = list->next;
    }
    list->next = node;
    dtv_cmd_list.cmd_num++;
    pthread_mutex_unlock(&dtv_cmd_mutex);
}

int dtv_patch_add_cmd(int cmd)
{
    struct cmd_list *list = NULL;
    struct cmd_list *cmd_list = NULL;
    int index = 0;
    if (dtv_cmd_list.initd == 0) {
        return 0;
    }
    pthread_mutex_lock(&dtv_cmd_mutex);
    for (index = 0; index < 16; index++) {
        if (cmd_array[index].used == 0) {
            break;
        }
    }
    if (index == 16) {
        pthread_mutex_unlock(&dtv_cmd_mutex);
        ALOGI("list is full, add by live \n");
        return -1;
    }
    cmd_list = &cmd_array[index];
    if (cmd_list == NULL) {
        pthread_mutex_unlock(&dtv_cmd_mutex);
        ALOGI("can't get cmd list, add by live \n");
        return -1;
    }
    cmd_list->cmd = cmd;
    cmd_list->next = NULL;
    cmd_list->used = 1;
    list = &dtv_cmd_list;
    while (list->next != NULL) {
        list = list->next;
    }
    list->next = cmd_list;
    dtv_cmd_list.cmd_num++;
    pthread_mutex_unlock(&dtv_cmd_mutex);
    ALOGI("add by live dtv_patch_add_cmd the cmd is %d \n", cmd);
    return 0;
}

int dtv_patch_get_cmd(void)
{
    int cmd = AUDIO_DTV_PATCH_CMD_NUM;
    struct cmd_list *list = NULL;
    ALOGI("enter dtv_patch_get_cmd funciton now\n");
    pthread_mutex_lock(&dtv_cmd_mutex);
    list = dtv_cmd_list.next;
    if (list != NULL) {
        dtv_cmd_list.next = list->next;
        cmd = list->cmd;
        dtv_cmd_list.cmd_num--;
    } else {
        cmd =  AUDIO_DTV_PATCH_CMD_NULL;
        pthread_mutex_unlock(&dtv_cmd_mutex);
        return cmd;
    }
    list->used = 0;
    pthread_mutex_unlock(&dtv_cmd_mutex);
    ALOGI("leave dtv_patch_get_cmd the cmd is %d \n", cmd);
    return cmd;
}
int dtv_patch_cmd_is_empty(void)
{
    pthread_mutex_lock(&dtv_cmd_mutex);
    if (dtv_cmd_list.next == NULL) {
        pthread_mutex_unlock(&dtv_cmd_mutex);
        return 1;
    }
    pthread_mutex_unlock(&dtv_cmd_mutex);
    return 0;
}

static int dtv_patch_status_info(void *args, INFO_TYPE_E info_flag)
{
    int ret = 0;
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    if (info_flag == BUFFER_SPACE)
        ret = get_buffer_write_space(ringbuffer);
    else if (info_flag == BUFFER_LEVEL)
        ret = get_buffer_read_space(ringbuffer);
    else if (info_flag == AD_MIXING_LEVLE)
        ret = aml_dev->mixing_level;
    return ret;
}

static int dtv_patch_audio_info(void *args, unsigned char ori_channum, unsigned char lfepresent)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    patch->dtv_NchOriginal = ori_channum;
    patch->dtv_lfepresent = lfepresent;
    return 1;
}


unsigned long dtv_hal_get_pts(struct aml_audio_patch *patch,
                              unsigned int lantcy)
{
    unsigned long val, offset;
    unsigned long pts;
    int data_width, channels, samplerate;
    unsigned long long frame_nums;
    unsigned long delay_pts;
    char value[PROPERTY_VALUE_MAX];

    channels = 2;
    samplerate = 48;
    data_width = 2;

    offset = patch->decoder_offset;

    // when first  look up apts,set offset 0
    if (patch->dtv_first_apts_flag == 0) {
        offset = 0;
    }
    offset = decoder_apts_lookup(offset);

    pts = offset;
    if (patch->dtv_first_apts_flag == 0) {
        if (pts == 0) {
            pts = decoder_checkin_firstapts();
            ALOGI("pts = 0,so get checkin_firstapts:0x%lx", pts);
        }
        patch->last_valid_pts = pts;
        return pts;
    }

    if (pts == 0 || pts == patch->last_valid_pts) {
        if (patch->last_valid_pts) {
            pts = patch->last_valid_pts;
        }
        frame_nums = (patch->outlen_after_last_validpts / (data_width * channels));
        pts += (frame_nums * 90 / samplerate);
        //ALOGI("decode_offset:%d out_pcm:%d   pts:%lx,audec->last_valid_pts %lx\n",
        //      patch->decoder_offset, patch->outlen_after_last_validpts, pts,
        //      patch->last_valid_pts);
        return 0;
    }
    val = pts - lantcy * 90;
    /*+[SE][BUG][SWPL-14811][zhizhong] set the real apts to last_valid_pts for sum cal*/
    patch->last_valid_pts = val;
    patch->outlen_after_last_validpts = 0;
    // ALOGI("====get pts:%lx offset:%d lan %d origin_apts:%lx\n",
    // val, patch->decoder_offset, lantcy, pts);
    return val;
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    audio_format_t afmt = get_output_format((struct audio_stream_out *)stream);
    const struct aml_stream_out *out = (const struct aml_stream_out *)stream;
    snd_pcm_sframes_t frames = out_get_latency_frames(stream);
    if (afmt == AUDIO_FORMAT_E_AC3 && out->dual_output_flag) {
        return (frames * 1000) / 4 / out->config.rate;
    } else {
        return (frames * 1000) / out->config.rate;
    }
}

static void dtv_do_ease_out(struct aml_audio_device *aml_dev)
{
    if (aml_dev && aml_dev->audio_ease) {
        ALOGI("%s(), do fade out", __func__);
        start_ease_out(aml_dev);
        //usleep(200 * 1000);
    }
}

static int dtv_get_tsync_mode(void)
{
    char tsync_mode_str[12];
    int tsync_mode = TSYNC_MODE_PCRMASTER;
    char buf[64];
    if (sysfs_get_systemfs_str(DTV_DECODER_TSYNC_MODE, buf, sizeof(buf)) == -1) {
        ALOGI("dtv_get_tsync_mode error");
        return tsync_mode;
    }
    if (sscanf(buf, "%d: %s", &tsync_mode, tsync_mode_str) < 1) {
        ALOGI("dtv_get_tsync_mode error.");
        return tsync_mode;
    }
    if (tsync_mode > TSYNC_MODE_PCRMASTER || tsync_mode < TSYNC_MODE_VMASTER) {
        tsync_mode = TSYNC_MODE_PCRMASTER;
    }
    return tsync_mode;
}

static unsigned int compare_clock(unsigned int clock1, unsigned int clock2)
{
    if (clock1 == clock2) {
        return 1 ;
    }
    if (clock1 > clock2) {
        if (clock1 < clock2 + 60) {
            return 1;
        }
    }
    if (clock1 < clock2) {
        if (clock2 < clock1 + 60) {
            return 1;
        }
    }
    return 0;
}

static void dtv_adjust_i2s_output_clock(struct aml_audio_patch* patch, int direct, int step)
{
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device * aml_dev = (struct aml_audio_device*)adev;
    struct aml_mixer_handle * handle = &(aml_dev->alsa_mixer);
    int output_clock = 0;
    unsigned int i2s_current_clock = 0;
    i2s_current_clock = aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL);
    if (i2s_current_clock > DEFAULT_I2S_OUTPUT_CLOCK * 4 ||
        i2s_current_clock == 0 || step <= 0 || step > DEFAULT_DTV_OUTPUT_CLOCK) {
        return;
    }
    if (get_tsync_pcr_debug())
        ALOGI("current:%d, default:%d\n", i2s_current_clock, patch->dtv_default_i2s_clock);
    if (direct == DIRECT_SPEED) {
        if (i2s_current_clock >= patch->dtv_default_i2s_clock) {
            if (i2s_current_clock - patch->dtv_default_i2s_clock >=
                DEFAULT_DTV_OUTPUT_CLOCK) {
                ALOGI("already > i2s_step_clk 1M,no need speed adjust\n");
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + step;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
        } else {
            int value = patch->dtv_default_i2s_clock - i2s_current_clock;
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + value;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
        }
    } else if (direct == DIRECT_SLOW) {
        if (i2s_current_clock <= patch->dtv_default_i2s_clock) {
            if (patch->dtv_default_i2s_clock - i2s_current_clock >
                DEFAULT_DTV_OUTPUT_CLOCK) {
                ALOGI("alread < 1M no need adjust slow, return\n");
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - step;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
        } else {
            int value = i2s_current_clock - patch->dtv_default_i2s_clock;
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - value;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - step;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
        }
    } else {
        if (compare_clock(i2s_current_clock, patch->dtv_default_i2s_clock)) {
            return ;
        }
        if (i2s_current_clock > patch->dtv_default_i2s_clock) {
            int value = i2s_current_clock - patch->dtv_default_i2s_clock;
            if (value < 60) {
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - value;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
        } else if (i2s_current_clock < patch->dtv_default_i2s_clock) {
            int value = patch->dtv_default_i2s_clock - i2s_current_clock;
            if (value < 60) {
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + value;
            aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_I2S_PLL, output_clock);
        } else {
            return ;
        }
    }
    return;
}

static void dtv_adjust_spdif_output_clock(struct aml_audio_patch* patch, int direct, int step)
{
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    struct aml_mixer_handle * handle = &(aml_dev->alsa_mixer);
    int output_clock, i;
    unsigned int spidif_current_clock = 0;
    spidif_current_clock = aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL);
    if (spidif_current_clock > DEFAULT_SPDIF_PLL_DDP_CLOCK * 4 ||
        spidif_current_clock == 0 || step <= 0 || step > DEFAULT_DTV_OUTPUT_CLOCK ||
        aml_dev->reset_dtv_audio == 1) {
        return;
    }
    if (direct == DIRECT_SPEED) {
        if (compare_clock(spidif_current_clock, patch->dtv_default_spdif_clock)) {
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + step / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            //ALOGI("spidif_clock 1 set %d to %d",spidif_current_clock,aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL));
        } else if (spidif_current_clock < patch->dtv_default_spdif_clock) {
            int value = patch->dtv_default_spdif_clock - spidif_current_clock;
            if (value > DEFAULT_DTV_OUTPUT_CLOCK) {
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + value / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + step / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            //ALOGI("spidif_clock 2 set %d to %d",spidif_current_clock,aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL));

        } else {
            //ALOGI("spdif_SPEED clk %d,default %d",spidif_current_clock,patch->dtv_default_spdif_clock);
            return ;
        }
    } else if (direct == DIRECT_SLOW) {
        if (compare_clock(spidif_current_clock, patch->dtv_default_spdif_clock)) {
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - step / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            //ALOGI("spidif_clock 3 set %d to %d",spidif_current_clock,aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL));
        } else if (spidif_current_clock > patch->dtv_default_spdif_clock) {
            int value = spidif_current_clock - patch->dtv_default_spdif_clock;
            if (value > DEFAULT_DTV_OUTPUT_CLOCK) {
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - value / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - step / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            //ALOGI("spidif_clock 4 set %d to %d",spidif_current_clock,aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL));
        } else {
            //ALOGI("spdif_SLOW clk %d,default %d",spidif_current_clock,patch->dtv_default_spdif_clock);
            return ;
        }
    } else {
        if (compare_clock(spidif_current_clock, patch->dtv_default_spdif_clock)) {
            return ;
        }
        if (spidif_current_clock > patch->dtv_default_spdif_clock) {
            int value = spidif_current_clock - patch->dtv_default_spdif_clock;
            if (value < 60 || value > DEFAULT_DTV_OUTPUT_CLOCK) {
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK - value / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            //ALOGI("spidif_clock 5 set %d to %d",spidif_current_clock,aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL));
        } else if (spidif_current_clock < patch->dtv_default_spdif_clock) {
            int value = patch->dtv_default_spdif_clock - spidif_current_clock;
            if (value < 60 || value > DEFAULT_DTV_OUTPUT_CLOCK) {
                return;
            }
            output_clock = DEFAULT_DTV_OUTPUT_CLOCK + value / DEFAULT_SPDIF_ADJUST_TIMES;
            for (i = 0; i < DEFAULT_SPDIF_ADJUST_TIMES; i++) {
                aml_mixer_ctrl_set_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL, output_clock);
            }
            //ALOGI("spidif_clock 6 set %d to %d",spidif_current_clock,aml_mixer_ctrl_get_int(handle, AML_MIXER_ID_CHANGE_SPIDIF_PLL));
        } else {
            return ;
        }
    }
}
static void dtv_adjust_output_clock(struct aml_audio_patch * patch, int direct, int step)
{
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    //ALOGI("dtv_adjust_output_clock not set,%x,%x",patch->decoder_offset,patch->dtv_pcm_readed);
    if (!aml_dev || step <= 0) {
        return;
    }
    if (patch->decoder_offset < 512 * 2 * 10 &&
        ((patch->aformat == AUDIO_FORMAT_AC3) ||
         (patch->aformat == AUDIO_FORMAT_E_AC3))) {
        return;
    }
    if (patch->dtv_default_spdif_clock > DEFAULT_I2S_OUTPUT_CLOCK * 4 ||
        patch->dtv_default_spdif_clock == 0) {
        return;
    }
    patch->pll_state = direct;
    if (direct == DIRECT_SPEED) {
        clock_gettime(CLOCK_MONOTONIC, &(patch->speed_time));
    } else if (direct == DIRECT_SLOW) {
        clock_gettime(CLOCK_MONOTONIC, &(patch->slow_time));
    }
    if (patch->spdif_format_set == 0) {
        if (patch->dtv_default_i2s_clock > DEFAULT_SPDIF_PLL_DDP_CLOCK * 4 ||
            patch->dtv_default_i2s_clock == 0) {
            return;
        }
        dtv_adjust_i2s_output_clock(patch, direct, patch->i2s_step_clk /
                                                    patch->i2s_div_factor);
    } else if (!aml_dev->bHDMIARCon && aml_dev->is_TV) {
        if (patch->dtv_default_i2s_clock > DEFAULT_SPDIF_PLL_DDP_CLOCK * 4 ||
            patch->dtv_default_i2s_clock == 0) {
            return;
        }
        dtv_adjust_i2s_output_clock(patch, direct, patch->i2s_step_clk);
        dtv_adjust_spdif_output_clock(patch, direct, patch->spdif_step_clk);
    } else {
        dtv_adjust_spdif_output_clock(patch, direct, patch->spdif_step_clk / 4);
    }
}

static void dtv_adjust_output_clock_continue(struct aml_audio_patch * patch, int direct)
{
    struct timespec current_time;
    int time_cost = 0;
    static int last_div = 0;
    int adjust_interval = 0;
    patch->i2s_div_factor = property_get_int32("media.audio_hal.div", 480);
    adjust_interval = property_get_int32("media.audio_hal.adjtime", 1000);
    if (last_div != patch->i2s_div_factor) {
        ALOGI("new_div=%d, adjust_interval=%d ms,spdif_format_set=%d\n",
            patch->i2s_div_factor, adjust_interval, patch->spdif_format_set);
        last_div = patch->i2s_div_factor;
    }
    if (patch->pll_state == DIRECT_NORMAL || patch->pll_state != direct) {
        ALOGI("pll_state=%d, direct=%d no need continue\n", patch->pll_state, direct);
        return;
    }
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    if (direct == DIRECT_SPEED) {
        time_cost = calc_time_interval_us(&patch->speed_time, &current_time)/1000;
    } else if (direct == DIRECT_SLOW) {
        time_cost = calc_time_interval_us(&patch->slow_time, &current_time)/1000;
    }
    if (time_cost > adjust_interval  && patch->spdif_format_set == 0) {
        ALOGI("over %d ms continue to adjust the clock\n", time_cost);
        dtv_adjust_output_clock(patch, direct, DEFAULT_DTV_ADJUST_CLOCK);
    }
    return;
}

static unsigned int dtv_calc_pcrpts_latency(struct aml_audio_patch *patch, unsigned int pcrpts)
{
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    if (aml_dev->bHDMIARCon == 0 || aml_dev->hdmi_format == PCM) {
        return pcrpts;
    } else {
        return pcrpts + DEFAULT_ARC_DELAY_MS * 90;
    }
    if (aml_dev->bHDMIARCon && aml_dev->hdmi_format == PCM) {
        if (patch->aformat == AUDIO_FORMAT_E_AC3) {
            pcrpts += 2 * DTV_PTS_CORRECTION_THRESHOLD;
        } else {
            pcrpts + DTV_PTS_CORRECTION_THRESHOLD;
        }
    } else if (eDolbyMS12Lib == aml_dev->dolby_lib_type && aml_dev->bHDMIARCon) {
        if (patch->aformat == AUDIO_FORMAT_E_AC3 && !aml_dev->disable_pcm_mixing) {
            pcrpts += 8 * DTV_PTS_CORRECTION_THRESHOLD;
        } else if (patch->aformat == AUDIO_FORMAT_E_AC3 && aml_dev->disable_pcm_mixing) {
            pcrpts += 6 * DTV_PTS_CORRECTION_THRESHOLD;
        } else {
            pcrpts += 3 * DTV_PTS_CORRECTION_THRESHOLD;
        }
    } else if (eDolbyDcvLib == aml_dev->dolby_lib_type && aml_dev->bHDMIARCon) {
        if (patch->aformat == AUDIO_FORMAT_E_AC3) {
            pcrpts += 4 * DTV_PTS_CORRECTION_THRESHOLD;
        } else {
            pcrpts += DTV_PTS_CORRECTION_THRESHOLD;
        }
    } else {
        pcrpts += DTV_PTS_CORRECTION_THRESHOLD;
    }
    return pcrpts;
}

static int dtv_calc_abuf_level(struct aml_audio_patch *patch, struct aml_stream_out *stream_out)
{
    if (!patch) {
        return 0;
    }
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    int main_avail = 0, min_buf_size;
    main_avail = get_buffer_read_space(ringbuffer);
    if ((patch->aformat == AUDIO_FORMAT_AC3) ||
        (patch->aformat == AUDIO_FORMAT_E_AC3)) {
        min_buf_size = stream_out->ddp_frame_size;
    } else if (patch->aformat == AUDIO_FORMAT_DTS) {
        min_buf_size = 1024;
    } else {
        min_buf_size = 32 * 48 * 4 * 2;
    }
    if (main_avail > min_buf_size) {
        return 1;
    }
    return 0;
}

static void dtv_check_audio_reset(struct aml_audio_device *aml_dev)
{
    ALOGI("reset_dtv_audio = %d\n", aml_dev->reset_dtv_audio);
    if (aml_dev->reset_dtv_audio) {
        ALOGI("dtv_audio_reset %d", aml_dev->reset_dtv_audio);
        aml_sysfs_set_str(AMSTREAM_AUDIO_PORT_RESET, "1");
        aml_dev->reset_dtv_audio = 0;
    }
}

static void dtv_set_pcr_latency(struct aml_audio_patch *patch, int mode)
{
    if (!patch) {
        return;
    }
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    //ALOGI("lib %d,arc %d,fromat %d,disable %d", aml_dev->dolby_lib_type,
    //aml_dev->bHDMIARCon, aml_dev->hdmi_format, aml_dev->disable_pcm_mixing);
    if (mode == 0) {
        if (decoder_get_latency() != DECODER_PTS_DEFAULT_LATENCY) {
            decoder_set_latency(DECODER_PTS_DEFAULT_LATENCY);
        }
        return;
    } else if (mode == 2) {
        if (decoder_get_latency() != DECODER_PTS_DEFAULT_LATENCY / 2) {
            decoder_set_latency(DECODER_PTS_DEFAULT_LATENCY / 2);
        }
        return;
    } else if (mode == 3) {
        if (decoder_get_latency() != DECODER_PTS_DEFAULT_LATENCY / 4) {
            decoder_set_latency(DECODER_PTS_DEFAULT_LATENCY / 4);
        }
        return;
    }
    if (eDolbyMS12Lib == aml_dev->dolby_lib_type &&
        aml_dev->bHDMIARCon && aml_dev->hdmi_format != PCM &&
        aml_dev->disable_pcm_mixing == 0 &&
        patch->aformat == AUDIO_FORMAT_E_AC3) {
        if (decoder_get_latency() != DECODER_PTS_MAX_LATENCY) {
            decoder_set_latency(DECODER_PTS_MAX_LATENCY);
        }
    } else {
        if (decoder_get_latency() != DECODER_PTS_DEFAULT_LATENCY) {
            decoder_set_latency(DECODER_PTS_DEFAULT_LATENCY);
        }
    }
}

static bool dtv_firstapts_lookup_over(struct aml_audio_patch *patch,
                                      struct aml_audio_device *aml_dev, int avail, bool a_discontinue)
{
    char buff[32];
    int ret;
    unsigned int first_checkinapts = 0xffffffff;
    unsigned int demux_pcr = 0xffffffff;
    if (!patch || !aml_dev) {
        return true;
    }
    patch->tsync_mode = dtv_get_tsync_mode();
    if (a_discontinue) {
        ret = aml_sysfs_get_str(TSYNC_LAST_CHECKIN_APTS, buff, sizeof(buff));
        if (ret > 0) {
            ret = sscanf(buff, "0x%x\n", &first_checkinapts);
        }
    } else {
        ret = aml_sysfs_get_str(TSYNC_FIRSTCHECKIN_APTS, buff, sizeof(buff));
        if (ret > 0) {
            ret = sscanf(buff, "0x%x\n", &first_checkinapts);
        }
    }
    if (patch->tsync_mode == TSYNC_MODE_PCRMASTER) {
        ret = aml_sysfs_get_str(TSYNC_PCRSCR, buff, sizeof(buff));
    } else {
        ret = aml_sysfs_get_str(TSYNC_DEMUX_PCR, buff, sizeof(buff));
    }
    if (ret > 0) {
        ret = sscanf(buff, "0x%x\n", &demux_pcr);
    }
    if (get_tsync_pcr_debug()) {
        ALOGI("demux_pcr %x first_apts %x, avail %d, mode %d",
              demux_pcr, first_checkinapts, avail, patch->tsync_mode);
    }
    if (patch->tsync_mode != TSYNC_MODE_PCRMASTER) {
        unsigned diff;
        int buffer_size = 0;
        if (demux_pcr > 300 * 90 && demux_pcr != 0xffffffff) {
            demux_pcr = demux_pcr - 300 * 90;
        } else {
            demux_pcr = 0;
        }
        diff = first_checkinapts - demux_pcr;
        if ((patch->aformat == AUDIO_FORMAT_AC3) ||
            (patch->aformat == AUDIO_FORMAT_E_AC3) ||
            patch->aformat == AUDIO_FORMAT_DTS) {
            buffer_size = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT * 2;
        } else {
            buffer_size = 48 * 4 * (DEMUX_PCR_APTS_LATENCY / 90);
        }
        if (first_checkinapts > demux_pcr && demux_pcr) {
            if (diff < AUDIO_PTS_DISCONTINUE_THRESHOLD) {
                return false;
            } else {
                unsigned diff = demux_pcr - first_checkinapts;
                aml_dev->dtv_droppcm_size = diff * 48 * 2 * 2 / 90;
                ALOGI("now must drop size %d\n", aml_dev->dtv_droppcm_size);
            }
        } else if (!demux_pcr && avail < buffer_size) {
            return false;
        }
    } else if ((first_checkinapts != 0xffffffff) || (demux_pcr != 0xffffffff)) {
        if (demux_pcr == 0 && first_checkinapts != 0) {
            ALOGI("demux pcr not set, wait, tsync_mode=%d, use_tsdemux_pcr=%d\n", dtv_get_tsync_mode(), get_dtv_pcr_sync_mode());
            return false;
        }

        if (patch->dtv_has_video && getprop_bool("vendor.media.audio.syncshow") &&
            patch->first_apts_lookup_over == 0) {
            aml_dev->start_mute_flag = 1;
            ALOGI("start_mute_flag 1.");
        }
        if (first_checkinapts > demux_pcr) {
            unsigned diff = first_checkinapts - demux_pcr;
            if (diff  < AUDIO_PTS_DISCONTINUE_THRESHOLD) {
                return false;
            }
        } else {
            unsigned diff = demux_pcr - first_checkinapts;
            aml_dev->dtv_droppcm_size = diff * 48 * 2 * 2 / 90;
            ALOGI("now must drop size %d\n", aml_dev->dtv_droppcm_size);
        }
    }
    return true;
}

static int dtv_set_audio_latency(int apts_diff)
{
    int ret, diff = 0;
    char buff[32];
    if (apts_diff == 0) {
        ret = aml_sysfs_get_str(TSYNC_APTS_DIFF, buff, sizeof(buff));
        if (ret > 0) {
            ret = sscanf(buff, "%d\n", &diff);
        }
        if (diff > DECODER_PTS_DEFAULT_LATENCY) {
            diff = DECODER_PTS_DEFAULT_LATENCY;
        }
        apts_diff = diff;
    }
    if (apts_diff < DEMUX_PCR_APTS_LATENCY && apts_diff > 0) {
        decoder_set_latency(DEMUX_PCR_APTS_LATENCY - apts_diff);
    } else {
        decoder_set_latency(DEMUX_PCR_APTS_LATENCY);
    }
    return apts_diff;
}
static int dtv_write_mute_frame(struct aml_audio_patch *patch,
                                struct audio_stream_out *stream_out)
{
    unsigned char mixbuffer[EAC3_IEC61937_FRAME_SIZE];
    uint16_t *p16_mixbuff = NULL;
    int main_size = 0, mix_size = 0;
    int dd_bsmod = 0;
    int type = 0;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream_out;
    size_t output_buffer_bytes = 0;
    void *output_buffer = NULL;
#if 0
    struct timespec before_read;
    struct timespec after_read;
    int us = 0;
    clock_gettime(CLOCK_MONOTONIC, &before_read);
#endif
    //package iec61937
    memset(mixbuffer, 0, sizeof(mixbuffer));
    //papbpcpd
    p16_mixbuff = (uint16_t*)mixbuffer;
    p16_mixbuff[0] = 0xf872;
    p16_mixbuff[1] = 0x4e1f;
    if (patch->aformat == AUDIO_FORMAT_AC3) {
        dd_bsmod = 6;
        p16_mixbuff[2] = ((dd_bsmod & 7) << 8) | 1;
        p16_mixbuff[3] = (sizeof(mute_dd_frame) * 8);
    } else {
        dd_bsmod = 12;
        p16_mixbuff[2] = ((dd_bsmod & 7) << 8) | 21;
        p16_mixbuff[3] = sizeof(mute_ddp_frame) * 8;
    }
    mix_size += 8;
    if (patch->aformat == AUDIO_FORMAT_AC3) {
        memcpy(mixbuffer + mix_size, mute_dd_frame, sizeof(mute_dd_frame));
    } else {
        memcpy(mixbuffer + mix_size, mute_ddp_frame, sizeof(mute_ddp_frame));
    }
    if (aml_out->status != STREAM_HW_WRITING ||
        patch->output_thread_exit == 1) {
        ALOGE("dtv_write_mute_frame exit");
        return -1;
    }
    if (aml_dev->sink_format == AUDIO_FORMAT_PCM_16_BIT) {
        type = 0;
    } else if (aml_dev->sink_format != AUDIO_FORMAT_PCM_16_BIT && patch->aformat == AUDIO_FORMAT_E_AC3) {
        memcpy(mixbuffer + mix_size, mute_ddp_frame, sizeof(mute_ddp_frame));
        type = 2;
    } else if (aml_dev->sink_format != AUDIO_FORMAT_PCM_16_BIT && patch->aformat == AUDIO_FORMAT_AC3) {
        memcpy(mixbuffer + mix_size, mute_dd_frame, sizeof(mute_dd_frame));
        type = 1;
    } else {
        type = 0;
    }
    if (type == 2) {
        audio_format_t output_format = AUDIO_FORMAT_E_AC3;
        size_t write_bytes = EAC3_IEC61937_FRAME_SIZE;
        if (audio_hal_data_processing(stream_out, (void*)mixbuffer, write_bytes, &output_buffer, &output_buffer_bytes, output_format) == 0) {
            hw_write(stream_out, output_buffer, output_buffer_bytes, output_format);
        }
    } else if (type == 1) {
        audio_format_t output_format = AUDIO_FORMAT_AC3;
        size_t write_bytes = AC3_IEC61937_FRAME_SIZE;
        if (audio_hal_data_processing(stream_out, (void*)mixbuffer, write_bytes, &output_buffer, &output_buffer_bytes, output_format) == 0) {
            hw_write(stream_out, output_buffer, output_buffer_bytes, output_format);
        }
    } else {
        audio_format_t output_format = AUDIO_FORMAT_PCM_16_BIT;
        size_t write_bytes = AC3_IEC61937_FRAME_SIZE;
        memset(mixbuffer, 0, sizeof(mixbuffer));
        if (audio_hal_data_processing(stream_out, (void*)mixbuffer, write_bytes, &output_buffer, &output_buffer_bytes, output_format) == 0) {
            hw_write(stream_out, output_buffer, output_buffer_bytes, output_format);
        }
    }
#if 0
    clock_gettime(CLOCK_MONOTONIC, &after_read);
    us = calc_time_interval_us(&before_read, &after_read);
    ALOGI("function gap =%d,sink %x,optional %x\n", us, aml_dev->sink_format, aml_dev->optical_format);
#endif
    return 0;
}

static int dtv_get_ac3_frame_size(struct aml_audio_patch *patch, int main_avail)
{
    unsigned char main_head[32];
    int main_frame_size = 0, main_head_offset = 0;
    int ret;

    while (main_frame_size == 0 && main_avail >= (int)sizeof(main_head)) {
        memset(main_head, 0, sizeof(main_head));
        ret = ring_buffer_read(&(patch->aml_ringbuffer), main_head, sizeof(main_head));
        main_frame_size = dcv_decoder_get_framesize(main_head,
                          ret, &main_head_offset);
        main_avail -= ret;
        ALOGI("++%s main_avail %d, main_frame_size %d", __FUNCTION__, main_avail, main_frame_size);
    }
    return main_frame_size;
}

static void dtv_audio_gap_monitor(struct aml_audio_patch *patch)
{
    char buff[32];
    unsigned int first_checkinapts = 0;
    int cur_pts_diff = 0;
    int audio_discontinue = 0;
    int ret;
    if (!patch) {
        return;
    }
    if (patch->tsync_mode != TSYNC_MODE_PCRMASTER) {
        return;
    }
    /*[SE][BUG][OTT-7302][zhizhong.zhang] detect audio discontinue by pts-diff*/
    if ((patch->last_apts != 0  && patch->last_apts != (unsigned long) - 1) &&
        (patch->last_pcrpts != 0  && patch->last_pcrpts != (unsigned long) - 1)) {
        cur_pts_diff = patch->last_pcrpts - patch->last_apts;
        if (audio_discontinue == 0 &&
            abs(cur_pts_diff) > DTV_PTS_CORRECTION_THRESHOLD * 5 &&
            get_video_discontinue() != 1) {
            audio_discontinue = 1;
            ALOGV("cur_pts_diff=%d, diff=%d, apts=0x%x, pcrpts=0x%x\n",
                cur_pts_diff, cur_pts_diff/90, patch->last_apts, patch->last_pcrpts);
        } else
            audio_discontinue = 0;
    }
    if ((audio_discontinue || get_audio_discontinue(patch)) && patch->dtv_audio_tune == AUDIO_RUNNING) {
        //ALOGI("%s size %d", __FUNCTION__, get_buffer_read_space(&(patch->aml_ringbuffer)));
        ret = aml_sysfs_get_str(TSYNC_LAST_CHECKIN_APTS, buff, sizeof(buff));
        if (ret > 0) {
            ret = sscanf(buff, "0x%x\n", &first_checkinapts);
        }
        if (first_checkinapts) {
            patch->dtv_audio_tune = AUDIO_BREAK;
            ALOGI("audio discontinue, audio_break");
        } else if (audio_discontinue == 1) {
            patch->dtv_audio_tune = AUDIO_BREAK;
            ALOGI("audio_discontinue set 1, tune -> AUDIO_BREAK\n");
        }
    }
}

static void dtv_do_drop_pcm(int avail, struct aml_audio_patch *patch,
                            struct audio_stream_out *stream_out)
{
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    struct aml_stream_out *out = (struct aml_stream_out *)stream_out;
    int drop_size, t1, t2;
    if (!patch || !patch->dev || !stream_out) {
        return;
    }
    if (patch->dtv_apts_lookup > 0) {
        drop_size = 48 * 4 * (patch->dtv_apts_lookup / 90);
        if (drop_size > avail) {
            drop_size = avail;
        }
        t1 = drop_size / patch->out_buf_size;
        for (t2 = 0; t2 < t1; t2++) {
            ring_buffer_read(&(patch->aml_ringbuffer), (unsigned char *)patch->out_buf, patch->out_buf_size);
        }
        ALOGI("dtv_do_drop:--drop %d,avail %d,diff %d ms\n", 48 * 4 * (patch->dtv_apts_lookup / 90), avail, (int)(drop_size) / 192);
    } else if (patch->dtv_apts_lookup < 0) {
        memset(patch->out_buf, 0, patch->out_buf_size);
        if (abs(patch->dtv_apts_lookup) / 90 > 1000) {
            t1 = 1000 * 192;
        } else {
            t1 =  192 * abs(patch->dtv_apts_lookup) / 90;
        }
        t2 = t1 / patch->out_buf_size;
        t1 = t1 & ~3;
        ALOGI("dtv_do_drop:++drop %d,lookup %d,diff %d ms,t2=%d,patch->out_buf_size=%u\n",
            t1, patch->dtv_apts_lookup, t1 / 192, t2, patch->out_buf_size);

        /*[SE][BUG][SWPL-21122][chengshun] when insert 0, need check write len,
         * and avoid dtv patch write together*/

        unsigned int cur_pcr = 0;
        struct timespec before_write;
        struct timespec after_write;
        clock_gettime(CLOCK_MONOTONIC, &before_write);
        do {
            unsigned int cur_pts = patch->last_apts;
            get_sysfs_uint(TSYNC_PCRSCR, (unsigned int *) & (cur_pcr));
            int ap_diff = cur_pts - cur_pcr;
            ALOGI("dtv_do_drop:cur_pts=0x%x, cur_pcr=0x%x,ap_diff=%d\n", cur_pts, cur_pcr, ap_diff);
            if (ap_diff < 90*10) {
                ALOGI("dtv_do_drop: write mute pcm enough,break\n");
                break;
            }
            memset(patch->out_buf, 0, patch->out_buf_size);
            int ret = 0;
            if ((size_t)t1 > patch->out_buf_size) {
                ret = ring_buffer_write(&(patch->aml_ringbuffer), (unsigned char *)patch->out_buf, patch->out_buf_size, 0);
            } else {
                ret = ring_buffer_write(&(patch->aml_ringbuffer), (unsigned char *)patch->out_buf, t1, 0);
            }
            t1 -= ret;
            int buff_len = ring_buffer_read(&(patch->aml_ringbuffer), (unsigned char *)patch->out_buf, patch->out_buf_size);
            int write_len = out_write_new(stream_out, patch->out_buf, buff_len);
            patch->dtv_pcm_readed += write_len;
            clock_gettime(CLOCK_MONOTONIC, &after_write);
            int write_used_ms = calc_time_interval_us(&before_write, &after_write)/1000;
            ALOGI("dtv_do_drop: write_used_ms = %d\n", write_used_ms);
            if (write_used_ms > 1000) {
                ALOGI("Warning write cost over 1s, break\n");
                break;
            }
            ALOGI("dtv_do_drop:++drop t1=%d, ret = %d", t1, ret);
        } while (t1 > 0);
    }
    patch->dtv_apts_lookup = 0;
}
/*+[SE][BUG][SWPL-14811][zhizhong] add ac3/e-ac3 pcm drop function*/
static int dtv_do_drop_ac3_pcm(struct aml_audio_patch *patch,
            struct audio_stream_out *stream_out)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream_out;
    struct aml_audio_device *adev = aml_out->dev;
    size_t frame_size = 0;
    switch (adev->sink_format) {
    case AUDIO_FORMAT_E_AC3:
        frame_size = AUDIO_EAC3_FRAME_SIZE;
        break;
    case AUDIO_FORMAT_AC3:
        frame_size = AUDIO_AC3_FRAME_SIZE;
        break;
    default:
        frame_size = (aml_out->is_tv_platform == true) ? AUDIO_TV_PCM_FRAME_SIZE : AUDIO_DEFAULT_PCM_FRAME_SIZE;
        break;
    }
    aml_out->need_drop_size = (patch->dtv_apts_lookup / 90) * 48 * frame_size;
    aml_out->need_drop_size &= ~(frame_size - 1);
    ALOGI("dtv_do_drop need_drop_size=%d,frame_size=%d\n",
        aml_out->need_drop_size, frame_size);
    return 0;
}

static void dtv_do_drop_insert_ac3(struct aml_audio_patch *patch,
                            struct audio_stream_out *stream_out)
{
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    struct aml_stream_out *out = (struct aml_stream_out *)stream_out;
    int fm_size;
    int drop_size, t1, t2;
    int write_used_ms = 0;
    unsigned int cur_pts = 0;
    unsigned int cur_pcr = 0;
    int ap_diff = 0;
    int write_times = 0;
    struct timespec before_write;
    struct timespec after_write;

    if (!patch || !patch->dev || !stream_out || aml_dev->tuner2mix_patch == 1) {
        return;
    }

    if (patch->dtv_apts_lookup > 0 && patch->ac3_pcm_dropping != 1) {
        patch->ac3_pcm_dropping = 1;
        dtv_do_drop_ac3_pcm(patch, stream_out);

    } else if (patch->dtv_apts_lookup < 0) {
        if (abs(patch->dtv_apts_lookup) / 90 > 1000) {
            t1 = 1000;
        } else {
            t1 =  abs(patch->dtv_apts_lookup) / 90;
        }
        t2 = t1 / 32;
        ALOGI("dtv_do_insert:++inset lookup %d,diff %d ms\n", patch->dtv_apts_lookup, t1);
        clock_gettime(CLOCK_MONOTONIC, &before_write);
        t1 = 0;
        while (t1 == 0 && t2 > 0 && patch->output_thread_exit == 0) {

            write_times++;
            t1 = dtv_write_mute_frame(patch, stream_out);
            usleep(5000);

            cur_pts = patch->last_apts;
            get_sysfs_uint(TSYNC_PCRSCR, (unsigned int *) & (cur_pcr));
            ap_diff = cur_pts - cur_pcr;
            ALOGI("cur_pts=0x%x, cur_pcr=0x%x,ap_diff=%d\n", cur_pts, cur_pcr, ap_diff);
            if (ap_diff < 90*10) {
                ALOGI("write mute pcm enough, write_times=%d break\n", write_times);
                break;
            }
            t2--;

            clock_gettime(CLOCK_MONOTONIC, &after_write);
            write_used_ms = calc_time_interval_us(&before_write, &after_write)/1000;
            ALOGI("write_used_ms = %d\n", write_used_ms);
            if (write_used_ms > 1000) {
                ALOGI("write cost over 1s write_times=%d, break\n", write_times);
                break;
            }
        }
    }
    ALOGI("dtv_do_drop_insert done\n");
}

static int dtv_audio_tune_check(struct aml_audio_patch *patch, int cur_pts_diff, int last_pts_diff, unsigned int apts)
{
    char tempbuf[128];
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    if (!patch || !patch->dev || aml_dev->tuner2mix_patch == 1) {
        patch->dtv_audio_tune = AUDIO_RUNNING;
        return 1;
    }
    if (get_audio_discontinue(patch) || patch->dtv_has_video == 0 ||
        patch->dtv_audio_mode == 1) {
        dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        patch->last_apts = 0;
        patch->last_pcrpts = 0;
        return 1;
    }
    if (abs(cur_pts_diff) >= AUDIO_PTS_DISCONTINUE_THRESHOLD) {
        sprintf(tempbuf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx",
                (unsigned long)apts);
        dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
            ALOGI("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
        }
        return 1;
    }
    if (patch->dtv_audio_tune == AUDIO_LOOKUP) {
        if (abs(last_pts_diff - cur_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD) {
            patch->dtv_apts_lookup = (last_pts_diff + cur_pts_diff) / 2;
            patch->dtv_audio_tune = AUDIO_DROP;
            ALOGI("dtv_audio_tune audio_lookup %d", patch->dtv_apts_lookup);
        }
        return 1;
    } else if (patch->dtv_audio_tune == AUDIO_LATENCY) {
        if (abs(last_pts_diff - cur_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD) {
            int pts_diff = (last_pts_diff + cur_pts_diff) / 2;
            int pts_latency = decoder_get_latency();
            pts_latency += pts_diff;
            if (patch->dtv_pcr_mode) {
                ALOGI("dtv_audio_tune audio_latency %d,pts_diff %d", (int)pts_latency / 90, pts_diff / 90);
                if (pts_latency >= 500 * 90) {
                    pts_latency = 499 * 90;
                } else if (pts_latency < 0) {
                    if (abs(pts_diff) < DTV_PTS_CORRECTION_THRESHOLD) {
                        pts_latency += abs(pts_diff);
                    } else {
                        pts_latency = 0;
                    }
                }
                decoder_set_latency(pts_latency);
            } else {
                uint pcrpts = 0;
                get_sysfs_uint(TSYNC_PCRSCR, &pcrpts);
                ALOGI("dtv_audio_tune audio_latency pts_diff %d, pcrsrc %x", pts_diff / 90, pcrpts);
                if (pts_diff > 1000 * 90) {
                    pts_diff = 1000 * 90;
                } else if (pts_diff < -1000 * 90) {
                    pts_diff = -1000 * 90;
                }
                pcrpts -= pts_diff;
                decoder_set_pcrsrc(pcrpts);
                ALOGI("dtv_audio_tune audio_latency end, pcrsrc %x", pcrpts);
            }
            patch->dtv_audio_tune = AUDIO_RUNNING;
        }
        return 1;
    } else if (patch->dtv_audio_tune != AUDIO_RUNNING) {
        return 1;
    } else if (!patch->dtv_pcr_mode && patch->dtv_audio_tune == AUDIO_RUNNING) {
        if (abs(cur_pts_diff) > SYSTIME_CORRECTION_THRESHOLD &&
            abs(last_pts_diff) > SYSTIME_CORRECTION_THRESHOLD &&
            abs(last_pts_diff - cur_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD) {
            patch->dtv_audio_tune = AUDIO_LATENCY;
            if (get_tsync_pcr_debug()) {
                ALOGI("dtv_audio_tune audio_latency cur_pts_diff %d, last_pts_diff %d",
                      cur_pts_diff / 90, last_pts_diff / 90);
            }
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
            return 1;
        }
    }
    return 0;
}

static void do_pll1_by_pts(unsigned int pcrpts, struct aml_audio_patch *patch,
                           unsigned int apts, struct aml_stream_out *stream_out)
{
    unsigned int last_pcrpts, last_apts;
    int pcrpts_diff, last_pts_diff, cur_pts_diff;
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    struct aml_mixer_handle * handle = &(aml_dev->alsa_mixer);
    if (get_tsync_pcr_debug()) {
        ALOGI("process_pts_sync, diff:%d,pcrpts %x,size %d, latency %d, mode %d",
              (int)(pcrpts - apts) / 90, pcrpts, get_buffer_read_space(&(patch->aml_ringbuffer)),
              (int)decoder_get_latency() / 90, patch->dtv_pcr_mode);
    }
    last_apts = patch->last_apts;
    last_pcrpts = patch->last_pcrpts;
    patch->last_pcrpts = pcrpts;
    patch->last_apts = apts;
    last_pts_diff = last_pcrpts - last_apts;
    cur_pts_diff = pcrpts - apts;
    pcrpts_diff = pcrpts - last_pcrpts;
    if (last_apts == 0 && last_pcrpts == 0) {
        return;
    }
    if (dtv_audio_tune_check(patch, cur_pts_diff, last_pts_diff, apts)) {
        return;
    }
    if (patch->pll_state == DIRECT_NORMAL) {
        if (abs(cur_pts_diff) <= DTV_PTS_CORRECTION_THRESHOLD * 3 ||
            abs(last_pts_diff + cur_pts_diff) / 2 <= DTV_PTS_CORRECTION_THRESHOLD * 3
            || abs(last_pts_diff) <= DTV_PTS_CORRECTION_THRESHOLD * 3) {
            return;
        } else {
            if (pcrpts > apts) {
                if (dtv_calc_abuf_level(patch, stream_out)) {
                    dtv_adjust_output_clock(patch, DIRECT_SPEED, DEFAULT_DTV_ADJUST_CLOCK);
                } else {
                    dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
                }
            } else {
                dtv_adjust_output_clock(patch, DIRECT_SLOW, DEFAULT_DTV_ADJUST_CLOCK);
            }
            return;
        }
    } else if (patch->pll_state == DIRECT_SPEED) {
        if (!dtv_calc_abuf_level(patch, stream_out)) {
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        }
        if (cur_pts_diff < 0 && ((last_pts_diff + cur_pts_diff) < 0 ||
                                 abs(last_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD)) {
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        }
        if (cur_pts_diff > 0)
            dtv_adjust_output_clock_continue(patch, DIRECT_SPEED);
    } else if (patch->pll_state == DIRECT_SLOW) {
        if (cur_pts_diff > 0 && ((last_pts_diff + cur_pts_diff) > 0 ||
                                abs(last_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD)) {
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        }
        if (cur_pts_diff < 0)
            dtv_adjust_output_clock_continue(patch, DIRECT_SLOW);
    }
}

static void do_pll2_by_pts(unsigned int pcrpts, struct aml_audio_patch *patch,
                           unsigned int apts, struct aml_stream_out *stream_out)
{
    unsigned int last_pcrpts, last_apts;
    unsigned int cur_vpts = 0;
    int pcrpts_diff, last_pts_diff, cur_pts_diff;
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    struct aml_mixer_handle * handle = &(aml_dev->alsa_mixer);
    int ret = 0;
    char buff[32] = {0};

    if (get_tsync_pcr_debug()) {
        char buff[32];
        unsigned int cur_vpts = 0;
        unsigned int demux_vpts = 0;
        unsigned int demux_apts = 0;
        int ret1 = aml_sysfs_get_str(TSYNC_VPTS, buff, sizeof(buff));
        if (ret1 > 0) {
            ret1 = sscanf(buff, "0x%x\n", &cur_vpts);
        }
        ret1 = aml_sysfs_get_str(TSYNC_DEMUX_APTS, buff, sizeof(buff));
        if (ret1 > 0) {
            ret1 = sscanf(buff, "%u\n", &demux_apts);
        }
        ret1 = aml_sysfs_get_str(TSYNC_DEMUX_VPTS, buff, sizeof(buff));
        if (ret1 > 0) {
            ret1 = sscanf(buff, "%u\n", &demux_vpts);
        }

        ALOGI("process_ac3_sync, diff:%d,apts %x(%x,cache:%dms), vpts %x(%x,cache:%dms), pcrpts %x, av-diff:%d, size %d, latency %d, mode %d, pll_state=%d\n",
              (int)(pcrpts - apts) / 90, apts, demux_apts, (int)(demux_apts - apts) / 90,cur_vpts, demux_vpts, (int)(demux_vpts - cur_vpts) / 90, pcrpts, (int)(apts - cur_vpts)/90,
              get_buffer_read_space(&(patch->aml_ringbuffer)), (int)decoder_get_latency() / 90, patch->dtv_pcr_mode, patch->pll_state);

    }
    last_apts = patch->last_apts;
    last_pcrpts = patch->last_pcrpts;
    patch->last_pcrpts = pcrpts;
    patch->last_apts = apts;
    last_pts_diff = last_pcrpts - last_apts;
    cur_pts_diff = pcrpts - apts;
    pcrpts_diff = pcrpts - last_pcrpts;
    if (last_apts == 0 && last_pcrpts == 0) {
        return;
    }
    if (dtv_audio_tune_check(patch, cur_pts_diff, last_pts_diff, apts)) {
        return;
    }
    if (aml_dev->bHDMIARCon) {
        int arc_delay = aml_getprop_int(PROPERTY_LOCAL_ARC_LATENCY) / 1000;
        if (get_video_delay() != -arc_delay) {
            ALOGI("arc:video_delay moved from %d ms to %d ms", get_video_delay(), -arc_delay);
            set_video_delay(-arc_delay);
            return;
        }
    }
    if (patch->pll_state == DIRECT_NORMAL) {
        if (abs(cur_pts_diff) <= DTV_PTS_CORRECTION_THRESHOLD ||
            abs(last_pts_diff + cur_pts_diff) / 2 <= DTV_PTS_CORRECTION_THRESHOLD
            || abs(last_pts_diff) <= DTV_PTS_CORRECTION_THRESHOLD) {
            return;
        } else {
            if (pcrpts > apts) {
                if (dtv_calc_abuf_level(patch, stream_out)) {
                    dtv_adjust_output_clock(patch, DIRECT_SPEED, DEFAULT_DTV_ADJUST_CLOCK);
                } else {
                    dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
                }
            } else {
                dtv_adjust_output_clock(patch, DIRECT_SLOW, DEFAULT_DTV_ADJUST_CLOCK);
            }
            return;
        }
    } else if (patch->pll_state == DIRECT_SPEED) {
        if (!dtv_calc_abuf_level(patch, stream_out)) {
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        }
        if (cur_pts_diff < 0 && ((last_pts_diff + cur_pts_diff) < 0 ||
                                 abs(last_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD)) {
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        }
        if (cur_pts_diff > 0)
            dtv_adjust_output_clock_continue(patch, DIRECT_SPEED);
    } else if (patch->pll_state == DIRECT_SLOW) {
        if (cur_pts_diff > 0 && ((last_pts_diff + cur_pts_diff) > 0 ||
                                abs(last_pts_diff) < DTV_PTS_CORRECTION_THRESHOLD)) {
            dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
        }
        if (cur_pts_diff < 0)
            dtv_adjust_output_clock_continue(patch, DIRECT_SLOW);
    }
}

static void do_tsync_by_apts(unsigned int pcrpts, struct aml_audio_patch *patch,
                             unsigned int apts)
{
    char tempbuf[128];
    unsigned int last_pcrpts, last_apts;
    int pcrpts_diff, last_pts_diff, cur_pts_diff;
    struct audio_hw_device *adev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) adev;
    if (!patch || !patch->dev) {
        return;
    }
    if (get_tsync_pcr_debug()) {
        ALOGI("do_tsync_by_apts, diff:%d,pcrpts %x,curpts %x, size %d",
              (int)(pcrpts - apts) / 90, pcrpts, apts,
              get_buffer_read_space(&(patch->aml_ringbuffer)));
    }
    last_apts = patch->last_apts;
    last_pcrpts = patch->last_pcrpts;
    patch->last_pcrpts = pcrpts;
    patch->last_apts = apts;
    last_pts_diff = last_pcrpts - last_apts;
    cur_pts_diff = pcrpts - apts;
    pcrpts_diff = pcrpts - last_pcrpts;
    if (last_apts == 0 || last_pcrpts == 0 || aml_dev->tuner2mix_patch == 1
        || patch->dtv_has_video == 0 || patch->dtv_audio_mode == 1) {
        return;
    }
    if (abs(last_pts_diff - cur_pts_diff) > DTV_PTS_CORRECTION_THRESHOLD) {
        return;
    }
    if (abs(cur_pts_diff) <= SYSTIME_CORRECTION_THRESHOLD) {
        // now the av is syncd ,so do nothing;
    } else if (abs(cur_pts_diff) < AUDIO_PTS_DISCONTINUE_THRESHOLD) {
        sprintf(tempbuf, "%u", (unsigned int)apts);
        if (get_tsync_pcr_debug()) {
            ALOGI("reset the apts to %x pcrpts %x pts_diff %d ms\n", apts, pcrpts, cur_pts_diff / 90);
        }
        sysfs_set_sysfs_str(TSYNC_APTS, tempbuf);
    } else {
        if (get_tsync_pcr_debug()) {
            ALOGI("AUDIO_TSTAMP_DISCONTINUITY, the apts %x, pcrpts %x pts_diff %d \n",
                  apts, pcrpts, cur_pts_diff);
        }
        sprintf(tempbuf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", (unsigned long)apts);
        if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
            ALOGI("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
        }
    }
}

void process_ac3_sync(struct aml_audio_patch *patch, unsigned long pts, struct aml_stream_out *stream_out)
{

    int channel_count = 2;
    int bytewidth = 2;
    int symbol = 48;
    char tempbuf[128];
    unsigned int pcrpts;
    unsigned int pts_diff;
    unsigned long cur_out_pts;

    if (patch->dtv_first_apts_flag == 0) {
        sprintf(tempbuf, "AUDIO_START:0x%x", (unsigned int)pts);
        ALOGI("dtv set tsync -> %s", tempbuf);
        if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
            ALOGE("set AUDIO_START failed \n");
        }
        patch->dtv_first_apts_flag = 1;
    } else {
        cur_out_pts = pts;
        get_sysfs_uint(TSYNC_PCRSCR, &pcrpts);
        if (!patch || !patch->dev || !stream_out) {
            return;
        }
        if (pts == 0) {
            return;
        }
        if (patch->tsync_mode == TSYNC_MODE_PCRMASTER) {
            pcrpts = dtv_calc_pcrpts_latency(patch, pcrpts);
            do_pll2_by_pts(pcrpts, patch, cur_out_pts, stream_out);
        } else {
            do_tsync_by_apts(pcrpts, patch, cur_out_pts);
        }
    }
}

void process_pts_sync(unsigned int pcm_lancty, struct aml_audio_patch *patch,
                      unsigned int rbuf_level, struct aml_stream_out *stream_out)
{
    int channel_count = 2;
    int bytewidth = 2;
    int sysmbol = 48;
    char tempbuf[128];
    char value[128];
    unsigned int pcrpts, apts;
    unsigned int calc_len = 0;
    unsigned long pts = 0, lookpts;
    unsigned long pts_add = 0;
    unsigned long cache_pts = 0;
    unsigned long cur_out_pts = 0;
    unsigned long cur_out_pts_add = 0;
    static unsigned int pts_diff_count = 0;
    unsigned long last_pts = patch->last_out_pts;

    unsigned long pts_threshold = 100*90;
    if (property_get("media.libplayer.pts_threshold", value, NULL) > 0) {
        pts_threshold = atoi(value);
    }

    pts = lookpts = dtv_patch_get_pts();
    if (pts == patch->last_valid_pts) {
        ALOGI("dtv_patch_get_pts pts  -> %lx", pts);
    }
    if (patch->dtv_first_apts_flag == 0) {
        if (pts == 0) {
            pts = decoder_checkin_firstapts();
            ALOGI("pts = 0,so get checkin_firstapts:0x%lx", pts);
        }
        sprintf(tempbuf, "AUDIO_START:0x%x", (unsigned int)pts);
        ALOGI("dtv set tsync -> %s", tempbuf);
        if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
            ALOGE("set AUDIO_START failed \n");
            pts_diff_count = 0;
        }
        patch->dtv_pcm_readed = 0;
        patch->dtv_first_apts_flag = 1;
        patch->last_valid_pts = pts;
        patch->last_out_pts = pts;
    } else {
        unsigned int pts_diff;
        pts_add = patch->last_out_pts;
        calc_len = patch->dtv_pcm_readed;
        cache_pts = (calc_len * 90) / (sysmbol * channel_count * bytewidth);
        cur_out_pts_add = pts_add + cache_pts;
        if (aml_getprop_bool("media.audiohal.debug"))
        ALOGI("======patch->last_out_pts = %lx cache_pts = %lx calc_len = %d, cur_out_pts_add = %lx", patch->last_out_pts, cache_pts, calc_len, cur_out_pts_add);

        if (pts != (unsigned long) - 1) {
            calc_len = (unsigned int)rbuf_level;
            cache_pts = (calc_len * 90) / (sysmbol * channel_count * bytewidth);
            if (pts > cache_pts) {
                cur_out_pts = pts - cache_pts;
            } else {
                return;
            }
            if (cur_out_pts > pcm_lancty * 90) {
                cur_out_pts = cur_out_pts - pcm_lancty * 90;
            } else {
                return;
            }
            patch->last_valid_pts = cur_out_pts;
            patch->dtv_pcm_readed = 0;
            patch->last_out_pts = cur_out_pts_add;
        } else {
            pts = patch->last_valid_pts;
            calc_len = patch->dtv_pcm_readed;
            cache_pts = (calc_len * 90) / (sysmbol * channel_count * bytewidth);
            cur_out_pts = pts + cache_pts;
            if (aml_getprop_bool("media.audiohal.debug"))
            ALOGI("======patch->last_valid_pts = %lx cache_pts = %lx calc_len = %d, cur_out_pts = %lx", patch->last_valid_pts, cache_pts, calc_len, cur_out_pts);
            if (patch->tsync_mode == TSYNC_MODE_PCRMASTER) {
                return;
            }
        }
        if (!patch || !patch->dev || !stream_out) {
            return;
        }
        get_sysfs_uint(TSYNC_PCRSCR, &pcrpts);
        if (patch->tsync_mode == TSYNC_MODE_PCRMASTER) {
            //pcrpts -= DTV_PTS_CORRECTION_THRESHOLD;
            do_pll1_by_pts(pcrpts, patch, cur_out_pts, stream_out);
            return;
        }
        if (pcrpts > cur_out_pts) {
            pts_diff = pcrpts - cur_out_pts;
        } else {
            pts_diff = cur_out_pts - pcrpts;
        }

        {
            get_sysfs_uint(TSYNC_APTS, &apts);
            if (lookpts != (unsigned long) - 1 && abs((int)(cur_out_pts - pcrpts)) > AUDIO_PTS_DISCONTINUE_THRESHOLD) {
                ALOGI("AUDIO_TSTAMP_DISCONTINUITY,not set sync event, the apts %x, apts %lx pcrpts %x pts_diff %d \n",
                      apts, cur_out_pts, pcrpts, pts_diff);
                sprintf(tempbuf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx",
                       (unsigned long)cur_out_pts);
                if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
                    ALOGI("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
                }
                return;
            }
        }

        if (aml_getprop_bool("media.audiohal.debug"))
            ALOGI("cur_out_pts and cur_out_pts_add!!! %lx/%lx, diff=%d", cur_out_pts, cur_out_pts_add, abs((int)(cur_out_pts - cur_out_pts_add)));
        if (abs((int)(cur_out_pts - cur_out_pts_add)) > (int)pts_threshold) {
            pts_diff_count ++;
            if (pts_diff_count > 10) {
                ALOGI("need use cur_out_pts to reset apts!!! %lx/%lx, diff=%d", cur_out_pts, cur_out_pts_add, abs((int)(cur_out_pts - cur_out_pts_add)));
                patch->last_out_pts = cur_out_pts_add = cur_out_pts;
                pts_diff_count = 0;
            }
        } else {
            pts_diff_count = 0;
        }
        if (last_pts != cur_out_pts_add) {
            sprintf(tempbuf, "%u", (unsigned int)cur_out_pts_add);
            sysfs_set_sysfs_str(TSYNC_APTS, tempbuf);
            if (aml_getprop_bool("media.audiohal.debug"))
                ALOGI("reset the apts to %lx pcrpts %x pts_diff %d \n", cur_out_pts_add, pcrpts, abs((int)(cur_out_pts_add - pcrpts)));
        }
    }
}

void dtv_avsync_process(struct aml_audio_patch* patch, struct aml_stream_out* stream_out)
{
    unsigned long pts ;
    int audio_output_delay = 0;
    unsigned int pcrpts, firstvpts;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    if (patch->dtv_decoder_state != AUDIO_DTV_PATCH_DECODER_STATE_RUNING) {
        return;
    }

    get_sysfs_uint(TSYNC_PCRSCR, &pcrpts);
    get_sysfs_uint(TSYNC_FIRST_VPTS, &firstvpts);

    if (patch->dtv_has_video && patch->show_first_frame == 0) {
        patch->show_first_frame = get_sysfs_int(VIDEO_FIRST_FRAME_SHOW);
        ALOGI("dtv_avsync_process: patch->show_first_frame=%d, firstvpts=0x%x, pcrpts=0x%x, cache:%dms",
               patch->show_first_frame, firstvpts, pcrpts, (int)(firstvpts - pcrpts)/90);
    }
    if (aml_dev->start_mute_flag && ((firstvpts != 0 && pcrpts + 10*90 > firstvpts) || patch->show_first_frame)) {
        ALOGI("start_mute_flag 0.");
        aml_dev->start_mute_flag = 0;
    }

    patch->dtv_pcr_mode = get_dtv_pcr_sync_mode();
    aml_dev->audio_discontinue = get_audio_discontinue(patch);

    audio_output_delay = aml_getprop_int(PROPERTY_LOCAL_PASSTHROUGH_LATENCY);
    if (patch->last_audio_delay != audio_output_delay) {
        patch->last_audio_delay = audio_output_delay;
        patch->dtv_audio_tune = AUDIO_LOOKUP;
        ALOGI("set audio_output_delay = %d\n", audio_output_delay);
    }

    if (patch->aformat == AUDIO_FORMAT_E_AC3 || patch->aformat == AUDIO_FORMAT_AC3) {
        if (stream_out != NULL) {
            /*The decoder has one frame cached,need to add 32ms to the latency*/
            unsigned int  pcm_lantcy = out_get_latency(&(stream_out->stream)) + 32
                                        + audio_output_delay;
            pts = dtv_hal_get_pts(patch, pcm_lantcy);
            process_ac3_sync(patch, pts, stream_out);
        }
    } else if (patch->aformat ==  AUDIO_FORMAT_DTS || patch->aformat == AUDIO_FORMAT_DTS_HD) {
        if (stream_out != NULL) {
            ringbuffer = &(patch->aml_ringbuffer);
            unsigned int pcm_lantcy = out_get_latency(&(stream_out->stream)) +
                                        audio_output_delay;
            pts = dtv_hal_get_pts(patch, pcm_lantcy);
            process_ac3_sync(patch, pts, stream_out);
        }
    } else {
        {
            if (stream_out != NULL) {
                unsigned int pcm_lantcy = out_get_latency(&(stream_out->stream)) +
                                            audio_output_delay;
                int abuf_level = get_buffer_read_space(ringbuffer);
                process_pts_sync(pcm_lantcy, patch, abuf_level, stream_out);
            }
        }
    }
    dtv_audio_gap_monitor(patch);
}

static int dtv_patch_pcm_write(unsigned char *pcm_data, int size,
                               int symbolrate, int channel, void *args)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    int left, need_resample;
    int write_size, return_size;
    unsigned char *write_buf;
    int16_t tmpbuf[OUTPUT_BUFFER_SIZE];
    int valid_paramters = 1;
    write_buf = pcm_data;
    if (pcm_data == NULL || size == 0) {
        return 0;
    }

    if (patch->dtv_decoder_state == AUDIO_DTV_PATCH_DECODER_STATE_INIT ||
        patch->dtv_decoder_state == AUDIO_DTV_PATCH_DECODER_STATE_PAUSE) {
        return 0;
    }

    patch->sample_rate = symbolrate;
    // In the case of fast switching channels such as mpeg/dra/..., there may be an
    // error "symbolrate" and "channel" paramters, so add the check to avoid it.
    if (symbolrate > 96000 || symbolrate < 8000) {
        valid_paramters = 0;
    }
    if (channel > 8 || channel < 1) {
        valid_paramters = 0;
    }
    patch->chanmask = channel;
    if (patch->sample_rate != 48000) {
        need_resample = 1;
    } else {
        need_resample = 0;
    }
    left = get_buffer_write_space(ringbuffer);

    if (left <= 0) {
        return 0;
    }
    if (need_resample == 0 && patch->chanmask == 1) {
        if (left >= 2 * size) {
            write_size = size;
        } else {
            write_size = left / 2;
        }
    } else if (need_resample == 1 && patch->chanmask == 2) {
        if (left >= size * 48000 / patch->sample_rate) {
            write_size = size;
        } else {
            return 0;
        }

    } else if (need_resample == 1 && patch->chanmask == 1) {
        if (left >= 2 * size * 48000 / patch->sample_rate) {
            write_size = size;
        } else {
            return 0;
        }
    } else {
        if (left >= size) {
            write_size = size;
        } else {
            write_size = left;
        }
    }

    return_size = write_size;

    if ((patch->aformat != AUDIO_FORMAT_E_AC3 &&
         patch->aformat != AUDIO_FORMAT_AC3 &&
         patch->aformat != AUDIO_FORMAT_DTS) && valid_paramters) {
        if (patch->chanmask == 1) {
            int16_t *buf = (int16_t *)write_buf;
            int i = 0, samples_num;
            samples_num = write_size / (patch->chanmask * sizeof(int16_t));
            for (; i < samples_num; i++) {
                tmpbuf[2 * (samples_num - i) - 1] = buf[samples_num - i - 1];
                tmpbuf[2 * (samples_num - i) - 2] = buf[samples_num - i - 1];
            }
            write_size = write_size * 2;
            write_buf = (unsigned char *)tmpbuf;
            if (write_size > left || write_size > (OUTPUT_BUFFER_SIZE * 2)) {
                ALOGI("resample, channel, write_size %d, left %d", write_size, left);
                write_size = ((left) < (OUTPUT_BUFFER_SIZE * 2)) ? (left) : (OUTPUT_BUFFER_SIZE * 2);
            }
        }
        if (need_resample == 1) {
            if (patch->dtv_resample.input_sr != (unsigned int)patch->sample_rate) {
                patch->dtv_resample.input_sr = patch->sample_rate;
                patch->dtv_resample.output_sr = 48000;
                patch->dtv_resample.channels = 2;
                resampler_init(&patch->dtv_resample);
            }
            if (!patch->resample_outbuf) {
                patch->resample_outbuf =
                    (unsigned char *)malloc(OUTPUT_BUFFER_SIZE * 3);
                if (!patch->resample_outbuf) {
                    ALOGE("malloc buffer failed\n");
                    return -1;
                }
                memset(patch->resample_outbuf, 0, OUTPUT_BUFFER_SIZE * 3);
            }
            int out_frame = write_size >> 2;
            out_frame = resample_process(&patch->dtv_resample, out_frame,
                                         (int16_t *)write_buf,
                                         (int16_t *)patch->resample_outbuf);
            write_size = out_frame << 2;
            write_buf = patch->resample_outbuf;
            if (write_size > left || write_size > (OUTPUT_BUFFER_SIZE * 3)) {
                ALOGI("resample, process, write_size %d, left %d", write_size, left);
                write_size = ((left) < (OUTPUT_BUFFER_SIZE * 3)) ? (left) : (OUTPUT_BUFFER_SIZE * 3);
            }
        }
    }

    ring_buffer_write(ringbuffer, (unsigned char *)write_buf, write_size,
                      UNCOVER_WRITE);

    // if ((patch->aformat != AUDIO_FORMAT_E_AC3)
    //     && (patch->aformat != AUDIO_FORMAT_AC3) &&
    //     (patch->aformat != AUDIO_FORMAT_DTS)) {
    //     int abuf_level = get_buffer_read_space(ringbuffer);
    //     process_pts_sync(0, patch, 0);
    // }

    if (aml_getprop_bool("media.audiohal.outdump")) {
        aml_audio_dump_audio_bitstreams("/data/audio_dtv.pcm",
            write_buf, write_size);
    }
    // ALOGI("[%s]ring_buffer_write now wirte %d to ringbuffer\
    //  now\n",
    //       __FUNCTION__, write_size);
    patch->dtv_pcm_writed += return_size;

    pthread_cond_signal(&patch->cond);
    return return_size;
}

static int dtv_patch_raw_wirte(unsigned char *raw_data, int size, void *args)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    int left;
    int write_size;
    if (raw_data == NULL) {
        return 0;
    }

    if (size == 0) {
        return 0;
    }

    left = get_buffer_write_space(ringbuffer);
    if (left > size) {
        write_size = size;
    } else {
        write_size = left;
    }

    ring_buffer_write(ringbuffer, (unsigned char *)raw_data, write_size,
                      UNCOVER_WRITE);
    return write_size;
}
static int raw_dump_fd = -1;
void dump_raw_buffer(const void *data_buf, int size)
{
    ALOGI("enter the dump_raw_buffer save %d len data now\n", size);
    if (raw_dump_fd < 0) {
        if (access("/data/raw.es", 0) == 0) {
            raw_dump_fd = open("/data/raw.es", O_RDWR);
            if (raw_dump_fd < 0) {
                ALOGE("%s, Open device file \"%s\" error: %s.\n", __FUNCTION__,
                      "/data/raw.es", strerror(errno));
            }
        } else {
            raw_dump_fd = open("/data/raw.es", O_RDWR);
            if (raw_dump_fd < 0) {
                ALOGE("%s, Create device file \"%s\" error: %s.\n", __FUNCTION__,
                      "/data/raw.es", strerror(errno));
            }
        }
    }

    if (raw_dump_fd >= 0) {
        write(raw_dump_fd, data_buf, size);
    }
    return;
}

extern int do_output_standby_l(struct audio_stream *stream);
extern void adev_close_output_stream_new(struct audio_hw_device *dev,
        struct audio_stream_out *stream);
extern int adev_open_output_stream_new(struct audio_hw_device *dev,
                                       audio_io_handle_t handle __unused,
                                       audio_devices_t devices,
                                       audio_output_flags_t flags,
                                       struct audio_config *config,
                                       struct audio_stream_out **stream_out,
                                       const char *address __unused);
ssize_t out_write_new(struct audio_stream_out *stream, const void *buffer,
                      size_t bytes);

int audio_dtv_patch_output_default(struct aml_audio_patch *patch,
                            struct audio_stream_out *stream_out, int *apts_diff)
{
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    //int apts_diff = 0;
    int ret = 0;

    char buff[32];
    int write_len;
    struct aml_stream_out *aml_out;
    aml_out = (struct aml_stream_out *)stream_out;
    int avail = get_buffer_read_space(ringbuffer);
    if (avail >= (int)patch->out_buf_size) {
        write_len = (int)patch->out_buf_size;
        if (!patch->first_apts_lookup_over) {
            *apts_diff = dtv_set_audio_latency(0);
            if (!dtv_firstapts_lookup_over(patch, aml_dev, avail, false) || avail < 48 * 4 * 50) {
                ALOGI("[%d]hold the aduio for cache data, avail %d", __LINE__, avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            patch->first_apts_lookup_over = 1;
            ALOGI("[%s,%d] dtv_audio_tune %d-> AUDIO_LOOKUP\n", __FUNCTION__, __LINE__, patch->dtv_audio_tune);
            patch->dtv_audio_tune = AUDIO_LOOKUP;
            //ALOGI("dtv_audio_tune audio_lookup\n");
            clean_dtv_patch_pts(patch);
            patch->out_buf_size = aml_out->config.period_size * audio_stream_out_frame_size(&aml_out->stream);
        } else if (patch->dtv_audio_tune == AUDIO_BREAK) {
            int a_discontinue = get_audio_discontinue(patch);
            dtv_set_audio_latency(*apts_diff);
            if (!dtv_firstapts_lookup_over(patch, aml_dev, avail, true) && !a_discontinue) {
                ALOGI("[%d]hold the aduio for cache data, avail %d", __LINE__, avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            ALOGI("[%s,%d] dtv_audio_tune AUDIO_BREAK-> AUDIO_LOOKUP\n", __FUNCTION__, __LINE__);
            patch->dtv_audio_tune = AUDIO_LOOKUP;
            //ALOGI("dtv_audio_tune audio_lookup\n");
            clean_dtv_patch_pts(patch);
        }
        if (patch->dtv_audio_tune == AUDIO_DROP) {
            dtv_do_drop_pcm(avail, patch, stream_out);
            clean_dtv_patch_pts(patch);
            ALOGI("[%s,%d] dtv_audio_tune AUDIO_DROP-> AUDIO_LATENCY\n", __FUNCTION__, __LINE__);
            patch->dtv_audio_tune = AUDIO_LATENCY;
        }
        ret = ring_buffer_read(ringbuffer, (unsigned char *)patch->out_buf, write_len);
        if (ret == 0) {
            pthread_mutex_unlock(&(patch->dtv_output_mutex));
            usleep(1000);
            /*ALOGE("%s(), live ring_buffer read 0 data!", __func__);*/
            return -EAGAIN;
        }

        if (aml_out->hal_internal_format != patch->aformat) {
            aml_out->hal_format = aml_out->hal_internal_format = patch->aformat;
            get_sink_format(stream_out);
        }
        ret = out_write_new(stream_out, patch->out_buf, ret);
        patch->dtv_pcm_readed += ret;
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
    } else {
        dtv_audio_gap_monitor(patch);
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
        usleep(1000);
    }

    return ret;
}



int audio_dtv_patch_output_dolby(struct aml_audio_patch *patch,
                        struct audio_stream_out *stream_out, int *apts_diff)
{
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream_out;
    int ret = 0;

    int remain_size = 0;
    char buff[32];
    int write_len, cur_frame_size = 0;
    unsigned long long all_pcm_len1 = 0;
    unsigned long long all_pcm_len2 = 0;
    unsigned long long all_zero_len = 0;
    int avail = get_buffer_read_space(ringbuffer);
    if (avail > 0) {
        if (avail > (int)patch->out_buf_size) {
            write_len = (int)patch->out_buf_size;
            if (write_len > 512) {
                write_len = 512;
            }
        } else {
            write_len = 512;
        }

        if (aml_dev->ddp.curFrmSize != 0) {
            write_len = aml_dev->ddp.curFrmSize;
        }
        if (!patch->first_apts_lookup_over) {
            *apts_diff = dtv_set_audio_latency(0);
            if (!dtv_firstapts_lookup_over(patch, aml_dev, avail, false) || avail < 512 * 2) {
                ALOGI("hold the aduio for cache data, avail %d", avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            patch->first_apts_lookup_over = 1;
            ALOGI("[%s,%d] dtv_audio_tune %d-> AUDIO_LOOKUP\n", __FUNCTION__, __LINE__, patch->dtv_audio_tune);
            patch->dtv_audio_tune = AUDIO_LOOKUP;
            clean_dtv_patch_pts(patch);
            //ALOGI("dtv_audio_tune audio_lookup\n");
        } else if (patch->dtv_audio_tune == AUDIO_BREAK) {
            int a_discontinue = get_audio_discontinue(patch);
            dtv_set_audio_latency(*apts_diff);
            if (!dtv_firstapts_lookup_over(patch, aml_dev, avail, true) && !a_discontinue) {
                ALOGI("hold the aduio for cache data, avail %d", avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            ALOGI("[%s,%d] dtv_audio_tune AUDIO_BREAK-> AUDIO_LOOKUP\n", __FUNCTION__, __LINE__);
            patch->dtv_audio_tune = AUDIO_LOOKUP;
            clean_dtv_patch_pts(patch);
            //ALOGI("dtv_audio_tune audio_lookup\n");
        } else if (patch->dtv_audio_tune == AUDIO_DROP) {
            dtv_do_drop_insert_ac3(patch, stream_out);
            if (patch->dtv_apts_lookup < 0 ||
                (patch->dtv_apts_lookup > 0 &&
                aml_out->need_drop_size == 0)) {
                clean_dtv_patch_pts(patch);
                patch->dtv_apts_lookup = 0;
                patch->ac3_pcm_dropping = 0;
                ALOGI("[%s,%d] dtv_audio_tune AUDIO_DROP-> AUDIO_LATENCY\n", __FUNCTION__, __LINE__);
                patch->dtv_audio_tune = AUDIO_LATENCY;
                ALOGI("dtv_audio_tune ac3 audio_latency\n");
            }
        }
        ret = ring_buffer_read(ringbuffer, (unsigned char *)patch->out_buf, write_len);
        if (ret == 0) {
            pthread_mutex_unlock(&(patch->dtv_output_mutex));
            /*ALOGE("%s(), ring_buffer read 0 data!", __func__);*/
            usleep(1000);
            return -EAGAIN;
        }
        {
            if (aml_out->hal_internal_format != patch->aformat) {
                aml_out->hal_format = aml_out->hal_internal_format = patch->aformat;
                get_sink_format(stream_out);
            }
        }
        if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
            remain_size = dolby_ms12_get_main_buffer_avail(NULL);
            dolby_ms12_get_pcm_output_size(&all_pcm_len1, &all_zero_len);
        } else {
            remain_size = aml_dev->ddp.remain_size;
        }
        /* +[SE] [BUG][SWPL-22893][yinli.xia]
              add: reset decode data when replay video*/
        if (patch->dtv_replay_flag) {
            remain_size = 0;
            patch->dtv_replay_flag = false;
        }
        ret = out_write_new(stream_out, patch->out_buf, ret);
        if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
            int size = dolby_ms12_get_main_buffer_avail(NULL);
            dolby_ms12_get_pcm_output_size(&all_pcm_len2, &all_zero_len);
            patch->decoder_offset += remain_size + ret - size;
            patch->outlen_after_last_validpts += (unsigned int)(all_pcm_len2 - all_pcm_len1);
            //ALOGI("remain_size %d,size %d,ret %d,validpts %d",remain_size,size,ret,patch->outlen_after_last_validpts);
            patch->dtv_pcm_readed += ret;
        } else {
            patch->outlen_after_last_validpts += aml_dev->ddp.outlen_pcm;
            patch->decoder_offset += remain_size + ret - aml_dev->ddp.remain_size;
            patch->dtv_pcm_readed += ret;
        }
        /* +[SE] [BUG][SWPL-22893][yinli.xia]
              add: reset decode data when replay video*/
        if (aml_dev->debug_flag) {
            ALOGI("after decode: decode_offset: %d, aml_dev->ddp.remain_size=%d\n",
                   patch->decoder_offset, aml_dev->ddp.remain_size);
        }
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
    } else {
        dtv_audio_gap_monitor(patch);
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
        usleep(1000);
    }

    return ret;
}


int audio_dtv_patch_output_dts(struct aml_audio_patch *patch, struct audio_stream_out *stream_out)
{
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    int apts_diff = 0;
    int ret = 0;

    int remain_size = 0;
    int avail = get_buffer_read_space(ringbuffer);
    if (avail > 0) {
        if (avail > (int)patch->out_buf_size) {
            avail = (int)patch->out_buf_size;
            if (avail > 1024) {
                avail = 1024;
            }
        } else {
            avail = 1024;
        }
        if (!patch->first_apts_lookup_over) {
            apts_diff = dtv_set_audio_latency(0);

            if (!dtv_firstapts_lookup_over(patch, aml_dev, avail, false) || avail < 512 * 2) {
                ALOGI("hold the aduio for cache data, avail %d", avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            patch->first_apts_lookup_over = 1;
        }
        ret = ring_buffer_read(ringbuffer, (unsigned char *)patch->out_buf,
                               avail);
        if (ret == 0) {
            pthread_mutex_unlock(&(patch->dtv_output_mutex));
            usleep(1000);
            /*ALOGE("%s(), live ring_buffer read 0 data!", __func__);*/
            return -EAGAIN;
        }

        remain_size = aml_dev->dts_hd.remain_size;

        /* +[SE] [BUG][SWPL-22893][yinli.xia]
              add: reset decode data when replay video*/
        if (patch->dtv_replay_flag) {
            remain_size = 0;
            patch->dtv_replay_flag = false;
        }

        ret = out_write_new(stream_out, patch->out_buf, ret);
        patch->outlen_after_last_validpts += aml_dev->dts_hd.outlen_pcm;

        patch->decoder_offset +=
            remain_size + ret - aml_dev->dts_hd.remain_size;
        patch->dtv_pcm_readed += ret;
        /* +[SE] [BUG][SWPL-22893][yinli.xia]
              add: reset decode data when replay video*/
        if (aml_dev->debug_flag) {
            ALOGI("after decode: aml_dev->ddp.remain_size=%d\n", aml_dev->ddp.remain_size);
        }
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
    } else {
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
        usleep(5000);
    }

    return ret;
}

int audio_dtv_patch_output_dolby_dual_decoder(struct aml_audio_patch *patch,
                                         struct audio_stream_out *stream_out, int *apts_diff)
{
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream_out;
    //int apts_diff = 0;

    unsigned char main_head[32];
    unsigned char ad_head[32];
    int main_frame_size = 0, last_main_frame_size = 0, main_head_offset = 0, main_head_left = 0;
    int ad_frame_size = 0, ad_head_offset = 0, ad_head_left = 0;
    unsigned char mixbuffer[EAC3_IEC61937_FRAME_SIZE];
    unsigned char ad_buffer[EAC3_IEC61937_FRAME_SIZE];
    uint16_t *p16_mixbuff = NULL;
    uint32_t *p32_mixbuff = NULL;
    int main_size = 0, ad_size = 0, mix_size = 0;
    int dd_bsmod = 0, remain_size = 0;
    unsigned long long all_pcm_len1 = 0;
    unsigned long long all_pcm_len2 = 0;
    unsigned long long all_zero_len = 0;
    int main_avail = get_buffer_read_space(ringbuffer);
    int ad_avail = dtv_assoc_get_avail();
    dtv_assoc_get_main_frame_size(&last_main_frame_size);
    char buff[32];
    int ret = 0;

    //ALOGI("AD main_avail=%d ad_avail=%d last_main_frame_size = %d",
    //main_avail, ad_avail, last_main_frame_size);
    if ((last_main_frame_size == 0 && main_avail >= 6144)
        || (last_main_frame_size != 0 && main_avail >= last_main_frame_size)) {
        if (!patch->first_apts_lookup_over) {
            *apts_diff = dtv_set_audio_latency(0);
            if (!dtv_firstapts_lookup_over(patch, aml_dev, main_avail, false) || main_avail < 512 * 2) {
                ALOGI("hold the aduio for cache data, avail %d", main_avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            patch->first_apts_lookup_over = 1;
            ALOGI("[%s,%d] dtv_audio_tune %d-> AUDIO_LOOKUP\n", __FUNCTION__, __LINE__, patch->dtv_audio_tune);
            patch->dtv_audio_tune = AUDIO_LOOKUP;
            //ALOGI("dtv_audio_tune audio_lookup\n");
            clean_dtv_patch_pts(patch);
        } else if (patch->dtv_audio_tune == AUDIO_BREAK) {
            int a_discontinue = get_audio_discontinue(patch);
            dtv_set_audio_latency(*apts_diff);
            if (a_discontinue == 0) {
                ALOGI("audio is resumed\n");
            } else if (!dtv_firstapts_lookup_over(patch, aml_dev, main_avail, true)) {
                ALOGI("hold the aduio for cache data, avail %d", main_avail);
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(5000);
                return -EAGAIN;
            }
            ALOGI("[%s,%d] dtv_audio_tune AUDIO_BREAK-> AUDIO_LOOKUP\n", __FUNCTION__, __LINE__);
            patch->dtv_audio_tune = AUDIO_LOOKUP;
            //ALOGI("dtv_audio_tune audio_lookup\n");
            clean_dtv_patch_pts(patch);
        } else if (patch->dtv_audio_tune == AUDIO_DROP) {
            dtv_do_drop_insert_ac3(patch, stream_out);
            if (patch->dtv_apts_lookup < 0 ||
                (patch->dtv_apts_lookup > 0 &&
                aml_out->need_drop_size == 0)) {
                clean_dtv_patch_pts(patch);
                patch->dtv_apts_lookup = 0;
                patch->ac3_pcm_dropping = 0;
                ALOGI("[%s,%d] dtv_audio_tune AUDIO_DROP-> AUDIO_LATENCY\n", __FUNCTION__, __LINE__);
                patch->dtv_audio_tune = AUDIO_LATENCY;
            }
        }

        //dtv_assoc_get_main_frame_size(&main_frame_size);
        //main_frame_size = 0, get from data
        while (main_frame_size == 0 && main_avail >= (int)sizeof(main_head)) {
            memset(main_head, 0, sizeof(main_head));
            ret = ring_buffer_read(ringbuffer, main_head, sizeof(main_head));
            main_frame_size = dcv_decoder_get_framesize(main_head,
                              ret, &main_head_offset);
            main_avail -= ret;
            if (main_frame_size != 0) {
                main_head_left = ret - main_head_offset;
                //ALOGI("AD main_frame_size=%d  ", main_frame_size);
            }
        }
        dtv_assoc_set_main_frame_size(main_frame_size);

        if (main_frame_size > 0 && (main_avail >= main_frame_size - main_head_left)) {
            //dtv_assoc_set_main_frame_size(main_frame_size);
            //dtv_assoc_set_ad_frame_size(ad_frame_size);
            //read left of frame;
            if (main_head_left > 0) {
                memcpy(patch->out_buf, main_head + main_head_offset, main_head_left);
            }
            ret = ring_buffer_read(ringbuffer, (unsigned char *)patch->out_buf + main_head_left ,
                                   main_frame_size - main_head_left);
            if (ret == 0) {
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                /*ALOGE("%s(), ring_buffer read 0 data!", __func__);*/
                usleep(1000);
                return -EAGAIN;

            }
            dtv_assoc_audio_cache(1);
            main_size = ret + main_head_left;
        } else {
            dtv_audio_gap_monitor(patch);
            pthread_mutex_unlock(&(patch->dtv_output_mutex));
            usleep(1000);
            return -EAGAIN;
        }

        if (ad_avail > 0) {
            //dtv_assoc_get_ad_frame_size(&ad_frame_size);
            //ad_frame_size = 0, get from data
            while (ad_frame_size == 0 && ad_avail >= (int)sizeof(ad_head)) {
                memset(ad_head, 0, sizeof(ad_head));
                ret = dtv_assoc_read(ad_head, sizeof(ad_head));
                ad_frame_size = dcv_decoder_get_framesize(ad_head,
                                ret, &ad_head_offset);
                ad_avail -= ret;
                //ALOGI("ad_avail=%d ret=%d ad_frame_size=%d", ad_avail, ret, ad_frame_size);
                if (ad_frame_size != 0) {
                    ad_head_left = ret - ad_head_offset;
                    //ALOGI("AD ad_frame_size=%d  ", ad_frame_size);
                }
                if (ret == 0) {
                    ALOGI("ad doesn't have data, don't wait");
                    break;
                }
            }
        }

        memset(ad_buffer, 0, sizeof(ad_buffer));
        if (ad_frame_size > 0 && (ad_avail >= ad_frame_size - ad_head_left)) {
            if (ad_head_left > 0) {
                memcpy(ad_buffer, ad_head + ad_head_offset, ad_head_left);
            }
            ret = dtv_assoc_read(ad_buffer + ad_head_left, ad_frame_size - ad_head_left);
            if (ret == 0) {
                ad_size = 0;
            } else {
                ad_size = ret + ad_head_left;
            }
        } else {
            ad_size = 0;
        }
        if (aml_dev->associate_audio_mixing_enable == 0) {
            ad_size = 0;
        }

        if (aml_out->hal_internal_format != patch->aformat) {
            aml_out->hal_format = aml_out->hal_internal_format = patch->aformat;
            get_sink_format(stream_out);
        }

        if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
            remain_size = dolby_ms12_get_main_buffer_avail(NULL);
            dolby_ms12_get_pcm_output_size(&all_pcm_len1, &all_zero_len);
        } else {
            remain_size = aml_dev->ddp.remain_size;
        }

        //package iec61937
        memset(mixbuffer, 0, sizeof(mixbuffer));
        //papbpcpd
        p16_mixbuff = (uint16_t*)mixbuffer;
        p16_mixbuff[0] = 0xf872;
        p16_mixbuff[1] = 0x4e1f;
        if (patch->aformat == AUDIO_FORMAT_AC3) {
            dd_bsmod = 6;
            p16_mixbuff[2] = ((dd_bsmod & 7) << 8) | 1;
            if (ad_size == 0) {
                p16_mixbuff[3] = (main_size + sizeof(mute_dd_frame)) * 8;
            } else {
                p16_mixbuff[3] = (main_size + ad_size) * 8;
            }
        } else {
            dd_bsmod = 12;
            p16_mixbuff[2] = ((dd_bsmod & 7) << 8) | 21;
            if (ad_size == 0) {
                p16_mixbuff[3] = main_size + sizeof(mute_ddp_frame);
            } else {
                p16_mixbuff[3] = main_size + ad_size;
            }
        }
        mix_size += 8;
        //main
        memcpy(mixbuffer + mix_size, patch->out_buf, main_size);
        mix_size += main_size;
        //ad
        if (ad_size == 0) {
            if (patch->aformat == AUDIO_FORMAT_AC3) {
                memcpy(mixbuffer + mix_size, mute_dd_frame, sizeof(mute_dd_frame));
            } else {
                memcpy(mixbuffer + mix_size, mute_ddp_frame, sizeof(mute_ddp_frame));
            }
        } else {
            memcpy(mixbuffer + mix_size, ad_buffer, ad_size);
        }

        if (patch->aformat == AUDIO_FORMAT_AC3) {//ac3 iec61937 package size 6144
            ret = out_write_new(stream_out, mixbuffer, AC3_IEC61937_FRAME_SIZE);
        } else {//eac3 iec61937 package size 6144*4
            ret = out_write_new(stream_out, mixbuffer, EAC3_IEC61937_FRAME_SIZE);
        }

        if ((mixbuffer[8] != 0xb && mixbuffer[8] != 0x77)
            || (mixbuffer[9] != 0xb && mixbuffer[9] != 0x77)
            || (mixbuffer[mix_size] != 0xb && mixbuffer[mix_size] != 0x77)
            || (mixbuffer[mix_size + 1] != 0xb && mixbuffer[mix_size + 1] != 0x77)) {
            ALOGD("AD mix main_size=%d ad_size=%d wirte_size=%d 0x%x 0x%x 0x%x 0x%x", main_size, ad_size, ret,
                  mixbuffer[8], mixbuffer[9], mixbuffer[mix_size], mixbuffer[mix_size + 1]);
        }

        if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
            int size = dolby_ms12_get_main_buffer_avail(NULL);
            dolby_ms12_get_pcm_output_size(&all_pcm_len2, &all_zero_len);
            patch->decoder_offset += remain_size + main_size - size;
            patch->outlen_after_last_validpts += (unsigned int)(all_pcm_len2 - all_pcm_len1);
            //ALOGD("remain_size %d,size %d,main_size %d,validpts %d",remain_size,size,main_size,patch->outlen_after_last_validpts);
            patch->dtv_pcm_readed += main_size;

        } else {
            patch->outlen_after_last_validpts += aml_dev->ddp.outlen_pcm;
            patch->decoder_offset += remain_size + main_size - aml_dev->ddp.remain_size;
            patch->dtv_pcm_readed += main_size;
        }

        pthread_mutex_unlock(&(patch->dtv_output_mutex));
    } else {
        dtv_audio_gap_monitor(patch);
        pthread_mutex_unlock(&(patch->dtv_output_mutex));
        usleep(1000);
    }

    return ret;
}

void *audio_dtv_patch_output_threadloop(void *data)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)data;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct audio_stream_out *stream_out = NULL;
    struct aml_stream_out *aml_out = NULL;
    struct audio_config stream_config;
    int write_bytes = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    int ret;
    int apts_diff = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    ALOGI("++%s live ", __FUNCTION__);
    // FIXME: get actual configs
    stream_config.sample_rate = 48000;
    stream_config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    stream_config.format = AUDIO_FORMAT_PCM_16_BIT;
    /*
    may we just exit from a direct active stream playback
    still here.we need remove to standby to new playback
    */
    pthread_mutex_lock(&aml_dev->lock);
    aml_out = direct_active(aml_dev);
    if (aml_out) {
        ALOGI("%s live stream %p active,need standby aml_out->usecase:%d ",
              __func__, aml_out, aml_out->usecase);
        pthread_mutex_unlock(&aml_dev->lock);
        /*
        there are several output cases. if there are no ms12 or submixing modules.
        we will output raw/lpcm directly.we need close device directly.
        we need call standy function to release the direct stream
        */
        if (out_standby_direct == aml_out->stream.common.standby)
             out_standby_direct((struct audio_stream *)aml_out);
        else
            out_standby_new((struct audio_stream *)aml_out);
        pthread_mutex_lock(&aml_dev->lock);
        if (aml_dev->need_remove_conti_mode == true) {
            ALOGI("%s,conntinous mode still there,release ms12 here", __func__);
            aml_dev->need_remove_conti_mode = false;
            aml_dev->continuous_audio_mode = 0;
        }
    } else {
        ALOGI("++%s live cant get the aml_out now!!!\n ", __FUNCTION__);
    }
    aml_dev->mix_init_flag = false;
    pthread_mutex_unlock(&aml_dev->lock);
#ifdef TV_AUDIO_OUTPUT
    patch->output_src = AUDIO_DEVICE_OUT_SPEAKER;
#else
    patch->output_src = AUDIO_DEVICE_OUT_AUX_DIGITAL;
#endif
    if (aml_dev->out_device & AUDIO_DEVICE_OUT_ALL_A2DP)
        patch->output_src = aml_dev->out_device;

    ret = adev_open_output_stream_new(patch->dev, 0,
                                      patch->output_src,        // devices_t
                                      AUDIO_OUTPUT_FLAG_DIRECT, // flags
                                      &stream_config, &stream_out, "AML_TV_SOURCE");
    if (ret < 0) {
        ALOGE("live open output stream fail, ret = %d", ret);
        goto exit_open;
    }

    ALOGI("++%s live create a output stream success now!!!\n ", __FUNCTION__);

    patch->out_buf_size = write_bytes * EAC3_MULTIPLIER;
    patch->out_buf = calloc(1, patch->out_buf_size);
    if (!patch->out_buf) {
        ret = -ENOMEM;
        goto exit_outbuf;
    }
    patch->dtv_audio_mode = get_dtv_audio_mode();
    patch->dtv_audio_tune = AUDIO_FREE;
    patch->first_apts_lookup_over = 0;
    patch->tsync_mode = TSYNC_MODE_PCRMASTER;
    patch->dtv_faded_out = 0;
    patch->dtv_ac3_fmsize = 0;
    ALOGI("++%s live start output pcm now patch->output_thread_exit %d!!!\n ",
          __FUNCTION__, patch->output_thread_exit);

    prctl(PR_SET_NAME, (unsigned long)"audio_output_patch");

    while (!patch->output_thread_exit) {
        if (patch->dtv_decoder_state == AUDIO_DTV_PATCH_DECODER_STATE_PAUSE) {
            usleep(1000);
            continue;
        }

        pthread_mutex_lock(&(patch->dtv_output_mutex));
        //int period_mul =
        //    (patch->aformat == AUDIO_FORMAT_E_AC3) ? EAC3_MULTIPLIER : 1;

        if ((patch->aformat == AUDIO_FORMAT_AC3) ||
            (patch->aformat == AUDIO_FORMAT_E_AC3)) {
            ALOGV("AD %d %d %d", aml_dev->dolby_lib_type, aml_dev->dual_decoder_support, aml_dev->sub_apid);
            if ((aml_dev->dual_decoder_support
                 && VALID_PID(aml_dev->sub_apid)
                 /*&& aml_dev->associate_audio_mixing_enable*/)) {
                ret = audio_dtv_patch_output_dolby_dual_decoder(patch, stream_out, &apts_diff);

            } else {
                ret = audio_dtv_patch_output_dolby(patch, stream_out, &apts_diff);
            }
        } else if (patch->aformat == AUDIO_FORMAT_DTS) {
            ret = audio_dtv_patch_output_dts(patch, stream_out);
        } else {
            ret = audio_dtv_patch_output_default(patch, stream_out, &apts_diff);
        }
    }
    free(patch->out_buf);
exit_outbuf:
    adev_close_output_stream_new(dev, stream_out);
exit_open:
    if (aml_dev->audio_ease) {
        aml_dev->patch_start = false;
    }
    if (get_video_delay() != 0) {
        set_video_delay(0);
    }
    decoder_stop_audio(patch);
    ALOGI("--%s live ", __FUNCTION__);
    return ((void *)0);
}




static void patch_thread_get_cmd(struct aml_audio_patch *patch, int *cmd)
{
    if (dtv_cmd_list.initd == 0) {
        *cmd = AUDIO_DTV_PATCH_CMD_NULL;
        return;
    }
    if (patch == NULL) {
        *cmd = AUDIO_DTV_PATCH_CMD_NULL;
        return;
    }
    if (patch->input_thread_exit == 1) {
        *cmd = AUDIO_DTV_PATCH_CMD_NULL;
        return;
    }
    if (dtv_patch_cmd_is_empty() == 1) {
        *cmd = AUDIO_DTV_PATCH_CMD_NULL;
    } else {
        *cmd = dtv_patch_get_cmd();
    }
}

static void *audio_dtv_patch_process_threadloop(void *data)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)data;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct audio_stream_in *stream_in = NULL;
    struct audio_config stream_config;
    // FIXME: add calc for read_bytes;
    int read_bytes = DEFAULT_CAPTURE_PERIOD_SIZE * CAPTURE_PERIOD_COUNT;
    int ret = 0, retry = 0;
    audio_format_t cur_aformat;
    unsigned int adec_handle;
    int cmd = AUDIO_DTV_PATCH_CMD_NUM;
    struct dolby_ddp_dec *ddp_dec = &(aml_dev->ddp);
    struct dca_dts_dec *dts_dec = &(aml_dev->dts_hd);
    patch->sample_rate = stream_config.sample_rate = 48000;
    patch->chanmask = stream_config.channel_mask = AUDIO_CHANNEL_IN_STEREO;
    patch->aformat = stream_config.format = AUDIO_FORMAT_PCM_16_BIT;

    ALOGI("++%s live \n", __FUNCTION__);
    patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_INIT;

    while (!patch->input_thread_exit) {
        pthread_mutex_lock(&patch->dtv_input_mutex);

        switch (patch->dtv_decoder_state) {
        case AUDIO_DTV_PATCH_DECODER_STATE_INIT: {
            ALOGI("++%s live now  open the audio decoder now !\n", __FUNCTION__);
            dtv_patch_input_open(&adec_handle, dtv_patch_pcm_write,
                                 dtv_patch_status_info,
                                 dtv_patch_audio_info, patch);
            patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_START;
        }
        break;
        case AUDIO_DTV_PATCH_DECODER_STATE_START:

            if (patch->input_thread_exit == 1) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                goto exit;
            }
            patch_thread_get_cmd(patch, &cmd);
            if (cmd == AUDIO_DTV_PATCH_CMD_NULL) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }

            ring_buffer_reset(&patch->aml_ringbuffer);

            if (cmd == AUDIO_DTV_PATCH_CMD_START) {
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_RUNING;
                memset(&patch->dtv_resample, 0, sizeof(struct resample_para));
                if (patch->resample_outbuf) {
                    memset(patch->resample_outbuf, 0, OUTPUT_BUFFER_SIZE * 3);
                }
                dtv_patch_input_start(adec_handle, patch->dtv_aformat,
                                      patch->dtv_has_video,
                                      aml_dev->dual_decoder_support,
                                      aml_dev->associate_audio_mixing_enable,
                                      aml_dev->mixing_level);
                create_dtv_output_stream_thread(patch);
                ALOGI("++%s live now  start the audio decoder now !\n",
                      __FUNCTION__);
                patch->dtv_first_apts_flag = 0;
                patch->outlen_after_last_validpts = 0;
                patch->last_valid_pts = 0;
                patch->last_out_pts = 0;
                patch->first_apts_lookup_over = 0;
                patch->ac3_pcm_dropping = 0;
                patch->last_audio_delay = 0;
                if (patch->dtv_aformat == ACODEC_FMT_AC3) {
                    patch->aformat = AUDIO_FORMAT_AC3;
                    ddp_dec->is_iec61937 = false;
                    patch->decoder_offset = 0;
                    patch->first_apts_lookup_over = 0;
                } else if (patch->dtv_aformat == ACODEC_FMT_EAC3) {
                    patch->aformat = AUDIO_FORMAT_E_AC3;
                    ddp_dec->is_iec61937 = false;
                    patch->decoder_offset = 0;
                    patch->first_apts_lookup_over = 0;
                } else if (patch->dtv_aformat == ACODEC_FMT_DTS) {
                    patch->aformat = AUDIO_FORMAT_DTS;
                    dca_decoder_init_patch(dts_dec);
                    dts_dec->is_dtv = true;
                    patch->decoder_offset = 0;
                    patch->first_apts_lookup_over = 0;
                } else {
                    patch->aformat = AUDIO_FORMAT_PCM_16_BIT;
                    patch->decoder_offset = 0;
                    patch->first_apts_lookup_over = 0;
                }
                patch->dtv_pcm_readed = patch->dtv_pcm_writed = 0;
                create_dtv_output_stream_thread(patch);
            } else {
                ALOGI("++%s line %d  live state unsupport state %d cmd %d !\n",
                      __FUNCTION__, __LINE__, patch->dtv_decoder_state, cmd);
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            break;
        case AUDIO_DTV_PATCH_DECODER_STATE_RUNING:

            if (patch->input_thread_exit == 1) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                goto exit;
            }
            /*[SE][BUG][SWPL-17416][chengshun] maybe sometimes subafmt and subapid not set before dtv patch start*/
            if (aml_dev->ad_start_enable == 0 && VALID_PID(aml_dev->sub_apid) && VALID_AD_FMT(aml_dev->sub_afmt)) {
                int ad_start_flag = dtv_assoc_audio_start(1, aml_dev->sub_apid, aml_dev->sub_afmt);
                if (ad_start_flag == 0) {
                    aml_dev->ad_start_enable = 1;
                }
            }

            patch_thread_get_cmd(patch, &cmd);
            if (cmd == AUDIO_DTV_PATCH_CMD_NULL) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            if (cmd == AUDIO_DTV_PATCH_CMD_PAUSE) {
                ALOGI("++%s live now start  pause  the audio decoder now \n",
                      __FUNCTION__);
                dtv_patch_input_pause(adec_handle);
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_PAUSE;
                ALOGI("++%s live now end  pause  the audio decoder now \n",
                      __FUNCTION__);
            } else if (cmd == AUDIO_DTV_PATCH_CMD_STOP) {
                ALOGI("++%s live now  stop  the audio decoder now \n",
                      __FUNCTION__);
                release_dtv_output_stream_thread(patch);
                dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
                dtv_do_ease_out(aml_dev);
                dtv_patch_input_stop(adec_handle);
                dtv_assoc_audio_stop(1);
                dts_dec->is_dtv = false;
                aml_dev->ad_start_enable = 0;
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_INIT;
            } else {
                ALOGI("++%s line %d  live state unsupport state %d cmd %d !\n",
                      __FUNCTION__, __LINE__, patch->dtv_decoder_state, cmd);
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }

            break;

        case AUDIO_DTV_PATCH_DECODER_STATE_PAUSE:
            patch_thread_get_cmd(patch, &cmd);
            if (cmd == AUDIO_DTV_PATCH_CMD_NULL) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            if (cmd == AUDIO_DTV_PATCH_CMD_RESUME) {
                dtv_patch_input_resume(adec_handle);
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_RUNING;
            } else if (cmd == AUDIO_DTV_PATCH_CMD_STOP) {
                ALOGI("++%s live now  stop  the audio decoder now \n", __FUNCTION__);
                release_dtv_output_stream_thread(patch);
                dtv_adjust_output_clock(patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
                dtv_do_ease_out(aml_dev);
                dtv_patch_input_stop(adec_handle);
                dtv_assoc_audio_stop(1);
                aml_dev->ad_start_enable = 0;
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_INIT;
            } else {
                ALOGI("++%s line %d  live state unsupport state %d cmd %d !\n",
                      __FUNCTION__, __LINE__, patch->dtv_decoder_state, cmd);
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            break;
        default:
            if (patch->input_thread_exit == 1) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                goto exit;
            }
            break;
        }
        pthread_mutex_unlock(&patch->dtv_input_mutex);
    }

exit:
    ALOGI("++%s now  live  release  the audio decoder", __FUNCTION__);
    dtv_patch_input_stop(adec_handle);
    dtv_assoc_audio_stop(1);
    aml_dev->ad_start_enable = 0;
    pthread_exit(NULL);
}

void release_dtvin_buffer(struct aml_audio_patch *patch)
{
    if (patch->dtvin_buffer_inited == 1) {
        patch->dtvin_buffer_inited = 0;
        ring_buffer_release(&(patch->dtvin_ringbuffer));
    }
}

void dtv_in_write(struct audio_stream_out *stream, const void* buffer, size_t bytes)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    struct aml_audio_patch *patch = adev->audio_patch;
    int abuf_level = 0;

    if (stream == NULL || buffer == NULL || bytes == 0) {
        ALOGI("[%s] pls check the input parameters \n", __FUNCTION__);
        return ;
    }
    if ((adev->patch_src == SRC_DTV) && (patch->dtvin_buffer_inited == 1)) {
        abuf_level = get_buffer_write_space(&patch->dtvin_ringbuffer);
        if (abuf_level <= (int)bytes) {
            ALOGI("[%s] dtvin ringbuffer is full", __FUNCTION__);
            return;
        }
        ring_buffer_write(&patch->dtvin_ringbuffer, (unsigned char *)buffer, bytes, UNCOVER_WRITE);
    }
    //ALOGI("[%s] dtvin write ringbuffer successfully,abuf_level=%d", __FUNCTION__,abuf_level);
}
int dtv_in_read(struct audio_stream_in *stream, void* buffer, size_t bytes)
{
    int ret = 0;
    unsigned int es_length = 0;
    struct aml_stream_in *in = (struct aml_stream_in *)stream;
    struct aml_audio_device *adev = in->dev;

    if (stream == NULL || buffer == NULL || bytes == 0) {
        ALOGI("[%s] pls check the input parameters \n", __FUNCTION__);
    }

    struct aml_audio_patch *patch = adev->audio_patch;
    struct dolby_ddp_dec *ddp_dec = & (adev->ddp);
    //ALOGI("[%s] patch->aformat=0x%x patch->dtv_decoder_ready=%d bytes:%d\n", __FUNCTION__,patch->aformat,patch->dtv_decoder_ready,bytes);

    if (patch->dtvin_buffer_inited == 1) {
        int abuf_level = get_buffer_read_space(&patch->dtvin_ringbuffer);
        if (abuf_level <= (int)bytes) {
            memset(buffer, 0, sizeof(unsigned char)* bytes);
            ret = bytes;
        } else {
            ret = ring_buffer_read(&patch->dtvin_ringbuffer, (unsigned char *)buffer, bytes);
            //ALOGI("[%s] abuf_level =%d ret=%d\n", __FUNCTION__,abuf_level,ret);
        }
        return bytes;
    } else {
        memset(buffer, 0, sizeof(unsigned char)* bytes);
        ret = bytes;
        return ret;
    }
    return ret;
}

int audio_set_spdif_clock(struct aml_stream_out *stream, int type)
{
    struct aml_audio_device *dev = stream->dev;
    if (!dev || !dev->audio_patch) {
        return 0;
    }
    if (dev->patch_src != SRC_DTV || !dev->audio_patch->is_dtv_src) {
        return 0;
    }
    if (!(dev->usecase_masks > 1)) {
        return 0;
    }
    if (type == AML_DOLBY_DIGITAL) {
        dev->audio_patch->spdif_format_set = 1;
    } else if (type == AML_DOLBY_DIGITAL_PLUS) {
        dev->audio_patch->spdif_format_set = 1;
    } else if (type == AML_DTS) {
        dev->audio_patch->spdif_format_set = 1;
    } else if (type == AML_DTS_HD) {
        dev->audio_patch->spdif_format_set = 1;
    } else if (type == AML_STEREO_PCM) {
        dev->audio_patch->spdif_format_set = 0;
    } else {
        dev->audio_patch->spdif_format_set = 0;
    }
    if (alsa_device_is_auge()) {
        if (dev->audio_patch->spdif_format_set) {
            if (stream->hal_internal_format == AUDIO_FORMAT_E_AC3 &&
                dev->bHDMIARCon && !is_dual_output_stream((struct audio_stream_out *)stream)) {
                dev->audio_patch->dtv_default_spdif_clock =
                    stream->config.rate * 128 * 4;
            } else {
                dev->audio_patch->dtv_default_spdif_clock =
                    stream->config.rate * 128;
            }
        } else {
            dev->audio_patch->dtv_default_spdif_clock =
                DEFAULT_I2S_OUTPUT_CLOCK / 2;
        }
    } else {
        if (dev->audio_patch->spdif_format_set) {
            dev->audio_patch->dtv_default_spdif_clock =
                stream->config.rate * 128 * 4;
        } else {
            dev->audio_patch->dtv_default_spdif_clock =
                DEFAULT_I2S_OUTPUT_CLOCK;
        }
    }
    dev->audio_patch->spdif_step_clk =
        dev->audio_patch->dtv_default_spdif_clock / 256;
    dev->audio_patch->i2s_step_clk =
        DEFAULT_I2S_OUTPUT_CLOCK / 256;
    ALOGI("[%s] type=%d,spdif %d,dual %d, arc %d", __FUNCTION__, type, dev->audio_patch->spdif_step_clk,
          is_dual_output_stream((struct audio_stream_out *)stream), dev->bHDMIARCon);
    dtv_adjust_output_clock(dev->audio_patch, DIRECT_NORMAL, DEFAULT_DTV_ADJUST_CLOCK);
    return 0;
}

#define AVSYNC_SAMPLE_INTERVAL (50)
#define AVSYNC_SAMPLE_MAX_CNT (10)

static int create_dtv_output_stream_thread(struct aml_audio_patch *patch)
{
    int ret = 0;
    ALOGI("++%s   ---- %d\n", __FUNCTION__, patch->ouput_thread_created);

    if (patch->ouput_thread_created == 0) {
        patch->output_thread_exit = 0;
        pthread_mutex_init(&patch->dtv_output_mutex, NULL);
        ret = pthread_create(&(patch->audio_output_threadID), NULL,
                             audio_dtv_patch_output_threadloop, patch);
        if (ret != 0) {
            ALOGE("%s, Create output thread fail!\n", __FUNCTION__);
            pthread_mutex_destroy(&patch->dtv_output_mutex);
            return -1;
        }

        patch->ouput_thread_created = 1;
    }
    ALOGI("--%s", __FUNCTION__);
    return 0;
}

static int release_dtv_output_stream_thread(struct aml_audio_patch *patch)
{
    int ret = 0;
    ALOGI("++%s   ---- %d\n", __FUNCTION__, patch->ouput_thread_created);
    if (patch->ouput_thread_created == 1) {
        patch->output_thread_exit = 1;
        pthread_join(patch->audio_output_threadID, NULL);
        pthread_mutex_destroy(&patch->dtv_output_mutex);

        patch->ouput_thread_created = 0;
    }
    ALOGI("--%s", __FUNCTION__);

    return 0;
}

int create_dtv_patch_l(struct audio_hw_device *dev, audio_devices_t input,
                       audio_devices_t output __unused)
{
    struct aml_audio_patch *patch;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    int period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    pthread_attr_t attr;
    struct sched_param param;

    int ret = 0;
    // ALOGI("++%s live period_size %d\n", __func__, period_size);
    //pthread_mutex_lock(&aml_dev->patch_lock);
    if (aml_dev->reset_dtv_audio) {
        aml_dev->reset_dtv_audio = 0;
    }
    if (aml_dev->audio_patch) {
        ALOGD("%s: patch exists, first release it", __func__);
        if (aml_dev->audio_patch->is_dtv_src) {
            release_dtv_patch_l(aml_dev);
        } else {
            release_patch_l(aml_dev);
        }
    }
    patch = calloc(1, sizeof(*patch));
    if (!patch) {
        ret = -1;
        goto err;
    }
    if (aml_dev->dtv_i2s_clock == 0) {
        aml_dev->dtv_i2s_clock = aml_mixer_ctrl_get_int(&(aml_dev->alsa_mixer), AML_MIXER_ID_CHANGE_I2S_PLL);
        aml_dev->dtv_spidif_clock = aml_mixer_ctrl_get_int(&(aml_dev->alsa_mixer), AML_MIXER_ID_CHANGE_SPIDIF_PLL);
    }

    memset(cmd_array, 0, sizeof(cmd_array));
    // save dev to patch
    patch->dev = dev;
    patch->input_src = input;
    patch->aformat = AUDIO_FORMAT_PCM_16_BIT;
    patch->avsync_sample_max_cnt = AVSYNC_SAMPLE_MAX_CNT;
    patch->is_dtv_src = true;

    patch->output_thread_exit = 0;
    patch->input_thread_exit = 0;
    patch->i2s_div_factor = property_get_int32("media.audio_hal.div", 16);
    if (patch->i2s_div_factor == 0)
        patch->i2s_div_factor = 1;
    aml_dev->audio_patch = patch;
    pthread_mutex_init(&patch->mutex, NULL);
    pthread_cond_init(&patch->cond, NULL);
    pthread_mutex_init(&patch->dtv_input_mutex, NULL);

    ret = ring_buffer_init(&(patch->aml_ringbuffer),
                           4 * period_size * PATCH_PERIOD_COUNT * 10);
    if (ret < 0) {
        ALOGE("Fail to init audio ringbuffer!");
        goto err_ring_buf;
    }

    if (aml_dev->useSubMix) {
        // switch normal stream to old tv mode writing
        switchNormalStream(aml_dev->active_outputs[STREAM_PCM_NORMAL], 0);
    }

    ret = pthread_create(&(patch->audio_input_threadID), NULL,
                         audio_dtv_patch_process_threadloop, patch);
    if (ret != 0) {
        ALOGE("%s, Create process thread fail!\n", __FUNCTION__);
        goto err_in_thread;
    }
    create_dtv_output_stream_thread(patch);
    //pthread_mutex_unlock(&aml_dev->patch_lock);
    init_cmd_list();
    if (aml_dev->tuner2mix_patch) {
        ret = ring_buffer_init(&patch->dtvin_ringbuffer, 4 * 8192 * 2 * 16);
        ALOGI("[%s] aring_buffer_init ret=%d\n", __FUNCTION__, ret);
        if (ret == 0) {
            patch->dtvin_buffer_inited = 1;
        }
    }
    dtv_assoc_init();
    patch->dtv_aformat = aml_dev->dtv_aformat;
    patch->dtv_output_clock = 0;
    patch->dtv_default_i2s_clock = aml_dev->dtv_i2s_clock;
    patch->dtv_default_spdif_clock = aml_dev->dtv_spidif_clock;
    patch->spdif_format_set = 0;
    patch->avsync_callback = dtv_avsync_process;
    patch->spdif_step_clk = 0;
    patch->i2s_step_clk = 0;

    ALOGI("--%s", __FUNCTION__);
    return 0;
err_parse_thread:
err_out_thread:
    patch->input_thread_exit = 1;
    pthread_join(patch->audio_input_threadID, NULL);
err_in_thread:
    ring_buffer_release(&(patch->aml_ringbuffer));
err_ring_buf:
    free(patch);
err:
    //pthread_mutex_unlock(&aml_dev->patch_lock);
    return ret;
}

int release_dtv_patch_l(struct aml_audio_device *aml_dev)
{
    if (aml_dev == NULL) {
        ALOGI("[%s]release the dtv patch failed  aml_dev == NULL\n", __FUNCTION__);
        return -1;
    }
    struct aml_audio_patch *patch = aml_dev->audio_patch;

    ALOGI("++%s live\n", __FUNCTION__);
    //pthread_mutex_lock(&aml_dev->patch_lock);
    if (patch == NULL) {
        ALOGI("release the dtv patch failed  patch == NULL\n");
        //pthread_mutex_unlock(&aml_dev->patch_lock);
        return -1;
    }
    deinit_cmd_list();
    patch->input_thread_exit = 1;
    pthread_join(patch->audio_input_threadID, NULL);

    pthread_mutex_destroy(&patch->dtv_input_mutex);
    if (patch->resample_outbuf) {
        free(patch->resample_outbuf);
        patch->resample_outbuf = NULL;
    }
    release_dtv_output_stream_thread(patch);
    release_dtvin_buffer(patch);
    dtv_assoc_deinit();
    ring_buffer_release(&(patch->aml_ringbuffer));
    free(patch);
    if (aml_dev->start_mute_flag != 0)
        aml_dev->start_mute_flag = 0;
    aml_dev->audio_patch = NULL;
    aml_dev->sub_apid = -1;
    aml_dev->sub_afmt = ACODEC_FMT_NULL;
    aml_dev->dual_decoder_support = 0;
    aml_dev->associate_audio_mixing_enable = 0;
    aml_dev->mixing_level = 0;
    dtv_check_audio_reset(aml_dev);
    ALOGI("--%s", __FUNCTION__);
    //pthread_mutex_unlock(&aml_dev->patch_lock);
    if (aml_dev->useSubMix) {
        switchNormalStream(aml_dev->active_outputs[STREAM_PCM_NORMAL], 1);
    }
    return 0;
}

int create_dtv_patch(struct audio_hw_device *dev, audio_devices_t input,
                     audio_devices_t output)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    int ret = 0;

    pthread_mutex_lock(&aml_dev->patch_lock);
    ret = create_dtv_patch_l(dev, input, output);
    pthread_mutex_unlock(&aml_dev->patch_lock);

    return ret;
}

int release_dtv_patch(struct aml_audio_device *aml_dev)
{
    int ret = 0;

    dtv_do_ease_out(aml_dev);
    pthread_mutex_lock(&aml_dev->patch_lock);
    ret = release_dtv_patch_l(aml_dev);
    pthread_mutex_unlock(&aml_dev->patch_lock);

    return ret;
}
