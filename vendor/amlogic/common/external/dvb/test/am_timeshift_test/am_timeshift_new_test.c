/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "am_debug.h"
#include "am_dmx.h"
#include "am_av.h"
#include "am_fend.h"
#include "am_rec.h"
#include <errno.h>
#include "am_evt.h"

#include "am_crypt.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define DMX_DEV_DVR 1
#define DMX_DEV_AV 0
#define ASYNC_FIFO_ID 0
#define AV_DEV_NO 0

#define rec_log_printf(...) __android_log_print(ANDROID_LOG_INFO, "rec" TAG_EXT, __VA_ARGS__)
#define tfile_log_printf(...) __android_log_print(ANDROID_LOG_INFO, "tfile" TAG_EXT, __VA_ARGS__)
#define player_log_printf(...) __android_log_print(ANDROID_LOG_INFO, "player" TAG_EXT, __VA_ARGS__)

AM_REC_Handle_t rec;
AM_TFile_t tfile;

static long tfile_start, tfile_end, player_now, player_stat;
static char *stat_string[] = {
    /*AV_TIMESHIFT_STAT_STOP*/ "stop",
    /*AV_TIMESHIFT_STAT_PLAY*/ "play",
    /*AV_TIMESHIFT_STAT_PAUSE*/"pause",
    /*AV_TIMESHIFT_STAT_FFFB*/ "fffb",
    /*AV_TIMESHIFT_STAT_EXIT*/ "exit",
    /*AV_TIMESHIFT_STAT_INITOK*/"init",
    /*AV_TIMESHIFT_STAT_SEARCHOK*/"searchok",
};
static char *stat2string(long stat)
{
    if (stat < 0 || stat > 6)
        return "unknown";
    return stat_string[stat];
}
static void pr_stat(long tf_start, long tf_end, long ply_now, int ply_stat)
{
    tfile_start = tf_start;
    tfile_end = tf_end;
    player_now = ply_now;
    player_stat = ply_stat;
    printf("\rtfile[%ld - %ld] player[%ld] [%s]",
        tfile_start, tfile_end, player_now, stat2string(player_stat));
    fflush(stdout);
}

static void display_usage(void)
{
    fprintf(stderr, "==================\n");
    fprintf(stderr, "*play\n");
    fprintf(stderr, "*pause\n");
    fprintf(stderr, "*resume\n");
    fprintf(stderr, "*ff speed(1=1X,2=4X,3=6X)\n");
    fprintf(stderr, "*fb speed(1=1X,2=4X,3=6X)\n");
    fprintf(stderr, "*seek time_in_msecond\n");
    fprintf(stderr, "*aud <pid>:<fmt>\n");
    fprintf(stderr, "*pvrpause\n");
    fprintf(stderr, "*pvrresume\n");
    fprintf(stderr, "*quit\n");
    fprintf(stderr, "==================\n");
}

static uint8_t des_key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

static void *des_open()
{
    char buf[4096];
    char *p1, *p2;

    AM_FileRead("/proc/cpuinfo", buf, sizeof(buf));
    if ((p1 = strstr(buf, "Serial"))) {
        if ((p2 = strstr(p1, ": ")))
            sscanf(p2, ": %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                &des_key[0], &des_key[1], &des_key[2], &des_key[3],
                &des_key[4], &des_key[5], &des_key[6], &des_key[7]);
    }

    printf("des key: %02x%02x%02x%02x%02x%02x%02x%02x\n",
        des_key[0], des_key[1], des_key[2], des_key[3],
        des_key[4], des_key[5], des_key[6], des_key[7]);

    return AM_CRYPT_des_open(des_key, 64);
}

static int des_close(void *cryptor)
{
    return AM_CRYPT_des_close(cryptor);
}

static void des_crypt(void *cryptor, uint8_t *dst, uint8_t *src, int len, int decrypt)
{
    AM_CRYPT_des_crypt(cryptor, dst, src, len, NULL, decrypt);
}

static AM_Crypt_Ops_t des_ops = {
    .open = des_open,
    .close = des_close,
    .crypt = des_crypt,
};

int start_timeshift_test(void)
{
    AM_Bool_t go = AM_TRUE;
    char buf[256];

    display_usage();

    while (go) {
        if (fgets(buf, sizeof(buf), stdin)) {

            if(!strncmp(buf, "quit", 4)) {
                go = AM_FALSE;
                AM_AV_StopTimeshift(AV_DEV_NO);
                continue;
            }
            else if (!strncmp(buf, "play", 4)) {
                AM_AV_PlayTimeshift(AV_DEV_NO);
            }
            else if (!strncmp(buf, "pause", 5)) {
                AM_AV_PauseTimeshift(AV_DEV_NO);
            }
            else if (!strncmp(buf, "resume", 6)) {
                AM_AV_ResumeTimeshift(AV_DEV_NO);
            }
            else if (!strncmp(buf, "ff", 2)) {
                int speed;
                sscanf(buf + 2, "%d", &speed);
                AM_AV_FastForwardTimeshift(AV_DEV_NO, speed);
                printf("fast forward timeshift dev_no is %d, speed is %d  \n",AV_DEV_NO,speed);
            }
            else if (!strncmp(buf, "fb", 2)) {
                int speed;
                sscanf(buf + 2, "%d", &speed);
                AM_AV_FastBackwardTimeshift(AV_DEV_NO, speed);
            }
            else if (!strncmp(buf, "seek", 4)) {
                int time;
                sscanf(buf + 4, "%d", &time);
                AM_AV_SeekTimeshift(AV_DEV_NO, time, AM_TRUE);
            }
            else if (!strncmp(buf, "aud", 3)) {
                int apid = 0x1fff, afmt = 0;
                sscanf(buf + 3, "%d:%d", &apid, &afmt);
                AM_AV_SwitchTSAudio(AV_DEV_NO, apid, afmt);
            }
            else if (!strncmp(buf, "pvrpause", 8)) {
                AM_REC_PauseRecord(rec);
                printf("rec paused.\n");
            }
            else if (!strncmp(buf, "pvrresume", 9)) {
                AM_REC_ResumeRecord(rec);
                printf("rec resumed.\n");
            }
            else {
                fprintf(stderr, "Unkown command: %s\n", buf);
                display_usage();
            }
        }
    }

    return 0;
}

static void rec_evt_cb(long dev_no, int event_type, void *param, void *data)
{
    char *user;
    AM_REC_GetUserData((AM_REC_Handle_t)dev_no, (void**)&user);
    rec_log_printf("evt from %s\n", user);

    switch (event_type) {
        case AM_REC_EVT_RECORD_END: {
            AM_REC_RecEndPara_t *endpara = (AM_REC_RecEndPara_t*)param;
            rec_log_printf("\tEND, id   :%s\n", (const char *)data);
            rec_log_printf("\t     ERROR:0x%x\n", endpara->error_code);
            rec_log_printf("\t     SIZE :%lld\n", endpara->total_size);
            rec_log_printf("\t     TIME :%d\n", endpara->total_time);
            }break;
        case AM_REC_EVT_RECORD_START: {
            rec_log_printf("\tSTART, id :%s\n", (const char *)data);
            }break;
        default:
            break;
    }

    /*rec_log_printf("rec_evt_callback: dev_no:0x%lx type:0x%x param=0x%lx\n",
        dev_no, event_type, (long)param);*/
}

static void tfile_evt_cb(long dev_no, int event_type, void *param, void *data)
{
    UNUSED(dev_no);
    tfile_log_printf("evt from %s\n", (const char *)data);
    switch (event_type) {
    case AM_TFILE_EVT_START_TIME_CHANGED:
        tfile_log_printf("\tTFILE START >>: %d\n", (int)(long)param);
        pr_stat((long)param, tfile_end, player_now, player_stat);
        break;
    case AM_TFILE_EVT_END_TIME_CHANGED:
        tfile_log_printf("\t\tTFILE END >>: %d\n", (int)(long)param);
        pr_stat(tfile_start, (long)param, player_now, player_stat);
        break;
    default:
        break;
    }
}

static void av_evt_cb(long dev_no, int event_type, void *param, void *data)
{
    UNUSED(dev_no);
    player_log_printf("evt from %s\n", (const char *)data);
    switch (event_type) {
    case AM_AV_EVT_AV_NO_DATA:
        player_log_printf("\tNODATA\n");
        break;
    case AM_AV_EVT_AV_DATA_RESUME:
        player_log_printf("\tRESUME\n");
        break;
    case AM_AV_EVT_VIDEO_SCAMBLED:
    case AM_AV_EVT_AUDIO_SCAMBLED:
        player_log_printf("\tVIDEO or AUDIO SCRAMBLED\n");
        break;
    case AM_AV_EVT_VIDEO_AVAILABLE:
        player_log_printf("\tVIDEO AVAILABLE\n");
        break;
    case AM_AV_EVT_PLAYER_UPDATE_INFO: {
        /*typedef enum
        {
            AV_TIMESHIFT_STAT_STOP,
            AV_TIMESHIFT_STAT_PLAY,
            AV_TIMESHIFT_STAT_PAUSE,
            AV_TIMESHIFT_STAT_FFFB,
            AV_TIMESHIFT_STAT_EXIT,
            AV_TIMESHIFT_STAT_INITOK,
            AV_TIMESHIFT_STAT_SEARCHOK,
        } AV_TimeshiftState_t;
        */
        AM_AV_TimeshiftInfo_t *info = (AM_AV_TimeshiftInfo_t *)param;
        player_log_printf("\tPLAYER UPDATE: time[%d] status[%d]\n", info->current_time, info->status);
        pr_stat(tfile_start, tfile_end, info->current_time, info->status);
        }break;
    default:
        break;
    }
    /*player_log_printf("av_evt_cb: dev:0x%lx type:0x%x param:0x%lx\n",
        dev_no, event_type, (long)param);*/
}

int main(int argc, char **argv)
{
    AM_DMX_OpenPara_t para;
    AM_REC_CreatePara_t rcparam;
    AM_AV_OpenPara_t apara;
    int vpid=1019, apid=1018, vfmt=0, afmt=0;
    int duration=60;
    int size=1024*1024*1024;
    int tssrc=0;
    int pause=0;
    char crypt[16] = { 0 };

    int i;
    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "v", 1))
            sscanf(argv[i], "v=%i:%i", &vpid, &vfmt);
        else if (!strncmp(argv[i], "a", 1))
            sscanf(argv[i], "a=%i:%i", &apid, &afmt);
        else if (!strncmp(argv[i], "dur", 3))
            sscanf(argv[i], "dur=%i", &duration);
        else if (!strncmp(argv[i], "size", 4))
            sscanf(argv[i], "size=%i", &size);
        else if (!strncmp(argv[i], "tsin", 4))
            sscanf(argv[i], "tsin=%i", &tssrc);
        else if (!strncmp(argv[i], "pause", 5))
            sscanf(argv[i], "pause=%i", &pause);
        else if (!strncmp(argv[i], "crypt", 5))
            sscanf(argv[i], "crypt=%s", &crypt[0]);
        else if (!strncmp(argv[i], "help", 4)) {
            printf("Usage: %s [v=pid:fmt] [a=pid:fmt] [tsin=n] [dur=s] [pause=n]\n", argv[0]);
            exit(0);
        }
    }

    printf("video:%d:%d(pid/fmt) audio:%d:%d(pid/fmt)\n",
        vpid, vfmt, apid, afmt);
    printf("duration:%d size:%d tsin:%d\n",
        duration, size, tssrc);
    printf("pause:%d\n",
        pause);

    memset(&para, 0, sizeof(para));
    AM_TRY(AM_DMX_Open(DMX_DEV_DVR, &para));
    AM_DMX_SetSource(DMX_DEV_DVR, tssrc);

    memset(&para, 0, sizeof(para));
    AM_TRY(AM_DMX_Open(DMX_DEV_AV, &para));
    AM_DMX_SetSource(DMX_DEV_AV, AM_DMX_SRC_HIU);

    memset(&apara, 0, sizeof(apara));
    AM_TRY(AM_AV_Open(AV_DEV_NO, &apara));
    AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_DMX0 + DMX_DEV_AV);

    AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_VIDEO_AVAILABLE, av_evt_cb, "av0");
    AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_AV_NO_DATA, av_evt_cb, "av0");
    AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_AV_DATA_RESUME, av_evt_cb, "av0");
    AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_VIDEO_SCAMBLED, av_evt_cb, "av0");
    AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_AUDIO_SCAMBLED, av_evt_cb, "av0");
    AM_EVT_Subscribe(AV_DEV_NO, AM_AV_EVT_PLAYER_UPDATE_INFO, av_evt_cb, "av0");

    memset(&rcparam, 0, sizeof(rcparam));
    rcparam.fend_dev = 0;
    rcparam.dvr_dev = DMX_DEV_DVR;
    rcparam.async_fifo_id = 0;
    strncpy(rcparam.store_dir, "/data", AM_REC_PATH_MAX);

    int ret;
    ret = AM_REC_Create(&rcparam, &rec);
    if (ret != AM_SUCCESS) {
        printf("rec create fail. err=0x%x\n", ret);
        return -1;
    }

    AM_REC_SetUserData(rec, "rec0");

    AM_EVT_Subscribe((long)rec, AM_REC_EVT_RECORD_START, rec_evt_cb, "r0");
    AM_EVT_Subscribe((long)rec, AM_REC_EVT_RECORD_END, rec_evt_cb, "r0");

    AM_REC_SetTFile(rec, NULL, REC_TFILE_FLAG_AUTO_CREATE);

    AM_REC_RecPara_t rsparam;
    memset(&rsparam, 0, sizeof(rsparam));
    /*timeshifting's file name must contain string "TimeShifting"*/
    strncpy(rsparam.prefix_name, "rec_TimeShifting", AM_REC_NAME_MAX);
    strncpy(rsparam.suffix_name, "ts", AM_REC_SUFFIX_MAX);
    rsparam.media_info.vid_pid = vpid;
    rsparam.media_info.vid_fmt = vfmt;
    rsparam.media_info.aud_cnt = 1;
    rsparam.media_info.audios[0].pid = apid;
    rsparam.media_info.audios[0].fmt = afmt;
    rsparam.is_timeshift = 1;
    rsparam.total_time = duration;
    rsparam.total_size = size;

    if (strncmp(crypt, "des", 3) == 0)
        rsparam.crypt_ops = &des_ops;

    ret = AM_REC_StartRecord(rec, &rsparam);
    if (ret != AM_SUCCESS) {
        printf("rec start fail. err=0x%x\n", ret);
        AM_EVT_Unsubscribe((long)rec, AM_REC_EVT_RECORD_START, rec_evt_cb, "r0");
        AM_EVT_Unsubscribe((long)rec, AM_REC_EVT_RECORD_END, rec_evt_cb, "r0");
        AM_REC_Destroy(rec);
        return -1;
    }

    int flag;
    ret = AM_REC_GetTFile(rec, &tfile, &flag);
    if (ret == AM_SUCCESS) {
        AM_REC_SetTFile(rec, tfile, flag | REC_TFILE_FLAG_DETACH);

        AM_EVT_Subscribe((long)tfile, AM_TFILE_EVT_START_TIME_CHANGED, tfile_evt_cb, "f0");
        AM_EVT_Subscribe((long)tfile, AM_TFILE_EVT_END_TIME_CHANGED, tfile_evt_cb, "f0");

        AM_TFile_TimeStart(tfile);
    } else {
        printf("rec get tfile fail, err=0x%x\n", ret);
    }

    if (strncmp(crypt, "des", 3) == 0)
        AM_AV_SetCryptOps(AV_DEV_NO, &des_ops);

    AM_AV_TimeshiftPara_t tparam;
    memset(&tparam, 0, sizeof(tparam));
    tparam.dmx_id = DMX_DEV_AV;
    tparam.mode = AM_AV_TIMESHIFT_MODE_TIMESHIFTING;
    tparam.tfile = tfile;
    tparam.offset_ms = 0;
    tparam.start_paused = pause ? AM_TRUE : AM_FALSE;
    tparam.media_info.vid_pid = vpid;
    tparam.media_info.vid_fmt = vfmt;
    tparam.media_info.aud_cnt = 1;
    tparam.media_info.audios[0].pid = apid;
    tparam.media_info.audios[0].fmt = afmt;
    tparam.media_info.duration = duration;
    ret = AM_AV_StartTimeshift(AV_DEV_NO, &tparam);
    if (ret != AM_SUCCESS) {
        printf("av start timeshifting fail. err=0x%x\n", ret);
    }

    //command loop
    start_timeshift_test();

    AM_REC_Destroy(rec);

    //remember to close the tfile, as it has been detached from recorder.
    AM_TFile_Close(tfile);

    AM_AV_Close(AV_DEV_NO);
    AM_DMX_Close(DMX_DEV_AV);
    AM_DMX_Close(DMX_DEV_DVR);

    return 0;
}
