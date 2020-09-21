/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVPLAYER_H_)
#define _CTVPLAYER_H_

#include <string.h>

#ifdef SUPPORT_ADTV
#include "am_misc.h"
#endif
#include "CTv.h"
#include "CTvRecord.h"
#include "CTvManager.h"
#include "tvutils.h"


using namespace android;

class CTv;

class CTvPlayer {
    public:
        CTvPlayer(CTv *tv);
        virtual ~CTvPlayer();

        void setId(const char *id) { if(mId) free((void*)mId); mId = strdup(id); }
        const char * getId() { return mId; }
        bool equals(CTvPlayer &player __unused) { return true;/*support only one player now.*/ }
        void sendTvEvent ( const CTvEv &ev );

        virtual int start(const char *param) = 0;
        virtual int stop(const char *param) = 0;
        virtual int pause(const char *param) = 0 ;
        virtual int resume(const char *param) = 0 ;
        virtual int seek(const char *param) = 0;
        virtual int set(const char *param) = 0;
        virtual int setupDefault(const char *param) = 0;

    protected:
        CTv *pTv;

    private:
        const char *mId;
};

class CDTVTvPlayer : public CTvPlayer, public CTvRecord::IObserver {

    static const bool bStartInTimeShift = true;

    public:
        CDTVTvPlayer(CTv *tv) ;
        ~CDTVTvPlayer() ;

        int start(const char *param);
        int stop(const char *param);
        int pause(const char *param) ;
        int resume(const char *param) ;
        int seek(const char *param);
        int set(const char *param);
        int setupDefault(const char *param);

        int setFEParam(const char *param) ;
        int setVideo(int pid, int format, const char *param) ;
        int setAudio(int pid, int format, const char *param) ;
        int setPcr(int pid, const char *param) ;
        int setParam(const char *param) ;

        void onEvent(const CTvRecord::RecEvent &ev) ;
        void onPlayUpdate(const CAv::AVEvent &ev);


    private:

        static const int PLAY_MODE_LIVE = 0;
        static const int PLAY_MODE_REC = 1;
        static const int PLAY_MODE_TIMESHIFT = 2;
        static const int PLAY_MODE_LIVE_WITHOUT_TIMESHIFT = 3;
        int mMode;

        bool mDisableTimeShifting;

        const char *mFEParam;

        int mVpid;
        int mVfmt;
        const char *mVparam;
        int mApid;
        int mAfmt;
        const char *mAparam;
        int mPpid;
        const char *mPparam;

        const char *mParam;

        int mOffset;

        bool mSourceChanged;
#ifdef SUPPORT_ADTV
        static int readMediaInfoFromFile(const char *file_path, AM_AV_TimeshiftMediaInfo_t *info);
        AM_TFile_t mTfile = nullptr;
#endif

        int tryCloseTFile();

        int startLive(const char *param);
        int startLiveTryTimeShift(const char *param);
        int startLiveTryRecording(const char *param);

        static void tfile_evt_callback(long dev_no, int event_type, void *param, void *data);
        static void player_info_callback(long dev_no, int event_type, void *param, void *data);

};

class CATVTvPlayer : public CTvPlayer {
    public:
        CATVTvPlayer(CTv *tv) ;
        ~CATVTvPlayer() ;

        int start(const char *param);
        int stop(const char *param);
        int pause(const char *param __unused) { return 0; };
        int resume(const char *param __unused) { return 0; };
        int seek(const char *param __unused) { return 0; };
        int set(const char *param);
        int setupDefault(const char *param);

        int setFEParam(const char *param) ;
        int setVideo(const char *param) ;
        int setAudio(const char *param) ;
        int setParam(const char *param) ;

    private:

        const char *mFEParam;
        const char *mVparam;
        const char *mAparam;

        const char *mParam;

};


#endif  //_CTVPLAYER_H_

