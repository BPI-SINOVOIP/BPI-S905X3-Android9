/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C file
 */

#ifdef SUPPORT_ADTV
#include <am_epg.h>
#endif
#include <jni.h>
#include <android/log.h>

#include <cutils/properties.h>

#define LOG_TAG "jnidtvepg"
#define log_info(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define log_error(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* EPG notify events*/
#define EVENT_PF_EIT_END             1
#define EVENT_SCH_EIT_END            2
#define EVENT_PMT_END                3
#define EVENT_SDT_END                4
#define EVENT_TDT_END                5
#define EVENT_NIT_END                6
#define EVENT_PROGRAM_AV_UPDATE      7
#define EVENT_PROGRAM_NAME_UPDATE    8
#define EVENT_PROGRAM_EVENTS_UPDATE  9
#define EVENT_TS_UPDATE              10
#define EVENT_EIT_CHANGED            11
#define EVENT_PMT_RATING             12

static JavaVM   *gJavaVM = NULL;
static jclass    gEventClass;
static jmethodID gEventInitID;
static jmethodID gOnEventID;
static jfieldID  gHandleID;
static jclass    gEvtClass;
static jmethodID gEvtInitID;
static jclass    gChannelClass;
static jmethodID gChannelInitID;
static jclass    gServiceInfosFromSDTClass;
static jmethodID gServiceInfosFromSDTInitID;
static jclass    gServiceInfoFromSDTClass;
static jmethodID gServiceInfoFromSDTInitID;

#ifndef SUPPORT_ADTV
typedef void* AM_EPG_Handle_t;
#endif

typedef struct{
    int dmx_id;
    int fend_id;
    AM_EPG_Handle_t handle;
    jobject obj;
}EPGData;
// dvb version array len
#define MAX_DVBVERSION_COUNT (128)
typedef struct {
    int type;
    int channelID;
    int programID;
    int dvbOrigNetID;
    int dvbTSID;
    int dvbServiceID;
    long time;
    int dvbVersion[MAX_DVBVERSION_COUNT];
    int eitNumber;
    char pmt_rating[1024];
}EPGEventData;

struct sdt_service;
typedef struct sdt_service sdt_service_t;

#define MAX_LEN_MULTINAME ((64+4)*4 + 1)
#define SPLIT_MULTINAME     {0x80}
#define LEN_SPLIT_MULTINAME 1

typedef struct {
    int valid;
    char name[MAX_LEN_MULTINAME];
    int mOriginalNetworkId;
    int mTransportStreamId;
    int mServiceId;
    int mType;
    int mServiceType;
    int mFrequency;
    int mBandwidth;
    int mModulation;
    int mSymbolRate;
    int mFEMisc;
    int mVideoPID;
    int mVideoFormat;
#ifdef SUPPORT_ADTV
    AM_SI_AudioInfo_t mAudioInfo;
    AM_SI_SubtitleInfo_t mSubtitleInfo;
    AM_SI_TeletextInfo_t mTeletextInfo;
    AM_SI_CaptionInfo_t mCcapInfo;
    AM_SI_IsdbsubtitleInfo_t mIsdbInfo;
    AM_SI_Scte27SubtitleInfo_t mScte27Info;
#endif
    int mPcrPID;
    int mSdtVersion;
    sdt_service_t *services;
    int mEitVersions[128];
    int mProgramsInPat;
    int mPatTsId;
}EPGChannelData;

struct sdt_service{
    int id;
    int type;
    char name[MAX_LEN_MULTINAME];
    int running;
    int free_ca;
    sdt_service_t *next;
};

#ifdef SUPPORT_ADTV
EPGChannelData gChannelMonitored = {.valid = 0};
static jbyteArray get_byte_array(JNIEnv* env, const char *str);
static jintArray get_int_array(JNIEnv* env, const int *str, int len);

static int epg_conf_get(char *prop, int def) {
    return property_get_int32(prop, def);
}

static void setDvbDebugLoglevel() {
    AM_DebugSetLogLevel(property_get_int32("tv.dvb.loglevel", 1));
}

static void epg_on_event(jobject obj, EPGEventData *evt_data)
{
    JNIEnv *env;
    int ret;
    int attached = 0;

    ret = (*gJavaVM)->GetEnv(gJavaVM, (void**) &env, JNI_VERSION_1_4);
    if (ret <0) {
        ret = (*gJavaVM)->AttachCurrentThread(gJavaVM,&env,NULL);
        if (ret <0) {
            log_error("callback handler:failed to attach current thread");
            return;
        }
        attached = 1;
    }
    jobject event = (*env)->NewObject(env, gEventClass, gEventInitID, obj, evt_data->type);
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "channelID", "I"), evt_data->channelID);
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "programID", "I"), evt_data->programID);
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "dvbOrigNetID", "I"), evt_data->dvbOrigNetID);
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "dvbTSID", "I"), evt_data->dvbTSID);
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "dvbServiceID", "I"), evt_data->dvbServiceID);
    (*env)->SetLongField(env,event,(*env)->GetFieldID(env, gEventClass, "time", "J"), evt_data->time);
    (*env)->SetObjectField(env,event,(*env)->GetFieldID(env, gEventClass, "dvbVersion", "[I"), get_int_array(env, evt_data->dvbVersion, MAX_DVBVERSION_COUNT));
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "eitNumber", "I"), evt_data->eitNumber);
    (*env)->SetIntField(env,event,(*env)->GetFieldID(env, gEventClass, "eitNumber", "I"), evt_data->eitNumber);
    (*env)->SetObjectField(env,event, (*env)->GetFieldID(env, gEventClass, "pmt_rrt_ratings", "[B"), get_byte_array(env, evt_data->pmt_rating));
    if (evt_data->type == 10)
        log_info("epg_on_event event==EVENT_TS_UPDATE");

    (*env)->CallVoidMethod(env, obj, gOnEventID, event);

    if (attached) {
        (*gJavaVM)->DetachCurrentThread(gJavaVM);
    }
}

static void epg_evt_callback(long dev_no, int event_type, void *param, void *user_data)
{
    EPGData *priv_data;
    EPGEventData edata;

    log_info("evt callback %d\n", event_type);

    AM_EPG_GetUserData((AM_EPG_Handle_t)dev_no, (void**)&priv_data);
    if (!priv_data)
        return;
    memset(&edata, 0, sizeof(edata));
    switch (event_type) {
        case AM_EPG_EVT_NEW_NIT:
            log_info(".....................AM_EPG_EVT_NEW_NIT.................%d\n",(int)(long)param);
            edata.type = EVENT_NIT_END;
            edata.dvbVersion[0] =(int)(long)param;
            epg_on_event(priv_data->obj, &edata);
            break;
        case AM_EPG_EVT_NEW_TDT:
        case AM_EPG_EVT_NEW_STT:
        {
            int utc_time;

            AM_EPG_GetUTCTime(&utc_time);
            edata.type = EVENT_TDT_END;
            edata.time = (long)utc_time;
            epg_on_event(priv_data->obj, &edata);
        }
            break;
        case AM_EPG_EVT_UPDATE_EVENTS:
            edata.type = EVENT_PROGRAM_EVENTS_UPDATE;
            edata.programID = (int)(long)param;
            epg_on_event(priv_data->obj, &edata);
            break;
        case AM_EPG_EVT_UPDATE_PROGRAM_AV:
            edata.type = EVENT_PROGRAM_AV_UPDATE;
            edata.programID = (int)(long)param;
            epg_on_event(priv_data->obj, &edata);
            break;
        case AM_EPG_EVT_UPDATE_PROGRAM_NAME:
            edata.type = EVENT_PROGRAM_NAME_UPDATE;
            edata.programID = (int)(long)param;
            epg_on_event(priv_data->obj, &edata);
            break;
        case AM_EPG_EVT_UPDATE_TS:
            edata.type = EVENT_TS_UPDATE;
            edata.channelID = (int)(long)param;
            epg_on_event(priv_data->obj, &edata);
            break;
        case EVENT_EIT_CHANGED:
            edata.type = EVENT_EIT_CHANGED;
            edata.eitNumber = (int)(long)param;
            // cp 128 byte
            {
                int k = 0;
                for (k = 0; k < MAX_DVBVERSION_COUNT; k++) {
                    edata.dvbVersion[k] = ((int *)(long)user_data)[k];
                    log_info("-evt eit version callback %d\n", edata.dvbVersion[k]);
                }
            }
            epg_on_event(priv_data->obj, &edata);
            break;
        case AM_EPG_EVT_PMT_RATING:
            //send pmt rating info
            log_info("evt PMT RATING callback %d\n", AM_EPG_EVT_PMT_RATING);
            edata.type = EVENT_PMT_RATING;
            edata.dvbServiceID = ((AM_EPG_PmtRating_t *)(long)param)->i_program_number;
            memcpy(edata.pmt_rating, ((AM_EPG_PmtRating_t *)(long)param)->rating, sizeof(edata.pmt_rating));
            epg_on_event(priv_data->obj, &edata);
            break;
        default:
            break;
    }
}

static jbyteArray get_byte_array(JNIEnv* env, const char *str)
{
    if (!str || !str[0])
        return NULL;

    int len = strlen(str);
    jbyteArray byteArray = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, byteArray, 0, len, (const jbyte*)str);

    return byteArray;
}


static jintArray get_int_array(JNIEnv* env, const int *array, int len)
{
    if (!array)
        return NULL;
    jintArray intArray = (*env)->NewIntArray(env, len);
    int i = 0;
    int *pa_tmp = malloc(len * sizeof(int));
    for (i = 0; i < len; i++)
        pa_tmp[i] = array[i];
    (*env)->SetIntArrayRegion(env, intArray, 0, len, pa_tmp);
    free(pa_tmp);
    return intArray;
}

static bool is_utf8(const void* pBuffer, long size)
{
    bool IsUTF8 = true;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;
    while (start < end)
    {
       if (*start < 0x80) // (10000000): value less then 0x80 ASCII char
       {
           start++;
       }
       else if (*start < (0xC0)) // (11000000): between 0x80 and 0xC0 UTF-8 char
       {
           IsUTF8 = false;
           break;
       }
       else if (*start < (0xE0)) // (11100000): 2 bytes UTF-8 char
       {
           if (start >= end - 1)
               break;
           if ((start[1] & (0xC0)) != 0x80)
           {
               IsUTF8 = false;
               break;
           }
           start += 2;
       }
       else if (*start < (0xF0)) // (11110000): 3 bytes UTF-8 char
       {
           if (start >= end - 2)
               break;
           if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
           {
               IsUTF8 = false;
               break;
           }
           start += 3;
       }
       else
       {
           IsUTF8 = false;
           break;
       }
    }
    return IsUTF8;
}

void Events_Update(AM_EPG_Handle_t handle, int event_count, AM_EPG_Event_t *pevents)
{
    int i;
    AM_EPG_Event_t *pevt;
    JNIEnv *env;
    int ret;
    int attached = 0;
    EPGData *priv_data;

    AM_EPG_GetUserData(handle, (void**)&priv_data);
    if (!priv_data)
        return;

    if (!event_count)
        return;

    ret = (*gJavaVM)->GetEnv(gJavaVM, (void**) &env, JNI_VERSION_1_4);
    if (ret <0) {
        ret = (*gJavaVM)->AttachCurrentThread(gJavaVM,&env,NULL);
        if (ret <0) {
            log_error("callback handler:failed to attach current thread");
            return;
        }
        attached = 1;
    }

    jobject event = (*env)->NewObject(env, gEventClass, gEventInitID, priv_data->obj, EVENT_PROGRAM_EVENTS_UPDATE);
    jobjectArray EvtsArray = (*env)->NewObjectArray(env, event_count, gEvtClass, 0);

    for (i = 0; i < event_count; i++) {
        pevt = &pevents[i];

        jobject evt = (*env)->NewObject(env, gEvtClass, gEvtInitID, event, 0);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "src", "I"), pevt->src);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "srv_id", "I"), pevt->srv_id);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "ts_id", "I"), pevt->ts_id);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "net_id", "I"), pevt->net_id);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "evt_id", "I"), pevt->evt_id);
        (*env)->SetLongField(env,evt,(*env)->GetFieldID(env, gEvtClass, "start", "J"), (jlong)pevt->start);
        (*env)->SetLongField(env,evt,(*env)->GetFieldID(env, gEvtClass, "end", "J"), (jlong)pevt->end);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "nibble", "I"), pevt->nibble);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "parental_rating", "I"), pevt->parental_rating);

        (*env)->SetObjectField(env,evt, (*env)->GetFieldID(env, gEvtClass, "name", "[B"), get_byte_array(env, pevt->name));
        (*env)->SetObjectField(env,evt, (*env)->GetFieldID(env, gEvtClass, "desc", "[B"), get_byte_array(env, pevt->desc));
        (*env)->SetObjectField(env,evt, (*env)->GetFieldID(env, gEvtClass, "ext_item", "[B"), get_byte_array(env, pevt->ext_item));
        (*env)->SetObjectField(env,evt, (*env)->GetFieldID(env, gEvtClass, "ext_descr", "[B"), get_byte_array(env, pevt->ext_descr));
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "sub_flag", "I"), pevt->sub_flag);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "sub_status", "I"), pevt->sub_status);
        (*env)->SetIntField(env,evt,(*env)->GetFieldID(env, gEvtClass, "source_id", "I"), pevt->source_id);
        (*env)->SetObjectField(env,evt, (*env)->GetFieldID(env, gEvtClass, "rrt_ratings", "[B"), get_byte_array(env, pevt->rrt_ratings));

        (*env)->SetObjectArrayElement(env, EvtsArray, i, evt);

    }

    (*env)->SetObjectField(env,event,(*env)->GetFieldID(env, gEventClass, "evts", "[Lcom/droidlogic/tvinput/services/DTVEpgScanner$Event$Evt;"), EvtsArray);

    (*env)->CallVoidMethod(env, priv_data->obj, gOnEventID, event);

    if (attached) {
        (*gJavaVM)->DetachCurrentThread(gJavaVM);
    }
    AM_EPG_FreeEvents(event_count, pevents);
}

static void format_audio_strings(AM_SI_AudioInfo_t *ai, char *pids, char *fmts, char *langs)
{
    int i;

    if (ai->audio_count < 0)
        ai->audio_count = 0;

    pids[0] = 0;
    fmts[0] = 0;
    langs[0] = 0;
    for (i=0; i<ai->audio_count; i++)
    {
        if (i == 0)
        {
            snprintf(pids, 256, "%d", ai->audios[i].pid);
            snprintf(fmts, 256, "%d", ai->audios[i].fmt);
            sprintf(langs, "%s", ai->audios[i].lang);
        }
        else
        {
            snprintf(pids, 256, "%s %d", pids, ai->audios[i].pid);
            snprintf(fmts, 256, "%s %d", fmts, ai->audios[i].fmt);
            snprintf(langs, 256, "%s %s", langs, ai->audios[i].lang);
        }
    }
}

#define gen_type_n_string(array, item, fmt, n, string, n_s) do { \
        int i; char tmp[32]; \
        (string)[0] = 0; \
        for (i=0; i<(n); i++) \
            snprintf(string, n_s, "%s " fmt, string, (array)[i].item); \
        }while(0)

#define gen_audio_4strings(array, n, strings, n_s) do { \
        gen_type_n_string(array,pid,"%d",n,(strings)[0],n_s); \
        gen_type_n_string(array,fmt,"%d",n,(strings)[1],n_s); \
        gen_type_n_string(array,lang,"%s",n,(strings)[2],n_s); \
        gen_type_n_string(array,audio_exten,"%d",n,(strings)[3],n_s); \
}while(0)
#define gen_scte27_1strings(array, n, strings, n_s) do { \
        gen_type_n_string(array,pid,"%d",n,(strings)[0],n_s); \
}while(0)

#define gen_isdb_3strings(array, n, strings, n_s) do { \
        gen_type_n_string(array,pid,"%d",n,(strings)[0],n_s); \
        gen_type_n_string(array,type,"%d",n,(strings)[1],n_s); \
        gen_type_n_string(array,lang,"%s",n,(strings)[2],n_s); \
}while(0)
#define gen_subtitle_5strings(array, n, strings, n_s) do { \
        gen_type_n_string(array,pid,"%d",n,(strings)[0],n_s); \
        gen_type_n_string(array,type,"%d",n,(strings)[1],n_s); \
        gen_type_n_string(array,comp_page_id,"%d",n,(strings)[2],n_s); \
        gen_type_n_string(array,anci_page_id,"%d",n,(strings)[3],n_s); \
        gen_type_n_string(array,lang,"%s",n,(strings)[4],n_s); \
        }while(0)
#define gen_teletext_5strings(array, n, strings, n_s) do { \
        gen_type_n_string(array,pid,"%d",n,(strings)[0],n_s); \
        gen_type_n_string(array,type,"%d",n,(strings)[1],n_s); \
        gen_type_n_string(array,magazine_no,"%d",n,(strings)[2],n_s); \
        gen_type_n_string(array,page_no,"%d",n,(strings)[3],n_s); \
        gen_type_n_string(array,lang,"%s",n,(strings)[4],n_s); \
        }while(0)
#define gen_cc_6strings(array, n, strings, n_s) do { \
        gen_type_n_string(array,service_number,"%d",n,(strings)[0],n_s); \
        gen_type_n_string(array,type,"%d",n,(strings)[1],n_s); \
        gen_type_n_string(array,pid_or_line21,"%d",n,(strings)[2],n_s); \
        gen_type_n_string(array,private_data,"0x%x",n,(strings)[3],n_s); \
        gen_type_n_string(array,flags,"%d",n,(strings)[4],n_s); \
        gen_type_n_string(array,lang,"%s",n,(strings)[5],n_s); \
}while(0)



static int check_pmt_update(EPGChannelData *c1, EPGChannelData *c2)
{
    int ret=0;
    int i, j;

    if (c1->mVideoPID != c2->mVideoPID || c1->mVideoFormat != c2->mVideoFormat) { //check video
        //notify
        ret = 1;
    }

    {
        if (c1->mAudioInfo.audio_count != c2->mAudioInfo.audio_count)
            ret |= 2;
    }
    {//check audio
        for (i=0; i<c1->mAudioInfo.audio_count; i++) {
            for (j=0; j<c2->mAudioInfo.audio_count; j++) {
                if (c1->mAudioInfo.audios[i].pid == c2->mAudioInfo.audios[j].pid &&
                    c1->mAudioInfo.audios[i].fmt == c2->mAudioInfo.audios[j].fmt &&
                    c1->mAudioInfo.audios[i].audio_exten == c2->mAudioInfo.audios[j].audio_exten &&
                    !strncmp(c1->mAudioInfo.audios[i].lang, c2->mAudioInfo.audios[j].lang, 3))
                    break;
            }
            if (j >= c2->mAudioInfo.audio_count) {
                //notify
                ret |= 2;
                break;
            }
        }
    }
    {
        if (c1->mSubtitleInfo.subtitle_count != c2->mSubtitleInfo.subtitle_count)
            ret |= 4;
    }
    {//check subtitle
        for (i=0; i<c1->mSubtitleInfo.subtitle_count; i++) {
            for (j=0; j<c2->mSubtitleInfo.subtitle_count; j++) {
                if (c1->mSubtitleInfo.subtitles[i].pid == c2->mSubtitleInfo.subtitles[j].pid &&
                    c1->mSubtitleInfo.subtitles[i].type == c2->mSubtitleInfo.subtitles[j].type &&
                    c1->mSubtitleInfo.subtitles[i].comp_page_id == c2->mSubtitleInfo.subtitles[j].comp_page_id &&
                    c1->mSubtitleInfo.subtitles[i].anci_page_id == c2->mSubtitleInfo.subtitles[j].anci_page_id &&
                    !strncmp(c1->mSubtitleInfo.subtitles[i].lang, c2->mSubtitleInfo.subtitles[j].lang, 3))
                    break;
            }
            if (j >= c2->mSubtitleInfo.subtitle_count) {
                //notify
                ret |= 4;
                break;
            }
        }
    }
    {
        int sub_txt_cnt = 0;
        for (i=0; i<c2->mTeletextInfo.teletext_count; i++) {
            if ((c2->mTeletextInfo.teletexts[i].type == 0x2)
                || (c2->mTeletextInfo.teletexts[i].type == 0x5))
                sub_txt_cnt++;
        }
        if (c1->mTeletextInfo.teletext_count != sub_txt_cnt)
            ret |= 8;
    }
    {//check teletext
        for (i=0; i<c1->mTeletextInfo.teletext_count; i++) {
            for (j=0; j<c2->mTeletextInfo.teletext_count; j++) {
                //remove the check below when teletext's stored for tvinput.
                if ((c2->mTeletextInfo.teletexts[j].type != 0x2)
                    && (c2->mTeletextInfo.teletexts[j].type != 0x5))
                    continue;
                if (c1->mTeletextInfo.teletexts[i].pid == c2->mTeletextInfo.teletexts[j].pid &&
                    c1->mTeletextInfo.teletexts[i].type == c2->mTeletextInfo.teletexts[j].type &&
                    c1->mTeletextInfo.teletexts[i].magazine_no == c2->mTeletextInfo.teletexts[j].magazine_no &&
                    c1->mTeletextInfo.teletexts[i].page_no == c2->mTeletextInfo.teletexts[j].page_no &&
                    !strncmp(c1->mTeletextInfo.teletexts[i].lang, c2->mTeletextInfo.teletexts[j].lang, 3))
                    break;
            }
            if (j >= c2->mTeletextInfo.teletext_count) {
                //notify
                ret |= 8;
                break;
            }
        }
    }
    {
        if (c1->mCcapInfo.caption_count != c2->mCcapInfo.caption_count)
            ret |= 16;
    }
    {//check subtitle
        for (i=0; i<c1->mCcapInfo.caption_count; i++) {
            for (j=0; j<c2->mCcapInfo.caption_count; j++) {
                if (c1->mCcapInfo.captions[i].service_number == c2->mCcapInfo.captions[j].service_number &&
                    c1->mCcapInfo.captions[i].type == c2->mCcapInfo.captions[j].type &&
                    c1->mCcapInfo.captions[i].pid_or_line21 == c2->mCcapInfo.captions[j].pid_or_line21 &&
                    c1->mCcapInfo.captions[i].flags == c2->mCcapInfo.captions[j].flags &&
                    c1->mCcapInfo.captions[i].private_data == c2->mCcapInfo.captions[j].private_data &&
                    !strncmp(c1->mCcapInfo.captions[i].lang, c2->mCcapInfo.captions[j].lang, 3))
                    break;
            }
            if (j >= c2->mCcapInfo.caption_count) {
                //notify
                ret |= 16;
                break;
            }
        }
    }

    {
        if (c1->mIsdbInfo.isdb_count != c2->mIsdbInfo.isdb_count)
            ret |= 32;
    }
    {//check subtitle
        for (i=0; i<c1->mIsdbInfo.isdb_count; i++) {
            for (j=0; j<c2->mIsdbInfo.isdb_count; j++) {
                if (c1->mIsdbInfo.isdbs[i].pid == c2->mIsdbInfo.isdbs[j].pid &&
                    c1->mIsdbInfo.isdbs[i].type == c2->mIsdbInfo.isdbs[j].type &&
                    !strncmp(c1->mIsdbInfo.isdbs[i].lang, c2->mIsdbInfo.isdbs[j].lang, 3))
                    break;
            }
            if (j >= c2->mIsdbInfo.isdb_count) {
                //notify
                ret |= 32;
                break;
            }
        }
    }
    {
            if (c1->mScte27Info.subtitle_count != c2->mScte27Info.subtitle_count)
            ret |= 64;
    }
    {//check subtitle
            for (i=0; i<c1->mScte27Info.subtitle_count; i++) {
            for (j=0; j<c2->mScte27Info.subtitle_count; j++) {
                if (c1->mScte27Info.subtitles[i].pid == c2->mScte27Info.subtitles[j].pid)
                    break;
            }
            if (j >= c2->mScte27Info.subtitle_count) {
                //notify
                ret |= 64;
                break;
            }
        }
    }

    {
        if (ret & 1) {
            log_info(">>> Video changed ");
            log_info("Video pid/fmt: (%d/%d) -> (%d/%d)",
                c1->mVideoPID, c1->mVideoFormat,
                c2->mVideoPID, c2->mVideoFormat);
        }
        if (ret & 2) {
            char str_prev_ainfo[4][256], str_cur_ainfo[4][256];
            gen_audio_4strings(c1->mAudioInfo.audios, c1->mAudioInfo.audio_count, str_prev_ainfo, 256);
            gen_audio_4strings(c2->mAudioInfo.audios, c2->mAudioInfo.audio_count, str_cur_ainfo, 256);
            log_info(">>> Audio changed pid/fmt/lang/ext:");
            log_info("pid [%s]\nfmt [%s]\nlang [%s]\next [%s]", str_prev_ainfo[0], str_prev_ainfo[1], str_prev_ainfo[2], str_prev_ainfo[3]);
            log_info("changed to ->\npid [%s]\nfmt [%s]\nlang [%s]\next [%s]", str_cur_ainfo[0], str_cur_ainfo[1], str_cur_ainfo[2], str_cur_ainfo[3]);
        }
        if (ret & 4) {
            char str_prev_sinfo[5][256], str_cur_sinfo[5][256];
            gen_subtitle_5strings(c1->mSubtitleInfo.subtitles, c1->mSubtitleInfo.subtitle_count, str_prev_sinfo, 256);
            gen_subtitle_5strings(c2->mSubtitleInfo.subtitles, c2->mSubtitleInfo.subtitle_count, str_cur_sinfo, 256);
            log_info(">>> Subtitle changed pid/type/id1/id2/lang");
            log_info("pid [%s]\ntype [%s]\nid1 [%s]\nid2 [%s]\nlang [%s]",
                str_prev_sinfo[0], str_prev_sinfo[1], str_prev_sinfo[2],str_prev_sinfo[3], str_prev_sinfo[4]);
            log_info("changed to ->\npid [%s]\ntype [%s]\nid1 [%s]\nid2 [%s]\nlang [%s]",
                str_cur_sinfo[0], str_cur_sinfo[1], str_cur_sinfo[2],str_cur_sinfo[3], str_cur_sinfo[4]);
        }
        if (ret & 8) {
            char str_prev_tinfo[5][256], str_cur_tinfo[5][256];
            gen_teletext_5strings(c1->mTeletextInfo.teletexts, c1->mTeletextInfo.teletext_count, str_prev_tinfo, 256);
            gen_teletext_5strings(c2->mTeletextInfo.teletexts, c2->mTeletextInfo.teletext_count, str_cur_tinfo, 256);
            log_info(">>> Teletext changed pid/type/id1/id2/lang");
            log_info("pid [%s]\ntype [%s]\nid1 [%s]\nid2 [%s]\nlang [%s]",
                str_prev_tinfo[0], str_prev_tinfo[1], str_prev_tinfo[2],str_prev_tinfo[3], str_prev_tinfo[4]);
            log_info("changed to ->\npid [%s]\ntype [%s]\nid1 [%s]\nid2 [%s]\nlang [%s]",
                str_cur_tinfo[0], str_cur_tinfo[1], str_cur_tinfo[2],str_cur_tinfo[3], str_cur_tinfo[4]);
        }
        if (ret & 16) {
            char str_prev_tinfo[6][256], str_cur_tinfo[6][256];
            gen_cc_6strings(c1->mCcapInfo.captions, c1->mCcapInfo.caption_count, str_prev_tinfo, 256);
            gen_cc_6strings(c2->mCcapInfo.captions, c2->mCcapInfo.caption_count, str_cur_tinfo, 256);
            log_info(">>> CC changed pid/type/id1/id2/id3/lang");
            log_info("pid [%s]\ntype [%s]\nid1 [%s]\nid2 [%s]\nid3 [%s]\nlang [%s]",
                str_prev_tinfo[0], str_prev_tinfo[1], str_prev_tinfo[2],str_prev_tinfo[3],
                str_prev_tinfo[4], str_prev_tinfo[5]);
            log_info("changed to ->\npid [%s]\ntype [%s]\nid1 [%s]\nid2 [%s]\nid3 [%s]\nlang [%s]",
                str_cur_tinfo[0], str_cur_tinfo[1], str_cur_tinfo[2],str_cur_tinfo[3],
                str_cur_tinfo[4], str_cur_tinfo[5]);
        }
        if (ret & 32) {
            char str_prev_tinfo[3][256], str_cur_tinfo[3][256];
            gen_isdb_3strings(c1->mIsdbInfo.isdbs, c1->mIsdbInfo.isdb_count, str_prev_tinfo, 256);
            gen_isdb_3strings(c2->mIsdbInfo.isdbs, c2->mIsdbInfo.isdb_count, str_cur_tinfo, 256);
            log_info(">>> Isdb changed pid/type/lang");
            log_info("pid [%s]\ntype [%s]\nlang [%s]",
                str_prev_tinfo[0], str_prev_tinfo[1], str_prev_tinfo[2]);
            log_info("changed to ->\npid [%s]\ntype [%s]\nlang [%s]",
                str_cur_tinfo[0], str_cur_tinfo[1], str_cur_tinfo[2]);
        }
        if (ret & 64) {
            char str_prev_tinfo[1][256], str_cur_tinfo[1][256];
            gen_scte27_1strings(c1->mScte27Info.subtitles, c1->mScte27Info.subtitle_count, str_prev_tinfo, 256);
            gen_scte27_1strings(c2->mScte27Info.subtitles, c2->mScte27Info.subtitle_count, str_cur_tinfo, 256);
            log_info(">>> Scte27 changed pid");
            log_info("pid [%s]\n",
                str_prev_tinfo[0]);
            log_info("changed to ->\npid [%s]",
                str_cur_tinfo[0]);
        }
    }
    return ret;
}

typedef struct {
    int sub_count;
    struct {
        int type;
        int pid;
        int stype;
        int id1;
        int id2;
        char lang[16];
    }subs[AM_SI_MAX_SUB_CNT];
}sub_t;

#define FUNC_get_int_array(_n, _S, _c, _s, _e) \
static jintArray get_##_n##_array(_S *s) \
{ \
    JNIEnv *env; \
    int attached = 0; \
    int ret = -1; \
    int i; \
    if (!s->_c) \
        return NULL; \
    ret = (*gJavaVM)->GetEnv(gJavaVM, (void**) &env, JNI_VERSION_1_4); \
    if (ret <0) { \
        ret = (*gJavaVM)->AttachCurrentThread(gJavaVM,&env,NULL); \
        if (ret <0) { \
            log_error("callback handler:failed to attach current thread"); \
            return NULL; \
        } \
        attached = 1; \
    } \
    jintArray result = (*env)->NewIntArray(env, s->_c); \
    int *pa_tmp = malloc(s->_c * sizeof(int)); \
    for (i=0; i < s->_c; i++) \
        pa_tmp[i] = s->_s[i]._e; \
    (*env)->SetIntArrayRegion(env, result, 0, s->_c, pa_tmp); \
    free(pa_tmp); \
    if (attached) { \
        log_info("callback handler:detach current thread"); \
        (*gJavaVM)->DetachCurrentThread(gJavaVM); \
    } \
    return result; \
}

#define FUNC_get_string_array(_n, _S, _c, _s, _e) \
static jobjectArray get_##_n##_array(_S *s)\
{ \
    JNIEnv *env; \
    jstring  str; \
    jobjectArray args = 0; \
    int  attached = 0; \
    char sa[10]; \
    int  i=0; \
    int  ret = -1; \
    if (!s->_c) \
        return NULL; \
    ret = (*gJavaVM)->GetEnv(gJavaVM, (void**) &env, JNI_VERSION_1_4); \
    if (ret <0) { \
        ret = (*gJavaVM)->AttachCurrentThread(gJavaVM,&env,NULL); \
        if (ret <0) { \
            log_error("callback handler:failed to attach current thread"); \
            return NULL; \
        } \
        attached = 1; \
    } \
    args = (*env)->NewObjectArray(env, s->_c,(*env)->FindClass(env, "java/lang/String"),0); \
    for ( i=0; i < s->_c; i++ ) \
    { \
        memcpy(sa, s->_s[i]._e, 10); \
        str = (*env)->NewStringUTF(env, sa);\
        (*env)->SetObjectArrayElement(env, args, i, str); \
    } \
    if (attached) { \
        log_info("callback handler:detach current thread"); \
        (*gJavaVM)->DetachCurrentThread(gJavaVM); \
    } \
    return args; \
}

/*for Audios*/
FUNC_get_int_array(aids, AM_SI_AudioInfo_t, audio_count, audios, pid);
FUNC_get_int_array(afmts, AM_SI_AudioInfo_t, audio_count, audios, fmt);
FUNC_get_string_array(alangs, AM_SI_AudioInfo_t, audio_count, audios, lang);
FUNC_get_int_array(aexts, AM_SI_AudioInfo_t, audio_count, audios, audio_exten);

/*for Subs*/
FUNC_get_int_array(sids, sub_t, sub_count, subs, pid);
FUNC_get_int_array(stypes, sub_t, sub_count, subs, type);
FUNC_get_int_array(sstypes, sub_t, sub_count, subs, stype);
FUNC_get_int_array(sid1s, sub_t, sub_count, subs, id1);
FUNC_get_int_array(sid2s, sub_t, sub_count, subs, id2);
FUNC_get_string_array(slangs, sub_t, sub_count, subs, lang);

void format_cc_to_subtitle(AM_SI_CaptionInfo_t *cap_info, AM_SI_SubtitleInfo_t* sub_info)
{
    int i;
    if (!cap_info || !sub_info)
        return;
    sub_info->subtitle_count = cap_info->caption_count;
    for (i=0; i<sub_info->subtitle_count; i++)
    {
        sub_info->subtitles[i].type = 4;
        sub_info->subtitles[i].pid = cap_info->captions[i].service_number
                    + (cap_info->captions[i].type ? (8) : (1));
        sub_info->subtitles[i].comp_page_id = cap_info->captions[i].private_data;
        sub_info->subtitles[i].anci_page_id = cap_info->captions[i].flags;
        strncpy(sub_info->subtitles[i].lang, cap_info->captions[i].lang, 10);
    }
}

static void PMT_Update(AM_EPG_Handle_t handle, dvbpsi_pmt_t *pmts)
{
    dvbpsi_pmt_t *pmt;
    dvbpsi_pmt_es_t *es;
    dvbpsi_descriptor_t *descr;
    EPGChannelData *pch_cur = &gChannelMonitored,
            ch;
    EPGData *p_data;
    EPGEventData edata;

    if (!pmts)
        return;

    if (!pch_cur->valid)
        return;

    memset(&ch, 0, sizeof(ch));

    ch.mAudioInfo.audio_count = 0;
    ch.mVideoPID = 0x1fff;
    ch.mVideoFormat = -1;
    ch.mPcrPID = (pmts) ? pmts->i_pcr_pid : 0x1fff;
    ch.valid = 1;
    ch.mTransportStreamId = pch_cur->mTransportStreamId;
    ch.mProgramsInPat = pch_cur->mProgramsInPat;
    ch.mSdtVersion = pch_cur->mSdtVersion;
    ch.mPatTsId = pch_cur->mPatTsId;

    log_info("PMT update");
    AM_SI_LIST_BEGIN(pmts, pmt)
        AM_SI_LIST_BEGIN(pmt->p_first_es, es)
            AM_SI_ExtractAVFromES(es, &ch.mVideoPID, &ch.mVideoFormat, &ch.mAudioInfo);
            AM_SI_ExtractDVBSubtitleFromES(es, &ch.mSubtitleInfo);
            AM_SI_ExtractDVBTeletextFromES(es, &ch.mTeletextInfo);
            AM_SI_ExtractATSCCaptionFromES(es, &ch.mCcapInfo);
            AM_SI_ExtractDVBIsdbsubtitleFromES(es, &ch.mIsdbInfo);
            AM_SI_ExtractScte27SubtitleFromES(es, &ch.mScte27Info);
            AM_SI_LIST_END()
    AM_SI_LIST_END()
    if (pch_cur->mServiceId == pmts->i_program_number
        && check_pmt_update(pch_cur, &ch)) {

        /*update current*/
        memcpy(pch_cur, &ch, sizeof(pch_cur[0]));

        /*merge ebu subtitle & dvb subtitle*/
        sub_t sub;
        {
            int i;
            sub.sub_count = ch.mSubtitleInfo.subtitle_count;
            for (i=0; i<ch.mSubtitleInfo.subtitle_count; i++) {
                sub.subs[i].type = 1;
                sub.subs[i].pid = ch.mSubtitleInfo.subtitles[i].pid;
                sub.subs[i].stype = ch.mSubtitleInfo.subtitles[i].type;
                sub.subs[i].id1 = ch.mSubtitleInfo.subtitles[i].comp_page_id;
                sub.subs[i].id2 = ch.mSubtitleInfo.subtitles[i].anci_page_id;
                strncpy(sub.subs[i].lang, ch.mSubtitleInfo.subtitles[i].lang, 10);
            }
            int nsub = sub.sub_count;
            for (i=0; i<ch.mTeletextInfo.teletext_count && nsub<AM_SI_MAX_SUB_CNT; i++) {
                if (ch.mTeletextInfo.teletexts[i].type != 0x2 &&
                            ch.mTeletextInfo.teletexts[i].type != 0x5)
                            continue;
                sub.subs[nsub].type = 2;
                sub.subs[nsub].pid = ch.mTeletextInfo.teletexts[i].pid;
                sub.subs[nsub].stype = ch.mTeletextInfo.teletexts[i].type;
                sub.subs[nsub].id1 = ch.mTeletextInfo.teletexts[i].magazine_no;
                sub.subs[nsub].id2 = ch.mTeletextInfo.teletexts[i].page_no;
                strncpy(sub.subs[nsub].lang, ch.mTeletextInfo.teletexts[i].lang, 10);
                nsub++;
            }
            sub.sub_count = nsub;
        }
        {
            int nsub = sub.sub_count;
            int i;
            for (i=0; i<ch.mCcapInfo.caption_count; i++)
            {
                sub.subs[i].type = 4;
                sub.subs[i].stype = 4;
                sub.subs[i].pid = ch.mCcapInfo.captions[i].service_number
                    + (ch.mCcapInfo.captions[i].type ? (8) : (1));
                sub.subs[i].id1 = ch.mCcapInfo.captions[i].private_data;
                sub.subs[i].id2 = ch.mCcapInfo.captions[i].flags;
                strncpy(sub.subs[i].lang, ch.mCcapInfo.captions[i].lang, 10);
                nsub++;
            }
            sub.sub_count = nsub;
        }
        {
            int nsub = sub.sub_count;
            int i;
            for (i=0; i<ch.mIsdbInfo.isdb_count; i++)
            {
                sub.subs[i].type = 7;
                sub.subs[i].stype = 7;
                sub.subs[i].pid = ch.mIsdbInfo.isdbs[i].pid;
                sub.subs[i].id1 = 0;
                sub.subs[i].id2 = 0;
                strncpy(sub.subs[i].lang, ch.mIsdbInfo.isdbs[i].lang, 10);
                nsub++;
            }
            sub.sub_count = nsub;
        }
        {
            int nsub = sub.sub_count;
            int i;
            for (i=0; i<ch.mScte27Info.subtitle_count; i++)
            {
                sub.subs[i].type = 8;
                sub.subs[i].stype = 8;
                sub.subs[i].pid = ch.mScte27Info.subtitles[i].pid;
                sub.subs[i].id1 = 0;
                sub.subs[i].id2 = 0;
                strncpy(sub.subs[i].lang, "SCTE", 10);
                nsub++;
                log_error("scte27 add to sub %d", i);
            }
            sub.sub_count = nsub;
        }

        JNIEnv *env;
        int ret;
        int attached = 0;
        EPGData *priv_data;

        AM_EPG_GetUserData(handle, (void**)&priv_data);
        if (!priv_data)
            return;

        ret = (*gJavaVM)->GetEnv(gJavaVM, (void**) &env, JNI_VERSION_1_4);
        if (ret <0) {
            ret = (*gJavaVM)->AttachCurrentThread(gJavaVM,&env,NULL);
            if (ret <0) {
                log_error("callback handler:failed to attach current thread");
                return;
            }
            attached = 1;
        }

        jobject event = (*env)->NewObject(env, gEventClass, gEventInitID, priv_data->obj, EVENT_PROGRAM_AV_UPDATE);
        jobject channel = (*env)->NewObject(env, gChannelClass, gChannelInitID, event, 0);

        (*env)->SetIntField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mServiceId", "I"),  pmts->i_program_number);
        (*env)->SetIntField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mVideoPid", "I"), ch.mVideoPID);
        (*env)->SetIntField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mVfmt", "I"), ch.mVideoFormat);
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mAudioPids", "[I"), get_aids_array(&ch.mAudioInfo));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mAudioFormats", "[I"), get_afmts_array(&ch.mAudioInfo));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mAudioLangs", "[Ljava/lang/String;"), get_alangs_array(&ch.mAudioInfo));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mAudioExts", "[I"), get_aexts_array(&ch.mAudioInfo));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mSubtitlePids", "[I"), get_sids_array(&sub));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mSubtitleTypes", "[I"), get_stypes_array(&sub));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mSubtitleStypes", "[I"), get_sstypes_array(&sub));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mSubtitleId1s", "[I"), get_sid1s_array(&sub));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mSubtitleId2s", "[I"), get_sid2s_array(&sub));
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mSubtitleLangs", "[Ljava/lang/String;"), get_slangs_array(&sub));
        (*env)->SetIntField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mPcrPid", "I"), ch.mPcrPID);

        (*env)->SetObjectField(env,event,(*env)->GetFieldID(env, gEventClass, "channel", "Lcom/droidlogic/app/tv/ChannelInfo;"), channel);

        (*env)->CallVoidMethod(env, priv_data->obj, gOnEventID, event);

        if (attached) {
            (*gJavaVM)->DetachCurrentThread(gJavaVM);
        }
    }
}

static int epg_sdt_update_check_version(EPGChannelData *ch, dvbpsi_sdt_t *sdts) {
    log_info("[ver:tsid:nid] [%x:%x:%x] - [%x:%x:%x]",
        ch->mSdtVersion, ch->mTransportStreamId, ch->mOriginalNetworkId,
        sdts->i_version, sdts->i_ts_id, sdts->i_network_id);
    if ((ch->mSdtVersion != 0xff)
        && (ch->mSdtVersion == sdts->i_version)
        && (ch->mTransportStreamId == sdts->i_ts_id)
        && (ch->mOriginalNetworkId == sdts->i_network_id))
        return 0;
    return 1;
}

static int string_cat(void *d, int n_d, void *s, int n_s) {
    int l = strlen(d);
    if ((l+n_s) < n_d) {
        memcpy(((unsigned char*)d)+l, s, n_s);
        ((unsigned char*)d)[l+n_s] = 0;
        return n_s;
    }
    return 0;
}

static int sdt_get_services(dvbpsi_sdt_t *sdts, EPGChannelData *pch_cur) {
    dvbpsi_sdt_t *sdt;

    AM_SI_LIST_BEGIN(sdts, sdt)
    dvbpsi_sdt_service_t *srv;
    AM_SI_LIST_BEGIN(sdt->p_first_service, srv)
        dvbpsi_descriptor_t *descr;

        sdt_service_t *pservice = calloc(sizeof(sdt_service_t), 1);
        if (!pservice) {
            log_error("No memory for sdt update");
            continue;
        }

        pservice->id = srv->i_service_id;
        pservice->running = srv->i_running_status;
        pservice->free_ca = srv->b_free_ca;

        AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
        if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE) {
            dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t*)descr->p_decoded;
            char name[AM_DB_MAX_SRV_NAME_LEN + 4];
            char *old_name = pch_cur->name;

            pservice->type = psd->i_service_type;

            if (psd->i_service_name_length > 0) {
                AM_SI_ConvertDVBTextCode((char*)psd->i_service_name, psd->i_service_name_length,\
                            name, AM_DB_MAX_SRV_NAME_LEN);
                name[AM_DB_MAX_SRV_NAME_LEN+3] = 0;

                sprintf(pservice->name, "xxx%s", name);

                if (pch_cur->mServiceId == pservice->id) {
                    log_info("SDT Update: Program name changed: %s -> %s", old_name, name);
                    memcpy(pch_cur->name, name, sizeof(pch_cur->name));
                }
            }
        }

        if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_MULTI_SERVICE_NAME) {
            int i;
            dvbpsi_multi_service_name_dr_t *pmsnd = (dvbpsi_multi_service_name_dr_t*)descr->p_decoded;
            char name[AM_DB_MAX_SRV_NAME_LEN + 4];
            for (i=0; i<pmsnd->i_name_count; i++) {
                name[0] = 0;
                AM_SI_ConvertDVBTextCode((char*)pmsnd->p_service_name[i].i_service_name,
                    pmsnd->p_service_name[i].i_service_name_length,
                    name, AM_DB_MAX_SRV_NAME_LEN);
                name[AM_DB_MAX_SRV_NAME_LEN] = 0;

                if (pservice->name[0]) {
                    char split[] = SPLIT_MULTINAME;
                    string_cat(pservice->name, MAX_LEN_MULTINAME, split, LEN_SPLIT_MULTINAME);
                }
                string_cat(pservice->name, MAX_LEN_MULTINAME, pmsnd->p_service_name[i].i_iso_639_code, 3);
                string_cat(pservice->name, MAX_LEN_MULTINAME, name, strlen(name));
            }
        }

        AM_SI_LIST_END()

        pservice->next = pch_cur->services;
        pch_cur->services = pservice;

    AM_SI_LIST_END()
    AM_SI_LIST_END()

    return 0;
}

static void SDT_Update(AM_EPG_Handle_t handle, EPGChannelData *pch) {
    JNIEnv *env;
    int ret;
    int attached = 0;
    EPGData *priv_data;

    AM_EPG_GetUserData(handle, (void**)&priv_data);
    if (!priv_data)
        return ;

    ret = (*gJavaVM)->GetEnv(gJavaVM, (void**) &env, JNI_VERSION_1_4);
    if (ret <0) {
        ret = (*gJavaVM)->AttachCurrentThread(gJavaVM,&env,NULL);
        if (ret <0) {
            log_error("callback handler:failed to attach current thread");
            return ;
        }
        attached = 1;
    }

    jobject event = (*env)->NewObject(env, gEventClass, gEventInitID, priv_data->obj, EVENT_PROGRAM_NAME_UPDATE);

    jobject channel = (*env)->NewObject(env, gChannelClass, gChannelInitID, event, 0);

    (*env)->SetIntField(env,channel,\
            (*env)->GetFieldID(env, gChannelClass, "mSdtVersion", "I"), pch->mSdtVersion);
    (*env)->SetIntField(env,channel,\
            (*env)->GetFieldID(env, gChannelClass, "mOriginalNetworkId", "I"), pch->mOriginalNetworkId);
    if (is_utf8(pch->name, strlen(pch->name))) {
        (*env)->SetObjectField(env,channel,\
                (*env)->GetFieldID(env, gChannelClass, "mDisplayName", "Ljava/lang/String;"), (*env)->NewStringUTF(env, pch->name));
    }
    (*env)->SetIntField(env,channel,\
            (*env)->GetFieldID(env, gChannelClass, "mServiceId", "I"), pch->mServiceId);

    (*env)->SetObjectField(env,event,(*env)->GetFieldID(env, gEventClass, "channel", "Lcom/droidlogic/app/tv/ChannelInfo;"), channel);


    jobject serviceinfos = (*env)->NewObject(env, gServiceInfosFromSDTClass, gServiceInfosFromSDTInitID, event, 0);

    (*env)->SetIntField(env, serviceinfos,\
            (*env)->GetFieldID(env, gServiceInfosFromSDTClass, "mNetworkId", "I"), pch->mOriginalNetworkId);
    (*env)->SetIntField(env, serviceinfos,\
            (*env)->GetFieldID(env, gServiceInfosFromSDTClass, "mTSId", "I"), pch->mTransportStreamId);
    (*env)->SetIntField(env, serviceinfos,\
            (*env)->GetFieldID(env, gServiceInfosFromSDTClass, "mVersion", "I"), pch->mSdtVersion);

    jclass list_cls = (*env)->FindClass(env, "java/util/ArrayList");
    jmethodID list_costruct = (*env)->GetMethodID(env, list_cls , "<init>","()V");
    jmethodID list_add  = (*env)->GetMethodID(env, list_cls,"add","(Ljava/lang/Object;)Z");

    jobject services =  (*env)->NewObject(env, list_cls, list_costruct, serviceinfos, 0);

    sdt_service_t *pservice = pch->services;
    while (pservice) {
        jobject service = (*env)->NewObject(env, gServiceInfoFromSDTClass, gServiceInfoFromSDTInitID, serviceinfos, 0);
        (*env)->SetIntField(env, service,\
            (*env)->GetFieldID(env, gServiceInfoFromSDTClass, "mId", "I"), pservice->id);
        (*env)->SetIntField(env, service,\
            (*env)->GetFieldID(env, gServiceInfoFromSDTClass, "mType", "I"), pservice->type);
        if (is_utf8(pservice->name, strlen(pservice->name))) {
            (*env)->SetObjectField(env,service,\
                 (*env)->GetFieldID(env, gServiceInfoFromSDTClass, "mName", "Ljava/lang/String;"), (*env)->NewStringUTF(env, pservice->name));
        }
        (*env)->SetIntField(env, service,\
            (*env)->GetFieldID(env, gServiceInfoFromSDTClass, "mRunning", "I"), pservice->running);
        (*env)->SetIntField(env, service,\
            (*env)->GetFieldID(env, gServiceInfoFromSDTClass, "mFreeCA", "I"), pservice->free_ca);

        (*env)->CallBooleanMethod(env, services, list_add, service);

        pservice = pservice->next;
    }

    (*env)->SetObjectField(env, serviceinfos,\
            (*env)->GetFieldID(env, gServiceInfosFromSDTClass, "mServices", "Ljava/util/ArrayList;"), services);

    (*env)->SetObjectField(env, event,\
            (*env)->GetFieldID(env, gEventClass, "services", "Lcom/droidlogic/tvinput/services/DTVEpgScanner$Event$ServiceInfosFromSDT;"), serviceinfos);

    (*env)->CallVoidMethod(env, priv_data->obj, gOnEventID, event);

    if (attached) {
        (*gJavaVM)->DetachCurrentThread(gJavaVM);
    }
}

static int epg_sdt_update(AM_EPG_Handle_t handle, int type, void *tables, void *user_data)
{
    dvbpsi_sdt_t *sdts = (dvbpsi_sdt_t*)tables;
    EPGChannelData *pch_cur = &gChannelMonitored;
    dvbpsi_sdt_t *sdt;

    UNUSED(type);
    UNUSED(user_data);

    if (sdts->i_table_id != AM_SI_TID_SDT_ACT)
        return 1;

    if (!pch_cur->valid)
        return 1;

    if (!epg_sdt_update_check_version(pch_cur, sdts))
        return 2;

    log_info("something changed.");

    /*nid/tsid changed*/
    if (/*((pch_cur->mOriginalNetworkId != -1) && (pch_cur->mOriginalNetworkId != sdts->i_network_id))
        || */(pch_cur->mTransportStreamId != sdts->i_ts_id)) {
        log_info("nid:[0x%04x->0x%04x] tsid:[0x%04x->0x%04x]",
            pch_cur->mOriginalNetworkId, sdts->i_network_id,
            pch_cur->mTransportStreamId, sdts->i_ts_id);
        pch_cur->mOriginalNetworkId = sdts->i_network_id;
        pch_cur->mTransportStreamId = sdts->i_ts_id;
        pch_cur->mSdtVersion = sdts->i_version;
        epg_evt_callback((long)handle, AM_EPG_EVT_UPDATE_TS, 0, NULL);
        return 0;
    }

    /*version changed*/
    if ((pch_cur->mSdtVersion != 0xff) && (pch_cur->mSdtVersion != sdts->i_version)) {
        log_info("sdt ver:[%d->%d]", pch_cur->mSdtVersion, sdts->i_version);
        pch_cur->mOriginalNetworkId = sdts->i_network_id;
        pch_cur->mTransportStreamId = sdts->i_ts_id;
        pch_cur->mSdtVersion = sdts->i_version;
        epg_evt_callback((long)handle, AM_EPG_EVT_UPDATE_TS, 0, NULL);
        return 0;
    }

    /*sdt not received yet, request name update*/
    pch_cur->mOriginalNetworkId = sdts->i_network_id;
    pch_cur->mTransportStreamId = sdts->i_ts_id;
    pch_cur->mSdtVersion = sdts->i_version;

    sdt_get_services(sdts, pch_cur);

    SDT_Update(handle, pch_cur);

    return 0;
}

static int epg_pat_update(AM_EPG_Handle_t handle, int type, void *tables, void *user_data)
{
    dvbpsi_pat_t *pats = (dvbpsi_pat_t*)tables;
    EPGChannelData *pch_cur = &gChannelMonitored;
    dvbpsi_pat_t *pat;
    dvbpsi_pat_program_t *prog;
    int prog_in_pat = 0;

    UNUSED(type);
    UNUSED(user_data);

    if (!pch_cur->valid)
        return 1;

    AM_SI_LIST_BEGIN(pats, pat) {
        AM_SI_LIST_BEGIN(pat->p_first_program, prog) {
            prog_in_pat ++;
   } AM_SI_LIST_END();
    } AM_SI_LIST_END();

    if ((pats->i_ts_id != pch_cur->mPatTsId)
           || (pch_cur->mProgramsInPat && (pch_cur->mProgramsInPat != prog_in_pat))) {
        log_info(" TS changed: tsid[%d]-->tsid[%d],  ProgramsInPat[%d]-->ProgramsInPat[%d]",
                 pch_cur->mPatTsId, pats->i_ts_id, pch_cur->mProgramsInPat, prog_in_pat);
        pch_cur->mPatTsId = pats->i_ts_id;
        pch_cur->mProgramsInPat = prog_in_pat;
        epg_evt_callback((long)handle, AM_EPG_EVT_UPDATE_TS, 0, NULL);
        return 0;
    }

    return 0;
}

static int epg_mgt_update(AM_EPG_Handle_t handle, int type, void *tables, void *user_data)
{
    dvbpsi_atsc_mgt_t *mgts = (dvbpsi_atsc_mgt_t*)tables;
    EPGChannelData *pch_cur = &gChannelMonitored;
    dvbpsi_atsc_mgt_t *mgt;
    dvbpsi_atsc_mgt_table_t *table;
    int mIsVerChanged = 0;
    UNUSED(type);
    UNUSED(user_data);

    if (!pch_cur->valid)
        return 1;

    AM_SI_LIST_BEGIN(mgts, mgt)
        AM_SI_LIST_BEGIN(mgt->p_first_table, table)
            switch(table->i_table_type) {
                case AM_SI_ATSC_TT_EIT0 ... AM_SI_ATSC_TT_EIT0+127: {
                    int k = table->i_table_type - AM_SI_ATSC_TT_EIT0;
                    //log_info("mgt: k:%d v:%d", k, table->i_table_type_version);
                    if (pch_cur->mEitVersions[k] != table->i_table_type_version) {
                        log_info("mgt: version chaned, eit[%d]: %d -> %d",
                            k, pch_cur->mEitVersions[k], table->i_table_type_version);
                        pch_cur->mEitVersions[k] = table->i_table_type_version;
                        mIsVerChanged = 1;
                    }
                }break;
                case AM_SI_ATSC_TT_ETT0 ... AM_SI_ATSC_TT_ETT0+127:
                break;
            }
        AM_SI_LIST_END()
    AM_SI_LIST_END()
    if (mIsVerChanged == 1) {
        int k = 0;
        epg_evt_callback((long)handle, EVENT_EIT_CHANGED,
        (void*)(long)k,
        (void*)pch_cur->mEitVersions);
    }
    return 0;
}

static int epg_vct_update(AM_EPG_Handle_t handle, int type, void *tables, void *user_data)
{
        dvbpsi_atsc_vct_t *vcts = (dvbpsi_atsc_vct_t*)tables;
        dvbpsi_atsc_vct_t *vct;
        EPGChannelData *pch_cur = &gChannelMonitored;

        UNUSED(type);
        UNUSED(user_data);

        if (!pch_cur->valid)
             return 1;

        if (NULL == vcts)
             return 2;

        AM_SI_LIST_BEGIN(vcts, vct)
             if ((vct->i_extension == pch_cur->mTransportStreamId) && (pch_cur->mSdtVersion != vcts->i_version)) {
                  log_info("##### vct: version changed ##### wow  VctVersion: [%d] -> [%d] ",pch_cur->mSdtVersion, vcts->i_version); //temp use mSdtVersion to store vctVersion.
                  if (pch_cur->mSdtVersion != 0xff)
                       epg_evt_callback((long)handle, AM_EPG_EVT_UPDATE_TS, 0, NULL);
                  pch_cur->mSdtVersion = vcts->i_version;
                  break;
             }
        AM_SI_LIST_END()
        return 0;
}

static void epg_table_callback(AM_EPG_Handle_t handle, int type, void *tables, void *user_data)
{
    if (!tables)
        return;

    switch (type) {
        case AM_EPG_TAB_SDT:
            epg_sdt_update(handle, type, tables, user_data);
        break;
        case AM_EPG_TAB_PAT:
            epg_pat_update(handle, type, tables, user_data);
        break;
        case AM_EPG_TAB_MGT:
            epg_mgt_update(handle, type, tables, user_data);
        break;
        case AM_EPG_TAB_VCT:
            epg_vct_update(handle, type, tables, user_data);
        break;
        default:
        break;
    }
}
#endif

static void epg_create(JNIEnv* env, jobject obj, jint fend_id, jint dmx_id, jint src, jstring ordered_text_langs)
{
#ifdef SUPPORT_ADTV
    AM_EPG_CreatePara_t para;
    EPGData *data;
    AM_ErrorCode_t ret;
    AM_FEND_OpenPara_t fend_para;
    AM_DMX_OpenPara_t dmx_para;

    data = (EPGData*)malloc(sizeof(EPGData));
    if (!data) {
        log_error("malloc failed");
        return;
    }

    setDvbDebugLoglevel();

    data->dmx_id = dmx_id;
    log_info("Opening demux%d ...", dmx_id);
    memset(&dmx_para, 0, sizeof(dmx_para));
    AM_DMX_Open(dmx_id, &dmx_para);

    memset(&para, 0, sizeof(para));
    para.fend_dev = fend_id;
    para.dmx_dev  = dmx_id;
    para.source   = src;
    para.hdb      = NULL;
    const char *strlang = (*env)->GetStringUTFChars(env, ordered_text_langs, 0);
    if (strlang != NULL) {
        snprintf(para.text_langs, sizeof(para.text_langs), "%s", strlang);
        (*env)->ReleaseStringUTFChars(env, ordered_text_langs, strlang);
    }

    ret = AM_EPG_Create(&para, &data->handle);
    if (ret != AM_SUCCESS) {
        free(data);
        log_error("AM_EPG_Create failed");
        return;
    }

    data->obj = (*env)->NewGlobalRef(env,obj);

    (*env)->SetLongField(env, obj, gHandleID, (long)data);

    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_NEW_NIT,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_NEW_TDT,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_NEW_STT,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_UPDATE_EVENTS,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_UPDATE_PROGRAM_AV,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_UPDATE_PROGRAM_NAME,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_UPDATE_TS,epg_evt_callback,NULL);
    AM_EVT_Subscribe((long)data->handle,AM_EPG_EVT_PMT_RATING,epg_evt_callback,NULL);
    AM_EPG_SetUserData(data->handle, (void*)data);

    /*handle epg events*/
    AM_EPG_SetEventsCallback(data->handle, Events_Update);

    /*disable internal default table procedure*/
    AM_EPG_DisableDefProc(data->handle, AM_TRUE);

    /*handle tables directly by user*/
    AM_EPG_SetTablesCallback(data->handle, AM_EPG_TAB_SDT, epg_table_callback, NULL);
    if (!epg_conf_get("tv.dtv.tsupdate.pat.disable", 0))
        AM_EPG_SetTablesCallback(data->handle, AM_EPG_TAB_PAT, epg_table_callback, NULL);
    AM_EPG_SetTablesCallback(data->handle, AM_EPG_TAB_MGT, epg_table_callback, NULL);
    if (epg_conf_get("tv.dtv.tsupdate.vct.enable", 0))
        AM_EPG_SetTablesCallback(data->handle, AM_EPG_TAB_VCT, epg_table_callback, NULL);
#endif
}

static void epg_destroy(JNIEnv* env, jobject obj)
{
#ifdef SUPPORT_ADTV
    EPGData *data;

    data = (EPGData*)(long)((*env)->GetLongField(env, obj, gHandleID));

    if (data) {
        AM_EVT_Unsubscribe((long)data->handle,AM_EPG_EVT_NEW_TDT,epg_evt_callback,NULL);
        AM_EVT_Unsubscribe((long)data->handle,AM_EPG_EVT_NEW_STT,epg_evt_callback,NULL);
        AM_EVT_Unsubscribe((long)data->handle,AM_EPG_EVT_UPDATE_EVENTS,epg_evt_callback,NULL);
        AM_EVT_Unsubscribe((long)data->handle,AM_EPG_EVT_UPDATE_PROGRAM_AV,epg_evt_callback,NULL);
        AM_EVT_Unsubscribe((long)data->handle,AM_EPG_EVT_UPDATE_PROGRAM_NAME,epg_evt_callback,NULL);
        AM_EVT_Unsubscribe((long)data->handle,AM_EPG_EVT_UPDATE_TS,epg_evt_callback,NULL);
        AM_EPG_Destroy(data->handle);
        log_info("EPGScanner on demux%d sucessfully destroyed", data->dmx_id);
        log_info("Closing demux%d ...", data->dmx_id);
        AM_DMX_Close(data->dmx_id);
        if (data->obj)
            (*env)->DeleteGlobalRef(env, data->obj);
        free(data);
    }
#endif
}

static void epg_change_mode(JNIEnv* env, jobject obj, jint op, jint mode)
{
#ifdef SUPPORT_ADTV
    EPGData *data;
    AM_ErrorCode_t ret;

    setDvbDebugLoglevel();

    data = (EPGData*)(long)((*env)->GetLongField(env, obj, gHandleID));

    ret = AM_EPG_ChangeMode(data->handle, op, mode);
    if (ret != AM_SUCCESS)
        log_error("AM_EPG_ChangeMode failed");
#endif
}

static int get_channel_data(JNIEnv* env, jobject obj, jobject channel, EPGChannelData *pch)
{
#ifdef SUPPORT_ADTV
    UNUSED(obj);

    memset(pch, 0, sizeof(*pch));

    if (!channel) {
        pch->valid = 0;
        return 0;
    }

    int i;
    jclass objclass =(*env)->FindClass(env,"com/droidlogic/app/tv/ChannelInfo");
    jstring Name = (*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mDisplayName", "Ljava/lang/String;"));
    if (Name) {
        const char *cName = (*env)->GetStringUTFChars(env, Name, 0);
        strncpy(pch->name, cName, (64+4)*4);
        (*env)->ReleaseStringUTFChars(env, Name, cName);
    }
    pch->mOriginalNetworkId = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mOriginalNetworkId", "I"));
    pch->mTransportStreamId = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mTransportStreamId", "I"));
    pch->mProgramsInPat = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mProgramsInPat", "I"));
    pch->mServiceId = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mServiceId", "I"));
    pch->mFrequency = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mFrequency", "I"));
    pch->mBandwidth = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mBandwidth", "I"));
    pch->mVideoPID = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mVideoPid", "I"));
    pch->mVideoFormat = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mVfmt", "I"));
    pch->mPcrPID = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mPcrPid", "I"));
    pch->mPatTsId = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mPatTsId", "I"));
    jintArray aids = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mAudioPids", "[I"));
    jintArray afmts = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mAudioFormats", "[I"));
    jobjectArray alangs = (jobjectArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mAudioLangs", "[Ljava/lang/String;"));
    jintArray aexts = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mAudioExts", "[I"));
    if (aids && afmts) {
        jint *paids = (*env)->GetIntArrayElements(env, aids, 0);
        jint *pafmts = (*env)->GetIntArrayElements(env, afmts, 0);
        jint *paexts = (*env)->GetIntArrayElements(env, aexts, 0);

        pch->mAudioInfo.audio_count = (*env)->GetArrayLength(env, aids);
        for (i=0; i<pch->mAudioInfo.audio_count; i++) {
            jstring jstr = (*env)->GetObjectArrayElement(env, alangs, i);
            const char *str = (char *)(*env)->GetStringUTFChars(env, jstr, 0);
            pch->mAudioInfo.audios[i].pid = paids[i];
            pch->mAudioInfo.audios[i].fmt = pafmts[i];
            pch->mAudioInfo.audios[i].audio_exten = paexts[i];
            strncpy(pch->mAudioInfo.audios[i].lang, str, 10);
            (*env)->ReleaseStringUTFChars(env, jstr, str);
            (*env)->DeleteLocalRef(env, jstr);
        }
        (*env)->ReleaseIntArrayElements(env, aids, paids, JNI_ABORT);
        (*env)->ReleaseIntArrayElements(env, afmts, pafmts, JNI_ABORT);
        (*env)->ReleaseIntArrayElements(env, aexts, paexts, JNI_ABORT);
    }
    jintArray stypes = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mSubtitleTypes", "[I"));
    jintArray sids = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mSubtitlePids", "[I"));
    jintArray sstypes = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mSubtitleStypes", "[I"));
    jintArray sid1s = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mSubtitleId1s", "[I"));
    jintArray sid2s = (jintArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mSubtitleId2s", "[I"));
    jobjectArray slangs = (jobjectArray)(*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mSubtitleLangs", "[Ljava/lang/String;"));

    if (sids && stypes && sstypes && sid1s && sid2s) {
        jint *psids = (*env)->GetIntArrayElements(env, sids, 0);
        jint *pstypes = (*env)->GetIntArrayElements(env, stypes, 0);
        jint *psstypes = (*env)->GetIntArrayElements(env, sstypes, 0);
        jint *psid1s = (*env)->GetIntArrayElements(env, sid1s, 0);
        jint *psid2s = (*env)->GetIntArrayElements(env, sid2s, 0);

        int subtitle_count = (*env)->GetArrayLength(env, sids);
        for (i=0; i<subtitle_count; i++) {
            if (pstypes[i] == 1) {//subtitle
                jstring jstr = (*env)->GetObjectArrayElement(env, slangs, i);
                const char *str = (char *)(*env)->GetStringUTFChars(env, jstr, 0);
                int ii = pch->mSubtitleInfo.subtitle_count;
                pch->mSubtitleInfo.subtitles[ii].pid = psids[i];
                pch->mSubtitleInfo.subtitles[ii].type = psstypes[i];
                pch->mSubtitleInfo.subtitles[ii].comp_page_id = psid1s[i];
                pch->mSubtitleInfo.subtitles[ii].anci_page_id = psid2s[i];
                strncpy(pch->mSubtitleInfo.subtitles[ii].lang, str, 10);
                (*env)->ReleaseStringUTFChars(env, jstr, str);
                (*env)->DeleteLocalRef(env, jstr);
                pch->mSubtitleInfo.subtitle_count++;
            } else if (pstypes[i] == 2) {//teletext subtitle
                jstring jstr = (*env)->GetObjectArrayElement(env, slangs, i);
                const char *str = (char *)(*env)->GetStringUTFChars(env, jstr, 0);
                int ii = pch->mTeletextInfo.teletext_count;
                pch->mTeletextInfo.teletexts[ii].pid = psids[i];
                pch->mTeletextInfo.teletexts[ii].type = psstypes[i];
                pch->mTeletextInfo.teletexts[ii].magazine_no = psid1s[i];
                pch->mTeletextInfo.teletexts[ii].page_no = psid2s[i];
                strncpy(pch->mTeletextInfo.teletexts[ii].lang, str, 10);
                (*env)->ReleaseStringUTFChars(env, jstr, str);
                (*env)->DeleteLocalRef(env, jstr);
                pch->mTeletextInfo.teletext_count++;
            } else if (pstypes[i] == 4) {//captions
                jstring jstr = (*env)->GetObjectArrayElement(env, slangs, i);
                const char *str = (char *)(*env)->GetStringUTFChars(env, jstr, 0);
                int ii = pch->mCcapInfo.caption_count;
                pch->mCcapInfo.captions[ii].service_number = psids[i];
                pch->mCcapInfo.captions[ii].type = psstypes[i];
                pch->mCcapInfo.captions[ii].private_data = psid1s[i];
                pch->mCcapInfo.captions[ii].flags = psid2s[i];
                strncpy(pch->mCcapInfo.captions[ii].lang, str, 10);
                (*env)->ReleaseStringUTFChars(env, jstr, str);
                (*env)->DeleteLocalRef(env, jstr);
                pch->mCcapInfo.caption_count++;
            } else if (pstypes[i] == 7) {//isdbts
                jstring jstr = (*env)->GetObjectArrayElement(env, slangs, i);
                const char *str = (char *)(*env)->GetStringUTFChars(env, jstr, 0);
                int ii = pch->mIsdbInfo.isdb_count;
                pch->mIsdbInfo.isdbs[ii].pid = psids[i];
                pch->mIsdbInfo.isdbs[ii].type = 7;
                strncpy(pch->mIsdbInfo.isdbs[ii].lang, str, 10);
                (*env)->ReleaseStringUTFChars(env, jstr, str);
                (*env)->DeleteLocalRef(env, jstr);
                pch->mIsdbInfo.isdb_count++;
            }
            else if (pstypes[i] == 8) {//scte27
                int ii = pch->mScte27Info.subtitle_count;
                pch->mScte27Info.subtitles[ii].pid = psids[i];
                pch->mScte27Info.subtitle_count++;
            }
        }
        (*env)->ReleaseIntArrayElements(env, sids,    psids,    JNI_ABORT);
        (*env)->ReleaseIntArrayElements(env, stypes,  pstypes,  JNI_ABORT);
        (*env)->ReleaseIntArrayElements(env, sstypes, psstypes, JNI_ABORT);
        (*env)->ReleaseIntArrayElements(env, sid1s,   psid1s,   JNI_ABORT);
        (*env)->ReleaseIntArrayElements(env, sid2s,   psid2s,   JNI_ABORT);
    }

    pch->mSdtVersion = (*env)->GetIntField(env, channel, (*env)->GetFieldID(env, objclass, "mSdtVersion", "I"));

    int versionCount = 0;
    jintArray eitVersions = (*env)->GetObjectField(env, channel, (*env)->GetFieldID(env, objclass, "mEitVersions", "[I"));
    if (eitVersions) {
        jint *pVersions = (*env)->GetIntArrayElements(env, eitVersions, 0);
        versionCount = (*env)->GetArrayLength(env, eitVersions);
        for (i=0; i<versionCount; i++)
            pch->mEitVersions[i] = pVersions[i];
    }

    for (i = versionCount; i < sizeof(pch->mEitVersions)/sizeof(pch->mEitVersions[0]); i++)
        pch->mEitVersions[i] = -1;

    char buf[2048] = "\0";
    for (i = 0; i < sizeof(pch->mEitVersions)/sizeof(pch->mEitVersions[0]); i++)
        sprintf(buf, "%s,%d", buf, pch->mEitVersions[i]);
    log_error("eitversions: %s\n", buf);

    pch->valid = 1;
#endif
    return 0;
}

static void epg_monitor_service(JNIEnv* env, jobject obj, jobject channel)
{
#ifdef SUPPORT_ADTV
    EPGData *data;
    AM_ErrorCode_t ret;
    EPGChannelData *pch = &gChannelMonitored;
    int ts_id, srv_id;

    //channel = NULL;

    int err = get_channel_data(env, obj, channel, pch);
    if (err) {
        log_error("EPGMonitorService get channel data failed");
        return;
    }

    if (pch->valid) {
        ts_id = pch->mTransportStreamId;
        srv_id = pch->mServiceId;
    } else {
        ts_id = srv_id = -1;
    }

    data = (EPGData*)(long)((*env)->GetLongField(env, obj, gHandleID));
    ret = AM_EPG_MonitorServiceByID(data->handle, ts_id, srv_id, PMT_Update);
    if (ret != AM_SUCCESS) {
        log_error("AM_EPG_MonitorService failed");
    }
#endif
}

static void epg_set_dvb_text_coding(JNIEnv* env, jobject obj, jstring coding)
{
#ifdef SUPPORT_ADTV
    const char *str = (*env)->GetStringUTFChars(env, coding, 0);

    UNUSED(obj);

    if (str != NULL) {
        if (!strcmp(str, "standard")) {
            AM_SI_SetDefaultDVBTextCoding("");
        } else {
            AM_SI_SetDefaultDVBTextCoding(str);
        }

        (*env)->ReleaseStringUTFChars(env, coding, str);
    }
#endif
}

static JNINativeMethod epg_methods[] =
{
    /* name, signature, funcPtr */
    {"native_epg_create", "(IIILjava/lang/String;)V", (void*)epg_create},
    {"native_epg_destroy", "()V", (void*)epg_destroy},
    {"native_epg_change_mode", "(II)V", (void*)epg_change_mode},
    {"native_epg_monitor_service", "(Lcom/droidlogic/app/tv/ChannelInfo;)V", (void*)epg_monitor_service},
    {"native_epg_set_dvb_text_coding", "(Ljava/lang/String;)V", (void*)epg_set_dvb_text_coding}
};

//JNIHelp.h ????
#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif
static int registerNativeMethods(JNIEnv* env, const char* className,
                                 const JNINativeMethod* methods, int numMethods)
{
    int rc;
    jclass clazz;

    clazz = (*env)->FindClass(env, className);

    if (clazz == NULL)
        return -1;

    if ((rc = ((*env)->RegisterNatives(env, clazz, methods, numMethods))) < 0)
        return -1;

    return 0;
}

JNIEXPORT jint
JNI_OnLoad(JavaVM* vm, void* reserved __unused)
{
    JNIEnv* env = NULL;
    jclass clazz;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
        return -1;

    if (registerNativeMethods(env, "com/droidlogic/tvinput/services/DTVEpgScanner", epg_methods, NELEM(epg_methods)) < 0)
        return -1;

    gJavaVM = vm;

    clazz = (*env)->FindClass(env, "com/droidlogic/tvinput/services/DTVEpgScanner");
    if (clazz == NULL) {
        log_error("FindClass com/droidlogic/tvinput/services/DTVEpgScanner failed");
        return -1;
    }

    gOnEventID = (*env)->GetMethodID(env, clazz, "onEvent", "(Lcom/droidlogic/tvinput/services/DTVEpgScanner$Event;)V");
    gHandleID = (*env)->GetFieldID(env, clazz, "native_handle", "J");
    gEventClass       = (*env)->FindClass(env, "com/droidlogic/tvinput/services/DTVEpgScanner$Event");
    gEventClass       = (jclass)(*env)->NewGlobalRef(env, (jobject)gEventClass);
    gEventInitID      = (*env)->GetMethodID(env, gEventClass, "<init>", "(Lcom/droidlogic/tvinput/services/DTVEpgScanner;I)V");

    gEvtClass       = (*env)->FindClass(env, "com/droidlogic/tvinput/services/DTVEpgScanner$Event$Evt");
    gEvtClass       = (jclass)(*env)->NewGlobalRef(env, (jobject)gEvtClass);
    gEvtInitID      = (*env)->GetMethodID(env, gEvtClass, "<init>", "(Lcom/droidlogic/tvinput/services/DTVEpgScanner$Event;)V");

    gChannelClass   = (*env)->FindClass(env, "com/droidlogic/app/tv/ChannelInfo");
    gChannelClass   = (jclass)(*env)->NewGlobalRef(env, (jobject)gChannelClass);
    gChannelInitID  = (*env)->GetMethodID(env, gChannelClass, "<init>", "()V");

    gServiceInfosFromSDTClass   = (*env)->FindClass(env, "com/droidlogic/tvinput/services/DTVEpgScanner$Event$ServiceInfosFromSDT");
    gServiceInfosFromSDTClass   = (jclass)(*env)->NewGlobalRef(env, (jobject)gServiceInfosFromSDTClass);
    gServiceInfosFromSDTInitID  = (*env)->GetMethodID(env, gServiceInfosFromSDTClass, "<init>", "(Lcom/droidlogic/tvinput/services/DTVEpgScanner$Event;)V");

    gServiceInfoFromSDTClass   = (*env)->FindClass(env, "com/droidlogic/tvinput/services/DTVEpgScanner$Event$ServiceInfosFromSDT$ServiceInfoFromSDT");
    gServiceInfoFromSDTClass   = (jclass)(*env)->NewGlobalRef(env, (jobject)gServiceInfoFromSDTClass);
    gServiceInfoFromSDTInitID  = (*env)->GetMethodID(env, gServiceInfoFromSDTClass, "<init>", "(Lcom/droidlogic/tvinput/services/DTVEpgScanner$Event$ServiceInfosFromSDT;)V");

    return JNI_VERSION_1_4;
}

JNIEXPORT void
JNI_OnUnload(JavaVM* vm, void* reserved __unused)
{
    JNIEnv* env = NULL;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return;
    }

    (*env)->DeleteGlobalRef(env, (jobject)gChannelClass);
    (*env)->DeleteGlobalRef(env, (jobject)gEventClass);
    (*env)->DeleteGlobalRef(env, (jobject)gEvtClass);
    (*env)->DeleteGlobalRef(env, (jobject)gServiceInfosFromSDTClass);
    (*env)->DeleteGlobalRef(env, (jobject)gServiceInfoFromSDTClass);
}

