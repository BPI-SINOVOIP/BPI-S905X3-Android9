/**
* @file codec_info.c
* @brief  Codec control lib functions
* @author chuanqi.wang <chuanqi.wang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <codec_error.h>
#include <codec_type.h>
#include <codec.h>
#include <audio_priv.h>
#include "codec_h_ctrl.h"
#include <adec-external-ctrl.h>
#include <amconfigutils.h>
#include <Amvideoutils.h>
#include "codec_info.h"

static int64_t codec_av_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int codec_get_blurred_screen(struct av_info_t *info,struct codec_quality_info* pblurred_screen)
{
        static int blurred_number = 0;
        static int blurred_number_first = 0;
        static int checkin_number_prev = 0;
        static int last_toggle_count = 0;
        int ratio = 0;
        static int checkin_number_after = 0;
        int cur_toggle_count = 0;
        static int64_t s_nlast_blur_time_ms = 0;
        int64_t cur_time_ms = 0;
        int send = 0;
        if (pblurred_screen == NULL ||info == NULL) {
            CODEC_PRINT("codec_get_blurred_screen NULL Pointer\n");
            return 0;
        }
        /* +[SE] [BUG][BUG-173477][zhizhong] modify:Not decrease drop count which cause blur_num 0*/
        blurred_number = info->dec_err_frame_count;
        cur_toggle_count = info->toggle_frame_count;
        #if 0
        CODEC_PRINT("blurred_number = %d,cur_toggle_count = %d,pblurred_screen->blurred_number_last = %d,pblurred_screen->blurred_flag = %d\n",
            blurred_number, cur_toggle_count, pblurred_screen->blurred_number_last, pblurred_screen->blurred_flag);
        #endif
        if ((blurred_number != pblurred_screen->blurred_number_last) && (pblurred_screen->blurred_flag == 0)) {
            pblurred_screen->blurred_flag = 1;
            blurred_number_first = blurred_number;
            pblurred_screen->blurred_number_last = blurred_number_first;
            cur_time_ms = codec_av_gettime()/1000;
            s_nlast_blur_time_ms = cur_time_ms;
            last_toggle_count = cur_toggle_count;
            checkin_number_prev = info->dec_frame_count;
            pblurred_screen->cur_time_ms = cur_time_ms;
            CODEC_PRINT("blur start: cnt:%d (err:%d,drop:%d) total:%d\n", blurred_number, info->dec_err_frame_count, info->dec_drop_frame_count,  checkin_number_prev);
            send = 1;
        }else if (pblurred_screen->blurred_flag == 1){
            cur_time_ms = codec_av_gettime()/1000;
            if (blurred_number == pblurred_screen->blurred_number_last) {
                if (cur_time_ms - s_nlast_blur_time_ms >= am_getconfig_int_def("media.amplayer.blurdelayms", 800)
                    && cur_toggle_count - last_toggle_count >= am_getconfig_int_def("media.amplayer.togglenum", 15)) {
                    pblurred_screen->blurred_flag = 0;
                    checkin_number_after =  info->dec_frame_count;
                    int decode_total = 0;
                    int decoder_total_err = 0;

                    decode_total = abs(checkin_number_after - checkin_number_prev);
                    CODEC_PRINT("blur end:total(%d - > %d), error(%d - > %d)\n",
                        checkin_number_prev, checkin_number_after, blurred_number_first, blurred_number);
                    if (decode_total > 0) {
                        decoder_total_err = abs(blurred_number - blurred_number_first);
                        ratio = 100 *decoder_total_err/decode_total;
                    }
                    pblurred_screen->cur_time_ms = cur_time_ms;
                    pblurred_screen->blurred_number_last = blurred_number;
                    pblurred_screen->ratio = ratio;
                    last_toggle_count = cur_toggle_count;
                    send = 1;
                }
            } else {
                s_nlast_blur_time_ms = cur_time_ms;
                pblurred_screen->blurred_number_last = blurred_number;
                last_toggle_count = cur_toggle_count;
            }
        }
        return send;
}

#if 0
/* +[SE] [BUG][IPTV-1021][yinli.xia] added: add probe event for blurred and unload event*/
int codec_get_upload(struct av_info_t *info,struct codec_quality_info* pupload_info)
{

        int ncur_toggle_count = 0;
        int64_t ncur_time_ms = 0;
        int send = 0;
        if (pupload_info == NULL ||info == NULL) {
            CODEC_PRINT("codec_get_upload NULL Pointer\n");
            return 0;
        }
        ncur_toggle_count = info->toggle_frame_count;
        ncur_time_ms = (int64_t)codec_av_gettime() / 1000;

        if (pupload_info->unload_flag == 0) {
            if (ncur_toggle_count - pupload_info->last_toggled_num >= info->fps - 5 &&
                (ncur_toggle_count >= 25) && (info->fps != 0)) {
                pupload_info->unload_flag = 0;
            } else if ((ncur_toggle_count - pupload_info->last_toggled_num < info->fps - 5) &&
                (ncur_toggle_count >= 25)) {
                pupload_info->unload_flag = 1;
                send = 1;
            }

            if (ncur_toggle_count > 10 && ncur_toggle_count == pupload_info->last_toggled_num) {
                if ((ncur_time_ms - pupload_info->last_untoggled_time_ms) >= am_getconfig_int_def("media.amplayer.unloadstartms", 500)) {
                    pupload_info->unload_flag = 1;
                    CODEC_PRINT("unload start\n");
                    send = 1;
                    pupload_info->last_untoggled_time_ms = ncur_time_ms;
                }
            } else {
                pupload_info->last_toggled_num = ncur_toggle_count;
                pupload_info->last_untoggled_time_ms = ncur_time_ms;
            }
        } else if (pupload_info->unload_flag == 1) {
            int toggled = ncur_toggle_count - pupload_info->last_toggled_num;
            //if (pupload_info->blurred_flag == 0) {
                if ((ncur_time_ms - pupload_info->last_untoggled_time_ms) >= am_getconfig_int_def("media.amplayer.unloadendms", 500)) {
                    if (toggled >= am_getconfig_int_def("media.amplayer.togglenum", 10)) {
                        CODEC_PRINT("unload end toggle(%d -> %d):\n", pupload_info->last_toggled_num, ncur_toggle_count);
                        pupload_info->unload_flag = 0;
                        send = 1;
                    }

                    //pupload_info->last_untoggled_time_ms = ncur_time_ms;
                    //pupload_info->last_toggled_num = ncur_toggle_count;
                //}
            } else {
                pupload_info->last_untoggled_time_ms = ncur_time_ms;
                pupload_info->last_toggled_num = ncur_toggle_count;
            }
        }
        return send;
}
#else
/* +[SE] [BUG][IPTV-1332][IPTV-1345][jipeng.zhao] revert IPTV-1021 change, use cmcc 20180721 version */
int codec_get_upload(struct av_info_t *info,struct codec_quality_info* pupload_info)
{

        int ncur_toggle_count = 0;
        int64_t ncur_time_ms = 0;
        int send = 0;
        if (pupload_info == NULL ||info == NULL) {
            CODEC_PRINT("codec_get_upload NULL Pointer\n");
            return 0;
        }
        ncur_toggle_count = info->toggle_frame_count;
        ncur_time_ms = (int64_t)codec_av_gettime() / 1000;
        if (pupload_info->unload_flag == 0) {
            if (ncur_toggle_count > 10 && ncur_toggle_count == pupload_info->last_toggled_num) {
                if ((ncur_time_ms - pupload_info->last_untoggled_time_ms) >= am_getconfig_int_def("media.amplayer.unloadstartms", 500)) {
                    pupload_info->unload_flag = 1;
                    CODEC_PRINT("unload start\n");
                    send = 1;
                    pupload_info->last_untoggled_time_ms = ncur_time_ms;
                }
            } else {
                pupload_info->last_toggled_num = ncur_toggle_count;
                pupload_info->last_untoggled_time_ms = ncur_time_ms;
            }
        } else if (pupload_info->unload_flag == 1) {
            int toggled = ncur_toggle_count - pupload_info->last_toggled_num;
            if (pupload_info->blurred_flag == 0) {
                if ((ncur_time_ms - pupload_info->last_untoggled_time_ms) >= am_getconfig_int_def("media.amplayer.unloadendms", 800)) {
                    if (toggled >= am_getconfig_int_def("media.amplayer.togglenum", 15)) {
                        CODEC_PRINT("unload end toggle(%d -> %d):\n", pupload_info->last_toggled_num, ncur_toggle_count);
                        pupload_info->unload_flag = 0;
                        send = 1;
                    }

                    pupload_info->last_untoggled_time_ms = ncur_time_ms;
                    pupload_info->last_toggled_num = ncur_toggle_count;
                }
            } else {
                pupload_info->last_untoggled_time_ms = ncur_time_ms;
                pupload_info->last_toggled_num = ncur_toggle_count;
            }
        }
        return send;
}
#endif
