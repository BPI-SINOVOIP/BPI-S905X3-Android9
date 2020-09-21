/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c++ file
*/
#include <pthread.h>
//#include <core/SkBitmap.h>
#include <am_sub2.h>
#include <am_tt2.h>
#include <am_dmx.h>
#include <am_pes.h>
#include <am_misc.h>
#include <am_scte27.h>
#include <am_userdata.h>

#include <am_cc.h>
#include <stdlib.h>
#include <pthread.h>
#include <jni.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>

#include <android/bitmap.h>
//#include <sub_set_sys.h>


extern "C" {

    typedef struct {
        pthread_mutex_t  lock;
        AM_SUB2_Handle_t sub_handle;
        AM_PES_Handle_t  pes_handle;
        AM_TT2_Handle_t  tt_handle;
        AM_CC_Handle_t   cc_handle;
        AM_SCTE27_Handle_t scte27_handle;
        int              dmx_id;
        int              filter_handle;
        jobject          obj;
        //SkBitmap        *bitmap;
        jobject          obj_bitmap;
        int              bmp_w;
        int              bmp_h;
        int              bmp_pitch;
        uint8_t         *buffer;
        int             sub_w;
        int             sub_h;
    } TVSubtitleData;

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG    "tvsubtitle_tv"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define CC_JSON_BUFFER_SIZE 8192


    static JavaVM   *gJavaVM = NULL;
    static jmethodID gUpdateID;
    static jfieldID  gBitmapID;
    static TVSubtitleData gSubtitleData;
    static jmethodID gUpdateDataID;
    static jmethodID gReportErrorID;
    static jmethodID gReportAvailableID;
    static jfieldID gCC_JsonID;
    static jstring gCC_JsonStr;
    static jmethodID gPassJsonStr;
    static jmethodID gPassLanguage;
    static char gJsonStr[CC_JSON_BUFFER_SIZE];
    static char width[10];
    static char height[10];
    pthread_t mSubThread;
    int ud_dev_no = 0;


    static jint sub_clear(JNIEnv *env, jobject obj);
    static void sub_update(jobject obj);
    static void data_update(jobject obj, char *json);
    static void error_report(jobject obj, int error);
    static void available_report(jobject obj);
    static void notify_bitmap_changed(jobject bitmap)
    {
    }

    static void set_subtitle_piture_param(int bitmap_w, int bitmap_h)
    {
        TVSubtitleData *data;
        data = &gSubtitleData;
        data->sub_w = bitmap_w;
        data->sub_h = bitmap_h;
        LOGE("[set_subtitle_piture_param]-sub_w:%d, sub_h:%d", data->sub_w, data->sub_h);
    }

    static void setBitmapWidthHeight()
    {
       char read_buff[8];
       int video_w, video_h;
       AM_FileRead("/sys/class/video/frame_height", read_buff, sizeof(read_buff));
       video_h = strtoul(read_buff, NULL, 10);

       //Meet a case, 525x480. So we check height and get width
       switch (video_h)
       {
             case 480:
                    video_w = 720;
                    break;
             case 576:
                    video_w = 720;
                    break;
             case 720:
                    video_w = 1280;
                    break;
             case 1080:
                    video_w = 1920;
                    break;
             default:
                    video_w = video_h /3 * 4;
                    break;
       }
       LOGE("[sub_start_scte27]-video_w:%d, video_h:%d", video_w, video_h);
       //set_subtitle_piture_param(video_w, video_h);
       sprintf(width, "%d", video_w);
       sprintf(height, "%d", video_w);
       LOGI("echo setBitmapWidthHeight w:%s h:%s", width, height);
       AM_FileEcho("/sys/class/subtitle/width", width);
       AM_FileEcho("/sys/class/subtitle/height", height);
    }

    static void draw_begin_cb(AM_TT2_Handle_t handle)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_TT2_GetUserData(handle);

        pthread_mutex_lock(&sub->lock);
    }

    static void draw_end_cb(AM_TT2_Handle_t handle, int page_type, int pgno, char* subs, int sub_cnt, int red, int green, int yellow, int blue, int curr_subpg)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_TT2_GetUserData(handle);

        pthread_mutex_unlock(&sub->lock);
        sub_update(sub->obj);
    }

    static void clear_bitmap(TVSubtitleData *sub)
    {
        uint8_t *ptr = sub->buffer;
        int y = sub->bmp_h;

        while (y--) {
            memset(ptr, 0, sub->bmp_pitch);
            ptr += sub->bmp_pitch;
        }
    }

    static uint8_t *lock_bitmap(JNIEnv *env, jobject bitmap)
    {
        int attached = 0;
        uint8_t *buf = NULL;
        if (!env) {
            int ret;
            ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
            if (ret < 0) {
                ret = gJavaVM->AttachCurrentThread(&env, NULL);
                if (ret < 0) {
                    LOGE("Can't attach thread");
                    return NULL;
                }
                attached = 1;
            }
        }
        AndroidBitmap_lockPixels(env, bitmap, (void **) &buf);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }

        return buf;
    }

    static void unlock_bitmap(JNIEnv *env, jobject bitmap)
    {
        int attached = 0;
        if (!env) {
            int ret;
            ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
            if (ret < 0) {
                ret = gJavaVM->AttachCurrentThread(&env, NULL);
                if (ret < 0) {
                    LOGE("Can't attach thread");
                    return;
                }
                attached = 1;
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }

    static void scte27_draw_begin_cb(AM_TT2_Handle_t handle)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_SCTE27_GetUserData(handle);

        pthread_mutex_lock(&sub->lock);

        sub->buffer = lock_bitmap(NULL, sub->obj_bitmap);
        //clear_bitmap(sub);
    }

    static void scte27_draw_end_cb(AM_SCTE27_Handle_t handle)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_SCTE27_GetUserData(handle);

        unlock_bitmap(NULL, sub->obj_bitmap);

        pthread_mutex_unlock(&sub->lock);
        setBitmapWidthHeight();
        LOGI("[scte27_draw_end_cb]bitmap buffer [%p]", sub->buffer);
        sub_update(sub->obj);
    }

    static void scte27_lang_cb(AM_SCTE27_Handle_t handle, char *buffer, int size)
    {
        LOGE("[scte27_lang_cb]buffer:%s, size:%d", buffer, size);
        if (buffer == NULL)
            return;

        JNIEnv *env;
        int ret;
        int attached = 0;
        jstring data;

        if (size == 0)
            return;
        TVSubtitleData *sub = (TVSubtitleData *)AM_SCTE27_GetUserData(handle);
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);

        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        data = env->NewStringUTF(buffer);
        env->CallVoidMethod(sub->obj, gPassLanguage, data);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }

    static void scte27_report_available_cb(AM_SCTE27_Handle_t handle)
    {
        LOGE("[scte27_report_available_cb]");
        TVSubtitleData *sub = (TVSubtitleData *)AM_SCTE27_GetUserData(handle);
        available_report(sub->obj);
    }

    static void dvb_report_available_cb(AM_SUB2_Handle_t handle)
    {
        LOGE("[dvb_report_available_cb]");
        TVSubtitleData *sub = (TVSubtitleData *)AM_SUB2_GetUserData(handle);
        available_report(sub->obj);
    }

    static void scte27_report_cb(AM_SCTE27_Handle_t handle, int error)
    {
        LOGE("[scte27_report_cb]error:%d", error);
        TVSubtitleData *sub = (TVSubtitleData *)AM_SCTE27_GetUserData(handle);
        error_report(sub->obj, error);
    }

    static void dvb_report_cb(AM_SUB2_Handle_t handle, int error)
    {
        LOGE("[dvb_report_cb]error:%d", error);
        TVSubtitleData *sub = (TVSubtitleData *)AM_SUB2_GetUserData(handle);
        error_report(sub->obj, error);
    }

    static void cc_report_cb(AM_CC_Handle_t handle, int error)
    {
        LOGE("[cc_report_cb]error:%d", error);
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);
        error_report(sub->obj, error);
    }

    static void cc_draw_begin_cb(AM_CC_Handle_t handle, AM_CC_DrawPara_t *draw_para)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);

        pthread_mutex_lock(&sub->lock);

    }

    static void cc_draw_end_cb(AM_CC_Handle_t handle, AM_CC_DrawPara_t *draw_para)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);

        sub->sub_w = draw_para->caption_width;
        sub->sub_h = draw_para->caption_height;

        pthread_mutex_unlock(&sub->lock);
        sub_update(sub->obj);

    }

    static void cc_rating_cb(AM_CC_Handle_t handle, vbi_rating *pr)
    {
        if (pr) {
            char *tag = "Aratings";
            TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);
            char json[64] = {0};
            snprintf(json, 64, "{%s:{g:%d,i:%d,dlsv:%d}}",
                    tag,
                    pr->auth,
                    pr->id,
                    pr->dlsv
                    );
            data_update(sub->obj, json);
        }
    }

    static void cc_data_cb(AM_CC_Handle_t handle, int mask)
    {
        LOGE("cc_data_cb");
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);
        char json[64];
        char *ratings = "";

        if (mask != (1 << AM_CC_CAPTION_DATA)) {
            if (!(mask & (1 << AM_CC_CAPTION_XDS))) {
                ratings = "Aratings:{},";
            }
        }

        sprintf(json, "{%scc:{data:%d}}", ratings, mask);
        data_update(sub->obj, json);
    }

    static void dtvcc_nodata_cb(AM_CC_Handle_t handle)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);
        char json[64] = "{dtvcc:{nodata:1}}";
        data_update(sub->obj, json);
    }

    static void channel_cb(AM_CC_Handle_t handle, AM_CC_CaptionMode_t chn)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);
        char json[64];
        sprintf(json, "{dtvcc:{found:%d}}", (int)chn);
        data_update(sub->obj, json);
    }

    static void pes_tt_cb(AM_PES_Handle_t handle, uint8_t *buf, int size)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_PES_GetUserData(handle);

        AM_TT2_Decode(sub->tt_handle, buf, size);
    }

    static TVSubtitleData *sub_get_data(JNIEnv *env, jobject obj)
    {
        return &gSubtitleData;
    }

    static void pes_data_cb(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data)
    {
        LOGE("[pes_data_cb]");
        TVSubtitleData *sub = (TVSubtitleData *)user_data;

        AM_PES_Decode(sub->pes_handle, (uint8_t *)data, len);
    }

    static void pes_sub_cb(AM_PES_Handle_t handle, uint8_t *buf, int size)
    {
        LOGE("[pes_sub_cb]");
        TVSubtitleData *sub = (TVSubtitleData *)AM_PES_GetUserData(handle);

        AM_SUB2_Decode(sub->sub_handle, buf, size);
    }

    static void show_sub_cb(AM_SUB2_Handle_t handle, AM_SUB2_Picture_t *pic)
    {
        TVSubtitleData *sub = (TVSubtitleData *)AM_SUB2_GetUserData(handle);
        LOGE("[show_sub_cb]");

        pthread_mutex_lock(&sub->lock);

        sub->buffer = lock_bitmap(NULL, sub->obj_bitmap);
        clear_bitmap(sub);
        if (pic) {
            AM_SUB2_Region_t *rgn = pic->p_region;
            sub->sub_w = pic->original_width;
            sub->sub_h = pic->original_height;
            while (rgn) {
                int sx, sy, dx, dy, rw, rh;
                if (! rgn->p_buf) {
                    rgn = rgn->p_next;
                    continue;
                }
                sx = 0;
                sy = 0;
                dx = pic->original_x + rgn->left;
                dy = pic->original_y + rgn->top;
                rw = rgn->width;
                rh = rgn->height;
                if (dx < 0) {
                    sx = -dx;
                    dx = 0;
                    rw += dx;
                }
                if (dx + rw > sub->bmp_w) {
                    rw = sub->bmp_w - dx;
                }
                if (dy < 0) {
                    sy = -dy;
                    dy = 0;
                    rh += dy;
                }
                if (dy + rh > sub->bmp_h) {
                    rh = sub->bmp_h - dy;
                }
                if ((rw > 0) && (rh > 0)) {
                    uint8_t *sbegin = rgn->p_buf + sy * rgn->width + sx;
                    uint8_t *dbegin = sub->buffer + dy * sub->bmp_pitch + dx * 4;
                    uint8_t *src, *dst;
                    int size;
                    while (rh) {
                        src = sbegin;
                        dst = dbegin;
                        size = rw;
                        while (size--) {
                            int c = src[0];
                            if (c < (int)rgn->entry) {
                                if (rgn->clut[c].a) {
                                    *dst++ = rgn->clut[c].r;
                                    *dst++ = rgn->clut[c].g;
                                    *dst++ = rgn->clut[c].b;
                                } else {
                                    dst += 3;
                                }
                                *dst++ = rgn->clut[c].a;
                            } else {
                                dst += 4;
                            }
                            src ++;
                        }
                        sbegin += rgn->width;
                        dbegin += sub->bmp_pitch;
                        rh--;
                    }
                }
                rgn = rgn->p_next;
            }
        }
        unlock_bitmap(NULL, sub->obj_bitmap);
        pthread_mutex_unlock(&sub->lock);

        sub_update(sub->obj);
    }

    static uint64_t get_pts_cb(void *handle, uint64_t pts)
    {
        char buf[32];
        AM_ErrorCode_t ret;
        uint32_t v;
        uint64_t r;

        ret = AM_FileRead("/sys/class/tsync/pts_pcrscr", buf, sizeof(buf));
        if (!ret) {
            v = strtoul(buf, 0, 16);
            if (pts & (1LL << 32)) {
                r = ((uint64_t)v) | (1LL << 32);
            } else {
                r = (uint64_t)v;
            }
        } else {
            r = 0LL;
        }

        return r;
    }

    static int open_dmx(TVSubtitleData *data, int dmx_id, int pid)
    {
        LOGI("[open_dmx]dmx_id:%d,pid:%d", dmx_id, pid);
        AM_DMX_OpenPara_t op;
        struct dmx_pes_filter_params pesp;
        AM_ErrorCode_t ret;

        data->dmx_id = -1;
        data->filter_handle = -1;
        memset(&op, 0, sizeof(op));

        ret = AM_DMX_Open(dmx_id, &op);
        if (ret != AM_SUCCESS)
            goto error;
        data->dmx_id = dmx_id;
        //LOGI("[open_dmx]AM_DMX_Open");

        ret = AM_DMX_AllocateFilter(dmx_id, &data->filter_handle);
        if (ret != AM_SUCCESS)
            goto error;
        //LOGI("[open_dmx]AM_DMX_AllocateFilter");

        ret = AM_DMX_SetBufferSize(dmx_id, data->filter_handle, 0x80000);
        if (ret != AM_SUCCESS)
            goto error;
        //LOGI("[open_dmx]AM_DMX_SetBufferSize");

        memset(&pesp, 0, sizeof(pesp));
        pesp.pid = pid;
        pesp.output = DMX_OUT_TAP;
        pesp.pes_type = DMX_PES_TELETEXT0;

        ret = AM_DMX_SetPesFilter(dmx_id, data->filter_handle, &pesp);
        if (ret != AM_SUCCESS)
            goto error;
        //LOGI("[open_dmx]AM_DMX_SetPesFilter");

        ret = AM_DMX_SetCallback(dmx_id, data->filter_handle, pes_data_cb, data);
        if (ret != AM_SUCCESS)
            goto error;
        //LOGI("[open_dmx]AM_DMX_SetCallback");

        ret = AM_DMX_StartFilter(dmx_id, data->filter_handle);
        if (ret != AM_SUCCESS)
            goto error;
        //LOGI("[open_dmx]AM_DMX_StartFilter");

        return 0;
error:
        if (data->filter_handle != -1) {
            AM_DMX_FreeFilter(dmx_id, data->filter_handle);
        }
        if (data->dmx_id != -1) {
            AM_DMX_Close(dmx_id);
        }

        return -1;
    }

    static int close_dmx(TVSubtitleData *data)
    {
        AM_DMX_FreeFilter(data->dmx_id, data->filter_handle);
        AM_DMX_Close(data->dmx_id);
        data->dmx_id = -1;
        data->filter_handle = -1;

        return 0;
    }

    static void sub_update(jobject obj)
    {
        JNIEnv *env;
        int ret;
        int attached = 0;

        if (!obj)
            return;

        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);

        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        env->CallVoidMethod(obj, gUpdateID, NULL);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }

    static void json_update_cb(AM_CC_Handle_t handle)
    {
        LOGE("[json_update_cb]");
        TVSubtitleData *sub = (TVSubtitleData *)AM_CC_GetUserData(handle);
        JNIEnv *env;
        int ret;
        jstring data;
        int attached = 0;
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        data = env->NewStringUTF(gJsonStr);
        env->CallVoidMethod(sub->obj, gPassJsonStr, data);
        env->DeleteLocalRef(data);

        if (attached) {
            gJavaVM->DetachCurrentThread();
        }

    }

    static void available_report(jobject obj)
    {
        LOGE("[available_report]");
        JNIEnv *env;
        int ret;
        int attached = 0;
        LOGE("[available_report]obj:%p", obj);
        if (!obj)
            return;
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        env->CallVoidMethod(obj, gReportAvailableID);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }


    static void error_report(jobject obj, int error)
    {
        LOGE("[error_report]error:%d", error);
        JNIEnv *env;
        int ret;
        int attached = 0;
        LOGE("[error_report]obj:%p", obj);
        if (!obj)
            return;
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        env->CallVoidMethod(obj, gReportErrorID, error);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }

    static void data_update(jobject obj, char *json)
    {
        LOGE("[data_update]");
        JNIEnv *env;
        int ret;
        int attached = 0;
        LOGE("[data_update]obj:%p", obj);
        if (!obj)
            return;
        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        jstring data = env->NewStringUTF(json);
        //LOGE("data_update-3-data:%s", data);
        env->CallVoidMethod(obj, gUpdateDataID, data);
        env->DeleteLocalRef(data);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }

    static jint get_subtitle_piture_width(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);
        return data->sub_w;
    }

    static jint get_subtitle_piture_height(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);
        return data->sub_h;
    }

    static uint8_t *get_bitmap(JNIEnv *env, TVSubtitleData *sub, int *w, int *h, int *pitch)
    {
        uint8_t *buf;
        int width, height, stride;

        buf = lock_bitmap(env, sub->obj_bitmap);
        LOGI("bitmap buffer [%p]", buf);

        if (!buf) {
            LOGE("allocate bitmap buffer failed");
        } else {
            AndroidBitmapInfo bitmapInfo;
            AndroidBitmap_getInfo(env, sub->obj_bitmap, &bitmapInfo);
            LOGI("init bitmap info w:%d h:%d s:%d", bitmapInfo.width, bitmapInfo.height, bitmapInfo.stride);

            if (w) {
                *w = bitmapInfo.width;
            }
            if (h) {
                *h = bitmapInfo.height;
            }
            if (pitch) {
                *pitch = bitmapInfo.stride;
            }
        }
        unlock_bitmap(env, sub->obj_bitmap);
        return buf;
    }

    static void bitmap_init(jobject obj, int bitmap_w, int bitmap_h)
    {
        JNIEnv *env;
        int ret;
        int attached = 0;
        jobject bmp;
        TVSubtitleData *data;

        if (!obj)
            return;
        data = &gSubtitleData;

        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);

        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }

        bmp = env->GetStaticObjectField(env->FindClass("com/tv/DTVSubtitleView"), gBitmapID);
        if (!data->obj_bitmap)
            data->obj_bitmap = env->NewGlobalRef(bmp);
        if (!data->buffer)
            data->buffer = get_bitmap(env, data, &data->bmp_w, &data->bmp_h, &data->bmp_pitch);
        data->sub_w = bitmap_w;
        data->sub_h = bitmap_h;

        LOGI("init_bitmap w:%d h:%d p:%d", data->bmp_w, data->bmp_h, data->bmp_pitch);
        if (!data->buffer) {
            env->DeleteGlobalRef(data->obj_bitmap);
            pthread_mutex_destroy(&data->lock);
            return;
        }
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }

    static void bitmap_deinit(jobject obj)
    {
        JNIEnv *env;
        TVSubtitleData *data;
        int ret;
        int attached = 0;

        data = &gSubtitleData;
        if (!obj)
            return;

        ret = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);

        if (ret < 0) {
            ret = gJavaVM->AttachCurrentThread(&env, NULL);
            if (ret < 0) {
                LOGE("Can't attach thread");
                return;
            }
            attached = 1;
        }
        env->DeleteGlobalRef(data->obj_bitmap);
        data->buffer = NULL;
        if (data->obj_bitmap)
            data->obj_bitmap = NULL;
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
    }


unsigned long sub_config_get(char *config, unsigned long def)
{
    char prop_value[PROPERTY_VALUE_MAX], tmp[32];

    memset(tmp, 0, sizeof(tmp));
    snprintf(tmp, sizeof(tmp)-1, "%d", def);
    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get(config, prop_value, "");
    if (strcmp(prop_value, "\0") != 0) {
        def = strtol(prop_value, NULL, 10);
    }
    return def;
}

static void setDvbDebugLogLevel() {
    AM_DebugSetLogLevel(property_get_int32("tv.dvb.loglevel", 1));
}

unsigned long getSDKVersion()
{
    unsigned long version = sub_config_get("ro.build.version.sdk", 0);
    LOGE("VERSION_NUM --- %ld", version);
    return version;
}

    static jint sub_init(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data;
        jclass bmp_clazz;
        jfieldID fid;
        jobject bmp;
        jlong hbmp;

        data = &gSubtitleData;
        memset(data, 0, sizeof(TVSubtitleData));

        pthread_mutex_init(&data->lock, NULL);

        data->obj = env->NewGlobalRef(obj);

        /*just init, no effect*/
        data->sub_w = 720;
        data->sub_h = 576;

        setDvbDebugLogLevel();
        return 0;
    }

    static jint sub_destroy(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        if (data) {
            sub_clear(env, obj);

            if (data->obj) {
                pthread_mutex_lock(&data->lock);
                env->DeleteGlobalRef(data->obj);
                data->obj = NULL;
                pthread_mutex_unlock(&data->lock);
            }

            pthread_mutex_destroy(&data->lock);
        }

        return 0;
    }

    static jint sub_lock(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        pthread_mutex_lock(&data->lock);

        return 0;
    }

    static jint sub_unlock(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        pthread_mutex_unlock(&data->lock);

        return 0;
    }

    static jint sub_clear(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        if (data->tt_handle) {
            AM_TT2_Destroy(data->tt_handle);
            data->tt_handle = NULL;
        }

        return 0;
    }

    static void scte27_section_callback(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
    {
        LOGE("callback");
        AM_SCTE27_Decode(user_data, data, len);
    }

    static void scte27_update_size(AM_SCTE27_Handle_t handle, int width, int height)
    {
             LOGE("scte27_update_size width:%d,height:%d", width, height);
             TVSubtitleData *sub = (TVSubtitleData *)AM_SCTE27_GetUserData(handle);


             pthread_mutex_lock(&sub->lock);
             sub->sub_w = width;
             sub->sub_h = height;
             pthread_mutex_unlock(&sub->lock);
    }

    static jint sub_start_scte27(JNIEnv *env, jobject obj, jint dmx_id, jint pid)
    {
        LOGE("[sub_start_scte27]");
        int ret;
        char read_buff[8];
        int video_w, video_h;
        //pid = 0;
        dmx_id = 1;
        LOGE("[sub_start_scte27]pid:%d, dmx_id:%d", pid, dmx_id);
        TVSubtitleData *data = sub_get_data(env, obj);
        AM_SCTE27_Para_t sctep;
        struct dmx_sct_filter_params param;

        setDvbDebugLogLevel();

        video_w = 1920;
        video_h = 1080;

        bitmap_init(obj, video_w, video_h);

        data->dmx_id = dmx_id;

        memset(&sctep, 0, sizeof(sctep));
        sctep.width = video_w;
        sctep.height = video_h;
        sctep.draw_begin     = scte27_draw_begin_cb;
        sctep.draw_end    = scte27_draw_end_cb;
        sctep.lang_cb    = scte27_lang_cb;
        sctep.report    = scte27_report_cb;
        sctep.report_available    = scte27_report_available_cb;
        sctep.bitmap    = &data->buffer;
        sctep.pitch	  = data->bmp_pitch;
        sctep.update_size = scte27_update_size;
        sctep.user_data = data;

        ret = AM_SCTE27_Create(&data->scte27_handle, &sctep);
        if (ret != AM_SUCCESS)
            goto error;
        ret = AM_SCTE27_Start(data->scte27_handle);
        if (ret != AM_SUCCESS)
            goto error;
        AM_DMX_OpenPara_t op;
        memset(&op, 0, sizeof(op));
        ret = AM_DMX_Open(data->dmx_id, &op);
        if (ret != AM_SUCCESS)
            goto error;
        ret = AM_DMX_AllocateFilter(data->dmx_id, &data->filter_handle);
        if (ret != AM_SUCCESS)
            goto error;
        ret = AM_DMX_SetCallback(data->dmx_id, data->filter_handle, scte27_section_callback, (void*)data->scte27_handle);
        if (ret != AM_SUCCESS)
            goto error;
        ret = AM_DMX_SetBufferSize(data->dmx_id, data->filter_handle, 1024*1024);
        if (ret != AM_SUCCESS)
            goto error;
        memset(&param, 0, sizeof(param));
        param.pid = pid;
        param.filter.filter[0] = SCTE27_TID;
        param.filter.mask[0] = 0xff;
        param.flags = DMX_CHECK_CRC;

        ret = AM_DMX_SetSecFilter(data->dmx_id, data->filter_handle, &param);
        if (ret != AM_SUCCESS)
            goto error;
        ret = AM_DMX_StartFilter(data->dmx_id, data->filter_handle);
        if (ret != AM_SUCCESS)
            goto error;

        return 0;
error:
        LOGE("scte start failed");
        if (data->scte27_handle) {
            AM_SCTE27_Destroy(data->scte27_handle);
            data->scte27_handle = NULL;
        }
        return 0;
    }

    static jint sub_stop_scte27(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        close_dmx(data);
        AM_SCTE27_Destroy(data->scte27_handle);

        AM_DMX_StopFilter(data->dmx_id, data->filter_handle);
        AM_DMX_FreeFilter(data->dmx_id, data->filter_handle);

        pthread_mutex_lock(&data->lock);
        data->buffer = lock_bitmap(env, data->obj_bitmap);
        clear_bitmap(data);
        unlock_bitmap(env, data->obj_bitmap);
        if (data->obj)
            sub_update(data->obj);
        bitmap_deinit(obj);
        pthread_mutex_unlock(&data->lock);

        data->sub_handle = NULL;
        data->pes_handle = NULL;
        return 0;
    }

    static jint sub_start_dvb_sub(JNIEnv *env, jobject obj, jint dmx_id, jint pid, jint page_id, jint anc_page_id)
    {
        LOGE("sub_start_dvb_sub");
        TVSubtitleData *data = sub_get_data(env, obj);
        AM_PES_Para_t pesp;
        AM_SUB2_Para_t subp;
        int ret;

        setDvbDebugLogLevel();

        bitmap_init(obj, 720, 576);
        memset(&pesp, 0, sizeof(pesp));
        pesp.packet    = pes_sub_cb;
        pesp.user_data = data;
        ret = AM_PES_Create(&data->pes_handle, &pesp);
        if (ret != AM_SUCCESS)
            goto error;

        memset(&subp, 0, sizeof(subp));
        subp.show      = show_sub_cb;
        subp.get_pts   = get_pts_cb;
        subp.report   = dvb_report_cb;
        subp.report_available   = dvb_report_available_cb;
        subp.composition_id = page_id;
        subp.ancillary_id   = anc_page_id;
        subp.user_data = data;
        ret = AM_SUB2_Create(&data->sub_handle, &subp);
        if (ret != AM_SUCCESS)
            goto error;

        ret = AM_SUB2_Start(data->sub_handle);
        if (ret != AM_SUCCESS)
            goto error;

        ret = open_dmx(data, dmx_id, pid);
        if (ret < 0)
            goto error;

        return 0;
error:
        if (data->sub_handle) {
            AM_SUB2_Destroy(data->sub_handle);
            data->sub_handle = NULL;
        }
        if (data->pes_handle) {
            AM_PES_Destroy(data->pes_handle);
            data->pes_handle = NULL;
        }
        return -1;
    }

    static jint sub_start_dtv_tt(JNIEnv *env, jobject obj, jint dmx_id, jint region_id, jint pid, jint page, jint sub_page, jboolean is_sub)
    {
        TVSubtitleData *data = sub_get_data(env, obj);
        AM_PES_Para_t pesp;
        AM_TT2_Para_t ttp;
        int ret;

        setDvbDebugLogLevel();

        if (!data->tt_handle) {
            memset(&ttp, 0, sizeof(ttp));
            ttp.draw_begin = draw_begin_cb;
            ttp.draw_end  = draw_end_cb;
            ttp.is_subtitle = is_sub;
            //ttp.bitmap    = data->buffer;
            ttp.pitch     = data->bmp_pitch;
            ttp.user_data = data;
            ttp.default_region = region_id;
            ret = AM_TT2_Create(&data->tt_handle, &ttp);
            if (ret != AM_SUCCESS)
                goto error;
        } else {
            AM_TT2_SetSubtitleMode(data->tt_handle, is_sub);
        }

        AM_TT2_GotoPage(data->tt_handle, page, sub_page);
        AM_TT2_Start(data->tt_handle, region_id);

        memset(&pesp, 0, sizeof(pesp));
        pesp.packet    = pes_tt_cb;
        pesp.user_data = data;
        ret = AM_PES_Create(&data->pes_handle, &pesp);
        if (ret != AM_SUCCESS)
            goto error;

        ret = open_dmx(data, dmx_id, pid);
        if (ret < 0)
            goto error;

        LOGI("Teletext started at demux %d, pid %d, page %d", dmx_id, pid, page);
        return 0;
error:
        if (data->pes_handle) {
            AM_PES_Destroy(data->pes_handle);
            data->pes_handle = NULL;
        }
        return -1;
    }

    static jint sub_stop_dvb_sub(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        close_dmx(data);
        AM_SUB2_Destroy(data->sub_handle);
        AM_PES_Destroy(data->pes_handle);
        pthread_mutex_lock(&data->lock);
        if (data->obj)
            sub_update(data->obj);
        pthread_mutex_unlock(&data->lock);

        data->sub_handle = NULL;
        data->pes_handle = NULL;

        return 0;
    }

    static jint sub_stop_dtv_tt(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        close_dmx(data);
        AM_PES_Destroy(data->pes_handle);
        data->pes_handle = NULL;
        AM_TT2_Stop(data->tt_handle);

        pthread_mutex_lock(&data->lock);
        if (data->obj)
            sub_update(data->obj);
        pthread_mutex_unlock(&data->lock);

        return 0;
    }

    static jint sub_tt_goto(JNIEnv *env, jobject obj, jint page)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        AM_TT2_GotoPage(data->tt_handle, page, AM_TT2_ANY_SUBNO);
        return 0;
    }

    static jint sub_tt_color_link(JNIEnv *env, jobject obj, jint color)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        AM_TT2_ColorLink(data->tt_handle, (AM_TT2_Color_t)color);
        return 0;
    }

    static jint sub_tt_home_link(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        AM_TT2_GoHome(data->tt_handle);
        return 0;
    }

    static jint sub_tt_next(JNIEnv *env, jobject obj, jint dir)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        AM_TT2_NextPage(data->tt_handle, dir);

        return 0;
    }

    static jint sub_tt_set_search_pattern(JNIEnv *env, jobject obj, jstring pattern, jboolean casefold)
    {
        TVSubtitleData *data = sub_get_data(env, obj);
        jclass clsstring = env->FindClass("java/lang/String");
        jstring strencode = env->NewStringUTF("utf-16");
        jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
        jbyteArray barr = (jbyteArray)env->CallObjectMethod(pattern, mid, strencode);
        jsize alen = env->GetArrayLength(barr);
        jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
        char *buf;

        buf = (char *)malloc(alen + 1);
        if (buf) {
            memcpy(buf, ba, alen);
            buf[alen] = 0;

            AM_TT2_SetSearchPattern(data->tt_handle, buf, casefold, AM_FALSE);
        }

        free(buf);
        env->DeleteLocalRef(strencode);
        env->ReleaseByteArrayElements(barr, ba, 0);
        return 0;
    }

    static jint sub_tt_search(JNIEnv *env, jobject obj, jint dir)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        AM_TT2_Search(data->tt_handle, dir);

        return 0;
    }

    //afd callback
    void afd_evt_callback(long dev_no, int event_type, void *param, void *user_data)
    {
        //LOGI("afd_evt_callback");
        int sAfdValue;
        char afdValue[10];
        AM_USERDATA_AFD_t *afd = (AM_USERDATA_AFD_t *)param;
        sAfdValue = afd->af;
        //LOGI("afd_evt_callback sAfdValue:%d", sAfdValue);
        char height[10];
        sprintf(afdValue, "%d", sAfdValue);
        property_set("vendor.sys.subtitleservice.afdValue", afdValue);
    }

    static jint sub_start_atsc_cc(JNIEnv *env, jobject obj, jint source, jint vfmt, jint caption, jint fg_color,

            jint fg_opacity, jint bg_color, jint bg_opacity, jint font_style, jint font_size)
    {
        TVSubtitleData *data = sub_get_data(env, obj);
        AM_CC_CreatePara_t cc_para;
        AM_CC_StartPara_t spara;
        AM_USERDATA_OpenPara_t para;
        int ret;
        //int afd_dev = 0;
        int mode;

        setDvbDebugLogLevel();

        LOGI("start cc: vfmt %d caption %d, fgc %d, bgc %d, fgo %d, bgo %d, fsize %d, fstyle %d",
                vfmt, caption, fg_color, bg_color, fg_opacity, bg_opacity, font_size, font_style);

        memset(&cc_para, 0, sizeof(cc_para));
        memset(&spara, 0, sizeof(spara));

        memset(&para, 0, sizeof(para));
        para.vfmt = vfmt;
        if (AM_USERDATA_Open(ud_dev_no, &para) != AM_SUCCESS)
        {
             LOGI("Cannot open userdata device %d", ud_dev_no);
             goto error;
        }

        cc_para.bmp_buffer = data->buffer;
        cc_para.pitch = data->bmp_pitch;
        cc_para.draw_begin = cc_draw_begin_cb;
        cc_para.draw_end = cc_draw_end_cb;
        cc_para.user_data = (void *)data;
        cc_para.input = (AM_CC_Input_t)source;
        cc_para.rating_cb = cc_rating_cb;
        cc_para.data_cb = cc_data_cb;
        cc_para.report = cc_report_cb;
        cc_para.data_timeout = 5000;//5s
        cc_para.switch_timeout = 3000;//3s
        spara.vfmt = vfmt;
        spara.caption1                 = (AM_CC_CaptionMode_t)caption;
        spara.caption2                 = AM_CC_CAPTION_NONE;
        cc_para.json_update = json_update_cb;
        cc_para.json_buffer = gJsonStr;

        spara.user_options.bg_color    = (AM_CC_Color_t)bg_color;
        spara.user_options.fg_color    = (AM_CC_Color_t)fg_color;
        spara.user_options.bg_opacity  = (AM_CC_Opacity_t)bg_opacity;
        spara.user_options.fg_opacity  = (AM_CC_Opacity_t)fg_opacity;
        spara.user_options.font_size   = (AM_CC_FontSize_t)font_size;
        spara.user_options.font_style  = (AM_CC_FontStyle_t)font_style;

        ret = AM_CC_Create(&cc_para, &data->cc_handle);
        if (ret != AM_SUCCESS)
            goto error;

        ret = AM_CC_Start(data->cc_handle, &spara);
        if (ret != AM_SUCCESS)
            goto error;
        LOGI("start cc successfully!");

        //add notify afd change
        LOGI("start afd notify change!");
        AM_USERDATA_GetMode(ud_dev_no, &mode);
        AM_USERDATA_SetMode(ud_dev_no, mode | AM_USERDATA_MODE_AFD);
        AM_EVT_Subscribe(ud_dev_no, AM_USERDATA_EVT_AFD, afd_evt_callback, NULL);
        return 0;
error:
        if (data->cc_handle != NULL) {
            AM_CC_Destroy(data->cc_handle);
        }
        LOGI("start cc failed!");
        return -1;
    }

    static jint sub_start_atsc_dtvcc(JNIEnv *env, jobject obj, jint vfmt, jint caption, jint fg_color,
            jint fg_opacity, jint bg_color, jint bg_opacity, jint font_style, jint font_size)
    {
        return sub_start_atsc_cc(env, obj, AM_CC_INPUT_USERDATA, vfmt, caption, fg_color,
                fg_opacity, bg_color, bg_opacity, font_style, font_size);
    }

    static jint sub_start_atsc_atvcc(JNIEnv *env, jobject obj, jint caption, jint fg_color,
            jint fg_opacity, jint bg_color, jint bg_opacity, jint font_style, jint font_size)
    {
        return sub_start_atsc_cc(env, obj, AM_CC_INPUT_VBI, 100, caption, fg_color,
                fg_opacity, bg_color, bg_opacity, font_style, font_size);
    }

    static jint sub_stop_atsc_cc(JNIEnv *env, jobject obj)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        LOGI("stop cc");
        AM_CC_Destroy(data->cc_handle);
        pthread_mutex_lock(&data->lock);
        if (data->obj)
            sub_update(data->obj);
        pthread_mutex_unlock(&data->lock);

        data->cc_handle = NULL;
        AM_USERDATA_Close(ud_dev_no);
        return 0;
    }

    static jint sub_set_atsc_cc_options(JNIEnv *env, jobject obj, jint fg_color,
            jint fg_opacity, jint bg_color, jint bg_opacity, jint font_style, jint font_size)
    {
        TVSubtitleData *data = sub_get_data(env, obj);
        AM_CC_UserOptions_t param;

        setDvbDebugLogLevel();

        memset(&param, 0, sizeof(AM_CC_UserOptions_t));
        param.bg_color    = (AM_CC_Color_t)bg_color;
        param.fg_color    = (AM_CC_Color_t)fg_color;
        param.bg_opacity  = (AM_CC_Opacity_t)bg_opacity;
        param.fg_opacity  = (AM_CC_Opacity_t)fg_opacity;
        param.font_size   = (AM_CC_FontSize_t)font_size;
        param.font_style  = (AM_CC_FontStyle_t)font_style;

        AM_CC_SetUserOptions(data->cc_handle, &param);
        return 0;
    }

    static jint sub_set_active(JNIEnv *env, jobject obj, jboolean active)
    {
        TVSubtitleData *data = sub_get_data(env, obj);

        if (active) {
            if (data->obj) {
                pthread_mutex_lock(&data->lock);
                env->DeleteGlobalRef(data->obj);
                data->obj = NULL;
                pthread_mutex_unlock(&data->lock);
            }

            data->obj = env->NewGlobalRef(obj);
        } else {
            if (env->IsSameObject(data->obj, obj)) {
                pthread_mutex_lock(&data->lock);
                env->DeleteGlobalRef(data->obj);
                data->obj = NULL;
                pthread_mutex_unlock(&data->lock);
            }
        }

        return 0;
    }

    static JNINativeMethod gMethods[] = {
        /* name, signature, funcPtr */
        {"native_sub_init", "()I", (void *)sub_init},
        {"native_sub_destroy", "()I", (void *)sub_destroy},
        {"native_sub_lock", "()I", (void *)sub_lock},
        {"native_sub_unlock", "()I", (void *)sub_unlock},
        {"native_sub_clear", "()I", (void *)sub_clear},
        {"native_sub_start_dvb_sub", "(IIII)I", (void *)sub_start_dvb_sub},
        {"native_sub_start_dtv_tt", "(IIIIIZ)I", (void *)sub_start_dtv_tt},
        {"native_sub_stop_dvb_sub", "()I", (void *)sub_stop_dvb_sub},
        {"native_sub_stop_dtv_tt", "()I", (void *)sub_stop_dtv_tt},
        {"native_sub_start_scte27", "(II)I", (void *)sub_start_scte27},
        {"native_sub_stop_scte27", "()I", (void *)sub_stop_scte27},
        {"native_sub_tt_goto", "(I)I", (void *)sub_tt_goto},
        {"native_sub_tt_color_link", "(I)I", (void *)sub_tt_color_link},
        {"native_sub_tt_home_link", "()I", (void *)sub_tt_home_link},
        {"native_sub_tt_next", "(I)I", (void *)sub_tt_next},
        {"native_sub_tt_set_search_pattern", "(Ljava/lang/String;Z)I", (void *)sub_tt_set_search_pattern},
        {"native_sub_tt_search_next", "(I)I", (void *)sub_tt_search},
        {"native_get_subtitle_picture_width", "()I", (void *)get_subtitle_piture_width},
        {"native_get_subtitle_picture_height", "()I", (void *)get_subtitle_piture_height},
        {"native_sub_start_atsc_cc", "(IIIIIIII)I", (void *)sub_start_atsc_dtvcc},
        {"native_sub_start_atsc_atvcc", "(IIIIIII)I", (void *)sub_start_atsc_atvcc},
        {"native_sub_stop_atsc_cc", "()I", (void *)sub_stop_atsc_cc},
        {"native_sub_set_atsc_cc_options", "(IIIIII)I", (void *)sub_set_atsc_cc_options},
        {"native_sub_set_active", "(Z)I", (void *)sub_set_active}
    };

    JNIEXPORT jint
        JNI_OnLoad(JavaVM *vm, void *reserved)
        {
            JNIEnv *env = NULL;
            jclass clazz;
            int rc;

            gJavaVM = vm;

            if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
                LOGE("GetEnv failed");
                return -1;
            }

            clazz = env->FindClass("com/tv/DTVSubtitleView");
            if (clazz == NULL) {
                LOGE("FindClass com/tv/DTVSubtitleView failed");
                return -1;
            }

            if ((rc = (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])))) < 0) {
                LOGE("RegisterNatives failed");
                return -1;
            }


            gUpdateID = env->GetMethodID(clazz, "update", "()V");

            gPassJsonStr = env->GetMethodID(clazz, "saveJsonStr", "(Ljava/lang/String;)V");

            gPassLanguage = env->GetMethodID(clazz, "saveLanguage", "(Ljava/lang/String;)V");

            gBitmapID = env->GetStaticFieldID(clazz, "bitmap", "Landroid/graphics/Bitmap;");

            gUpdateDataID = env->GetMethodID(clazz, "updateData", "(Ljava/lang/String;)V");

            gReportErrorID = env->GetMethodID(clazz, "reportError", "(I)V");

            gReportAvailableID = env->GetMethodID(clazz, "reportAvailable", "()V");
            LOGI("load tvsubtitle_tv ok");
            return JNI_VERSION_1_4;
        }

    JNIEXPORT void
        JNI_OnUnload(JavaVM *vm, void *reserved)
        {
            JNIEnv *env = NULL;
            int i;

            if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
                return;
            }
        }

} /*extern "C"*/

