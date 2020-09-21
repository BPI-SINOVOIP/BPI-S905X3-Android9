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
 *
 *  DESCRIPTION:
 *      brief  Functions Of Pts Manage.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <adec-pts-mgt.h>
#include <cutils/properties.h>
#include <sys/time.h>
#include <amthreadpool.h>
#include <sys/ioctl.h>

#include "Amsysfsutils.h"
#include "amconfigutils.h"

#define LOG_TAG "adec-pts-mgt"
#define DROP_PCM_DURATION_THRESHHOLD 4 //unit:s
#define DROP_PCM_MAX_TIME 1000 // unit :ms
#define DROP_PCM_PTS_DIFF_THRESHHOLD   90000*10
#define DROP_PCM_RESET_PCR_THRESHOLD 90000/2

int adec_pts_droppcm(aml_audio_dec_t *audec);
int vdec_pts_resume(void);
//static int vdec_pts_pause(void);
int droppcm_use_size(aml_audio_dec_t *audec, int drop_size);
void droppcm_prop_ctrl(int *audio_ahead, int *pts_ahead_val);
int droppcm_get_refpts(aml_audio_dec_t *audec, unsigned long *refpts);

extern int match_types(const char *filetypestr, const char *typesetting);

static int64_t gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int sysfs_get_int(char *path, unsigned long *val)
{
    char buf[64];

    if (amsysfs_get_sysfs_str(path, buf, sizeof(buf)) == -1) {
        adec_print("unable to open file %s,err: %s", path, strerror(errno));
        return -1;
    }
    if (sscanf(buf, "0x%lx", val) < 1) {
        adec_print("unable to get pts from: %s", buf);
        return -1;
    }

    return 0;
}
/*
get vpts when refresh apts,  do not use sys write servie as it is too slow sometimes.
*/
static int mysysfs_get_sysfs_int16(const char *path)
{
    int fd;
    char valstr[64];
    int val;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr, 0, 64);
        read(fd, valstr, 64 - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        adec_print("unable to open file %s\n", path);
        return -1;
    }
    if (sscanf(valstr, "0x%x", &val) < 1) {
        adec_print("unable to get pts from: %s", valstr);
        return -1;
    }
    return val;
}
/*
static int mysysfs_get_sysfs_int(const char *path)
{
    int fd;
    int val = 0;
    char  bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    } else {
        adec_print("unable to open file %s,err: %s", path, strerror(errno));
    }
    return val;
}*/
/*
static int set_tsync_enable(int enable)
{
    char *path = "/sys/class/tsync/enable";
    return amsysfs_set_sysfs_int(path, enable);
}*/
/**
 * \brief calc current pts
 * \param audec pointer to audec
 * \return aurrent audio pts
 */
unsigned long adec_calc_pts(aml_audio_dec_t *audec)
{
    unsigned long pts, delay_pts;
    audio_out_operations_t *out_ops;
    dsp_operations_t *dsp_ops;

    out_ops = &audec->aout_ops;
    dsp_ops = &audec->adsp_ops;

    pts = dsp_ops->get_cur_pts(dsp_ops);
    if ((int)pts == -1) {
        adec_print("get get_cur_pts failed\n");
        return -1;
    }
    dsp_ops->kernel_audio_pts = pts;

    if (out_ops == NULL || out_ops->latency == NULL) {
        adec_print("cur_out is NULL!\n ");
        return -1;
    }
    delay_pts = out_ops->latency(audec) * 90;

    if (!audec->apts_start_flag) {
        return pts;
    }

    int diff = abs((int)(delay_pts - pts));
    // in some case, audio latency got from audiotrack is bigger than pts<hw not enabled>, so when delay_pts-pts < 100ms ,so set apts to 0.
    //because 100ms pcr-apts diff cause  apts reset .
    if (delay_pts/*+audec->first_apts*/ < pts) {
        pts -= delay_pts;
    } else if (diff < 9000) {
        pts = 0;
    }
    return pts;
}

/**
 * \brief start pts manager
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 */
int adec_pts_start(aml_audio_dec_t *audec)
{
    unsigned long pts = 0;
    //char *file;
    char buf[64];
    dsp_operations_t *dsp_ops;
    char value[PROPERTY_VALUE_MAX] = {0};
    //int tsync_mode;
    unsigned long first_apts = 0;

    adec_print("adec_pts_start");
    dsp_ops = &audec->adsp_ops;
    memset(buf, 0, sizeof(buf));

    if (audec->avsync_threshold <= 0) {
        if (am_getconfig_bool("media.libplayer.wfd")) {
            audec->avsync_threshold = SYSTIME_CORRECTION_THRESHOLD * 2 / 3;
            adec_print("use 2/3 default av sync threshold!\n");
        } else {
            audec->avsync_threshold = SYSTIME_CORRECTION_THRESHOLD;
            adec_print("use default av sync threshold!\n");
        }
    }

    first_apts = adec_calc_pts(audec);
    if (sysfs_get_int(TSYNC_FIRSTAPTS, (unsigned long *)&audec->first_apts) == -1) {
        adec_print("## [%s::%d] unable to get first_apts! \n", __FUNCTION__, __LINE__);
        return -1;
    }

    adec_print("av sync threshold is %d , no_first_apts=%d,first_apts = 0x%lx, audec->first_apts = 0x%lx \n", audec->avsync_threshold, audec->no_first_apts, first_apts, (long)audec->first_apts);


    dsp_ops->last_pts_valid = 0;

    //default enable drop pcm
    int enable_drop_pcm = 1;
    if (property_get("sys.amplayer.drop_pcm", value, NULL) > 0) {
        enable_drop_pcm = atoi(value);
    }
    adec_print("[%s:%d] enable_drop_pcm :%d \n", __FUNCTION__, __LINE__, enable_drop_pcm);
    if (enable_drop_pcm) {
        char *path = "/sys/class/tsync/enable";
        char buf[32] = "";

        if (amsysfs_get_sysfs_str(path, buf, 32) >= 0) {
            if (!strncmp(buf, "1: enabled", 10)) {
                adec_pts_droppcm(audec);
            }
        } else {
            adec_print("## [%s::%d] unable to get tsync enable status! \n", __FUNCTION__, __LINE__);
        }
    }

    // before audio start or pts start
    if (amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PRE_START") == -1) {
        return -1;
    }

    amthreadpool_thread_usleep(1000);

    if (audec->no_first_apts) {
        if (amsysfs_get_sysfs_str(TSYNC_APTS, buf, sizeof(buf)) == -1) {
            adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
            return -1;
        }

        if (sscanf(buf, "0x%lx", &pts) < 1) {
            adec_print("unable to get vpts from: %s", buf);
            return -1;
        }

    } else {
        pts = adec_calc_pts(audec);

        if ((int)pts == -1) {

            adec_print("pts==-1");

            if (amsysfs_get_sysfs_str(TSYNC_APTS, buf, sizeof(buf)) == -1) {
                adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
                return -1;
            }

            if (sscanf(buf, "0x%lx", &pts) < 1) {
                adec_print("unable to get apts from: %s", buf);
                return -1;
            }
        }
    }

    return 0;
}

static void enable_slowsync_repeate()
{
    //const char * slowsync_path = "/sys/class/tsync/slowsync_enable";
    const char * slowsync_repeate_path = "/sys/class/video/slowsync_repeat_enable";
    //amsysfs_set_sysfs_int(slowsync_path, 1);
    amsysfs_set_sysfs_int(slowsync_repeate_path, 1);
    adec_print("enable slowsync repeate. \n");
}

int adec_pts_droppcm(aml_audio_dec_t *audec)
{
    unsigned long refpts, apts, oldapts;
    int drop_size, droppts, dropms;
    char value[PROPERTY_VALUE_MAX] = {0};
    int64_t starttime, endtime;
    int audio_ahead = 0;
    unsigned pts_ahead_val = SYSTIME_CORRECTION_THRESHOLD;
    int diff;
    int sync_switch_ms = 200; // ms
    int count = 0;

    unsigned long cur_pcr;
    struct am_io_param am_io;

    starttime = gettime();

    int samplerate = 0;
    int channels = 0;
    //ARM based decoder,should got from decoder
    adec_print("adec_pts_droppcm start get audio sr/ch info \n");
    while (!audec->need_stop) {
        if (audec->g_bst) {
            samplerate = audec->g_bst->samplerate;
            channels = audec->g_bst->channels;
        }
        //DSP decoder must have got audio info when feeder_init
        else {
            samplerate = audec->samplerate;
            channels = audec->channels;
        }
        if (!samplerate  || !channels) {
            amthreadpool_thread_usleep(10000);
            continue;
        }
        break;
    }
    adec_print("adec_pts_droppcm  get audio sr/ch info done sr %d, ch %d \n", samplerate, channels);

    // drop pcm according to media.amplayer.dropms
    memset(value, 0, sizeof(value));
    if (property_get("media.amplayer.dropms", value, NULL) > 0) {
        dropms = atoi(value);
        audec->droppcm_ms = dropms;
        drop_size = dropms * (audec->samplerate / 1000) * audec->channels * 2;
        if (drop_size > 0) {
            if (droppcm_use_size(audec, drop_size) == -1) {
                adec_print("[%s::%d] data not enough, drop failed! \n", __FUNCTION__, __LINE__);
            }

            if (am_getconfig_bool("media.amplayer.dropmsquit")) {
                adec_print("[%s::%d] fast droppcm: %d ms!  \n", __FUNCTION__, __LINE__, dropms);
                return 0;
            }
        }
    }

    // pre drop, according to first check in vpts

    adec_print("[%s::%d] start pre drop !  \n", __FUNCTION__, __LINE__);
    if (am_getconfig_bool("media.amplayer.pre_droppcm")) {
        unsigned long checkin_firstvpts = 0;
        while (!checkin_firstvpts) {
            if (audec->need_stop) {
                return 0;
            }
            if (sysfs_get_int(TSYNC_CHECKIN_FIRSTVPTS, &checkin_firstvpts) == -1) {
                adec_print("## [%s::%d] unable to get TSYNC_CHECKIN_FIRSTVPTS! \n", __FUNCTION__, __LINE__);
                return -1;
            }
            if (count >= 50) {
                adec_print("Unable to get checkin_firstvpts, failed to pre_droppcm\n");
                break;
            } else if (checkin_firstvpts) {
                adec_print("get checkin_firstvpts, checkin_firstvpts=0x%lx\n",checkin_firstvpts);
                break;
            }
            count++;
            amthreadpool_thread_usleep(20000); // 20ms
        }
        if (checkin_firstvpts) {
            apts = adec_calc_pts(audec);
            diff = (apts > checkin_firstvpts) ? (apts - checkin_firstvpts) : (checkin_firstvpts - apts);
            adec_print("before drop pre --apts 0x%lx,checkin_firstvpts 0x%lx,apts %s, diff 0x%x\n", apts, checkin_firstvpts, (apts > checkin_firstvpts) ? "big" : "small", diff);
            if ((apts < checkin_firstvpts) && (diff < DROP_PCM_PTS_DIFF_THRESHHOLD)) {
                droppts = checkin_firstvpts - apts;
                audec->droppcm_ms = droppts / 90;
                drop_size = (droppts / 90) * (audec->samplerate / 1000) * audec->channels * 2;
                adec_print("[%s::%d]pre audec->samplerated = %d, audec->channels = %d! \n", __FUNCTION__, __LINE__, audec->samplerate, audec->channels);
                adec_print("[%s::%d]pre droppts:0x%x, drop_size=%d,  audio ahead %d,ahead pts value %d \n", __FUNCTION__, __LINE__,
                           droppts, drop_size, audio_ahead, pts_ahead_val);
                if (droppcm_use_size(audec, drop_size) == -1) {
                    adec_print("[%s::%d] timeout! data not enough! \n", __FUNCTION__, __LINE__);
                }
                oldapts = apts;
                apts = adec_calc_pts(audec);
                diff = (apts > checkin_firstvpts) ? (apts - checkin_firstvpts) : (checkin_firstvpts - apts);
                adec_print("after drop pre:--apts 0x%lx,droped:0x%lx, checkin_firstvpts 0x%lx,apts %s, diff 0x%x\n", apts, apts - oldapts, checkin_firstvpts, (apts > checkin_firstvpts) ? "big" : "small", diff);
            }
        }
    }

    if (droppcm_get_refpts(audec, &refpts) == -1) {
        adec_print("[%s::%d] get refpts failed! \n", __FUNCTION__, __LINE__);
        return -1;
    }

    while (!audec->first_apts) {
        unsigned long first_apts = 0;

        if (audec->need_stop) {
            return 0;
        }

        if (count > 10) {
            break;
        }

        first_apts = adec_calc_pts(audec);

        if (sysfs_get_int(TSYNC_FIRSTAPTS, (unsigned long *)&audec->first_apts) == -1) {
            adec_print("## [%s::%d] unable to get first_apts! \n", __FUNCTION__, __LINE__);
            return -1;
        }
        count++;
        amthreadpool_thread_usleep(50000); // 50ms
        adec_print("wait audec->first_apts first_apts = 0x%lx, audec->first_apts = 0x%lx\n", first_apts, (long)audec->first_apts);
    }
    diff = (audec->first_apts > refpts) ? (audec->first_apts - refpts) : (refpts - audec->first_apts);
    adec_print("before drop --audec->first_apts 0x%lx,refpts 0x%lx,audec->first_apts %s, diff 0x%x\n", (long)audec->first_apts, refpts, (audec->first_apts > refpts) ? "big" : "small", diff);
    {
        sysfs_get_int(TSYNC_PCRSCR, &cur_pcr);
        ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&am_io);
        adec_print("before drop ab_level=%x, cur_pcr=%lx", am_io.status.data_len, cur_pcr);
    }
    if (property_get("media.amplayer.sync_switch_ms", value, NULL) > 0) {
        sync_switch_ms = atoi(value);
    }

    if (am_getconfig_bool("media.libplayer.show_firstframe") &&
        audec->tsync_mode == TSYNC_MODE_PCRMASTER) {
        const char * show_first_frame = "/sys/class/video/show_first_frame_nosync";
        amsysfs_set_sysfs_int(show_first_frame, 1);
        sync_switch_ms = 10000;//10s
    }
    adec_print("sync switch setting: %d ms \n", sync_switch_ms);
    //auto switch -- if diff not too much, using slow sync
    if (audec->first_apts >= refpts || diff / 90 < sync_switch_ms) {
        //enable_slowsync_repeate();
        adec_print("diff less than %d (ms), use slowsync repeate mode \n", sync_switch_ms);
        return 0;
    }
    //when start to play,may audio discontinue happens, in this case, don't drop pcm operation
#if 1
    else if (diff > DROP_PCM_PTS_DIFF_THRESHHOLD) {
        adec_print("pts diff 0x%x bigger than %d (ms), don't drop pcm \n", diff, DROP_PCM_PTS_DIFF_THRESHHOLD * 1000 / 90000);
        return 0;
    }
#endif
    // calculate ahead pcm size
    droppcm_prop_ctrl(&audio_ahead, (int *)&pts_ahead_val);

    // drop pcm
    droppts = refpts - audec->first_apts + pts_ahead_val * audio_ahead;
    audec->droppcm_ms = droppts / 90;
    drop_size = (droppts / 90) * (audec->samplerate / 1000) * audec->channels * 2;
    adec_print("[%s::%d] audec->samplerated = %d, audec->channels = %d! \n", __FUNCTION__, __LINE__, audec->samplerate, audec->channels);
    adec_print("[%s::%d] droppts:0x%x, drop_size=%d,  audio ahead %d,ahead pts value %d \n",    __FUNCTION__, __LINE__,
               droppts, drop_size, audio_ahead, pts_ahead_val);
    if (droppcm_use_size(audec, drop_size) == -1) {
        adec_print("[%s::%d] timeout! data not enough! \n", __FUNCTION__, __LINE__);
    }

    // show apts refpts after droppcm
    if (sysfs_get_int(TSYNC_VPTS, &refpts) == -1) {
        adec_print("## [%s::%d] unable to get vpts! \n", __FUNCTION__, __LINE__);
        return -1;
    }

    oldapts = audec->first_apts;
    apts = adec_calc_pts(audec);
    diff = (apts > refpts) ? (apts - refpts) : (refpts - apts);
    adec_print("after drop:--apts 0x%lx,droped:0x%lx, vpts 0x%lx,apts %s, diff 0x%x\n", apts, apts - oldapts, refpts, (apts > refpts) ? "big" : "small", diff);
		if (diff > DROP_PCM_RESET_PCR_THRESHOLD){
		    char buf[64];
				sprintf(buf, "0x%lx", apts);
				adec_print("## [%s::%d] reset pcr \n", __FUNCTION__, __LINE__);
        if (amsysfs_set_sysfs_str(TSYNC_PCRSCR, buf)){
					adec_print("## [%s::%d] reset pcr error \n", __FUNCTION__, __LINE__);
				}
		}
		
    {
        sysfs_get_int(TSYNC_PCRSCR, &cur_pcr);
        ioctl(audec->adsp_ops.amstream_fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&am_io);
        adec_print("after drop ab_level=%x, cur_pcr=%lx", am_io.status.data_len, cur_pcr);
    }

    endtime = gettime();
    adec_print("==starttime is :%ld ms endtime is:%ld ms   usetime:%ld ms  \n", (long)starttime / 1000 , (long)endtime / 1000 , (long)(endtime - starttime) / 1000);

    return 0;
}

/**
 * \brief pause pts manager
 * \return 0 on success otherwise -1
 */
int adec_pts_pause(void)
{
    adec_print("adec_pts_pause");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PAUSE");
}

/**
 * \brief resume pts manager
 * \return 0 on success otherwise -1
 */
int adec_pts_resume(void)
{
    adec_print("adec_pts_resume");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_RESUME");

}

/**
 * \brief refresh current audio pts
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 */
static int apts_interrupt = 0;
static int pcrmaster_droppcm_flag = 0;
static int pre_filltime = -1;
int adec_refresh_pts(aml_audio_dec_t *audec)
{
    unsigned long pts;
    unsigned long systime;
    unsigned long last_pts = audec->adsp_ops.last_audio_pts;
    unsigned long last_kernel_pts = audec->adsp_ops.kernel_audio_pts;
    int apts_start_flag = 0;//store the last flag
    int samplerate = 48000;
    int channels = 2;
    char buf[64];
    char ret_val = -1;
    if (audec->auto_mute == 1) {
        return 0;
    }
    if (pre_filltime == -1) {
        char value[PROPERTY_VALUE_MAX] = {0};
        if (property_get("media.amadec.prefilltime", value, NULL) > 0) {
            pre_filltime = atoi(value);
        } else {
            pre_filltime = 170;
        }
        adec_print("[%s:%d] amadec- pre_filltime:%d ms. \n", __FUNCTION__, __LINE__, pre_filltime);
    }

    apts_start_flag = audec->apts_start_flag;
    //if the audio start has not been triggered to tsync,calculate the audio  pcm data which writen to audiotrack
    if (!audec->apts_start_flag) {
        int latency;
        int wait = pre_filltime;
        if (audec->g_bst) {
            samplerate = audec->g_bst->samplerate;
            channels = audec->g_bst->channels;
        }
        //DSP decoder must have got audio info when feeder_init
        else {
            samplerate = audec->samplerate;
            channels = audec->channels;
        }
        // 170 ms  audio hal have  triggered the output hw.
#ifdef USE_AOUT_IN_ADEC
        latency = audec->aout_ops.latency(audec);
#else
        latency = 0;//audec->aout_ops.latency(audec);
#endif
        if (latency > 0  && ((audec->pcm_bytes_readed * 1000 / (samplerate * channels * 2)) >= wait)) {
            adec_print("unable to getsystime--\n\n [[[%ld,%d,%d,%d,%d]]]\n",
            (long)audec->pcm_bytes_readed
            ,samplerate
            ,channels
            ,wait,
            latency);
            audec->apts_start_flag =  1;
            if (!am_getconfig_bool("libplayer.slowsync.disable"))
                enable_slowsync_repeate();
        }
    }
    memset(buf, 0, sizeof(buf));

    systime = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
    //if (systime == -1) {
    //    adec_print("unable to getsystime");
    //    audec->pcrscr64 = 0;
    //    return -1;
    //}

    /* get audio time stamp */
    pts = adec_calc_pts(audec);
    if ((apts_start_flag != audec->apts_start_flag)) {
        adec_print("audio pts start from 0x%lx", pts);
        sprintf(buf, "AUDIO_START:0x%lx", pts);
        if (amsysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1) {
            return -1;
        }
    }
    if (last_pts == pts) {
        //close(fd);
        //if (pts == -1) {
        audec->pcrscr64 = (int64_t)systime;
        return -1;
        //}
    }
#if 1
    unsigned int u32_vpts = mysysfs_get_sysfs_int16(TSYNC_VPTS);
    //unsigned int last_checkin_apts = mysysfs_get_sysfs_int16(TSYNC_LAST_CHECKIN_APTS);
    //int play_mode = mysysfs_get_sysfs_int(TSYNC_PCR_PLAY_MODE);

    if (audec->tsync_mode == TSYNC_MODE_PCRMASTER  && audec->pcrtsync_enable
        && ((((int64_t)pts) - audec->last_apts64 > TIME_UNIT90K / 10) || ((int64_t)pts < audec->last_apts64))) {

        int64_t apts64 = (int64_t)pts;
        int64_t pcrscr64 = (int64_t)systime;
        audec->apts64 = apts64;
        audec->pcrscr64 = pcrscr64;

        if (audec->adis_flag && abs((int)(audec->pcrscr64 - audec->last_apts64)) > APTS_DISCONTINUE_THRESHOLD) {
            audec->adis_flag = 0;
            adec_print("[%s:%d] pcr discontinue: last_pcr:%lx, pcr:%lx, --\n", __FUNCTION__, __LINE__, (long)audec->last_pcrscr64, (long)audec->pcrscr64);
        }
        audec->last_pcrscr64 = pcrscr64;
        if (apts64 < audec->last_apts64) {
            audec->last_apts64 = apts64;
        }

        //unsigned int adiff = (unsigned int)((int64_t)last_checkin_apts - apts64);
        adec_print("## tsync_mode:%d, pcr:%lx,apts:%lx, u32_vpts = 0x%x,lastapts:%lx, flag:%d,%d,---\n",
            audec->tsync_mode, (long)pcrscr64, (long)apts64, u32_vpts, (long)audec->last_apts64, pcrmaster_droppcm_flag, audec->adis_flag);
        //adec_print("## tsync_mode:%d,apts:%llx,last_checkin_apts=0x%x,diff=%ld, play_mode=%d\n",
        //    audec->tsync_mode, apts64, last_checkin_apts, adiff, play_mode);
        // drop pcm
        if (pcrscr64 - apts64 > audec->pcrmaster_droppcm_thsh && abs((int)(pcrscr64 - apts64)) < 3 * 90000  && abs((int)(u32_vpts - apts64)) >= 45000) {
            if (pcrmaster_droppcm_flag++ > 20) {
                int drop_size, droppts;
                droppts = pcrscr64 - apts64;
                audec->droppcm_ms = droppts / 90;
                drop_size = (droppts / 90) * (audec->samplerate / 1000) * audec->channels * 2;
                pcrmaster_droppcm_flag = 0;
                adec_print("## pcrmaster, droppcm pcr:%lx,apts:%lx,lastapts:%lx,---\n", systime, pts, last_pts);

                if (droppcm_use_size(audec, drop_size) == -1) {
                    adec_print("[%s::%d] timeout! data not enough! \n", __FUNCTION__, __LINE__);
                }
            }
        } else {
            pcrmaster_droppcm_flag = 0;
        }
    }
#endif
    if ((abs((int)(pts - last_pts)) > APTS_DISCONTINUE_THRESHOLD) && (audec->adsp_ops.last_pts_valid)) {
        /* report audio time interruption */
        adec_print("pts = %lx, last pts = %lx, u32_vpts=0x%x, pcr=0x%lx\n", pts, last_pts, u32_vpts, systime);

        adec_print("audio time interrupt: 0x%lx->0x%lx, 0x%x\n", last_pts, pts, abs((int)(pts - last_pts)));
        int tsync_pcr_discontinue = 0;
        //unsigned int adiff = (unsigned int)((unsigned long)last_checkin_apts - pts);
        if (audec->tsync_mode == TSYNC_MODE_PCRMASTER) {
            tsync_pcr_discontinue = mysysfs_get_sysfs_int16(TSYNC_PCR_DISCONTINUE);
            adec_print("%s,tsync_pcr_discontinue=0x%x-\n",__FUNCTION__,tsync_pcr_discontinue);
            //adec_print("audio interrupt:%d,apts:%llx,last_checkin_apts=0x%x,diff=%ld, play_mode=%d\n",
            //    audec->tsync_mode, pts, last_checkin_apts, adiff,play_mode);

        }
        if (audec->tsync_mode == TSYNC_MODE_PCRMASTER && audec->pcrtsync_enable && (tsync_pcr_discontinue & VIDEO_DISCONTINUE)) {
            unsigned long tsync_pcr_dispoint = 0;
            int count = 50;
            audec->apts64 = (int64_t)pts;
            audec->adis_flag = 0;

            do {
                if (sysfs_get_int(TSYNC_PCR_DISPOINT, &tsync_pcr_dispoint) == -1) {
                    adec_print("## [%s::%d] unable to get TSYNC_PCR_DISPOINT! \n", __FUNCTION__, __LINE__);
                    audec->adis_flag = 100;
                    break;
                }
                amthreadpool_thread_usleep(20000);
                count--;
            } while (tsync_pcr_dispoint == 0 && count > 0);

            if (tsync_pcr_dispoint != 0) {
                int64_t apts64 = (int64_t)pts;
                int64_t pcrscr64 = (int64_t)tsync_pcr_dispoint;
                audec->pcrscr64 = (int64_t)pcrscr64;
                audec->tsync_pcr_dispoint = (int64_t)tsync_pcr_dispoint;
                audec->adis_flag = 100;

                adec_print("## pcrmaster, tsync_pcr_dispoint,pcr:%lx,apts:%lx,lastapts:%lx,---\n", tsync_pcr_dispoint, pts, last_pts);
                // drop pcm, if larger than 10s, not to drop pcm
                if (pcrscr64 - apts64 > APTS_DISCONTINUE_THRESHOLD && pcrscr64 - apts64 < 10 * 90000) {
                    int drop_size, droppts;
                    droppts = pcrscr64 - apts64;

                    audec->droppcm_ms = droppts / 90;

                    drop_size = (droppts / 90) * (audec->samplerate / 1000) * audec->channels * 2;
                    adec_print("## pcrmaster, droppcm pcr:%lx,apts:%lx,lastapts:%lx,---\n", systime, pts, last_pts);

                    if (droppcm_use_size(audec, drop_size) == -1) {
                        adec_print("[%s::%d] timeout! data not enough! \n", __FUNCTION__, __LINE__);
                    }
                }
            }
        }

        sprintf(buf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", pts);

        if (amsysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1) {
            adec_print("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
            return -1;
        }

        audec->adsp_ops.last_audio_pts = pts;
        audec->adsp_ops.last_pts_valid = 1;
        adec_print("[%s:%d]set automute orig %d!\n", __FUNCTION__, __LINE__, audec->auto_mute);
        audec->auto_mute = 0;
        apts_interrupt = 5;
        return 0;
    }

    if (last_kernel_pts == audec->adsp_ops.kernel_audio_pts) {
        audec->pcrscr64 = (int64_t)systime;
        return 0;
    }

    audec->adsp_ops.last_audio_pts = pts;
    audec->adsp_ops.last_pts_valid = 1;

    if (abs((int)(pts - systime)) < audec->avsync_threshold) {
        apts_interrupt = 0;
        return 0;
    } else if (apts_interrupt > 0) {
        apts_interrupt --;
        return 0;
    }

    /* report apts-system time difference */
    if (!apts_start_flag) {
        return 0;
    }

    {/*add by zz*/
        /*
        reget pts to make sure pts is calc has no errors.
        because we may have accuracy problem on
        many thread update the pts related infos.
        */
        unsigned long re_getpts = adec_calc_pts(audec);
        if (abs((int)(pts - re_getpts)) > 900) { /*100 ms * 10%*/
            /*ignore this diff.*/
            adec_print("## [%s::%d] ixet apts: %ld ,%ld \n",
                __FUNCTION__, __LINE__,
            pts, re_getpts);
            return 0;
        } else {
            /*used new pts.*/
            pts = re_getpts;
        }
    }
    if (audec->refresh_pts_readytime_ms > 0) {
        /*pts_readytime not reached ,wait and delay reset apts.*/
        if (audec->refresh_pts_readytime_ms > gettime()/1000)
        return 0;
        audec->refresh_pts_readytime_ms = 0;
    }
#if 0
    if (audec->apts_reset_scr_delay_ms > 0) {
        unsigned long vpts = 0;
        if ((vpts = mysysfs_get_sysfs_int16(TSYNC_VPTS)) < 0) {
            adec_print("## [%s::%d] unable to get vpts! \n", __FUNCTION__, __LINE__);
            return -1;
        }
        //adec_print("vpts %x,last apts %x \n,diff %x \n",vpts,audec->last_checkout_apts,abs(vpts-audec->last_discontinue_apts));
        if ((gettime() - audec->last_discontinue_time) < audec->apts_reset_scr_delay_ms * 1000 && (abs(vpts - audec->last_discontinue_apts) > audec->avsync_threshold)) {
            return 0;
        } else {
            adec_print("after %lld ms,apts reset scr delay time out \n", (gettime() - audec->last_discontinue_time) / 1000);
            audec->apts_reset_scr_delay_ms = 0;
        }
    } else if (abs(pts - last_pts) >= 90000 && abs(pts - last_pts) < APTS_DISCONTINUE_THRESHOLD) {
        audec->last_discontinue_time  = gettime();
        audec->last_discontinue_apts = pts;
        audec->apts_reset_scr_delay_ms = audio_get_decoded_pcm_delay(audec)/*+audec->aout_ops.latency(audec)+100*/;
        adec_print("apts reset time delay %d ms,pts diff 0x%x,timestamp %d ms \n", audec->apts_reset_scr_delay_ms, abs(pts - last_pts),
                   abs(pts - last_pts) / 90);
        return 0;
    }
#endif

    if (audec->adsp_ops.set_cur_apts) {
        ret_val = audec->adsp_ops.set_cur_apts(&audec->adsp_ops, pts);
    } else {
        sprintf(buf, "0x%lx", pts);
        ret_val = amsysfs_set_sysfs_str(TSYNC_APTS, buf);
    }
    //if (ret_val == -1) {
    //    adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
    //    return -1;
    //}
    //if (abs(pts - systime) >= 90000 / 10) {
        //adec_print("report apts as 0x%x,system pts=0x%x, difference= %ld\n", pts, systime, (pts - systime));
   // }
    return 0;
}

/**
 * \brief Disable or Enable av sync
 * \param e 1 =  enable, 0 = disable
 * \return 0 on success otherwise -1
 */
int avsync_en(int e)
{
    return amsysfs_set_sysfs_int(TSYNC_ENABLE, e);
}

/**
 * \brief calc pts when switch audio track
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 *
 * When audio track switch occurred, use this function to judge audio should
 * be played or not. If system time fall behind audio pts , and their difference
 * is greater than SYSTIME_CORRECTION_THRESHOLD, auido should wait for
 * video. Otherwise audio can be played.
 */
int track_switch_pts(aml_audio_dec_t *audec)
{
    //unsigned long vpts;
    unsigned long apts;
    unsigned long pcr;
    char buf[32];

    memset(buf, 0, sizeof(buf));

    pcr = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
    //if (pcr == -1) {
    //    adec_print("unable to get pcr");
    //    return 1;
    //}

    apts = adec_calc_pts(audec);
    //if (apts == -1) {
   //     adec_print("unable to get apts");
    //    return 1;
    //}

    if ((apts > pcr) && (apts - pcr > 0x100000)) {
        return 0;
    }

    if (abs((int)(apts - pcr)) < audec->avsync_threshold || (apts <= pcr)) {
        return 0;
    } else {
        return 1;
    }

}
/*
static int vdec_pts_pause(void)
{
    char *path = "/sys/class/video_pause";
    return amsysfs_set_sysfs_int(path, 1);

}
*/
int vdec_pts_resume(void)
{
    adec_print("vdec_pts_resume\n");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "VIDEO_PAUSE:0x0");
}

#define DROPPCM_TMPBUF_SIZE (8*1024)
int droppcm_use_size(aml_audio_dec_t *audec, int drop_size)
{
    char value[PROPERTY_VALUE_MAX] = {0};
    char buffer[DROPPCM_TMPBUF_SIZE];
    //int drop_duration = drop_size / audec->channels / 2 / audec->samplerate;
    int nDropCount = 0;
    int size = drop_size;
    int ret = 0;
    int drop_max_time = DROP_PCM_MAX_TIME; // unit:ms
    int64_t start_time, nDropConsumeTime;
    char platformtype[20];
    char *buffer_raw = NULL;
    int drop_raw = 0;
    int drop_raw_size = 0;
    int nDropCount_raw = 0;
    if (audec->codec_type > 0) {
        buffer_raw = malloc(DROPPCM_TMPBUF_SIZE * audec->codec_type);
        if (buffer_raw == NULL) {
            adec_print("[%s %d]WARNING:malloc memory for buffer_raw failed!\n", __FUNCTION__, __LINE__);
        } else {
            drop_raw = 1;
        }
    }
    memset(platformtype, 0, sizeof(platformtype));
    memset(value, 0, sizeof(value));
    if (property_get("media.amplayer.dropmaxtime", value, NULL) > 0) {
        drop_max_time = atoi(value);
    }

    if (audec->droppcm_ms > 50) {
        drop_max_time = audec->droppcm_ms - 50;
    }
    memset(value, 0, sizeof(value));
    if (property_get("ro.board.platform", value, NULL) > 0) {
        if (match_types("meson6", value)) {
            memcpy(platformtype, "meson6", 6);
        } else if (match_types("meson8", value)) {
            memcpy(platformtype, "meson8", 6);
        }
    }

    start_time = gettime();
    adec_print("before droppcm: drop_size=%d, nDropCount:%d, drop_max_time:%d,platform:%s, ---\n", drop_size, nDropCount, drop_max_time, platformtype);
    while (drop_size > 0 && !audec->need_stop/*&& drop_duration < DROP_PCM_DURATION_THRESHHOLD*/) {
        ret = audec->adsp_ops.dsp_read(&audec->adsp_ops, buffer, MIN(drop_size, DROPPCM_TMPBUF_SIZE));
        if (drop_raw > 0 && ret > 0) {
            drop_raw_size = ret * audec->codec_type;
            nDropCount_raw = 0;
            while (drop_raw_size > 0) {
                drop_raw_size -= audec->adsp_ops.dsp_read_raw(&audec->adsp_ops, buffer_raw, drop_raw_size);
                nDropCount_raw++;
                if (nDropCount_raw > 10) {
                    adec_print("[%s %d]WARNING:no enough rawdata/%d to Drop,break!\n", __FUNCTION__, __LINE__, drop_raw_size);
                    break;
                }
            }
        }
        //apts = adec_calc_pts(audec);
        //adec_print("==drop_size=%d, ret=%d, nDropCount:%d apts=0x%x,-----------------\n",drop_size, ret, nDropCount,apts);
        nDropConsumeTime = gettime() - start_time;
        if (nDropConsumeTime / 1000 > drop_max_time) {
            adec_print("==ret:%d no pcm nDropCount:%d ,nDropConsumeTime:%ld reached drop_max_time:%d, \n", ret, nDropCount, (long)nDropConsumeTime / 1000, drop_max_time);
            drop_size -= ret;
            ret = -1;
            break;
        }
        if (ret == 0) { //no data in pcm buf
            if (nDropCount >= 40) {
                ret = -1;
                break;
            } else {
                nDropCount++;
            }
            if (!strcmp(platformtype, "messon8")) {
                amthreadpool_thread_usleep(50000);  // 10ms
            } else {
                amthreadpool_thread_usleep(50000);  // 50ms
            }
            adec_print("==ret:0 no pcm nDropCount:%d \n", nDropCount);
        } else {
            nDropCount = 0;
            drop_size -= ret;
        }
    }
    if (buffer_raw != NULL) {
        free(buffer_raw);
    }
    adec_print("after droppcm: drop_size=%d, droped:%d, nDropConsumeTime:%ld us,---\n", drop_size, size - drop_size, (long)nDropConsumeTime);

    return ret;
}

void droppcm_prop_ctrl(int *audio_ahead, int *pts_ahead_val)
{
    char value[PROPERTY_VALUE_MAX] = {0};

    if (am_getconfig_bool("media.libplayer.wfd")) {
        *pts_ahead_val = *pts_ahead_val * 2 / 3;
    }

    if (property_get("media.amplayer.apts", value, NULL) > 0) {
        if (!strcmp(value, "slow")) {
            *audio_ahead = -1;
        } else if (!strcmp(value, "fast")) {
            *audio_ahead = 1;
        }
    }
    memset(value, 0, sizeof(value));
    if (property_get("media.amplayer.apts_val", value, NULL) > 0) {
        *pts_ahead_val = atoi(value);
    }
}

int droppcm_get_refpts(aml_audio_dec_t *audec, unsigned long *refpts)
{
    char value[PROPERTY_VALUE_MAX] = {0};
    char buf[32];
    char tsync_mode_str[10];
    int tsync_mode;
    int circount = 3000; // default: 3000ms
    int refmode = TSYNC_MODE_AMASTER;
    int64_t start_time = gettime();
    unsigned long firstvpts = 0;
    unsigned long cur_vpts = 0;
    audio_out_operations_t * aout_ops = &audec->aout_ops;
    if (amsysfs_get_sysfs_str(TSYNC_MODE, buf, sizeof(buf)) == -1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }
    if (sscanf(buf, "%d: %s", &tsync_mode, tsync_mode_str) < 1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }
    if ((tsync_mode == TSYNC_MODE_AMASTER && !strcmp(tsync_mode_str, "amaster"))
        || (tsync_mode == TSYNC_MODE_VMASTER && !strcmp(tsync_mode_str, "vmaster") && audec->droppcm_flag)/* switch audio flag, firstvpts is unvalid when switch audio*/) {
        refmode = TSYNC_MODE_AMASTER;
    } else if (tsync_mode == TSYNC_MODE_VMASTER && !strcmp(tsync_mode_str, "vmaster") && !audec->droppcm_flag) {
        refmode = TSYNC_MODE_VMASTER;
    } else if (tsync_mode == TSYNC_MODE_PCRMASTER && !strcmp(tsync_mode_str, "pcrmaster")) {
        refmode = TSYNC_MODE_PCRMASTER;
    }

    adec_print("## [%s::%d] refmode:%d, tsync_mode:%d,%s, droppcm_flag:%d, ---\n", __FUNCTION__, __LINE__,
               refmode, tsync_mode, tsync_mode_str, audec->droppcm_flag);
    if (refmode == TSYNC_MODE_AMASTER || refmode == TSYNC_MODE_VMASTER) {
        if (sysfs_get_int(TSYNC_VPTS, refpts) == -1) {
            adec_print("## [%s::%d] unable to get vpts! \n", __FUNCTION__, __LINE__);
            return -1;
        }
        adec_print("## [%s::%d] default vpts:0x%lx, ---\n", __FUNCTION__, __LINE__, *refpts);
    } else if (tsync_mode == TSYNC_MODE_PCRMASTER && !strcmp(tsync_mode_str, "pcrmaster")) {
        if (sysfs_get_int(TSYNC_PCRSCR, refpts) == -1) {
            adec_print("## [%s::%d] unable to get pts_pcrscr! \n", __FUNCTION__, __LINE__);
            return -1;
        }
        int wait_count = 0;
        while ((*refpts) == 0) {
            if (audec->need_stop) {
                return 0;
            }
            sysfs_get_int(TSYNC_PCRSCR, refpts);
            //amthreadpool_thread_usleep(100000);
            wait_count++;
            adec_print("## [%s::%d] wait_count = %d, refpts = 0x%lx ---\n", __FUNCTION__, __LINE__, wait_count, *refpts);
            if (wait_count == 1) { //wait 2s
                break;
            }
        }
        adec_print("## [%s::%d] pcrmaster, refpts:0x%lx, ---\n", __FUNCTION__, __LINE__, *refpts);
    }

    memset(value, 0, sizeof(value));
    if (property_get("media.amplayer.dropwaitxms", value, NULL) > 0) {
        circount = atoi(value);
    }
    adec_print("drop wait max ms = %d \n", circount);

    //media.amplayer.refmode : 0 vpts 1 other case
#if 0
    int use_vpts = 1;
    if (property_get("media.amplayer.refmode", value, NULL) > 0) {
        if (atoi(value) != 0) {
            use_vpts = 0;
        }
    }

    if (use_vpts == 0) {
        return 0;
    }
    adec_print("drop pcm use_vpts = %d \n", use_vpts);
#endif

    unsigned long checkin_firstvpts = 0;
    while ((!firstvpts || !checkin_firstvpts)) {
        if (audec->need_stop) {
            return 0;
        }
        aout_ops = &audec->aout_ops;
        if (aout_ops) {
            /*cts if audio track rate is set,skip */
            if (aout_ops->track_rate != 8.8f)
                return 0;
        }
        if (sysfs_get_int(TSYNC_FIRSTVPTS, &firstvpts) == -1) {
            adec_print("## [%s::%d] unable to get firstpts! \n", __FUNCTION__, __LINE__);
            return -1;
        }

        if (sysfs_get_int(TSYNC_CHECKIN_FIRSTVPTS, &checkin_firstvpts) == -1) {
            adec_print("## [%s::%d] unable to get TSYNC_CHECKIN_FIRSTVPTS! \n", __FUNCTION__, __LINE__);
            return -1;
        }

        if (gettime() - start_time >= (circount * 1000)) {
            adec_print("## [%s::%d] max time reached! %d ms \n", __FUNCTION__, __LINE__, circount);
            break;
        }
        amthreadpool_thread_usleep(10000); // 10ms
    }
    adec_print("## [%s::%d] firstvpts:0x%lx, use:%ld us, maxtime:%d ms, ---\n", __FUNCTION__, __LINE__, firstvpts, (long)(gettime() - start_time), circount);

    if (sysfs_get_int(TSYNC_VPTS, &cur_vpts) == -1) {
        adec_print("## [%s::%d] unable to get vpts! \n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (audec->tsync_mode == TSYNC_MODE_PCRMASTER) {
        if (cur_vpts && cur_vpts < firstvpts && cur_vpts != checkin_firstvpts) {
            *refpts = cur_vpts;
        } else if (firstvpts) {
            *refpts = firstvpts;
        } else {
            *refpts = checkin_firstvpts;
        }
    } else {
        if (firstvpts) {
            *refpts = firstvpts;
        } else {
            *refpts = cur_vpts;
        }
    }
    adec_print("## [%s::%d] cur_vpts:0x%lx,checkin_firstvpts=0x%lx,firstvpts:0x%lx refpts:0x%lx, ---\n",
    __FUNCTION__, __LINE__, cur_vpts, checkin_firstvpts, firstvpts, *refpts);


    return 0;
}

int adec_get_tsync_info(int *tsync_mode)
{
    //char value[PROPERTY_VALUE_MAX] = {0};
    char buf[32];
    char tsync_mode_str[10];

    if (amsysfs_get_sysfs_str(TSYNC_MODE, buf, sizeof(buf)) == -1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }
    if (sscanf(buf, "%d: %s", tsync_mode, tsync_mode_str) < 1) {
        adec_print("unable to get tsync_mode from: %s", buf);
        return -1;
    }

    if (/*tsync_mode == TSYNC_MODE_AMASTER && */!strcmp(tsync_mode_str, "amaster")) {
        *tsync_mode = TSYNC_MODE_AMASTER;
    } else if (/*tsync_mode == TSYNC_MODE_VMASTER && */!strcmp(tsync_mode_str, "vmaster")) {
        *tsync_mode = TSYNC_MODE_VMASTER;
    } else if (/*tsync_mode == TSYNC_MODE_PCRMASTER &&*/ !strcmp(tsync_mode_str, "pcrmaster")) {
        *tsync_mode = TSYNC_MODE_PCRMASTER;
    }

    return *tsync_mode;
}
