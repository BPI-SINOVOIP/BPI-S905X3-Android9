/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */
#if !defined(_CTVRECORD_H_)
#define _CTVRECORD_H_

#ifdef SUPPORT_ADTV
#include "am_rec.h"
#endif
#include "CTvEv.h"

#include <utils/Vector.h>

using namespace android;

class CTvRecord {
public:
    CTvRecord();
    ~CTvRecord();

    void setId(const char* id) { mId = strdup(id); }
    const char* getId() { return mId; }

    int setFilePath(const char *name);

    int setFileName(const char *prefix, const char *suffix);

    static const int REC_EXT_TYPE_PMTPID = 0;
    static const int REC_EXT_TYPE_PN = 1;
    static const int REC_EXT_TYPE_ADD_PID = 2;
    static const int REC_EXT_TYPE_REMOVE_PID = 3;
    int setMediaInfoExt(int type, int val);

    int setTimeShiftMode(bool enable, int duration, int size);

    static const int REC_DEV_TYPE_FE = 0;
    static const int REC_DEV_TYPE_DVR = 1;
    static const int REC_DEV_TYPE_FIFO = 2;
    int setDev(int type, int id);

    int setupDefault(const char *param);

    int start(const char *param);

    int stop(const char *param);

    int setRecCurTsOrCurProgram(int sel);  // 1: all program in the Ts; 0:current program

    //for timeshifting
    int getStartPosition();
    int getWritePosition();

    bool equals(CTvRecord &recorder);

    class RecEvent : public CTvEv {
    public:
        RecEvent(): CTvEv(CTvEv::TV_EVENT_RECORDER)
        {
            type = 0;
            error = 0;
            size = 0;
            time = 0;
        }
        ~RecEvent() {}
        static const int EVENT_REC_START=1;
        static const int EVENT_REC_STOP=2;
        static const int EVENT_REC_STARTPOSITION_CHANGED=3;
        std::string id;
	int type;
	int error;
	int size;
	long time;
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const RecEvent &ev) = 0;
    };

    int setObserver(IObserver *ob) {
        mpObserver = ob;
        return 0;
    }

#ifdef SUPPORT_ADTV
    int setMediaInfo(AM_REC_MediaInfo_t *info);
    int getInfo(AM_REC_RecInfo_t *info);
    AM_TFile_t getFileHandler();
    AM_TFile_t detachFileHandler();

    AM_REC_CreatePara_t mCreateParam;
#endif
private :

    char *mId;
#ifdef SUPPORT_ADTV
    AM_REC_Handle_t mRec;
    AM_REC_RecPara_t mRecParam;
    AM_REC_RecInfo_t mRecInfo;
#endif
    Vector<int> mExtPids;
    IObserver *mpObserver;
    RecEvent mEvent;

    static void rec_evt_cb(long dev_no, int event_type, void *param, void *data);

};

#endif //_CTVRECORD_H_
