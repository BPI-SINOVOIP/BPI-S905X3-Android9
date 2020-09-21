/**
 * @file 		iptv_player_jni.cpp
 * @author    	zhouyj
 * @date      	2012/9/5
 * @version   	ver 1.0
 * @brief     	定义CTC_MediaProcessor类中方法的jni接口，供上层调用。
 * @attention
*/
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"
#include "Proxy_MediaProcessor.h"
#include <utils/Log.h>
#include <utils/Mutex.h>
#if ANDROID_PLATFORM_SDK_VERSION <= 27
#include "player.h"
#include "player_type.h"
#include "vformat.h"
#include "aformat.h"
#include <gui/Surface.h>
#endif
using namespace android;

#include "android/log.h"
#include "jni.h"
#include "stdio.h"
#include <android/bitmap.h>
#include <string.h>
#include <gui/Surface.h>
#include "../media_processor/CTC_Utils.h"
#include <media/stagefright/SurfaceUtils.h>

#define  LOG_TAG    "CTC_MediaControl"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define BUFF_SIZE (32*1024)

Mutex    gMutexLock;

static FILE * g_TestResultLogHandle = NULL;

static CTC_MediaInfo g_mediaInfo;
static int audio_index = 0;
static int audio_count = 0;

static FILE * GetTestResultLogHandle()
{
    if (g_TestResultLogHandle == NULL) {
        g_TestResultLogHandle = fopen("/data/data/com.ctc/mediaTestSuite.txt", "wb");
        if (g_TestResultLogHandle == NULL) {
            LOGE("create file :error");
            return NULL;
        } else {
            char writebuf[2000];
            unsigned int buflen = 2000;
            memset(writebuf,0,buflen);
            snprintf(writebuf,buflen,"%s\r\n%s\r\n","ctc_player test result:","*********************");
            writebuf[buflen -1] = 0;
            fwrite(writebuf,1,strlen(writebuf),g_TestResultLogHandle);
            fflush(g_TestResultLogHandle);
        }
    }

    return g_TestResultLogHandle;
}

#ifdef __cplusplus
extern "C" {
#endif

Proxy_MediaProcessor* proxy_mediaProcessor[2] ={NULL, NULL};
FILE *fp[2];
int g_quit = 0;

char* m_pcBuf[2] = {NULL};
int isPause = 0;
#define WRITE_DATA_SIZE (32*1024)
void Java_com_ctc_MediaProcessorDemoActivity_nativeWriteFile(JNIEnv* env, jobject thiz, jstring Function, jstring Return, jstring Result)
{
    FILE *result_fp = GetTestResultLogHandle();

    if (result_fp == NULL) {
        LOGE("create file :error");
        return;
    }
    const char* Function_t = (*env).GetStringUTFChars(Function, NULL);
    const char* Return_t = (*env).GetStringUTFChars(Return, NULL);
    const char* Result_t = (*env).GetStringUTFChars(Result, NULL);
    char *divide_str = "*********************";
    char  writebuf[2000];
    unsigned int buflen = 2000;
    memset(writebuf,0,buflen);
    snprintf(writebuf,buflen,"%s\r\n%s\r\n%s\r\n%s\r\n",Function_t, Return_t, Result_t, divide_str);
    writebuf[buflen -1] = 0;
    fwrite(writebuf,1,strlen(writebuf),result_fp);
    fflush(result_fp);
    return;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeCreateSurface(JNIEnv* env, jobject thiz, jobject pSurface, int w, int h, int use_omx_decoder)
{
    LOGI("get the surface");
    sp<Surface> surface(android_view_Surface_getSurface(env, pSurface));
    LOGI("success: get surface:%p", surface.get());
    status_t err = OK;
    if ((err = nativeWindowConnect(surface.get(), "setVideoSurfaceTexture")) != OK) {
        ALOGE("nativeWindowConnect failed: %d", err);
    }
    proxy_mediaProcessor[use_omx_decoder]->Proxy_SetSurface(surface.get());
    //proxy_mediaProcessor[use_omx_decoder]->Proxy_SetEPGSize(w, h);

    return 0;
}

void Java_com_ctc_MediaProcessorDemoActivity_nativeSetEPGSize(JNIEnv* env, jobject thiz, int w, int h, int use_omx_decoder)
{
    proxy_mediaProcessor[use_omx_decoder]->Proxy_SetEPGSize(w, h);
    return;
}

static void signal_handler(int signum)
{
    ALOGI("Get signum=%x",signum);
#if ANDROID_PLATFORM_SDK_VERSION <= 27
    player_progress_exit();
#endif
    signal(signum, SIG_DFL);
    raise (signum);
}

#if 0
int _media_info_dump(media_info_t* minfo)
{
    int i = 0;
    ALOGI("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    ALOGI("======||file size:%lld\n",minfo->stream_info.file_size);
    ALOGI("======||file type:%d\n",minfo->stream_info.type);
    ALOGI("======||has internal subtitle?:%s\n",minfo->stream_info.has_sub>0?"YES!":"NO!");
    ALOGI("======||internal subtile counts:%d\n",minfo->stream_info.total_sub_num);
    ALOGI("======||has video track?:%s\n",minfo->stream_info.has_video>0?"YES!":"NO!");
    ALOGI("======||has audio track?:%s\n",minfo->stream_info.has_audio>0?"YES!":"NO!");
    ALOGI("======||duration:%d\n",minfo->stream_info.duration);
    if (minfo->stream_info.has_video &&minfo->stream_info.total_video_num > 0) {
        ALOGI("======||video counts:%d\n",minfo->stream_info.total_video_num);
        ALOGI("======||video width:%d\n",minfo->video_info[0]->width);
        ALOGI("======||video height:%d\n",minfo->video_info[0]->height);
        ALOGI("======||video bitrate:%d\n",minfo->video_info[0]->bit_rate);
        ALOGI("======||video format:%d\n",minfo->video_info[0]->format);
    }

    if (minfo->stream_info.has_audio && minfo->stream_info.total_audio_num> 0) {
        ALOGI("======||audio counts:%d\n",minfo->stream_info.total_audio_num);
        if (NULL !=minfo->audio_info[0]->audio_tag) {
            ALOGI("======||track title:%s",minfo->audio_info[0]->audio_tag->title!=NULL?minfo->audio_info[0]->audio_tag->title:"unknow");
            ALOGI("\n======||track album:%s",minfo->audio_info[0]->audio_tag->album!=NULL?minfo->audio_info[0]->audio_tag->album:"unknow");
            ALOGI("\n======||track author:%s\n",minfo->audio_info[0]->audio_tag->author!=NULL?minfo->audio_info[0]->audio_tag->author:"unknow");
            ALOGI("\n======||track year:%s\n",minfo->audio_info[0]->audio_tag->year!=NULL?minfo->audio_info[0]->audio_tag->year:"unknow");
            ALOGI("\n======||track comment:%s\n",minfo->audio_info[0]->audio_tag->comment!=NULL?minfo->audio_info[0]->audio_tag->comment:"unknow");
            ALOGI("\n======||track genre:%s\n",minfo->audio_info[0]->audio_tag->genre!=NULL?minfo->audio_info[0]->audio_tag->genre:"unknow");
            ALOGI("\n======||track copyright:%s\n",minfo->audio_info[0]->audio_tag->copyright!=NULL?minfo->audio_info[0]->audio_tag->copyright:"unknow");
            ALOGI("\n======||track track:%d\n",minfo->audio_info[0]->audio_tag->track);
        }

        for (i = 0;i<minfo->stream_info.total_audio_num;i++) {
            ALOGI("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            ALOGI("======||%d 'st audio track codec type:%d\n",i,minfo->audio_info[i]->aformat);
            ALOGI("======||%d 'st audio track audio_channel:%d\n",i,minfo->audio_info[i]->channel);
            ALOGI("======||%d 'st audio track bit_rate:%d\n",i,minfo->audio_info[i]->bit_rate);
            ALOGI("======||%d 'st audio track audio_samplerate:%d\n",i,minfo->audio_info[i]->sample_rate);
            ALOGI("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
        }
    }

    if (minfo->stream_info.has_sub &&minfo->stream_info.total_sub_num>0) {
        for (i = 0;i < minfo->stream_info.total_sub_num;i++) {
            if (0 == minfo->sub_info[i]->internal_external) {
                ALOGI("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
                ALOGI("======||%d 'st internal subtitle pid:%d\n",i,minfo->sub_info[i]->id);
#if ANDROID_PLATFORM_SDK_VERSION <= 27
                ALOGI("======||%d 'st internal subtitle language:%s\n",i,minfo->sub_info[i]->sub_language?minfo->sub_info[i]->sub_language:"unknow");
#endif
                ALOGI("======||%d 'st internal subtitle width:%d\n",i,minfo->sub_info[i]->width);
                ALOGI("======||%d 'st internal subtitle height:%d\n",i,minfo->sub_info[i]->height);
                ALOGI("======||%d 'st internal subtitle resolution:%d\n",i,minfo->sub_info[i]->resolution);
                ALOGI("======||%d 'st internal subtitle subtitle size:%lld\n",i,minfo->sub_info[i]->subtitle_size);
                ALOGI("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            }
        }
    }
    ALOGI("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    return 0;
}
#endif

void test_player_evt_func(IPTV_PLAYER_EVT_E evt, void *handler, uint32_t param1, uint32_t param2)
{
    switch (evt) {
        case 5: ALOGI("evt:VIDEO_BUFFSIZE\n");break;
        case 6: ALOGI("evt:VIDEO_BUFF_USED\n");break;
        case 7: ALOGI("evt:AUDIO_BUFFSIZE\n");break;
        case 8: ALOGI("evt:AUDIO_BUFF_USED \n");break;
        case 9: ALOGI("evt:VIDEO_RATIO\n");break;
        case 10:ALOGI("evt:VIDEO_W_H\n");break;
        case 11:ALOGI("evt:VIDEO_F_F_MODE\n");break;
        case 12:ALOGI("evt:AUDIO_SAMPLE_RATE\n");break;
        case 13:ALOGI("evt:AUDIO_CUR_BITRATE\n");break;
        case 14:ALOGI("evt:VIDEO_PTS_ERROR\n");break;
        case 15:ALOGI("evt:AUDIO_PTS_ERROR\n");break;
        case 16:ALOGI("evt:VDEC_ERROR\n");break;
        case 17:ALOGI("evt:ADEC_ERROR\n");break;
        case 18:ALOGI("evt:UNDERFLOW\n");break;
        case 19:ALOGI("evt:ADEC_UNDERFLOW\n");break;
        default:ALOGI("evt: %d\n",evt);break;
    }
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeInit(JNIEnv* env, jobject thiz, jstring url, int use_omx_decoder)
{
    g_quit = 0;

    const char* URL = (*env).GetStringUTFChars(url, NULL);
    ALOGI("native_init URL:%s", URL);

    if (getMediaInfo(URL, &g_mediaInfo) < 0) {
        ALOGE("getMediaInfo faild!");
        return -1;
    }

    audio_count = g_mediaInfo.aParaCount;

    proxy_mediaProcessor[use_omx_decoder]=new Proxy_MediaProcessor(use_omx_decoder);
    proxy_mediaProcessor[use_omx_decoder]->Proxy_InitVideo(&g_mediaInfo.vPara);
    proxy_mediaProcessor[use_omx_decoder]->Proxy_InitAudio(g_mediaInfo.aParas + audio_index);
    proxy_mediaProcessor[use_omx_decoder]->Proxy_InitSubtitle(&g_mediaInfo.sParas[0]);
    proxy_mediaProcessor[use_omx_decoder]->Proxy_playerback_register_evt_cb(test_player_evt_func, NULL);

    if (m_pcBuf[0] == NULL) {
        m_pcBuf[0] = (char* )malloc(WRITE_DATA_SIZE);
    }

    if (m_pcBuf[1] == NULL) {
        m_pcBuf[1] = (char* )malloc(WRITE_DATA_SIZE);
    }

    return 0;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStartPlay(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_StartPlay();
    return result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeWriteData(JNIEnv* env, jobject thiz, jstring url, jint bufsize, int use_omx_decoder)
{
    const char* URL = (*env).GetStringUTFChars(url, NULL);

    int rd_result = 0;
    LOGI("Java_com_ctc_MediaProcessorDemoActivity_nativeWriteData : ur:%s\n",URL);
    fp[use_omx_decoder] = fopen(URL, "rb");
    if (fp[use_omx_decoder] == NULL) {
        LOGE("open file:error! %d, %s", errno, strerror(errno));
        return -1;
    }

    while (!g_quit) {
        Mutex::Autolock l(gMutexLock);
        /*bufsize = bufsize*1024;
        if (bufsize < BUFF_SIZE)
            bufsize = BUFF_SIZE;
        char* buffer = (char* )malloc(bufsize);
        rd_result = fread(buffer, bufsize, 1, fp);*/
        bufsize = WRITE_DATA_SIZE;
        rd_result = fread(m_pcBuf[use_omx_decoder], WRITE_DATA_SIZE, 1, fp[use_omx_decoder]);
        if (rd_result <= 0) {
            LOGE("read the end of file");
            return 0;
        }

        while (bufsize > 0) {
            int wd_result = 0;
            if (proxy_mediaProcessor[use_omx_decoder] != nullptr) {
                wd_result = proxy_mediaProcessor[use_omx_decoder]->Proxy_WriteData((unsigned char*) m_pcBuf[use_omx_decoder], (unsigned int) bufsize);
            }

            if (wd_result < 0) {
                usleep(60*1000);
                continue;
            } else if (wd_result == 0) {
                break;
            }
            bufsize = bufsize - wd_result;
        }
    }
    LOGI("nativeWriteData end!");
    return 0;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetPlayMode(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    int result = proxy_mediaProcessor[use_omx_decoder]->Proxy_GetPlayMode();
    LOGE("step:1");

    return (jint)result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeSetVideoWindow(JNIEnv* env, jobject thiz ,jint x, jint y, jint width, jint height, int use_omx_decoder)
{
    int result = proxy_mediaProcessor[use_omx_decoder]->Proxy_SetVideoWindow(x, y, width, height);
    LOGE("SetVideoWindow result:[%d]", result);
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativePause(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    LOGE("NEXT:Pause");
    isPause = 1;
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_Pause();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeResume(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    LOGE("NEXT:Resume");
    isPause = 0;
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_Resume();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSeek(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    LOGE("nativeSeek---0");
    Mutex::Autolock l(gMutexLock);
    LOGE("nativeSeek---1");
    fseek(fp[use_omx_decoder], 0, 0);
    LOGE("nativeSeek---2");
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_Seek();
    LOGE("nativeSeek---3");
    return result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeVideoShow(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jint result = proxy_mediaProcessor[use_omx_decoder]->Proxy_VideoShow();
    return result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeVideoHide(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jint result = proxy_mediaProcessor[use_omx_decoder]->Proxy_VideoHide();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeFast(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_Fast();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStopFast(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_StopFast();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeStop(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    LOGI("native stop: %d", use_omx_decoder);

    g_quit = 1;
    usleep(500*1000);

    ::fclose(fp[use_omx_decoder]);
    fp[use_omx_decoder] = NULL;

    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_Stop();
    if (m_pcBuf[0] != NULL) {
        free(m_pcBuf[0]);
        m_pcBuf[0] = NULL;
    }
    if (m_pcBuf[1] != NULL) {
        free(m_pcBuf[1]);
        m_pcBuf[1] = NULL;
    }
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeDelete(JNIEnv* env, jobject thiz)
{
    LOGI("native delete!");

    jboolean result = true;

    for (size_t i = 0; i < sizeof(proxy_mediaProcessor)/sizeof(*proxy_mediaProcessor); ++i) {
        auto p = proxy_mediaProcessor[i];
        if (p) {
            delete p;
        }
    }

    return result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetVolume(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jint result = proxy_mediaProcessor[use_omx_decoder]->Proxy_GetVolume();
    LOGE("the volume is [%d]",result);
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetVolume(JNIEnv* env, jobject thiz,jint volume, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_SetVolume(volume);
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetRatio(JNIEnv* env, jobject thiz,jint nRatio, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_SetRatio(nRatio);
    return result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetAudioBalance(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jint result = proxy_mediaProcessor[use_omx_decoder]->Proxy_GetAudioBalance();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSetAudioBalance(JNIEnv* env, jobject thiz, jint nAudioBalance, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_SetAudioBalance(nAudioBalance);
    return result;
}

void Java_com_ctc_MediaProcessorDemoActivity_nativeGetVideoPixels(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    int width;
    int height;
    LOGE("the video prixels ");
    proxy_mediaProcessor[use_omx_decoder]->Proxy_GetVideoPixels(width, height);
    LOGE("the video prixels :[%d]*[%d]",width, height);
    return;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeIsSoftFit(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jboolean result = proxy_mediaProcessor[use_omx_decoder]->Proxy_IsSoftFit();
    return result;
}

jint Java_com_ctc_MediaProcessorDemoActivity_nativeGetCurrentPlayTime(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    jint result = proxy_mediaProcessor[use_omx_decoder]->Proxy_GetCurrentPlayTime();
    return result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSwitchAudioTrack(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    if ((audio_count > 0) && (audio_count < MAX_AUDIO_PARAM_SIZE)) {
        int pid = 0;
        audio_index++;

        if (audio_index >= audio_count)
            audio_index = 0;
        pid = g_mediaInfo.aParas[audio_index].pid;
        LOGI("Auido index: %d, count: %d, pid: %d\n", audio_index, audio_count, pid);
        proxy_mediaProcessor[use_omx_decoder]->Proxy_SwitchAudioTrack(pid);
        return true;
    } else {
        return false;
    }
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeInitSubtitle(JNIEnv* env, jobject thiz, int use_omx_decoder)
{
    //jboolean result = proxy_mediaProcessor->Proxy_InitSubtitle();
    return true;//result;
}

jboolean Java_com_ctc_MediaProcessorDemoActivity_nativeSwitchSubtitle(JNIEnv* env, jobject thiz, jint sub_pid, int use_omx_decoder)
{
    proxy_mediaProcessor[use_omx_decoder]->Proxy_SwitchSubtitle(sub_pid);
    return true;
}

#ifdef __cplusplus
}
#endif
