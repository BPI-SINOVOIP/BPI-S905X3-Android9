/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _CTVEV_H_
#define _CTVEV_H_

#include <utils/String8.h>
#define CC_MAX_SERIAL_RD_BUF_LEN   (1200)

using namespace android;

class CTvEv {
public:
    static const int TV_EVENT_COMMOM = 0;
    static const int TV_EVENT_SCANNER = 1;
    static const int TV_EVENT_EPG = 2;//EPG
    static const int TV_EVENT_SOURCE_SWITCH = 3;
    static const int TV_EVENT_SIGLE_DETECT = 4;
    static const int TV_EVENT_ADC_CALIBRATION = 5;
    static const int TV_EVENT_VGA = 6;//VGA
    static const int TV_EVENT_3D_STATE = 7;//3D
    static const int TV_EVENT_AV_PLAYBACK = 8;//PLAYBACK EVENT MSG
    static const int TV_EVENT_SERIAL_COMMUNICATION = 9;
    static const int TV_EVENT_SOURCE_CONNECT = 10;
    static const int TV_EVENT_HDMIRX_CEC = 11;
    static const int TV_EVENT_BLOCK = 12;
    static const int TV_EVENT_CC = 13; //close caption
    static const int TV_EVENT_VCHIP = 14; //VCHIP
    static const int TV_EVENT_HDMI_IN_CAP = 15;
    static const int TV_EVENT_UPGRADE_FBC = 16;
    static const int TV_EVENT_2d4G_HEADSET = 17;
    static const int TV_EVENT_AV = 18;
    static const int TV_EVENT_SUBTITLE = 19;
    static const int TV_EVENT_SCANNING_FRAME_STABLE = 20;
    static const int TV_EVENT_FRONTEND = 21;
    static const int TV_EVENT_RECORDER = 22;
    static const int TV_EVENT_RRT = 23;//RRT
    static const int TV_EVENT_EAS = 24;//EAS
    static const int TV_EVENT_AUDIO_CB = 25;

    CTvEv(int type);
    virtual ~CTvEv() {};
    int getEvType() const {
        return mEvType;
    };
private:
    int mEvType;
};

namespace  TvEvent {
    //events
    class SignalInfoEvent: public CTvEv {
    public:
        SignalInfoEvent() : CTvEv ( CTvEv::TV_EVENT_SIGLE_DETECT )
        {
            mTrans_fmt = 0;
            mFmt = 0;
            mStatus = 0;
            mReserved = 0;
        }
        ~SignalInfoEvent() {}
        int mTrans_fmt;
        int mFmt;
        int mStatus;
        int mReserved;
    };

    class VGAEvent: public CTvEv {
    public:
        VGAEvent() : CTvEv ( CTvEv::TV_EVENT_VGA )
        {
            mState = 0;
        }
        ~VGAEvent() {}
        int mState;
    };

    class ADCCalibrationEvent: public CTvEv {
    public:
        ADCCalibrationEvent() : CTvEv ( CTvEv::TV_EVENT_ADC_CALIBRATION )
        {
            mState = 0;
        }
        ~ADCCalibrationEvent() {}
        int mState;
    };

    class SerialCommunicationEvent: public CTvEv {
    public:
        SerialCommunicationEvent(): CTvEv(CTvEv::TV_EVENT_SERIAL_COMMUNICATION)
        {
            mDevId = 0;
            mDataCount = 0;
        }
        ~SerialCommunicationEvent() {}

        int mDevId;
        int mDataCount;
        unsigned char mDataBuf[CC_MAX_SERIAL_RD_BUF_LEN];
    };

    class SourceConnectEvent: public CTvEv {
    public:
        SourceConnectEvent() : CTvEv ( CTvEv::TV_EVENT_SOURCE_CONNECT )
        {
            mSourceInput = 0;
            connectionState = 0;
        }
        ~SourceConnectEvent() {}
        int mSourceInput;
        int connectionState;
    };

    class SourceSwitchEvent: public CTvEv {
    public:
        SourceSwitchEvent() : CTvEv ( CTvEv::TV_EVENT_SOURCE_SWITCH )
        {
            DestSourceInput = 0;
            DestSourcePortNum = 0;
        }
        ~SourceSwitchEvent() {}
        int DestSourceInput;
        int DestSourcePortNum;
    };

    class HDMIRxCECEvent: public CTvEv {
    public:
        HDMIRxCECEvent() : CTvEv ( CTvEv::TV_EVENT_HDMIRX_CEC )
        {
            mDataCount = 0;
            memset(mDataBuf, 0, sizeof(mDataBuf));
        }
        ~HDMIRxCECEvent() {}
        int mDataCount;
        int mDataBuf[32];
    };

    class AVPlaybackEvent: public CTvEv {
    public:
        AVPlaybackEvent() : CTvEv ( CTvEv::TV_EVENT_AV_PLAYBACK )
        {
            mMsgType = 0;
            mProgramId = 0;
        }
        ~AVPlaybackEvent() {}
        static const int EVENT_AV_PLAYBACK_NODATA       = 1;
        static const int EVENT_AV_PLAYBACK_RESUME       = 2;
        static const int EVENT_AV_SCAMBLED              = 3;
        static const int EVENT_AV_UNSUPPORT             = 4;
        static const int EVENT_AV_VIDEO_AVAILABLE       = 5;

        static const int EVENT_AV_TIMESHIFT_REC_FAIL = 6;
        static const int EVENT_AV_TIMESHIFT_PLAY_FAIL = 7;
        static const int EVENT_AV_TIMESHIFT_START_TIME_CHANGED = 8;
        static const int EVENT_AV_TIMESHIFT_CURRENT_TIME_CHANGED = 9;

        int mMsgType;
        int mProgramId;
    };
    class AVAudioCBEvent:public CTvEv {
    public:
        AVAudioCBEvent():CTvEv ( CTvEv::TV_EVENT_AUDIO_CB ) {
            cmd = 0;
            param1 = 0;
            param2 = 0;
        }
        ~AVAudioCBEvent(){}

        int cmd;
        int param1;
        int param2;
    };
    class BlockEvent: public CTvEv {
    public:
        BlockEvent() : CTvEv ( CTvEv::TV_EVENT_BLOCK )
        {
            block_status = false;
            programBlockType = 0;
            vchipDimension = String8("");
            vchipAbbrev = String8("");
            vchipAbbtext = String8("");
        }
        ~BlockEvent() {}

        bool block_status;
        int programBlockType;
        String8 vchipDimension;
        String8 vchipAbbrev;
        String8 vchipAbbtext;
    };

    class UpgradeFBCEvent: public CTvEv {
    public:
        UpgradeFBCEvent() : CTvEv ( CTvEv::TV_EVENT_UPGRADE_FBC )
        {
            mState = 0;
            param = 0;
        }
        ~UpgradeFBCEvent() {}
        int mState;
        int param;
    };

    class HeadSetOf2d4GEvent: public CTvEv {
    public:
        HeadSetOf2d4GEvent(): CTvEv(CTvEv::TV_EVENT_2d4G_HEADSET)
        {
            state = 0;
            para = 0;
        }
        ~HeadSetOf2d4GEvent() {}

        int state;
        int para;
    };

    class SubtitleEvent: public CTvEv {
    public:
        SubtitleEvent(): CTvEv(CTvEv::TV_EVENT_SUBTITLE)
        {
            pic_width = 0;
            pic_height = 0;
        }
        ~SubtitleEvent() {}

        int pic_width;
        int pic_height;
    };

    class ScanningFrameStableEvent: public CTvEv {
    public:
        ScanningFrameStableEvent(): CTvEv(CTvEv::TV_EVENT_SCANNING_FRAME_STABLE)
        {
            CurScanningFreq = 0;
        }
        ~ScanningFrameStableEvent() {}
        int CurScanningFreq;
    };

    class FrontendEvent: public CTvEv {
    public:
        FrontendEvent() : CTvEv ( CTvEv::TV_EVENT_FRONTEND )
        {
            mStatus = 0;
            mFrequency = 0;
            mParam1 = 0;
            mParam2 = 0;
            mParam3 = 0;
            mParam4 = 0;
            mParam5 = 0;
            mParam6 = 0;
            mParam7 = 0;
            mParam8 = 0;
        }
        ~FrontendEvent() {}
        static const int EVENT_FE_HAS_SIG = 0x01;
        static const int EVENT_FE_NO_SIG = 0x02;
        static const int EVENT_FE_INIT = 0x03;

        int mStatus;
        int mFrequency;
        int mParam1;
        int mParam2;
        int mParam3;
        int mParam4;
        int mParam5;
        int mParam6;
        int mParam7;
        int mParam8;
    };

    class RecorderEvent: public CTvEv {
    public:
        RecorderEvent() : CTvEv ( CTvEv::TV_EVENT_RECORDER)
        {
            mId = String8("");
            mStatus = 0;
            mError = 0;
        }
        ~RecorderEvent() {}
        static const int EVENT_RECORD_START = 0x01;
        static const int EVENT_RECORD_STOP = 0x02;

        String8 mId;
        int mStatus;
        int mError;
    };

};
#endif

