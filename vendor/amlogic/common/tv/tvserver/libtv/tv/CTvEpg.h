/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifdef SUPPORT_ADTV
#include <am_debug.h>
#include <am_scan.h>
#include <am_epg.h>
#include <am_mem.h>
#endif

#include <utils/Log.h>
#include "CTvEv.h"
#include <tvutils.h>
#if !defined(_CDTVEPG_H)
#define _CDTVEPG_H
class CTvEpg {
public :
    static const int MODE_ADD    = 0;
    static const int MODE_REMOVE = 1;
    static const int MODE_SET    = 2;

    static const int SCAN_PAT = 0x01;
    static const int SCAN_PMT = 0x02;
    static const int SCAN_CAT = 0x04;
    static const int SCAN_SDT = 0x08;
    static const int SCAN_NIT = 0x10;
    static const int SCAN_TDT = 0x20;
    static const int SCAN_EIT_PF_ACT = 0x40;
    static const int SCAN_EIT_PF_OTH = 0x80;
    static const int SCAN_EIT_SCHE_ACT = 0x100;
    static const int SCAN_EIT_SCHE_OTH = 0x200;
    static const int SCAN_MGT = 0x400;
    static const int SCAN_VCT = 0x800;
    static const int SCAN_STT = 0x1000;
    static const int SCAN_RRT = 0x2000;
    static const int SCAN_PSIP_EIT   = 0x4000;
    static const int SCAN_PSIP_ETT = 0x8000;
    static const int SCAN_EIT_PF_ALL = SCAN_EIT_PF_ACT | SCAN_EIT_PF_OTH;
    static const int SCAN_EIT_SCHE_ALL = SCAN_EIT_SCHE_ACT | SCAN_EIT_SCHE_OTH;
    static const int SCAN_EIT_ALL = SCAN_EIT_PF_ALL | SCAN_EIT_SCHE_ALL;
    static const int SCAN_ALL = SCAN_PAT | SCAN_PMT | SCAN_CAT | SCAN_SDT | SCAN_NIT | SCAN_TDT | SCAN_EIT_ALL |
                                SCAN_MGT | SCAN_VCT | SCAN_STT | SCAN_RRT | SCAN_PSIP_EIT | SCAN_PSIP_ETT;

    static const int INVALID_ID = -1;

    //egp notify
    /*static const int EVENT_PF_EIT_END = 1;
    static const int EVENT_SCH_EIT_END = 2;
    static const int EVENT_PMT_END = 3;
    static const int EVENT_SDT_END = 4;
    static const int EVENT_TDT_END = 5;
    static const int EVENT_NIT_END = 6;
    static const int EVENT_PROGRAM_AV_UPDATE = 7;
    static const int EVENT_PROGRAM_NAME_UPDATE = 8;
    static const int EVENT_PROGRAM_EVENTS_UPDATE = 9;
    static const int EVENT_CHANNEL_UPDATE = 10;*/
    //
    class EpgEvent : public CTvEv {
    public:
        EpgEvent(): CTvEv(CTvEv::TV_EVENT_EPG)
        {
            type = 0;
            channelID = 0;
            programID = 0;
            dvbOrigNetID = 0;
            dvbTSID = 0;
            dvbServiceID = 0;
            time = 0;
            dvbVersion = 0;
        };
        ~EpgEvent()
        {
        };
        static const int EVENT_PF_EIT_END            = 1;
        static const int EVENT_SCH_EIT_END           = 2;
        static const int EVENT_PMT_END               = 3;
        static const int EVENT_SDT_END               = 4;
        static const int EVENT_TDT_END               = 5;
        static const int EVENT_NIT_END               = 6;
        static const int EVENT_PROGRAM_AV_UPDATE     = 7;
        static const int EVENT_PROGRAM_NAME_UPDATE   = 8;
        static const int EVENT_PROGRAM_EVENTS_UPDATE = 9;
        static const int EVENT_CHANNEL_UPDATE        = 10;
        static const int EVENT_CHANNEL_UPDATE_END    = 11;

        int type;
        int channelID;
        long programID;
        int dvbOrigNetID;
        int dvbTSID;
        int dvbServiceID;
        long time;
        int dvbVersion;
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const EpgEvent &ev) = 0;
    };
    //1 VS n
    //int addObserver(IObserver* ob);
    //int removeObserver(IObserver* ob);

    //1 VS 1
    int setObserver(IObserver *ob)
    {
        mpObserver = ob;
        return 0;
    }

    CTvEpg()
    {
        mCurScanChannelId  = INVALID_ID;
        mCurScanProgramId  = INVALID_ID;
        #ifdef SUPPORT_ADTV
        mEpgScanHandle = NULL;
        #endif
        mpObserver = NULL;
        mDmx_dev_id = INVALID_ID;
        mFend_dev_id = 0;
        mFend_mod = 0;

    }
    void Init(int fend, int dmx, int fend_mod, char *textLanguages, char *dvb_text_coding);

    ~CTvEpg()
    {
        epg_destroy();
    }

    /*Enter a channel.*/
    void enterChannel(int chan_id);
    /*Leave the channel.*/
    void leaveChannel();
    /*Enter the program.*/
    void enterProgram(int prog_id);
    /*Leave the program.*/
    void leaveProgram();

private:
    //
    void epg_create(int fend_id, int dmx_id, int src, char *textLangs);
    void epg_destroy();
    void epg_change_mode(int op, int mode);
    void epg_monitor_service(int srv_id);
    void epg_set_dvb_text_coding(char *coding);


    /*Start scan the sections.*/
    void startScan(int mode);
    /*Stop scan the sections.*/
    void stopScan(int mode);

    static  void epg_evt_callback(long dev_no, int event_type, void *param, void *user_data);

    //
    IObserver *mpObserver;

#ifdef SUPPORT_ADTV
    void * mEpgScanHandle;
#endif
    int mFend_dev_id;
    int mDmx_dev_id ;
    int mFend_mod;
    int mCurScanChannelId ;
    int mCurScanProgramId ;

    //
    EpgEvent mCurEpgEv;
};
#endif //_CDTVEPG_H
