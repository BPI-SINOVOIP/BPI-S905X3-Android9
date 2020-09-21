/*
 * Copyright (c) 2018-08-30
 * yinli.xia<yinli.xia@amlogic.com>
 * This file is designed for frameinfo
 * so that all players will have a uniform API to get the frameinfo
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <sys/times.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <sys/ioctl.h>
#include <codec.h>
#include <codec_info.h>
#include <codec_type.h>
#include <codec_videoinfo.h>
#include <Amsysfsutils.h>
#include <amports/vformat.h>
#include <amports/aformat.h>
#include <amports/amstream.h>


static int perform_flag =0;
static int frame_rate_ctc_codec = 0;
static int threshold_value = 0;
static int threshold_ctl_flag = 0;
static int underflow_ctc = 0;
static int underflow_kernel = 0;
static int underflow_tmp = 0;
static int underflow_count = 0;
static int qos_count = 0;
static int first_frame = 0;
static unsigned int prev_vread_buffer = 0;
static int vrp_is_buffer_changed = 0;
static int last_data_len = 0;
static int last_data_len_statistics = 0;
static int64_t m_StartPlayTimePoint = 0;
static int prop_softdemux = 0;
static int prop_shouldshowlog = 0;
char underflow_statistics[60] = {0};

struct vid_frame_info videoFrmInfo_in[QOS_FRAME_NUM];
struct vid_frame_info videoFrmInfo_out[QOS_FRAME_NUM];

#define LOGV(...) \
    do { \
        if (prop_shouldshowlog) { \
            __android_log_print(ANDROID_LOG_VERBOSE, "FrameInfo", __VA_ARGS__); \
        } \
    } while (0)

#define LOGD(...) \
    do { \
        if (prop_shouldshowlog) { \
            __android_log_print(ANDROID_LOG_DEBUG, "FrameInfo", __VA_ARGS__); \
        } \
    } while (0)

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO  , "FrameInfo", __VA_ARGS__)

int64_t video_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int amvideo_checkunderflow( codec_para_t  *vcodec)
{
    int ret = 0;
    struct buf_status video_buf;
    //codec_para_t  *pcodec = NULL;
    //codec_para_t  *vcodec = NULL;
    codec_get_vbuf_state(vcodec, &video_buf);

    #if 1
    if (prev_vread_buffer != video_buf.read_pointer)
        vrp_is_buffer_changed = 1;
    else
        vrp_is_buffer_changed = 0;
    prev_vread_buffer = video_buf.read_pointer;
    #endif

    LOGD("checkunderflow pcodec->video_type is %d, video_buf.data_len is %d\n", vcodec->video_type, video_buf.data_len);
    if (vcodec->has_video) {
        if (vcodec->video_type == VFORMAT_MJPEG) {
            if(!vrp_is_buffer_changed &&
                (video_buf.data_len < (threshold_value >> 2))) {
                ret = 2;//underflow
                LOGI("checkunderflow video mjpeg is low level now\n");
            } else {
                ret = 0;
            }
        }
        else {
            if(!vrp_is_buffer_changed &&
                (video_buf.data_len< threshold_value)) {
                ret = 2;
                LOGI("checkunderflow video is low level now\n");
            } else {
                ret = 0;
            }
            if (video_buf.data_len == last_data_len)
                last_data_len_statistics++;
            last_data_len = video_buf.data_len;
        }
    }
    vrp_is_buffer_changed = 1;
    return ret;
}

void amvideo_checkunderflow_type(codec_para_t  *vcodec) {
    underflow_ctc = amvideo_checkunderflow(vcodec);
    underflow_kernel = amsysfs_get_sysfs_int("/sys/module/amports/parameters/decode_underflow");
    LOGD("report underflow_ctc = %d,underflow_kernel = %d,frame_rate_ctc = %d",
        underflow_ctc,underflow_kernel,frame_rate_ctc_codec);
    if (2 == underflow_ctc)
        underflow_tmp = 2;
    else if (3 == underflow_kernel)
        underflow_tmp = 3;
    else if ((2 == underflow_ctc) && (3 == underflow_kernel))
        underflow_tmp = 2;

    if (qos_count >= 60 )
        qos_count  = 0;
    underflow_statistics[qos_count++] = underflow_tmp;
    underflow_tmp = 0;
    underflow_ctc = 0;
    underflow_kernel = 0;
    amsysfs_set_sysfs_int("/sys/module/amports/parameters/decode_underflow", 0);
}

struct vid_frame_info* codec_get_frame_info(void)
{
    int i = 0;
    #if 0
    for (i = 0; i < frame_rate_ctc_codec; i++) {
         LOGD("Frame Info codec: type %d size %d nMinQP %d nMaxQP %d nAvgQP %d nMaxMV %d nMinMV %d nAvgMV %d SkipRatio %d nUnderflow %d\n",
            videoFrmInfo_out[i].enVidFrmType,
            videoFrmInfo_out[i].nVidFrmSize,
            videoFrmInfo_out[i].nMinQP,
            videoFrmInfo_out[i].nMaxQP,
            videoFrmInfo_out[i].nAvgQP,
            videoFrmInfo_out[i].nMaxMV,
            videoFrmInfo_out[i].nMinMV,
            videoFrmInfo_out[i].nAvgMV,
            videoFrmInfo_out[i].SkipRatio,
            videoFrmInfo_out[i].nUnderflow);
    }
    #endif
	return NULL;
}

int amvideo_ReportVideoFrameInfo(struct vframe_qos_s * pframe_qos)
{
    int i;
    int caton_num = 0,show_num = 0,num_set = 0;
    int igmp_num = 0,igmp_numset = 0,invalid_num = 0;
    int tmp_mv = 0;
    char value[256] = {0};

    memset(&videoFrmInfo_out, 0, QOS_FRAME_NUM * sizeof(struct vid_frame_info));

    property_get("iptv.shouldshowlog", value, "0");
    prop_shouldshowlog = atoi(value);

    /* +[SE] [REQ][BUG-171714][yinli.xia] add prop for frame sensitivity adjust*/
    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("iptv.frameinfo.igmpnum", value, "5");
    igmp_numset = atoi(value);

    for (i=0;(i<frame_rate_ctc_codec)&&(i<QOS_FRAME_NUM);i++) {
        if (0 != underflow_statistics[i])
            caton_num++;
        if (0 == pframe_qos[i].size)
            invalid_num++;
        if (last_data_len_statistics == (frame_rate_ctc_codec - 1))
            underflow_statistics[i] = 2;
        else
            underflow_statistics[i] = 0;

        if (pframe_qos[i].size == 0)
            underflow_statistics[i] = 2;
    }

    for (i=0;(i<frame_rate_ctc_codec)&&(i<QOS_FRAME_NUM);i++) {
        if ((pframe_qos[i].avg_mv > pframe_qos[i].max_mv) &&
            (pframe_qos[i].avg_mv > pframe_qos[i].min_mv)) {
            pframe_qos[i].avg_mv = (pframe_qos[i].max_mv + pframe_qos[i].min_mv) / 2;
         } else if (pframe_qos[i].max_mv < pframe_qos[i].min_mv) {
            tmp_mv = pframe_qos[i].max_mv;
            pframe_qos[i].max_mv = pframe_qos[i].min_mv;
            pframe_qos[i].min_mv = tmp_mv;
            pframe_qos[i].avg_mv = (pframe_qos[i].max_mv + pframe_qos[i].min_mv) / 2;
        }

        if (0 != underflow_statistics[i]) {
            videoFrmInfo_in[i].enVidFrmType = (vid_frame_type_x)0;
            videoFrmInfo_in[i].nVidFrmSize = 0;
            videoFrmInfo_in[i].nMinQP = 0;
            videoFrmInfo_in[i].nMaxQP = 0;
            videoFrmInfo_in[i].nAvgQP = 0;
            videoFrmInfo_in[i].nMaxMV = 0;
            videoFrmInfo_in[i].nMinMV = 0;
            videoFrmInfo_in[i].nAvgMV = 0;
            videoFrmInfo_in[i].SkipRatio = 0;
            videoFrmInfo_in[i].nUnderflow = underflow_statistics[i];
        } else {
            videoFrmInfo_in[i].enVidFrmType = (vid_frame_type_x) pframe_qos[i].type;
            videoFrmInfo_in[i].nVidFrmSize = pframe_qos[i].size;
            videoFrmInfo_in[i].nMinQP = pframe_qos[i].min_qp;
            videoFrmInfo_in[i].nMaxQP = pframe_qos[i].max_qp;
            videoFrmInfo_in[i].nAvgQP = pframe_qos[i].avg_qp;
            videoFrmInfo_in[i].nMaxMV = pframe_qos[i].max_mv;
            videoFrmInfo_in[i].nMinMV = pframe_qos[i].min_mv;
            videoFrmInfo_in[i].nAvgMV = pframe_qos[i].avg_mv;
            videoFrmInfo_in[i].SkipRatio = pframe_qos[i].avg_skip;
            videoFrmInfo_in[i].nUnderflow = underflow_statistics[i];
        }

        if (0 == videoFrmInfo_in[i].nVidFrmSize) {
            if (((frame_rate_ctc_codec - 1) < QOS_FRAME_NUM) &&
                ((frame_rate_ctc_codec - 1) >= 0) &&
                (pframe_qos[frame_rate_ctc_codec - 1].size == 0) &&
                (pframe_qos[0].size != 0) &&
                (invalid_num <= igmp_numset))
                videoFrmInfo_in[i].nUnderflow = 0;
        }

        if (((pframe_qos[i].type == 4) &&
            (0 == underflow_statistics[i])) ||
            (pframe_qos[i].type == 1)) {
            videoFrmInfo_in[i].enVidFrmType = VID_FRAME_TYPE_I;
            videoFrmInfo_in[i].SkipRatio = 0;
        }
        if (0 != underflow_statistics[i]) {
            videoFrmInfo_in[i].enVidFrmType = (vid_frame_type_x)0;
        }
        memcpy(&videoFrmInfo_out[i], &videoFrmInfo_in[i], sizeof(struct vid_frame_info));
    }
    //codec_get_frame_info();
    for (i = 0;i < 60;i++) {
        underflow_statistics[i] = 0;
    }
    qos_count = 0;
    last_data_len = 0;
    last_data_len_statistics = 0;
    return 0;
}

void amvideo_updateframeinfo(struct av_param_info_t av_param_info, struct av_param_qosinfo_t av_param_qosinfo)
{
    int i = 0;
    int crop_value = 0;
    int frame_height_t = 0;
    char value[PROPERTY_VALUE_MAX] = {0};

    frame_rate_ctc_codec = av_param_info.av_info.fps;
    LOGD("frame_rate_ctc = %u,av_param_info.av_info.first_pic_coming = %d\n",
        frame_rate_ctc_codec,av_param_info.av_info.first_pic_coming);
    if (frame_rate_ctc_codec > 0) {
        if (av_param_info.av_info.width == 0) {
            LOGI("updateCTCInfo:get first picture, width not get\n");
            return;
        }
        //m_sCtsplayerState.first_picture_comming = 1;
        if (!threshold_ctl_flag) {
            if (av_param_info.av_info.width > 1920)
                threshold_value = 700;
            else if (av_param_info.av_info.width <= 1920
                && av_param_info.av_info.width > 720)
                threshold_value = 300;
            else if (av_param_info.av_info.width <= 720)
                threshold_value = 250;
            threshold_ctl_flag = 1;
        }
    } else {
        return;
    }
    amvideo_ReportVideoFrameInfo(av_param_qosinfo.vframe_qos);
}






