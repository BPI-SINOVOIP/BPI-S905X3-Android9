/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvRecord"

#include <tvutils.h>
#include "CTvRecord.h"
#include "CTvLog.h"

#define FEND_DEV_NO 0
#define DVR_DEV_NO 0
#define DVR_BUF_SIZE 1024*1024
#define DVR_DEV_COUNT      (2)

CTvRecord::CTvRecord()
{
#ifdef SUPPORT_ADTV
    memset(&mCreateParam, 0, sizeof(mCreateParam));
    memset(&mRecParam, 0, sizeof(mRecParam));
    memset(&mRecInfo, 0, sizeof(mRecInfo));
    mCreateParam.fend_dev = FEND_DEV_NO;
    mCreateParam.dvr_dev = DVR_DEV_NO;
    mCreateParam.async_fifo_id = 0;
    mRec = NULL;
    mId = NULL;
    mpObserver = NULL;
#endif
}

CTvRecord::~CTvRecord()
{
    if (mId)
        free((void*)mId);
#ifdef SUPPORT_ADTV
    if (mRec) {
        AM_REC_Destroy(mRec);
        mRec = NULL;
    }
#endif
}

int CTvRecord::setFilePath(const char *name)
{
    LOGD("setFilePath(%s)", toReadable(name));
#ifdef SUPPORT_ADTV
    strncpy(mCreateParam.store_dir, name, AM_REC_PATH_MAX);
    mCreateParam.store_dir[AM_REC_PATH_MAX-1] = 0;
#endif
    return 0;
}

int CTvRecord::setFileName(const char *prefix, const char *suffix)
{
    LOGD("setFileName(%s,%s)", toReadable(prefix), toReadable(suffix));
#ifdef SUPPORT_ADTV
    strncpy(mRecParam.prefix_name, prefix, AM_REC_NAME_MAX);
    mRecParam.prefix_name[AM_REC_NAME_MAX-1] = 0;
    strncpy(mRecParam.suffix_name, suffix, AM_REC_SUFFIX_MAX);
    mRecParam.suffix_name[AM_REC_SUFFIX_MAX-1] = 0;
#endif
    return 0;
}

#ifdef SUPPORT_ADTV
int CTvRecord::setMediaInfo(AM_REC_MediaInfo_t *info)
{
    LOGD("setMediaInfo()" );
    memcpy(&mRecParam.media_info, info, sizeof(AM_REC_MediaInfo_t));
    return 0;
}

int CTvRecord::getInfo(AM_REC_RecInfo_t *info)
{
    if (!mRec || !info)
        return -1;
    return AM_REC_GetRecordInfo(mRec, info);
}

AM_TFile_t CTvRecord::getFileHandler()
{
    if (!mRec)
        return NULL;

    AM_TFile_t file = NULL;
    AM_ErrorCode_t err = DVB_SUCCESS;
    err = AM_REC_GetTFile(mRec, &file, NULL);
    if (err != DVB_SUCCESS)
        return NULL;
    return file;
}

AM_TFile_t CTvRecord::detachFileHandler()
{
    if (!mRec)
        return NULL;

    AM_TFile_t file = NULL;
    int flag;
    AM_ErrorCode_t err = DVB_SUCCESS;
    err = AM_REC_GetTFile(mRec, &file, &flag);
    if (err != DVB_SUCCESS) {
        LOGD("get tfile fail(%d)", err);
        return NULL;
    }
    AM_REC_SetTFile(mRec, file, flag |REC_TFILE_FLAG_DETACH);
    return file;
}
#endif

int CTvRecord::setMediaInfoExt(int type, int val)
{
    LOGD("setMediaInfoExt(%d,%d)", type, val );
#ifdef SUPPORT_ADTV
    switch (type) {
        case REC_EXT_TYPE_PMTPID:
            mRecParam.program.i_pid = val;
            break;
        case REC_EXT_TYPE_PN:
            mRecParam.program.i_number = val;
            break;
        case REC_EXT_TYPE_ADD_PID:
        case REC_EXT_TYPE_REMOVE_PID:
            mExtPids.add(val);
            break;
        default:
            return -1;
            break;
    }
#endif
    return 0;
}

int CTvRecord::setTimeShiftMode(bool enable, int duration, int size)
{
    LOGD("setTimeShiftMode(%d, duration:%d - size:%d)", enable, duration, size );
#ifdef SUPPORT_ADTV
    mRecParam.is_timeshift = enable? true : false;
    mRecParam.total_time = enable ? duration : 0;
    mRecParam.total_size = enable ? size : 0;
#endif
    return 0;
}

int CTvRecord::setDev(int type, int id)
{
    LOGD("setDev(%d,%d)", type, id );
#ifdef SUPPORT_ADTV
    switch (type) {
        case REC_DEV_TYPE_FE:
            mCreateParam.fend_dev = id;
            break;
        case REC_DEV_TYPE_DVR:
            mCreateParam.dvr_dev = id;
            break;
        case REC_DEV_TYPE_FIFO:
            mCreateParam.async_fifo_id = id;
            break;
        default:
            return -1;
            break;
    }
#endif
    return 0;
}

void CTvRecord::rec_evt_cb(long dev_no, int event_type, void *param, void *data)
{
    CTvRecord *rec;
#ifdef SUPPORT_ADTV
    AM_REC_GetUserData((AM_REC_Handle_t)dev_no, (void**)&rec);
    if (!rec)
        return;

    switch (event_type) {
        case AM_REC_EVT_RECORD_END :{
            AM_REC_RecEndPara_t *endpara = (AM_REC_RecEndPara_t*)param;
            rec->mEvent.type = RecEvent::EVENT_REC_STOP;
            rec->mEvent.id = std::string((const char*)data);
            rec->mEvent.error = endpara->error_code;
            rec->mEvent.size = endpara->total_size;
            rec->mEvent.time = endpara->total_time;
            rec->mpObserver->onEvent(rec->mEvent);
            }break;
        case AM_REC_EVT_RECORD_START: {
            rec->mEvent.type = RecEvent::EVENT_REC_START;
            rec->mEvent.id = std::string((const char*)data);
            rec->mpObserver->onEvent(rec->mEvent);
            }break;
        default:
            break;
    }
#endif
    LOGD ( "rec_evt_callback : dev_no %ld type %d param = %ld\n",
        dev_no, event_type, (long)param);
}

int CTvRecord::start(const char *param)
{
    int ret = -1;
    LOGD("start(%s:%s)", toReadable(mId), toReadable(param));
#ifdef SUPPORT_ADTV
    ret = AM_REC_Create(&mCreateParam, &mRec);
    if (ret != DVB_SUCCESS) {
        LOGD("create fail(%d)", ret);
        mRec = NULL;
        return ret;
    }

    AM_REC_SetUserData(mRec, this);
    AM_EVT_Subscribe((long)mRec, AM_REC_EVT_RECORD_START, rec_evt_cb, (void*)getId());
    AM_EVT_Subscribe((long)mRec, AM_REC_EVT_RECORD_END, rec_evt_cb, (void*)getId());

    AM_REC_SetTFile(mRec, NULL, REC_TFILE_FLAG_AUTO_CREATE);
    ret = AM_REC_StartRecord(mRec, &mRecParam);
    if (ret != DVB_SUCCESS) {
        LOGD("start fail(%d)", ret);
        AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_START, rec_evt_cb, (void*)getId());
        AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_END, rec_evt_cb, (void*)getId());
        AM_REC_Destroy(mRec);
        mRec = NULL;
    } else {
        LOGD("start ok.");
    }
#endif
    return ret;
}

int CTvRecord::stop(const char *param)
{
    LOGD("stop(%s:%s)", toReadable(mId), toReadable(param));
#ifdef SUPPORT_ADTV
    int ret = 0;
    if (!mRec)
        return -1;

    ret = AM_REC_StopRecord(mRec);

    AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_START, rec_evt_cb, (void*)getId());
    AM_EVT_Unsubscribe((long)mRec, AM_REC_EVT_RECORD_END, rec_evt_cb, (void*)getId());

    return ret;
#else
    return -1;
#endif
}

int CTvRecord::setRecCurTsOrCurProgram(int sel)
{
    LOGD("setRecCurTsOrCurProgram(%s:%d)", toReadable(mId), sel);
#ifdef SUPPORT_ADTV
    char buf[64];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "/sys/class/stb/dvr%d_mode", mCreateParam.dvr_dev);
    if (sel)
        tvWriteSysfs(buf, "ts");
    else
        tvWriteSysfs(buf, "pid");
#endif
    return 0;
}

bool CTvRecord::equals(CTvRecord &recorder)
{
#ifdef SUPPORT_ADTV
    return mCreateParam.fend_dev == recorder.mCreateParam.fend_dev
        && mCreateParam.dvr_dev == recorder.mCreateParam.dvr_dev
        && mCreateParam.async_fifo_id == recorder.mCreateParam.async_fifo_id;
#else
    return false;
#endif
}

int CTvRecord::getStartPosition()
{
#ifdef SUPPORT_ADTV
    if (!mRec)
        return 0;
#endif
    return 0;
}

int CTvRecord::getWritePosition()
{
    return 0;
}

int CTvRecord::setupDefault(const char *param)
{
#ifdef SUPPORT_ADTV
    int i = 0;
    char buff[32] = {0};
    setDev(CTvRecord::REC_DEV_TYPE_FE, paramGetInt(param, NULL, "fe", 0));
    setDev(CTvRecord::REC_DEV_TYPE_DVR, paramGetInt(param, NULL, "dvr", 0));
    setDev(CTvRecord::REC_DEV_TYPE_FIFO, paramGetInt(param, NULL, "fifo", 0));
    setFilePath(paramGetString(param, NULL, "path", "/storage").c_str());
    setFileName(paramGetString(param, NULL, "prefix", "REC").c_str(), paramGetString(param, NULL, "suffix", "ts").c_str());
    AM_REC_MediaInfo_t info;
    info.duration = 0;
    info.vid_pid = paramGetInt(param, "v", "pid", -1);
    info.vid_fmt = paramGetInt(param, "v", "fmt", -1);
    info.aud_cnt = 1;
    info.audios[0].pid = paramGetInt(param, "a", "pid", -1);
    info.audios[0].fmt = paramGetInt(param, "a", "fmt", -1);
    info.sub_cnt = paramGetInt(param, NULL, "subcnt", 0);
    for (i=0; i<info.sub_cnt; i++)
    {
        sprintf(buff, "pid%d", i);
        info.subtitles[i].pid = paramGetInt(param, NULL, buff, 0);
        //LOGE("Get subpid%d %x count %d", i, info.subtitles[i].pid,info.sub_cnt);
    }
    info.ttx_cnt = 0;
    memset(info.program_name, 0, sizeof(info.program_name));
    setMediaInfo(&info);
    setTimeShiftMode(
        paramGetInt(param, NULL, "timeshift", 0) ? true : false,
        paramGetInt(param, "max", "time", 60),
        paramGetInt(param, "max", "size", -1));
#endif
    return 0;
}

