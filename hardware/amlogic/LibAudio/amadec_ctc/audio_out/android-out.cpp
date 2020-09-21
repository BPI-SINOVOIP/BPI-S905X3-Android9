/**
 * \file android-out.cpp/
 * \brief  Functions of Auduo output control for Android Platform
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <utils/threads.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <media/AudioParameter.h>
#include <system/audio_policy.h>
#include <cutils/properties.h>

extern "C" {
#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <log-print.h>
#include "aml_resample.h"
#include <audiodsp_update_format.h>
#include <Amsysfsutils.h>
#include <amthreadpool.h>
}
#include "adec-external-ctrl.h"

#define  LOG_TAG    "android-out"

namespace android
{

#if ANDROID_PLATFORM_SDK_VERSION >= 21
//android 5.0 level= 20
#define AUDIO_FORMAT_EAC3 AUDIO_FORMAT_E_AC3
#endif
#if ANDROID_PLATFORM_SDK_VERSION >= 23
#define AUDIO_FORMAT_DTS_MASTER  0x0E000000UL
#endif
#define DOLBY_SYSTEM_CHANNEL "ds1.audio.multichannel.support"
static Mutex mLock;
static Mutex mLock_raw;
//get default output  sample rate which maybe changed by raw output
static  int default_sr = 48000;
static int fill_audiotrack_zero = 0;
static int buffering_audio_data = 0;
static int skip_unnormal_discontinue = 0;
static int unnormal_discontinue = 0;
static int unnormal_discontinue1 = 0;
extern "C" int get_audio_decoder(void);
static int get_digitalraw_mode(struct aml_audio_dec* audec)
{
    //return amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
    return audec->raw_enable;
}
#define DTSETC_DECODE_VERSION_CORE  350
#define DTSETC_DECODE_VERSION_M6_M8 380
#define DTSHD_IEC958_PKTTYPE_CORE      0 //common feature for DTSETC_DECODE_VERSION 350/380,so set it to 0 by default
#define DTSHD_IEC958_PKTTYPE_SINGLEI2S 1
#define DTSHD_IEC958_PKTTYPE_FOURI2S   2

void restore_system_samplerate(struct aml_audio_dec* audec)
{
    if (audec->format == ACODEC_FMT_TRUEHD ||
        (audec->format == ACODEC_FMT_DTS && audec->VersionNum == DTSETC_DECODE_VERSION_M6_M8 && audec->DTSHDIEC958_FS > 192000 && amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw") == 2)) {
        ;//do nothing
    } else if (audec->samplerate == 48000 || (audec->format != ACODEC_FMT_DTS && \
               audec->format != ACODEC_FMT_AC3 && audec->format != ACODEC_FMT_EAC3 &&
               audec->format != ACODEC_FMT_TRUEHD)) {
        return ;
    }
    audio_io_handle_t handle = -1;
    //for mx, raw/pcm use the same audio hal
#ifndef USE_ARM_AUDIO_DEC
    if (handle > 0) {
        char str[64];
        memset(str, 0, sizeof(str));
        sprintf(str, "sampling_rate=%d", default_sr);
        AudioSystem::setParameters(handle, String8(str));
    }
#else

    if (handle > 0) {
        char str[64];
        memset(str, 0, sizeof(str));
        sprintf(str, "sampling_rate=%d", default_sr);
        AudioSystem::setParameters(handle, String8(str));
#if ANDROID_PLATFORM_SDK_VERSION >= 22
        AudioSystem::releaseOutput(handle, AUDIO_STREAM_DEFAULT, AUDIO_SESSION_OUTPUT_STAGE);
#else
        AudioSystem::releaseOutput(handle);
#endif
    } else {
        adec_print("WARNIN: handle/%d resetore sysFs failed!\n", handle);
    }
#endif
}

#if 1
static size_t old_frame_count = 0;
#else
static int old_frame_count = 0;
#endif

void restore_system_framesize()
{
    if (old_frame_count == 0) {
        adec_print("frame size can't be zero !\n");
        return;
    }
    adec_print("restore system frame size\n");
    //int sr = 0;
    audio_io_handle_t handle = -1;
    if (handle > 0) {
        char str[64];
        int ret;
        memset(str, 0, sizeof(str));
#if defined(ANDROID_VERSION_JBMR2_UP)
        sprintf(str, "frame_count=%zd", old_frame_count);
        ret = AudioSystem::setParameters(handle, String8(str));
        adec_print("restore frame success: %zd\n", old_frame_count);
#else
        sprintf(str, "frame_count=%zd", old_frame_count);
        ret = AudioSystem::setParameters(handle, String8(str));
        adec_print("restore frame success: %zd\n", old_frame_count);
#endif
    }
}

void reset_system_samplerate(struct aml_audio_dec* audec)
{
    unsigned digital_raw = 0;
    audio_io_handle_t handle = -1;
    int dtsFS_88_96_Directout = 0;
    digital_raw = get_digitalraw_mode(audec);
    if (audec->format == ACODEC_FMT_DTS && audec->samplerate > 48000 && !digital_raw) {
        char tmp[128] = {0};
        if (property_get("media.libplayer.88_96K", tmp, "0") > 0 && !strcmp(tmp, "1")) {
            dtsFS_88_96_Directout = 1;
        }
    }
    if (!audec || (!digital_raw && audec->channels != 8 && !dtsFS_88_96_Directout)) {
        return;
    }
    /*
    1)32k,44k dts
    2) 32k,44k ac3
    3)44.1k eac3 when hdmi passthrough
    4)32k,44k eac3 when spdif pasthrough
    */
    adec_print("[%s %d]format %d,audec->samplerate%d DTSHDIEC958_FS/%d\n", __FUNCTION__, __LINE__, audec->format , audec->samplerate, audec->DTSHDIEC958_FS);
    int Samplerate = audec->samplerate;
    int dts_raw_reset_sysFS = 0;
    if ((audec->format  == ACODEC_FMT_DTS && digital_raw == 1)
        || (audec->format == ACODEC_FMT_DTS && digital_raw == 2 && audec->VersionNum != DTSETC_DECODE_VERSION_M6_M8)
        || (audec->format == ACODEC_FMT_DTS && digital_raw == 2 && audec->VersionNum == DTSETC_DECODE_VERSION_M6_M8 && audec->DTSHDIEC958_PktType == DTSHD_IEC958_PKTTYPE_CORE)
       ) {
        //DTS raw_stream Directoutput
        if (audec->samplerate % 48000 != 0) {
            dts_raw_reset_sysFS = 1;
        }
        if (audec->samplerate == 32000 || audec->samplerate == 44100) {
            Samplerate = audec->samplerate;
        } else if (audec->samplerate == 88200 || audec->samplerate == 96000) {
            Samplerate = audec->samplerate / 2;
        } else if (audec->samplerate == 176400 || audec->samplerate == 192000) {
            Samplerate = audec->samplerate / 4;
        } else {
            adec_print("[%s %d] Unvalid samplerate/%d for DTSCore Rawoutput\n", __FUNCTION__, __LINE__, audec->samplerate);
            return;
        }
    } else if (audec->format == ACODEC_FMT_DTS && digital_raw == 2 && audec->VersionNum == DTSETC_DECODE_VERSION_M6_M8 && audec->DTSHDIEC958_PktType == DTSHD_IEC958_PKTTYPE_SINGLEI2S) {
        //DTSHD SingleI2s raw_stream Directoutput
        if (audec->DTSHDIEC958_FS == 48000 || audec->DTSHDIEC958_FS == 44100) {
            Samplerate = audec->DTSHDIEC958_FS;
        } else if (audec->DTSHDIEC958_FS == 192000 || audec->DTSHDIEC958_FS == 1764000) {
            Samplerate = audec->DTSHDIEC958_FS / 4;
        } else {
            adec_print("[%s %d] Unvalid DTSHDIEC958_FS/%d for DTSHD RawOutput\n", __FUNCTION__, __LINE__, audec->DTSHDIEC958_FS);
            return;
        }
        if (Samplerate % 48000 != 0) {
            dts_raw_reset_sysFS = 1;
        }
    } else if (audec->format == ACODEC_FMT_DTS && digital_raw == 2 && audec->VersionNum == DTSETC_DECODE_VERSION_M6_M8 && audec->DTSHDIEC958_PktType == DTSHD_IEC958_PKTTYPE_FOURI2S) {
        //DTSLL FourI2s raw_stream Directoutput
        if (audec->DTSHDIEC958_FS == 192000 || audec->DTSHDIEC958_FS == 384000 || audec->DTSHDIEC958_FS == 768000 ||
            audec->DTSHDIEC958_FS == 176400 || audec->DTSHDIEC958_FS == 352800 || audec->DTSHDIEC958_FS == 705600) {
            Samplerate = audec->DTSHDIEC958_FS / 4;
        } else {
            adec_print("[%s %d] Unvalid DTSHDIEC958_FS/%d for DTSLL RawOutput\n", __FUNCTION__, __LINE__, audec->DTSHDIEC958_FS);
            return;
        }
        if (Samplerate != 48000) {
            dts_raw_reset_sysFS = 1;
        }
    } else if (audec->format == ACODEC_FMT_DTS && (audec->samplerate > 48000 || (audec->channels == 8 && audec->samplerate != 48000))) {
        //DTS HD_PCM(FS>48000) Directoutput/8ch DTS_PCM output
        dts_raw_reset_sysFS = 1;
        Samplerate = audec->samplerate;
    } else if (audec->format == ACODEC_FMT_TRUEHD && (digital_raw == 1 || digital_raw == 2)) {
        //Dobly TrueHD RawOutput
        Samplerate = 192000;
    }
    if ((audec->format == ACODEC_FMT_AC3  && (audec->samplerate == 32000 || audec->samplerate == 44100))
        || (audec->format == ACODEC_FMT_EAC3 && digital_raw == 2 && audec->samplerate == 44100)
        || (audec->format == ACODEC_FMT_EAC3 && digital_raw == 1 && (audec->samplerate == 32000 || audec->samplerate == 44100))
        || dts_raw_reset_sysFS
        || (audec->format == ACODEC_FMT_TRUEHD && (digital_raw == 1 || digital_raw == 2)/* &&
        (audec->samplerate == 192000 || audec->samplerate == 96000)*/))

    {
        adec_print("[%s %d]Change AudioSysFS to/%d\n", __FUNCTION__, __LINE__, Samplerate);
        //int sr = 0;
        if (/*sr*/48000 != Samplerate) {
#ifndef USE_ARM_AUDIO_DEC

            if (handle > 0) {
                char str[64];
                memset(str, 0, sizeof(str));
                sprintf(str, "sampling_rate=%d", audec->samplerate);
                AudioSystem::setParameters(handle, String8(str));
            }
#else

            if (handle > 0) {
                char str[64];
                memset(str, 0, sizeof(str));
                sprintf(str, "sampling_rate=%d", Samplerate);
                AudioSystem::setParameters(handle, String8(str));
#if ANDROID_PLATFORM_SDK_VERSION >= 22
                AudioSystem::releaseOutput(handle, AUDIO_STREAM_DEFAULT, AUDIO_SESSION_OUTPUT_STAGE);
#else
                AudioSystem::releaseOutput(handle);
#endif
            } else {
                adec_print("WARNIN:handle/%d reset sysFs failed!\n", handle);
            }
#endif

        }


    }
}

/**
 * \brief callback function invoked by android
 * \param event type of event notified
 * \param user pointer to context for use by the callback receiver
 * \param info pointer to optional parameter according to event type
 */

#define AMSTREAM_IOC_MAGIC 'S'
#define AMSTREAM_IOC_GET_LAST_CHECKIN_APTS   _IOR(AMSTREAM_IOC_MAGIC, 0xa9, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKIN_VPTS   _IOR(AMSTREAM_IOC_MAGIC, 0xaa, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKOUT_APTS  _IOR(AMSTREAM_IOC_MAGIC, 0xab, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKOUT_VPTS  _IOR(AMSTREAM_IOC_MAGIC, 0xac, unsigned long)
#define AMSTREAM_IOC_AB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x09, unsigned long)

static unsigned long long ttt = 0;

static int resample = 0, last_resample = 0, resample_step = 0, last_step = 0;
static int xxx;
static int diff_record[0x40], diff_wp = 0;
static int wfd_enable = 0, bytes_skipped = 0x7fffffff;
static int wfd_ds_thrdhold = 250; //audio pts delay threadhold for down sampling
static int wfd_us_thrdhold = 150;//audio pts delay threadhold for up  sampling

#if ANDROID_PLATFORM_SDK_VERSION >= 19
static sp<AudioTrack> mpAudioTrack;
static sp<AudioTrack> mpAudioTrack_raw;
#endif

struct buf_status {
    int size;
    int data_len;
    int free_len;
    unsigned int read_pointer;
    unsigned int write_pointer;
};

struct am_io_param {
    int data;
    int len; //buffer size;
    struct buf_status status;
};

static int  mix_lr_channel_enable = 0;
static int  pre_gain_enable = 0;
static void momo2_mode_mix(short*buf, int nsamps, int channels)
{
    int i;
    int tmpSum;
    short *p16tmp = (short*)buf;
    if (channels != 2) {
        return;
    }
    for (i = 0; i < nsamps; i += 2) {
        tmpSum = (p16tmp[i] * 3 / 4) + (p16tmp[i + 1] * 3 / 4);
        tmpSum = tmpSum > 32767 ? 32767 : tmpSum;
        tmpSum = tmpSum < -32768 ? -32768 : tmpSum;
        p16tmp[i] = (tmpSum & 0x0000ffff);
        p16tmp[i + 1] = (tmpSum & 0x0000ffff);
    }
}

static void apply_audio_pregain(void *buf, int size, float gain)
{
    int i;
    short *sample = (short*)buf;
    for (i = 0; i < size / (int)sizeof(short); i++) {
        sample[i] = gain * sample[i];
    }
}

#define  RESAMPLE_THRESHOLD     (30 * TIME_UNIT90K / 1000)
void audioCallback(int event, void* user, void *info)
{
    int i;
    unsigned last_checkin, last_checkout;
    unsigned diff, diff_avr;
    AudioTrack::Buffer *buffer = static_cast<AudioTrack::Buffer *>(info);
    aml_audio_dec_t *audec = static_cast<aml_audio_dec_t *>(user);
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
    //adec_print("[%s %d]", __FUNCTION__, __LINE__);
    //unsigned long apts, pcrscr;
    struct am_io_param am_io;

    if (event != AudioTrack::EVENT_MORE_DATA) {
        //adec_refresh_pts(audec);
        adec_print(" ****************** audioCallback: event = %d g_bst->buf_level/%d g_bst_raw->buf_level/%d [-1:mean unknown] \n", event, audec->g_bst == NULL ? -1 : audec->g_bst->buf_level, audec->g_bst_raw == NULL ? -1 : audec->g_bst_raw->buf_level);
        return;
    }

    if (buffer == NULL || buffer->size == 0) {
        adec_print("audioCallback: Wrong buffer\n");
        return;
    }

    if (wfd_enable) {
        ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_GET_LAST_CHECKIN_APTS, &last_checkin);
        last_checkout = dsp_ops->get_cur_pts(dsp_ops);
        if (last_checkin < last_checkout) {
            diff = 0;
        } else {
            diff = (last_checkin - last_checkout) / 90;
        }

        //ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_GET_LAST_CHECKOUT_APTS, (int)&last_checkout);
    }
    if (wfd_enable) {
        // filtering
        diff_record[diff_wp++] = diff;
        diff_wp = diff_wp & 0x3f;

        diff_avr = 0;
        for (i = 0; i < 0x40; i++) {
            diff_avr += diff_record[i];
        }
        diff_avr = diff_avr / 0x40;

        //   if ((xxx++ % 30) == 0)
        //    adec_print("audioCallback start: request %d, in: %d, out: %d, diff: %d, filtered: %d",buffer->size, last_checkin/90, last_checkout/90, diff, diff_avr);
        if (bytes_skipped == 0 && diff < 200) {
            if (dsp_ops->set_skip_bytes) {
                dsp_ops->set_skip_bytes(&audec->adsp_ops, 0x7fffffff);
            }
            bytes_skipped = 0x7fffffff;
        }

        if (diff > 1000) { // too much data in audiobuffer,should be skipped
            if (dsp_ops->set_skip_bytes) {
                dsp_ops->set_skip_bytes(&audec->adsp_ops, 0);
            }
            bytes_skipped = 0;
            adec_print("skip more data: last_checkin[%d]-last_checkout[%d]=%d, diff=%d\n", last_checkin / 90, last_checkout / 90, (last_checkin - last_checkout) / 90, diff);
        }

        if ((int)diff_avr > /*220*/wfd_ds_thrdhold) {
            resample = 1;
            resample_step = 2;
        } else if ((int)diff_avr </*180*/wfd_us_thrdhold) {
            // once we see a single shot of low boundry we finish down-sampling
            resample = 1;
            resample_step = -2;
        } else if (resample && (diff_avr < 200)) {
            resample = 0;
        }

        if (last_resample != resample || last_step != resample_step) {
            last_resample = resample;
            last_step = resample_step;
            adec_print("resample changed to %d, step=%d, diff = %d", resample, resample_step, diff_avr);
        }
    }

    if (audec->adsp_ops.dsp_on) {
        int channels;
#if ANDROID_PLATFORM_SDK_VERSION >= 19
        channels = mpAudioTrack->channelCount();
#elif defined(ANDROID_VERSION_JBMR2_UP)
        audio_out_operations_t *out_ops = &audec->aout_ops;
        AudioTrack *track = (AudioTrack *)out_ops->private_data;
        channels = track->channelCount();
#else
        channels = buffer->channelCount;
#endif
        if (wfd_enable) {
            af_resample_api((char*)(buffer->i16), (unsigned int*)&buffer->size, channels, audec, resample, resample_step);
        } else if (audec->tsync_mode == TSYNC_MODE_PCRMASTER) {
            af_pcrmaster_resample_api((char*)(buffer->i16), (unsigned int*)&buffer->size, channels, audec);
        } else {
            //adec_print("[%s %d]", __FUNCTION__, __LINE__);
            af_resample_api_normal((char*)(buffer->i16), (unsigned int*)&buffer->size, channels, audec);
        }
        if (!audec->pre_mute) {
            if (audec->pre_gain_enable >= 0) {
                pre_gain_enable = audec->pre_gain_enable;
            }
            if (pre_gain_enable) {
                apply_audio_pregain(buffer->i16, buffer->size, audec->pre_gain);
            }
            if (audec->mix_lr_channel_enable >= 0) {
                mix_lr_channel_enable = audec->mix_lr_channel_enable;
            }
            if (mix_lr_channel_enable && channels == 2) {
                momo2_mode_mix((short*)(buffer->i16), buffer->size / 2, channels);
            }
        } else {
            //mute the buffer by pre-mute flag on
            memset(buffer->i16, 0, buffer->size);
        }
    } else {
        adec_print("audioCallback: dsp not work!\n");
    }


    if (wfd_enable) {
        if (buffer->size == 0) {
            adec_print("no sample from DSP !!! in: %d, out: %d, diff: %d, filtered: %d", last_checkin / 90, last_checkout / 90, (last_checkin - last_checkout) / 90, diff_avr);

            ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&am_io);
            if (am_io.status.size > 0)
                adec_print("ab_level=%x,ab_size=%x, alevel:%f, ab_rd_ptr=%x",
                           am_io.status.data_len,  am_io.status.size, (float)(am_io.status.data_len) / (am_io.status.size), am_io.status.read_pointer);
        }
    }
    return;
}
#ifdef USE_ARM_AUDIO_DEC
//-------------------------------------------------------------------------
static void i2s_iec958_sync_force(struct aml_audio_dec* audec, int bytes_readed_diff_allowd __unused)
{
    //int bytes_err= audec->raw_bytes_readed-audec->pcm_bytes_readed*audec->codec_type;
    //int bytes_cnt;
    //int len;
    //int goon_read_data_flag=1;
    //char tmp[2048];
    int64_t tmp64;
#if 0
    if (-bytes_err > bytes_readed_diff_allowd) {
        //958 is lower than i2s :so we discard data
        bytes_err = -bytes_err;
        int raw_size_discard = bytes_err - bytes_readed_diff_allowd;
        bytes_cnt = 0;
        while (bytes_cnt < raw_size_discard && !audec->need_stop) {
            len = audec->adsp_ops.dsp_read_raw(&audec->adsp_ops, tmp, (raw_size_discard - bytes_cnt) > 2048 ? 2048 : (raw_size_discard - bytes_cnt));
            bytes_cnt += len;
            if (len = 0) {
                break;
            }
        }
        audec->raw_bytes_readed += bytes_cnt;
    } else if (bytes_err > bytes_readed_diff_allowd) {
        //958 is faster than i2s
        int pcm_size_discard = (bytes_err - bytes_readed_diff_allowd) / audec->codec_type;
        bytes_cnt = 0;
        while (bytes_cnt < pcm_size_discard && !audec->need_stop) {
            len = audec->adsp_ops.dsp_read(&audec->adsp_ops, tmp, (pcm_size_discard - bytes_cnt) > 2048 ? 2048 : (pcm_size_discard - bytes_cnt));
            bytes_cnt += len;
            if (len = 0) {
                break;
            }
        }
        audec->pcm_bytes_readed += bytes_cnt;
    }
#endif
    tmp64 = audec->pcm_bytes_readed > audec->raw_bytes_readed ? audec->raw_bytes_readed : audec->pcm_bytes_readed;
    audec->pcm_bytes_readed -= tmp64;
    audec->raw_bytes_readed -= tmp64;
};
void audioCallback_raw(int event, void* user, void *info)
{
    int len;
    AudioTrack::Buffer *buffer = static_cast<AudioTrack::Buffer *>(info);
    aml_audio_dec_t *audec = static_cast<aml_audio_dec_t *>(user);
    //adec_print("[%s %d]", __FUNCTION__, __LINE__);
    if (event != AudioTrack::EVENT_MORE_DATA) {
        adec_print("[%s %d]audioCallback: event = %d \n", __FUNCTION__, __LINE__, event);
        return;
    }
    if (buffer == NULL || buffer->size == 0) {
        adec_print("[%s %d]audioCallback: Wrong buffer\n", __FUNCTION__, __LINE__);
        return;
    }

    if (audec->adsp_ops.dsp_on) {
        int bytes_cnt = 0;
        while (bytes_cnt < (int)buffer->size && !audec->need_stop) {
            len = audec->adsp_ops.dsp_read_raw(&audec->adsp_ops, (char*)(buffer->i16) + bytes_cnt, buffer->size - bytes_cnt);
            bytes_cnt += len;
            if (len == 0) {
                break;
            }
        }
        buffer->size = bytes_cnt;
        audec->raw_bytes_readed += bytes_cnt;
    } else {
        adec_print("[%s %d]audioCallback: dsp not work!\n", __FUNCTION__, __LINE__);
    }
    // memset raw data when start playback to walkround HDMI audio format changed noise for some kind of TV set
    if (audec->format != ACODEC_FMT_TRUEHD) {
        if (audec->raw_bytes_readed < 16 * 4 * 1024) {
            memset((char *)(buffer->i16), 0, buffer->size);
        }
    }
    return;
}

extern "C" int android_init_raw(struct aml_audio_dec* audec)
{
    Mutex::Autolock _l(mLock_raw);
    adec_print("[%s %d]android raw_out init", __FUNCTION__, __LINE__);
    status_t status;
    AudioTrack *track;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    int SampleRate = audec->samplerate;
    out_ops->private_data_raw = NULL;
    audio_format_t aformat = AUDIO_FORMAT_INVALID;
    if ((audec->format != ACODEC_FMT_DTS) &&
        (audec->format != ACODEC_FMT_AC3) &&
        (audec->format != ACODEC_FMT_EAC3) &&
        (audec->format != ACODEC_FMT_TRUEHD)) {
        adec_print("[%s %d]NOTE: now just ACODEC_FMT_DTS_rawoutpu was support! ", __FUNCTION__, __LINE__);
        return 0;
    } else if (get_digitalraw_mode(audec) == 0) {
        adec_print("[%s %d]DIGITAL_RAW WAS DISABLE !", __FUNCTION__, __LINE__);
        return 0;
    }

    if (audec->format == ACODEC_FMT_DTS) {
        aformat = AUDIO_FORMAT_DTS;
    } else if (audec->format == ACODEC_FMT_AC3) {
        aformat = AUDIO_FORMAT_IEC61937;
    } else if (audec->format == ACODEC_FMT_EAC3) {
        aformat = AUDIO_FORMAT_IEC61937;
    } else if (audec->format == ACODEC_FMT_TRUEHD) {
        aformat = AUDIO_FORMAT_DOLBY_TRUEHD;
    }

    if (audec->format == ACODEC_FMT_TRUEHD) {
        audec->codec_type = 16;
    }

    int dgraw = get_digitalraw_mode(audec);
    if (dgraw == 1) {
        if ((audec->format == ACODEC_FMT_AC3) ||
            (audec->format == ACODEC_FMT_EAC3)) {
            /*if spdif pass through,force to DD otuput */
            aformat = AUDIO_FORMAT_IEC61937;
            audec->codec_type = 1;
        } else if (audec->format == ACODEC_FMT_DTS) {
            if (audec->samplerate == 88200 || audec->samplerate == 96000) {
                audec->codec_type = 0.50;
                SampleRate = audec->samplerate / 2;
            } else if (audec->samplerate == 176400 || audec->samplerate == 192000) {
                audec->codec_type = 0.25;
                SampleRate = audec->samplerate / 4;
            } else if (audec->samplerate == 352800 || audec->samplerate == 384000) {
                adec_print("[%s %d]NOTE:Current FS/%d was support! ", __FUNCTION__, __LINE__, audec->samplerate);
                return 0;
            } else { //audec->samplerate<=48000
                audec->codec_type = 1;
            }
        }
    } else if (dgraw == 2) {
        if (audec->format == ACODEC_FMT_AC3) {
            audec->codec_type = 1;
        } else if (audec->format == ACODEC_FMT_EAC3) {
            audec->codec_type = 4;
        } else if (audec->format == ACODEC_FMT_DTS && (audec->VersionNum != DTSETC_DECODE_VERSION_M6_M8 || audec->DTSHDIEC958_PktType == DTSHD_IEC958_PKTTYPE_CORE)) {
            if (audec->samplerate == 88200 || audec->samplerate == 96000) {
                audec->codec_type = 0.50;
                SampleRate = SampleRate / 2;
            } else if (audec->samplerate == 176400 || audec->samplerate == 192000) {
                audec->codec_type = 0.25;
                SampleRate = SampleRate / 4;
            } else if (audec->samplerate == 352800 || audec->samplerate == 384000) {
                adec_print("[%s %d]NOTE:Current FS/%d was support! ", __FUNCTION__, __LINE__, audec->samplerate);
                return 0;
            } else { //audec->samplerate<=48000
                audec->codec_type = 1;
            }
        } else if (audec->format == ACODEC_FMT_DTS && audec->VersionNum == DTSETC_DECODE_VERSION_M6_M8) {
            int unvalidpara = 0;
            if (audec->DTSHDIEC958_PktType == DTSHD_IEC958_PKTTYPE_SINGLEI2S) {
                if (audec->DTSHDIEC958_FS == 48000 || audec->DTSHDIEC958_FS == 44100) {
                    SampleRate = audec->DTSHDIEC958_FS;
                } else if (audec->DTSHDIEC958_FS == 192000 || audec->DTSHDIEC958_FS == 176400) { // clock need Mutiple 4
                    SampleRate = audec->DTSHDIEC958_FS / 4;
#if ANDROID_PLATFORM_SDK_VERSION >= 21//android 5.0
                    aformat = (audio_format_t)AUDIO_FORMAT_DTS_HD;
                    SampleRate = audec->DTSHDIEC958_FS;
#endif
                } else {
                    unvalidpara = 1;
                }
            } else if (audec->DTSHDIEC958_PktType == DTSHD_IEC958_PKTTYPE_FOURI2S) { // clock need Mutiple 4
                if (audec->DTSHDIEC958_FS == 192000 || audec->DTSHDIEC958_FS == 384000 || audec->DTSHDIEC958_FS == 768000 ||
                    audec->DTSHDIEC958_FS == 176400 || audec->DTSHDIEC958_FS == 352800 || audec->DTSHDIEC958_FS == 705600) {
                    SampleRate = audec->DTSHDIEC958_FS / 4;
#if ANDROID_PLATFORM_SDK_VERSION >= 21//android 5.0
                    aformat = (audio_format_t)AUDIO_FORMAT_DTS_MASTER;
                    SampleRate = audec->DTSHDIEC958_FS;
#endif
                } else {
                    unvalidpara = 2;
                }
            } else {
                unvalidpara = 3;
            }
            if (unvalidpara) {
                adec_print("[%s %d]NOTE:Unvalid Paras/%d for RawOutput:PCM_FS/%d IEC958_FS/%d PCMSamsInFrm/%d IEC958PktFrmSize/%d ",
                           __FUNCTION__, __LINE__, unvalidpara, audec->samplerate, audec->DTSHDIEC958_FS, audec->DTSHDPCM_SamsInFrmAtMaxSR, audec->DTSHDIEC958_PktFrmSize);
                return 0;
            }
            audec->codec_type = audec->DTSHDIEC958_FS / audec->samplerate;
        }
    }

    if (audec->format == ACODEC_FMT_TRUEHD) {
        SampleRate = 192000;
    }
    adec_print("[%s %d]SampleRate used for init rawoutput:%d\n", __FUNCTION__, __LINE__, SampleRate);

#if ANDROID_PLATFORM_SDK_VERSION < 19
    track = new AudioTrack();
    if (track == NULL) {
        adec_print("[%s %d]AudioTrack_raw Create Failed!", __FUNCTION__, __LINE__);
        return -1;
    }
#else
    mpAudioTrack_raw = new AudioTrack();
    track = mpAudioTrack_raw.get();
#endif

    audio_session_t SessionID = AUDIO_SESSION_NONE;//audec->SessionID;
    adec_print("[%s %d]SessionID = %d audec->codec_type/%f audec->samplerate/%d", __FUNCTION__, __LINE__, SessionID, audec->codec_type, audec->samplerate);
    int flags  = AUDIO_OUTPUT_FLAG_DIRECT;
    //only defined from android M
#if ANDROID_PLATFORM_SDK_VERSION >= 23
    flags |= AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO;
#endif
    int framecount = 0;
    if (audec->format == ACODEC_FMT_AC3) {
        framecount = 1536;
    } else if (audec->format == ACODEC_FMT_EAC3) {
        if (audec->raw_enable == 1) {
            framecount = 1536;
        } else if (audec->raw_enable == 2) {
            framecount = 6144;
            SampleRate = SampleRate * 4;
        }
    }
    status = track->set(AUDIO_STREAM_MUSIC,
                        SampleRate,
                        aformat,
                        AUDIO_CHANNEL_OUT_STEREO,
                        0,                       // frameCount
                        (audio_output_flags_t)flags,
                        audioCallback_raw,
                        audec,       // user when callback
                        framecount,           // notificationFrames
                        0,           // shared buffer
                        false,       // threadCanCallJava
                        SessionID);  // sessionId
    adec_print("status %d", status);
    if (status != NO_ERROR) {
        adec_print("[%s %d]track->set returns %d", __FUNCTION__, __LINE__, status);
        adec_print("[%s %d]audio out samplet  %d", __FUNCTION__, __LINE__, audec->samplerate);
        adec_print("[%s %d]audio out channels %d", __FUNCTION__, __LINE__, audec->channels);
#if ANDROID_PLATFORM_SDK_VERSION < 19
        delete track;
        track = NULL;
#else
        track = NULL;
        mpAudioTrack_raw.clear();
#endif
        out_ops->audio_out_raw_enable = 0;
        out_ops->private_data_raw = NULL;
        return -1;
    }

    out_ops->private_data_raw = (void *)track;

    //1/10=0.1s=100ms
    audec->raw_frame_size = audec->channels * (audec->adec_ops->bps >> 3);
    audec->max_bytes_readded_diff = audec->samplerate * audec->raw_frame_size * audec->codec_type / 10;
    audec->i2s_iec958_sync_gate = audec->samplerate * audec->raw_frame_size * audec->codec_type * 0.4; //400ms
    return 0;
}
#endif
//-------------------------------------------------------------------------
/**
 * \brief output initialization
 * \param audec pointer to audec
 *st write occurred (msecs): 846290 \return 0 on success otherwise negative error code
 */
extern "C" int android_init(struct aml_audio_dec* audec)
{
    Mutex::Autolock _l(mLock);
    status_t status;
    AudioTrack *track;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    char wfd_prop[PROPERTY_VALUE_MAX];
    fill_audiotrack_zero = 0;
    buffering_audio_data = 0;
    skip_unnormal_discontinue = 0;
    unnormal_discontinue = 0;
    unnormal_discontinue1 = 0;
    ttt = 0;
    resample = 0;
    last_resample = 0;
    xxx = 0;

    adec_print("[%s %d]", __FUNCTION__, __LINE__);

    memset(&diff_record[0], 0, 0x40 * sizeof(diff_record[0]));
    diff_wp = 0;
    if (get_audio_decoder() == AUDIO_ARC_DECODER) {
        wfd_ds_thrdhold = 220;
        wfd_us_thrdhold = 180;

    } else {
        wfd_ds_thrdhold = 250;
        wfd_us_thrdhold = 150;
    }
    adec_print("up/down sampling thread 	 %d /%d ms \n", wfd_us_thrdhold, wfd_ds_thrdhold);
    if (property_get("media.libplayer.wfd", wfd_prop, "0") > 0) {
        wfd_enable = (strcmp(wfd_prop, "1") == 0);
        if (wfd_enable) {
            audio_io_handle_t handle = -1;
            if (handle > 0) {
                char str[64];
                status_t ret;
                memset(str, 0, sizeof(str));
                // backup old framecount
#if ANDROID_PLATFORM_SDK_VERSION >= 21 //FIXME on 5.0
                AudioSystem::getFrameCount(handle, &old_frame_count);
#else
                AudioSystem::getFrameCount(handle, AUDIO_STREAM_MUSIC, &old_frame_count);
#endif

                sprintf(str, "frame_count=%d", 256);
                ret = AudioSystem::setParameters(handle, String8(str));
                if (ret != 0) {
                    adec_print("change frame count failed: ret = %d\n", ret);
                }
                adec_print("wfd: %s", str);
            }
        }
    } else {
        wfd_enable = 0;
    }

    adec_get_tsync_info(&(audec->tsync_mode));
    adec_print("wfd_enable = %d, tsync_mode=%d, ", wfd_enable, audec->tsync_mode);
    //--------------------------------------------
    //alwas effect in case:
    //1: rawoutput==1 &&  format==ACODEC_FMT_AC3 &&  (FS==32000 ||FS = =44100)
    //2: rawoutput==1 &&  format==ACODEC_FMT_EC3 &&  (FS==32000 ||FS = =44100)
    //3: rawoutput==1 &&  format==ACODEC_FMT_DTS &&  (FS==32000 ||FS = =44100||FS = =88200||FS = =96000|| FS = =176400|| FS = =192000)
    //4: rawoutput==2 &&  format==ACODEC_FMT_EC3 &&   FS==44100
    //5: rawoutput==2 &&  format==ACODEC_FMT_DTS &&  (FS= =32000||FS = =44100||FS = =88200||FS = =96000|| FS = =176400|| FS = =192000)
    //6: rawoutput==0 &&  format==ACODEC_FMT_AC3 && CH==8
    //summary:always effect in case: rawoutput>0 && (format=ACODEC_FMT_DTS or ACODEC_FMT_AC3) or 8chPCM _output
    /*after 6.0,need not this code*/
#if ANDROID_PLATFORM_SDK_VERSION < 23
    reset_system_samplerate(audec);
#endif
    int user_raw_enable = get_digitalraw_mode(audec);
#ifdef USE_ARM_AUDIO_DEC
    out_ops->audio_out_raw_enable = user_raw_enable && (audec->format == ACODEC_FMT_DTS ||
                                    audec->format == ACODEC_FMT_AC3 ||
                                    audec->format == ACODEC_FMT_EAC3 ||
                                    (audec->format == ACODEC_FMT_TRUEHD && user_raw_enable == 2));
    if (out_ops->audio_out_raw_enable) {
        android_init_raw(audec);
        return 0;
    }
#endif
    //---------------------------
    adec_print("[%s %d]android out init", __FUNCTION__, __LINE__);
#if ANDROID_PLATFORM_SDK_VERSION < 19
    track = new AudioTrack();
    if (track == NULL) {
        adec_print("[%s %d]AudioTrack Create Failed!", __FUNCTION__, __LINE__);
        return -1;
    }
#else
    adec_print("[%s %d] audec->samplerate:%d", __FUNCTION__, __LINE__, audec->samplerate);
    mpAudioTrack = new AudioTrack();

    track = mpAudioTrack.get();
#endif

    audio_session_t SessionID = audec->SessionID;
    adec_print("[%s %d]SessionID = %d audec->dtshdll_flag/%d audec->channels/%d", __FUNCTION__, __LINE__, SessionID, audec->dtshdll_flag, audec->channels);
#if 1
    char tmp[128] = {0};
    int FS_88_96_enable = 0;
    if (property_get("media.libplayer.88_96K", tmp, "0") > 0 && !strcmp(tmp, "1")) {
        FS_88_96_enable = 1;
        if (audec->format == ACODEC_FMT_DTS && audec->samplerate > 48000 && !user_raw_enable) {
            adec_print("set digital_codec to AUDIO_FORMAT_DTS_PCM_88K_96K");
        }
    }
    if (audec->channels == 8 ||
        (audec->format == ACODEC_FMT_DTS && audec->samplerate > 48000 && user_raw_enable == 0 && FS_88_96_enable == 1)
       ) {
        //8ch PCM: use direct output
        //DTS PCMoutput && FS>48000: use direct output
        audio_channel_mask_t ChMask;
        audio_output_flags_t Flag;
        audio_format_t aformat;
        memset(tmp, 0, sizeof(tmp));
        if (audec->channels == 8) {
            adec_print("create multi-channel track use DirectOutput\n");
            property_set(DOLBY_SYSTEM_CHANNEL, "true");
            property_get(DOLBY_SYSTEM_CHANNEL, tmp, "0");
            if (!strcmp(tmp, "true")) {
                adec_print("[%s %d]ds1.audio.multichannel.support set success!\n", __FUNCTION__, __LINE__);
            } else {
                adec_print("[%s %d]ds1.audio.multichannel.support set fail!\n", __FUNCTION__, __LINE__);
            }
            ChMask = AUDIO_CHANNEL_OUT_7POINT1;
            Flag = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
            aformat = AUDIO_FORMAT_PCM_16_BIT;
        } else {
            adec_print("create HD-PCM(Fs/%d>48000)Direct Ouputtrack\n", audec->samplerate);
            ChMask = AUDIO_CHANNEL_OUT_STEREO;
            Flag = AUDIO_OUTPUT_FLAG_DIRECT;
            //TODO
#if ANDROID_PLATFORM_SDK_VERSION < 23
            aformat = AUDIO_FORMAT_DTS;
#else
            aformat = AUDIO_FORMAT_PCM_16_BIT;
#endif
        }
        status = track->set(AUDIO_STREAM_MUSIC,
                            audec->samplerate,
                            aformat,
                            ChMask,
                            0,       // frameCount
                            Flag/*AUDIO_OUTPUT_FLAG_NONE*/, // flags
                            audioCallback,
                            audec,    // user when callback
                            0,       // notificationFrames
                            0,       // shared buffer
                            false,   // threadCanCallJava
                            SessionID);      // sessionId
    } else {
        //here calculate the min framecount and set the audiotrack
        //refered to android_media_AudioTrack_get_min_buff_size
        //return frameCount * channelCount * bytesPerSample;
        size_t frameCount = 0;
        status = AudioTrack::getMinFrameCount(&frameCount, AUDIO_STREAM_MUSIC, audec->samplerate);
        if (status ==   NO_ERROR) {
            frameCount = audec->channels * 2 * frameCount;
        } else {
            frameCount = 0;
        }
        if (audec->format == ACODEC_FMT_AC3 || audec->format == ACODEC_FMT_EAC3) {
            frameCount = 6144;
        }
        adec_print("[%s %d] samplerate:%d channels:%d frameCount:%d\n", __FUNCTION__, __LINE__, audec->samplerate, audec->channels, frameCount);
        status = track->set(AUDIO_STREAM_DEFAULT,
                            audec->samplerate,
                            AUDIO_FORMAT_DEFAULT,
                            (audec->channels == 1) ? AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO,
                            frameCount,       // frameCount
                            AUDIO_OUTPUT_FLAG_NONE, // flags
                            audioCallback,
                            audec,    // user when callback
                            0,       // notificationFrames
                            0,       // shared buffer
                            false,   // threadCanCallJava
                            SessionID);      // sessionId

        adec_print("[%s %d]status:%d\n", __FUNCTION__, __LINE__, status);
    }

#elif defined(_VERSION_ICS)
    status = track->set(AUDIO_STREAM_MUSIC,
                        audec->samplerate,
                        AUDIO_FORMAT_PCM_16_BIT,
                        (audec->channels == 1) ? AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO,
                        0,       // frameCount
                        0,       // flags
                        audioCallback,
                        audec,    // user when callback
                        0,       // notificationFrames
                        0,       // shared buffer
                        false,   // threadCanCallJava
                        SessionID);      // sessionId
#else   // GB or lower:
    status = track->set(AudioSystem::MUSIC,
                        audec->samplerate,
                        AudioSystem::PCM_16_BIT,
                        (audec->channels == 1) ? AudioSystem::CHANNEL_OUT_MONO : AudioSystem::CHANNEL_OUT_STEREO,
                        0,       // frameCount
                        0,       // flags
                        audioCallback,
                        audec,    // user when callback
                        0,       // notificationFrames
                        0,       // shared buffer
                        SessionID);
#endif

    if (status != NO_ERROR) {
        adec_print("[%s %d]track->set returns %d", __FUNCTION__, __LINE__, status);
        adec_print("[%s %d]audio out samplet %d" , __FUNCTION__, __LINE__, audec->samplerate);
        adec_print("[%s %d]audio out channels %d", __FUNCTION__, __LINE__, audec->channels);
#if ANDROID_PLATFORM_SDK_VERSION < 19
        delete track;
        track = NULL;
#else
        track = NULL;
        mpAudioTrack.clear();
#endif
        if (audec->channels == 8) {
            property_set(DOLBY_SYSTEM_CHANNEL, "false");
        }
        return -1;

    }
    af_resample_linear_init(audec);
    out_ops->private_data = (void *)track;
    return 0;
}
#ifdef USE_ARM_AUDIO_DEC
extern "C" int android_start_raw(struct aml_audio_dec* audec)
{
    Mutex::Autolock _l(mLock_raw);
    adec_print("[%s %d]android raw_out start", __FUNCTION__, __LINE__);
    status_t status;
    audio_out_operations_t *out_ops = &audec->aout_ops;

#if ANDROID_PLATFORM_SDK_VERSION < 19
    AudioTrack *track = (AudioTrack *)out_ops->private_data_raw;
#else
    AudioTrack *track = mpAudioTrack_raw.get();
#endif
    if (track == 0) {
        adec_print("[%s %d]No track instance!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    status = track->initCheck();
    if (status != NO_ERROR) {
#if ANDROID_PLATFORM_SDK_VERSION < 19
        delete track;
#else
        mpAudioTrack_raw.clear();
#endif
        out_ops->private_data_raw = NULL;
        return -1;
    }

    track->start();
    adec_print("[%s %d]AudioTrack_raw initCheck OK and started.", __FUNCTION__, __LINE__);
    return 0;
}
#endif
/**
 * \brief start output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 *
 * Call android_start(), then the callback will start being called.
 */
extern "C" int android_start(struct aml_audio_dec* audec)
{

    Mutex::Autolock _l(mLock);
    status_t status;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    adec_print("android_start ");
    if (audec->raw_enable) {
        android_start_raw(audec);
    } else {
#if ANDROID_PLATFORM_SDK_VERSION < 19
        AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
        AudioTrack *track = mpAudioTrack.get();
#endif

#ifdef USE_ARM_AUDIO_DEC
        i2s_iec958_sync_force(audec, 0);

#endif
        adec_print("android out start");
        ttt = 0;
        resample = 0;
        last_resample = 0;
        xxx = 0;
        memset(&diff_record[0], 0, 0x40 * sizeof(diff_record[0]));
        diff_wp = 0;
        if (track == 0) {
            adec_print("No track instance!\n");
            return -1;
        }
        status = track->initCheck();
        if (status != NO_ERROR) {
#if ANDROID_PLATFORM_SDK_VERSION < 19
            delete track;
#else
            mpAudioTrack.clear();
#endif
            out_ops->private_data = NULL;
            return -1;
        }
        track->start();

    }

    adec_print("AudioTrack initCheck OK and started.");

    return 0;
}

extern "C" int android_pause_raw(struct aml_audio_dec* audec __unused)
{
    Mutex::Autolock _l(mLock_raw);
    adec_print("[%s %d]android raw_out pause", __FUNCTION__, __LINE__);

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data_raw;
#else
    AudioTrack *track = mpAudioTrack_raw.get();
#endif
    if (track == 0) {
        adec_print("[%s %d]No track instance!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    track->pause();
    return 0;
}

/**
 * \brief pause output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_pause(struct aml_audio_dec* audec)
{
    Mutex::Autolock _l(mLock);

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (audec->raw_enable) {
        android_pause_raw(audec);
    } else {
#ifdef USE_ARM_AUDIO_DEC

#endif
        adec_print("android out pause");
        if (track == 0) {
            adec_print("No track instance!\n");
            return -1;
        }
        track->pause();
#ifdef USE_ARM_AUDIO_DEC
        //adec_print("[%s %d] PRE_PAUSE:raw_bytes_readed/%ld pcm_bytes_readed/%ld delta/%ld\n",__FUNCTION__,__LINE__,
        //        audec->raw_bytes_readed,audec->pcm_bytes_readed,audec->pcm_bytes_readed-audec->raw_bytes_readed);
        i2s_iec958_sync_force(audec, 0);
        //adec_print("[%s %d] POST_PAUSE:raw_bytes_readed/%ld pcm_bytes_readed/%ld delta/%ld\n",__FUNCTION__,__LINE__,
        //        audec->raw_bytes_readed,audec->pcm_bytes_readed,audec->pcm_bytes_readed-audec->raw_bytes_readed);
        audec->i2s_iec958_sync_flag = 1;
#endif
    }
    return 0;
}

extern "C" int android_resume_raw(struct aml_audio_dec* audec __unused)
{
    Mutex::Autolock _l(mLock_raw);
    adec_print("[%s %d]android raw_out resume", __FUNCTION__, __LINE__);

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data_raw;
#else
    AudioTrack *track = mpAudioTrack_raw.get();
#endif
    if (track == 0) {
        adec_print("[%s %d]No track instance!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    track->start();
    return 0;
}
/**
 * \brief resume output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_resume(struct aml_audio_dec* audec)
{

    Mutex::Autolock _l(mLock);

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (audec->raw_enable) {
        android_resume_raw(audec);
    } else {
#ifdef USE_ARM_AUDIO_DEC
        //i2s_iec958_sync_force(audec,0);

#endif
        adec_print("android out resume");
        ttt = 0;
        resample = 0;
        last_resample = 0;
        xxx = 0;
        memset(&diff_record[0], 0, 0x40 * sizeof(diff_record[0]));
        diff_wp = 0;
        if (track == 0) {
            adec_print("No track instance!\n");
            return -1;
        }
        track->start();
    }

    return 0;
}
#ifdef USE_ARM_AUDIO_DEC
extern "C" int android_stop_raw(struct aml_audio_dec* audec __unused)
{
    Mutex::Autolock _l(mLock_raw);
    adec_print("[%s %d]android raw_out stop", __FUNCTION__, __LINE__);

    audio_out_operations_t *out_ops = &audec->aout_ops;
#if ANDROID_PLATFORM_SDK_VERSION < 19
    //AudioTrack *track = (AudioTrack *)out_ops->private_data_raw;
#else
    AudioTrack *track = mpAudioTrack_raw.get();
#endif

    if (track == 0) {
        adec_print("[%s %d]No track instance!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    track->stop();
    /* release AudioTrack */
#if ANDROID_PLATFORM_SDK_VERSION < 19
    delete track;
#else
    adec_print("mpAudioTrack_raw.clear");
    mpAudioTrack_raw.clear();
#endif

    out_ops->private_data_raw = NULL;
    return 0;
}
#endif
/**
 * \brief stop output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_stop(struct aml_audio_dec* audec)
{
    Mutex::Autolock _l(mLock);

    audio_out_operations_t *out_ops = &audec->aout_ops;
#if ANDROID_PLATFORM_SDK_VERSION < 19
    //AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (audec->raw_enable) {
        android_stop_raw(audec);
    }
    {
#ifdef USE_ARM_AUDIO_DEC

#endif
        adec_print("android out stop");
        if (audec->channels == 8) {
            property_set(DOLBY_SYSTEM_CHANNEL, "false");
        }
        if (track == 0) {
            adec_print("No track instance!\n");
            return -1;
        }

        track->stop();
        /* release AudioTrack */
#if ANDROID_PLATFORM_SDK_VERSION < 19
        delete track;
#else
        mpAudioTrack.clear();
#endif
        out_ops->private_data = NULL;
        /*after 6.0,not need this code*/
#if ANDROID_PLATFORM_SDK_VERSION < 23
        restore_system_samplerate(audec);
#endif
        if (wfd_enable) {
            restore_system_framesize();
        }
    }


    return 0;
}

/**
 * \brief get output latency in ms
 * \param audec pointer to audec
 * \return output latency
 */
extern "C" unsigned long android_latency(struct aml_audio_dec* audec)
{
    unsigned long latency;

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = NULL;
    if (audec->raw_enable) {
        track = mpAudioTrack_raw.get();
    } else {
        track = mpAudioTrack.get();
    }
#endif
    if (audec->use_get_out_posion && audec->aout_ops.get_out_position) {
        return 0;
    }
    if (audec->raw_enable) {
        if (track) {
            status_t s;
            //int ret = -1;
            int64_t write_samples;
            int cache_samples;
            int delay_us, t_us;
            AudioTimestamp timestamp;
            s  = track->getTimestamp(timestamp);
            if (s != NO_ERROR || timestamp.mPosition < 1) {
                /*
                timestamp.mPosition <= 0 we think audio not have start.
                the latency is not accurate
                */
                return 0;
            } else {
                struct timespec timenow;
                write_samples = audec->pcm_bytes_readed / (audec->channels * 2);
                cache_samples = write_samples -  timestamp.mPosition;
                if (cache_samples < 0) {
                    cache_samples = 0;
                }
                delay_us = 1000 * (cache_samples * 1000 / audec->samplerate);
                clock_gettime(CLOCK_MONOTONIC, &timenow);
                t_us = (timenow.tv_sec - timestamp.mTime.tv_sec) * 1000000LL +
                       (timenow.tv_nsec - timestamp.mTime.tv_nsec) / 1000;
                delay_us -= t_us;
                if (delay_us < 0) {
                    delay_us = 0;
                }
                return delay_us / 1000;
            }
        }
    }
    if (track) {
        latency = track->latency();
        return latency;
    }
    return 0;
}

/**
 * \brief get output latency in ms
 * \param audec pointer to audec
 * \return output latency
 */
extern "C" int android_get_position(struct aml_audio_dec* audec __unused,
                                    int64_t *position,
                                    int64_t *timeus)
{
    AudioTimestamp timestamp;
    status_t s;
    int ret = -1 ;
#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else

    AudioTrack *track = NULL;
    if (audec->raw_enable) {
        track = mpAudioTrack_raw.get();
    } else {
        track = mpAudioTrack.get();
    }

#endif

    if (track) {
        s  = track->getTimestamp(timestamp);
        if (s == NO_ERROR) {
            *position = timestamp.mPosition;
            *timeus = timestamp.mTime.tv_sec * 1000000LL + timestamp.mTime.tv_nsec / 1000;
            return 0;
        }
    }
    return ret;
}


#ifndef ANDROID_VERSION_JBMR2_UP
/**
 * \brief mute output
 * \param audec pointer to audec
 * \param en  1 = mute, 0 = unmute
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_mute_raw(struct aml_audio_dec* audec, adec_bool_t en)
{
    Mutex::Autolock _l(mLock_raw);
    adec_print("[%s %d]android raw_out mute", __FUNCTION__, __LINE__);

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data_raw;
#else
    audio_out_operations_t *out_ops;
    out_ops = NULL; //temp for pass compile
    en = 0; //temp for pass compile
    out_ops = &audec->aout_ops;
    AudioTrack *track = mpAudioTrack_raw.get();
#endif
    if (track == 0) {
        adec_print("No track instance!\n");
        return -1;
    }

    //track->mute(en);
    return 0;
}
#endif
extern "C" int android_mute(struct aml_audio_dec* audec __unused, adec_bool_t en __unused)
{
    Mutex::Autolock _l(mLock);
    adec_print("android out mute");

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

#ifdef ANDROID_VERSION_JBMR2_UP
#else
#ifdef USE_ARM_AUDIO_DEC
    //if(out_ops->audio_out_raw_enable)
    //android_mute_raw(audec,en);
#endif
    //track->mute(en);
#endif

    return 0;
}

/**
 * \brief set output volume
 * \param audec pointer to audec
 * \param vol volume value
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_set_volume(struct aml_audio_dec* audec __unused, float vol)
{
    Mutex::Autolock _l(mLock);
    adec_print("android set volume");

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->setVolume(vol, vol);

    return 0;
}

/**
 * \brief set left/right output volume
 * \param audec pointer to audec
 * \param lvol refer to left volume value
 * \param rvol refer to right volume value
 * \return 0 on success otherwise negative error code
 */
extern "C" int android_set_lrvolume(struct aml_audio_dec* audec __unused, float lvol, float rvol)
{
    Mutex::Autolock _l(mLock);
    adec_print("android set left and right volume separately");

#if ANDROID_PLATFORM_SDK_VERSION < 19
    audio_out_operations_t *out_ops = &audec->aout_ops;
    AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->setVolume(lvol, rvol);

    return 0;
}
extern "C" int android_set_track_rate(struct aml_audio_dec* audec __unused, void *rate)
{
#if ANDROID_PLATFORM_SDK_VERSION >= 23
    Mutex::Autolock _l(mLock);
    adec_print("android_set_track_rate");
    struct AudioPlaybackRate  Rate = *(struct AudioPlaybackRate*)rate;
    audio_out_operations_t *out_ops = &audec->aout_ops;
#if ANDROID_PLATFORM_SDK_VERSION < 19
    //AudioTrack *track = (AudioTrack *)out_ops->private_data;
#else
    AudioTrack *track = mpAudioTrack.get();
#endif
    if (!track) {
        adec_print("No track instance!\n");
        return -1;
    }

    track->setPlaybackRate(Rate);
    out_ops->track_rate = Rate.mSpeed;
#endif
    return 0;
}

extern "C" void android_basic_init()
{
    Mutex::Autolock _l(mLock);
    adec_print("android basic init!");
    sp<ProcessState> proc(ProcessState::self());
}

/**
 * \brief get output handle
 * \param audec pointer to audec
 */
extern "C" void get_output_func(struct aml_audio_dec* audec)
{
    audio_out_operations_t *out_ops = &audec->aout_ops;
    adec_print("[%s %d]", __FUNCTION__, __LINE__);

    out_ops->init = android_init;
    out_ops->start = android_start;
    out_ops->pause = android_pause;
    out_ops->resume = android_resume;
    out_ops->stop = android_stop;
    out_ops->latency = android_latency;
    out_ops->mute = android_mute;
    out_ops->set_volume = android_set_volume;
    out_ops->set_lrvolume = android_set_lrvolume;
    out_ops->set_track_rate = android_set_track_rate;
    out_ops->get_out_position = android_get_position;
    out_ops->audio_out_raw_enable = 1;
    /* default set a invalid value*/
    out_ops->track_rate = 8.8f;
}

}

