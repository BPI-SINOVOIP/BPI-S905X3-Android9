/**
 * \file adec-internal-mgt.c
 * \brief  Audio Dec Message Loop Thread
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#define LOG_TAG "amadec"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <cutils/properties.h>
#include <dts_enc.h>
#include <Amsysfsutils.h>
#include <amthreadpool.h>
#include <amconfigutils.h>
#include <unistd.h>
#include "audiodsp_update_format.h"



#define MULTICH_SUPPORT_PROPERTY "media.multich.support.info"
#define PCM_88_96_SUPPORT        "media.libplayer.88_96K"

extern int RegisterDecode(aml_audio_dec_t *audec, int type);
extern void get_output_func(struct aml_audio_dec* audec);

static int64_t gettime_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec/1000;
}


/*
static int set_tsync_enable(int enable)
{

    char *path = "/sys/class/tsync/enable";
    return amsysfs_set_sysfs_int(path, enable);
}
*/
typedef struct {
    //  int no;
    int audio_id;
    char type[16];
} audio_type_t;

audio_type_t audio_type[] = {
    {ACODEC_FMT_AAC, "aac"},
    {ACODEC_FMT_AC3, "ac3"},
    {ACODEC_FMT_DTS, "dts"},
    {ACODEC_FMT_FLAC, "flac"},
    {ACODEC_FMT_COOK, "cook"},
    {ACODEC_FMT_AMR, "amr"},
    {ACODEC_FMT_RAAC, "raac"},
    {ACODEC_FMT_WMA, "wma"},
    {ACODEC_FMT_WMAPRO, "wmapro"},
    {ACODEC_FMT_ALAC, "alac"},
    {ACODEC_FMT_VORBIS, "vorbis"},
    {ACODEC_FMT_AAC_LATM, "aac_latm"},
    {ACODEC_FMT_APE, "ape"},
    {ACODEC_FMT_MPEG, "mp3"},//mp3
    {ACODEC_FMT_MPEG2, "mp3"},//mp2
    {ACODEC_FMT_MPEG1, "mp3"},//mp1
    {ACODEC_FMT_EAC3, "eac3"},
    {ACODEC_FMT_TRUEHD, "thd"},
    {ACODEC_FMT_PCM_S16BE, "pcm"},
    {ACODEC_FMT_PCM_S16LE, "pcm"},
    {ACODEC_FMT_PCM_U8, "pcm"},
    {ACODEC_FMT_PCM_BLURAY, "pcm"},
    {ACODEC_FMT_WIFIDISPLAY, "pcm"},
    {ACODEC_FMT_ALAW, "pcm"},
    {ACODEC_FMT_MULAW, "pcm"},

    {ACODEC_FMT_ADPCM, "adpcm"},
    {ACODEC_FMT_WMAVOI, "wmavoi"},
    {ACODEC_FMT_DRA, "dra"},
    {ACODEC_FMT_NULL, "null"},

};

extern int match_types(const char *filetypestr, const char *typesetting);

static int audio_decoder = AUDIO_ARM_DECODER;
static int audio_hardware_ctrl(hw_command_t cmd)
{
    int fd;

    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        adec_print("Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (cmd) {
    case HW_CHANNELS_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case HW_LEFT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case HW_RIGHT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case HW_STEREO_MODE:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        adec_print("Unknow Command %d!", cmd);
        break;

    };

    close(fd);

    return 0;

}

/**
 * \brief start audio dec when receive START command.
 * \param audec pointer to audec
 */
static void start_adec(aml_audio_dec_t *audec)
{
    int ret;
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
    unsigned long  vpts, apts;
    int times = 0;
    char buf[32];
    apts = vpts = 0;

    char value[PROPERTY_VALUE_MAX] = {0};
    int wait_count = 100;
    if (property_get("media.amadec.wait_count", value, NULL) > 0) {
        wait_count = atoi(value);
    }
    adec_print("wait first apts count :%d \n", wait_count);

    audec->no_first_apts = 0;
    if (audec->state == INITTED) {

        while ((!audiodsp_get_first_pts_flag(dsp_ops)) && (!audec->need_stop) && (!audec->no_first_apts)) {
            adec_print("wait first pts checkin complete times=%d,!\n", times);
            times++;

            if (times >= wait_count) {
                // read vpts
                amsysfs_get_sysfs_str(TSYNC_VPTS, buf, sizeof(buf));
                if (sscanf(buf, "0x%lx", &vpts) < 1) {
                    adec_print("unable to get vpts from: %s", buf);
                    return;
                }

                // save vpts to apts
                adec_print("## can't get first apts, save vpts to apts,vpts=%lx, \n", vpts);

                sprintf(buf, "0x%lx", vpts);

                amsysfs_set_sysfs_str(TSYNC_APTS, buf);

                audec->no_first_apts = 1;
            }
            amthreadpool_thread_usleep(100000);
        }
        /*Since audio_track->start consumed too much time
        *for the first time after platform restart,
        *so execute start cmd before adec_pts_start
        */
        aout_ops->start(audec);
        aout_ops->pause(audec);
        /*start  the  the pts scr,...*/
        ret = adec_pts_start(audec);

        //adec_pts_droppcm(audec);

        if (audec->auto_mute) {
            avsync_en(0);
            audiodsp_automute_on(dsp_ops);
            adec_pts_pause();

            while ((!audec->need_stop) && track_switch_pts(audec)) {
                amthreadpool_thread_usleep(1000);
            }

            audiodsp_automute_off(dsp_ops);
            avsync_en(1);
            adec_pts_resume();

            audec->auto_mute = 0;
        }

        aout_ops->resume(audec);
        audec->state = ACTIVE;

    }
}

/**
 * \brief pause audio dec when receive PAUSE command.
 * \param audec pointer to audec
 */
static void pause_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->state == ACTIVE) {
        audec->state = PAUSED;
        adec_pts_pause();
        aout_ops->pause(audec);
    }
}

/**
 * \brief resume audio dec when receive RESUME command.
 * \param audec pointer to audec
 */
static void resume_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->state == PAUSED) {
        audec->state = ACTIVE;
        audec->refresh_pts_readytime_ms = gettime_ms() +
            am_getconfig_int_def("media.amadec.wait_fresh_ms", 200);
        aout_ops->resume(audec);
        adec_pts_resume();
    }
}

/**
 * \brief stop audio dec when receive STOP command.
 * \param audec pointer to audec
 */
static void stop_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->state > INITING) {
        audec->state = STOPPED;
        aout_ops->mute(audec, 1); //mute output, some repeat sound in audioflinger after stop
        aout_ops->stop(audec);
        feeder_release(audec);
    }
}

/**
 * \brief release audio dec when receive RELEASE command.
 * \param audec pointer to audec
 */
static void release_adec(aml_audio_dec_t *audec)
{
    audec->state = TERMINATED;
}

/**
 * \brief mute audio dec when receive MUTE command.
 * \param audec pointer to audec
 * \param en 1 = mute, 0 = unmute
 */
static void mute_adec(aml_audio_dec_t *audec, int en)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (aout_ops->mute) {
        adec_print("%s the output !\n", (en ? "mute" : "unmute"));
        aout_ops->mute(audec, en);
        audec->muted = en;
    }
}

/**
 * \brief set volume to audio dec when receive SET_VOL command.
 * \param audec pointer to audec
 * \param vol volume value
 */
static void adec_set_volume(aml_audio_dec_t *audec, float vol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (aout_ops->set_volume) {
        adec_print("set audio volume! vol = %f\n", vol);
        aout_ops->set_volume(audec, vol);
    }
}

/**
 * \brief set volume to audio dec when receive SET_LRVOL command.
 * \param audec pointer to audec
 * \param lvol left channel volume value
 * \param rvol right channel volume value
 */
static void adec_set_lrvolume(aml_audio_dec_t *audec, float lvol, float rvol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (aout_ops->set_lrvolume) {
        adec_print("set audio volume! left vol = %f,right vol:%f\n", lvol, rvol);
        aout_ops->set_lrvolume(audec, lvol, rvol);
    }
}
static void adec_flag_check(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;

    if (audec->auto_mute && (audec->state > INITTED) && (audec->state != PAUSED)) {
        aout_ops->pause(audec);
        adec_print("automute, pause audio out!\n");
        amthreadpool_thread_usleep(10000);
        while ((!audec->need_stop) && track_switch_pts(audec)) {
            amthreadpool_thread_usleep(1000);
        }
        aout_ops->resume(audec);
        adec_print("resume audio out, automute invalid\n");
        audec->auto_mute = 0;
    }
}
/*
static int write_buffer(char *outbuf __unused, int outlen __unused)
{
    return 0;
}
*/

static void *adec_message_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    adec_cmd_t *msg = NULL;

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;

    while (!audec->need_stop) {
        audec->state = INITING;
        ret = feeder_init(audec);
        if (ret == 0) {
            ret = aout_ops->init(audec);
            if (ret) {
                adec_print("Audio out device init failed!");
                feeder_release(audec);
                continue;
            }
            audec->state = INITTED;
            adec_print("Audio out device init ok!");
            start_adec(audec);
            //if (dtsenc_init() != -1) {
             //   dtsenc_start();
           // }

            break;
        }

        if (!audec->need_stop) {
            amthreadpool_thread_usleep(100000);
        }
    }

    do {
        //if(message_pool_empty(audec))
        //{
        //adec_print("there is no message !\n");
        //  amthreadpool_thread_usleep(100000);
        //  continue;
        //}
        adec_reset_track(audec);
        adec_flag_check(audec);

        msg = adec_get_message(audec);
        if (!msg) {
            amthreadpool_thread_usleep(100000);
            continue;
        }

        switch (msg->ctrl_cmd) {
        case CMD_START:

            adec_print("Receive START Command!\n");
            start_adec(audec);
            //dtsenc_start();
            break;

        case CMD_PAUSE:

            adec_print("Receive PAUSE Command!");
            pause_adec(audec);
            //dtsenc_pause();
            break;

        case CMD_RESUME:

            adec_print("Receive RESUME Command!");
            resume_adec(audec);
            //dtsenc_resume();
            break;

        case CMD_STOP:

            adec_print("Receive STOP Command!");
            adec_print("audec->state=%d (INITING/3) when Rec_STOP_CMD\n", audec->state);
            stop_adec(audec);
            //dtsenc_stop();
            break;

        case CMD_MUTE:

            adec_print("Receive Mute Command!");
            if (msg->has_arg) {
                mute_adec(audec, msg->value.en);
            }
            break;

        case CMD_SET_VOL:

            adec_print("Receive Set Vol Command!");
            if (msg->has_arg) {
                adec_set_volume(audec, msg->value.volume);
            }
            break;
        case CMD_SET_LRVOL:

            adec_print("Receive Set LRVol Command!");
            if (msg->has_arg) {
                adec_set_lrvolume(audec, msg->value.volume, msg->value_ext.volume);
            }
            break;

        case CMD_CHANL_SWAP:

            adec_print("Receive Channels Swap Command!");
            audio_hardware_ctrl(HW_CHANNELS_SWAP);
            break;

        case CMD_LEFT_MONO:

            adec_print("Receive Left Mono Command!");
            audio_hardware_ctrl(HW_LEFT_CHANNEL_MONO);
            break;

        case CMD_RIGHT_MONO:

            adec_print("Receive Right Mono Command!");
            audio_hardware_ctrl(HW_RIGHT_CHANNEL_MONO);
            break;

        case CMD_STEREO:

            adec_print("Receive Stereo Command!");
            audio_hardware_ctrl(HW_STEREO_MODE);
            break;

        case CMD_RELEASE:

            adec_print("Receive RELEASE Command!");
            release_adec(audec);
            //dtsenc_release();
            break;

        default:
            adec_print("Unknow Command!");
            break;

        }

        if (msg) {
            adec_message_free(msg);
            msg = NULL;
        }
    } while (audec->state != TERMINATED);

    adec_print("Exit Message Loop Thread!");
    pthread_exit(NULL);
    return NULL;
}

/**
 * \brief start audio dec
 * \param audec pointer to audec
 * \return 0 on success otherwise -1 if an error occurred
 */
int match_types(const char *filetypestr, const char *typesetting)
{
    const char * psets = typesetting;
    const char *psetend;
    int psetlen = 0;
    char typestr[64] = "";
    if (filetypestr == NULL || typesetting == NULL) {
        return 0;
    }

    while (psets && psets[0] != '\0') {
        psetlen = 0;
        psetend = strchr(psets, ',');
        if (psetend != NULL && psetend > psets && psetend - psets < 64) {
            psetlen = psetend - psets;
            memcpy(typestr, psets, psetlen);
            typestr[psetlen] = '\0';
            psets = &psetend[1]; //skip ";"
        } else {
            strcpy(typestr, psets);
            psets = NULL;
        }
        if (strlen(typestr) > 0 && (strlen(typestr) == strlen(filetypestr))) {
            if (strstr(filetypestr, typestr) != NULL) {
                return 1;
            }
        }
    }
    return 0;
}

/*
digital_raw:
0  2ch PCM
1  spdif passthrough ,for DTS,DD
2  HDMI passthrough for DD,DTSHD,TrueHD
3  Multi-channel PCM HDMI passthrough
*/
static void set_multichs_prop()
{
    char * infobuf;
    int channel = 0;
    int dgraw = 0;
    dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
    infobuf = (char *)malloc(1024 * sizeof(char));
    property_set(PCM_88_96_SUPPORT, "0");
    if (infobuf == NULL) {
        adec_print("%s: malloc infobuf failed.\n", __FUNCTION__);
    } else {
        int fd = open("/sys/class/amhdmitx/amhdmitx0/aud_cap", O_RDONLY);
        if (fd != -1) {
            int nread = read(fd, infobuf, 1024);
            int i;
            nread = nread - 5;
            for (i = 0; i < nread; i++) {
                if ((infobuf[i] == 'P') && (infobuf[i + 1] == 'C') && (infobuf[i + 2] == 'M')) {
                    if (dgraw == 3) { // if configured mutli-channel output,check if the HDMI sink support multi-channel PCM output
                        if (infobuf[i + 5] == '8') {
                            property_set(MULTICH_SUPPORT_PROPERTY, "hdmi8");
                            channel = channel < 8 ? 8 : channel;
                        }
                        if (infobuf[i + 5] == '6') {
                            property_set(MULTICH_SUPPORT_PROPERTY, "hdmi6");
                            channel = channel < 6 ? 6 : channel;
                        }
                        if (infobuf[i + 5] == '2') {
                            property_set(MULTICH_SUPPORT_PROPERTY, "hdmi2");
                            channel = channel < 2 ? 2 : channel;
                        }
                    } else {
                        property_set(MULTICH_SUPPORT_PROPERTY, "hdmi2");
                        channel = channel < 2 ? 2 : channel;
                    }
                } else if (
                    ((infobuf[i] == '8') && (infobuf[i + 1] == '8') && (infobuf[i + 2] == '.') && (infobuf[i + 3] == '2'))
                    || ((infobuf[i] == '9') && (infobuf[i + 1] == '6'))) {
                    property_set(PCM_88_96_SUPPORT, "1");
                }
            }
            if (channel == 0) {
                property_set(MULTICH_SUPPORT_PROPERTY, "speaker");
            }
            adec_print("%s: aud_cap support channel number: %d ch\n", __FUNCTION__, channel);
            close(fd);
            free(infobuf);
            infobuf = NULL;
        } else {
            adec_print("%s: open hdmi driver aud_cap node failed \n", __FUNCTION__);
            free(infobuf);
            infobuf = NULL;
        }
    }
}

static int set_audio_decoder(aml_audio_dec_t *audec)
{
    int audio_id;
    int i;
    int num;
    int ret;
    audio_type_t *t;
    char value[PROPERTY_VALUE_MAX];


    audio_id = audec->format;

    num = ARRAY_SIZE(audio_type);
    for (i = 0; i < num; i++) {
        t = &audio_type[i];
        if (t->audio_id == audio_id) {
            break;
        }
    }

    if (match_types(t->type, "thd") || match_types(t->type, "wmavoi") || match_types(t->type, "dra") ) {
        adec_print("audio format is %s, so chose AUDIO_ARM_DECODER", t->type);
        audio_decoder = AUDIO_ARM_DECODER;
        goto exit;
    }


    ret = property_get("media.arm.audio.decoder", value, NULL);
    adec_print("media.amplayer.audiocodec = %s, t->type = %s\n", value, t->type);
    if (ret > 0 && match_types(t->type, value)) {
        char type_value[] = "ac3,eac3";
        set_multichs_prop();
        if (match_types(t->type, type_value)) {
#if 0
#ifdef DOLBY_USE_ARMDEC
            adec_print("DOLBY_USE_ARMDEC=%d", DOLBY_USE_ARMDEC);
#ifndef USE_ARM_AUDIO_DEC
            if (access("/system/etc/firmware/audiodsp_codec_ddp_dcv.bin",0 /*F_OK*/)) {
#endif
                audio_decoder = AUDIO_ARM_DECODER;
#ifndef USE_ARM_AUDIO_DEC
            }else{
                audio_decoder = AUDIO_ARC_DECODER;
            }
#endif
#else
            audio_decoder = AUDIO_ARC_DECODER;
            adec_print("<DOLBY_USE_ARMDEC> is not DEFINED,use ARC_Decoder\n!");
#endif
#endif
       audio_decoder = AUDIO_ARM_DECODER;
       adec_print("---ac3,eac3,use ARM_Decoder\n!");
        }
#ifndef USE_ARM_AUDIO_DEC
        else if (match_types(t->type, "dts")) {
            if (access("/system/etc/firmware/audiodsp_codec_dtshd.bin", 0/*F_OK*/)) {
                adec_print("using no license dts component");
                audio_decoder = AUDIO_ARM_DECODER;
            } else {
                adec_print("using audiodsp dts decoder");
                audio_decoder = AUDIO_ARC_DECODER;
            }
        }
#endif
        else {
            audio_decoder = AUDIO_ARM_DECODER;
        }
        goto exit;
    }

    ret = property_get("media.arc.audio.decoder", value, NULL);
    adec_print("media.amplayer.audiocodec = %s, t->type = %s\n", value, t->type);
    if (ret > 0 && match_types(t->type, value)) {
        if (audec->dspdec_not_supported == 0) {
            audio_decoder = AUDIO_ARC_DECODER;
        } else {
            audio_decoder = AUDIO_ARM_DECODER;
            adec_print("[%s:%d]arc decoder not support this audio yet,switch to ARM decoder \n", __FUNCTION__, __LINE__);
        }
        goto exit;
    }

    ret = property_get("media.ffmpeg.audio.decoder", value, NULL);
    adec_print("media.amplayer.audiocodec = %s, t->type = %s\n", value, t->type);
    if (ret > 0 && match_types(t->type, value)) {
        audio_decoder = AUDIO_FFMPEG_DECODER;
        goto exit;
    }

    audio_decoder = AUDIO_ARC_DECODER; //set arc decoder as default
    if (audec->dspdec_not_supported == 1) {
        audio_decoder = AUDIO_ARM_DECODER;
        adec_print("[%s:%d]arc decoder not support this audio yet,switch to ARM decoder \n", __FUNCTION__, __LINE__);
    }
exit:
    if (am_getconfig_bool("media.libplayer.wfd") && (audio_id == ACODEC_FMT_WIFIDISPLAY || audio_id == ACODEC_FMT_AAC)) {
        adec_print("wfd use arm decoder \n");
        audio_decoder = AUDIO_ARMWFD_DECODER;
    }
    return 0;
}

int get_audio_decoder(void)
{
    //adec_print("audio_decoder = %d\n", audio_decoder);
    return audio_decoder;
}

int vdec_pts_pause(void)
{
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "VIDEO_PAUSE:0x1");
}

int  adec_thread_wait(aml_audio_dec_t *audec, int microseconds)
{
    struct timespec pthread_ts;
    struct timeval now;
    adec_thread_mgt_t *mgt = &audec->thread_mgt;
    int ret;

    gettimeofday(&now, NULL);
    pthread_ts.tv_sec = now.tv_sec + (microseconds + now.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((microseconds + now.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&mgt->pthread_mutex);
    ret = pthread_cond_timedwait(&mgt->pthread_cond, &mgt->pthread_mutex, &pthread_ts);
    pthread_mutex_unlock(&mgt->pthread_mutex);
    return ret;
}
int adec_thread_wakeup(aml_audio_dec_t *audec)
{
    adec_thread_mgt_t *mgt = &audec->thread_mgt;
    int ret;

    pthread_mutex_lock(&mgt->pthread_mutex);
    ret = pthread_cond_signal(&mgt->pthread_cond);
    pthread_mutex_unlock(&mgt->pthread_mutex);

    return ret;
}

int audiodec_init(aml_audio_dec_t *audec)
{
    int ret = 0;
    pthread_t    tid;
    //char value[PROPERTY_VALUE_MAX] = {0};
    unsigned wfd = 0;
    adec_print("[%s %d]", __FUNCTION__, __LINE__);
    adec_message_pool_init(audec);
    get_output_func(audec);
    int nCodecType = audec->format;
    set_audio_decoder(audec);
    audec->format_changed_flag = 0;
    audec->audio_decoder_enabled  = -1;//default set a invalid value
    audec->mix_lr_channel_enable  = -1;
    audec->pre_gain_enable  = -1;
    audec->pre_gain = 1.0;
    audec->pre_mute = 0;
    audec->VersionNum = -1;
    audec->refresh_pts_readytime_ms = 0;
    if (am_getconfig_bool("media.libplayer.wfd"))  {
        wfd = 1;
    }
    if (get_audio_decoder() == AUDIO_ARC_DECODER) {
        audec->adsp_ops.dsp_file_fd = -1;
        ret = amthreadpool_pthread_create(&tid, NULL, (void *)adec_message_loop, (void *)audec);
        pthread_setname_np(tid, "AmadecMsgloop");
    } else if (wfd && (nCodecType == ACODEC_FMT_AAC || nCodecType ==  ACODEC_FMT_WIFIDISPLAY)) {
        adec_print("using wfd audio decoder \n");
        ret = amthreadpool_pthread_create(&tid, NULL, (void *)adec_wfddec_msg_loop, (void *)audec);
        audec->audio_decoder_enabled = 0x1;
        pthread_setname_np(tid, "AmadecWFDMsgloop");
    } else {
        int codec_type = get_audio_decoder();
        RegisterDecode(audec, codec_type);
        ret = amthreadpool_pthread_create(&tid, NULL, (void *)adec_armdec_loop, (void *)audec);
        pthread_mutex_init(&audec->thread_mgt.pthread_mutex, NULL);
        pthread_cond_init(&audec->thread_mgt.pthread_cond, NULL);
        audec->thread_mgt.pthread_id = tid;
        pthread_setname_np(tid, "AmadecArmdecLP");
    }
    if (ret != 0) {
        adec_print("Create adec main thread failed!\n");
        return ret;
    }
    adec_print("Create adec main thread success! tid = %ld \n", tid);
    audec->thread_pid = tid;
    return ret;
}
