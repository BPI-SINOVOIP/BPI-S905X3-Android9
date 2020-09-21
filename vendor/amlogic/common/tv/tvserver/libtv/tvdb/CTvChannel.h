/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVCHANNEL_H)
#define _CTVCHANNEL_H
#include <utils/Vector.h>
#include "CTvDatabase.h"
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <stdlib.h>

#include "CTvLog.h"
using namespace android;

//must sync with dvb frontend.h
enum {
	TV_FE_HAS_SIGNAL	= 0x01,   /* found something above the noise level */
	TV_FE_HAS_CARRIER	= 0x02,   /* found a DVB signal  */
	TV_FE_HAS_VITERBI	= 0x04,   /* FEC is stable  */
	TV_FE_HAS_SYNC	= 0x08,   /* found sync bytes  */
	TV_FE_HAS_LOCK	= 0x10,   /* everything's working... */
	TV_FE_TIMEDOUT	= 0x20,   /* no lock within the last ~2 seconds */
	TV_FE_REINIT	= 0x40    /* frontend was reinitialized,  */
};			  /* application is recommended to reset */
//end

class CTvChannel: public LightRefBase<CTvChannel> {

public :
    static const int MODE_QPSK = 0;

    static const int MODE_QAM  = 1;

    static const int MODE_OFDM = 2;

    static const int MODE_ATSC = 3;

    static const int MODE_ANALOG = 4;

    static const int MODE_DTMB = 5;

    static const int MODE_ISDBT = 6;



    static const int BANDWIDTH_8_MHZ = 0;

    static const int BANDWIDTH_7_MHZ = 1;

    static const int BANDWIDTH_6_MHZ = 2;

    static const int BANDWIDTH_AUTO  = 3;

    static const int BANDWIDTH_5_MHZ = 4;

    static const int BANDWIDTH_10_MHZ = 5;


    static const int MODULATION_QPSK    = 0;

    static const int MODULATION_QAM_16  = 1;

    static const int MODULATION_QAM_32  = 2;

    static const int MODULATION_QAM_64  = 3;

    static const int MODULATION_QAM_128 = 4;

    static const int MODULATION_QAM_256 = 5;

    static const int MODULATION_QAM_AUTO = 6;

    static const int MODULATION_VSB_8   = 7;

    static const int MODULATION_VSB_16  = 8;

    static const int MODULATION_PSK_8   = 9;

    static const int MODULATION_APSK_16 = 10;

    static const int MODULATION_APSK_32 = 11;

    static const int MODULATION_DQPSK   = 12;


    static const int AUDIO_MONO   = 0x0000;

    static const int AUDIO_STEREO = 0x0001;

    static const int AUDIO_LANG2  = 0x0002;

    static const int AUDIO_SAP    = 0x0002;

    static const int AUDIO_LANG1  = 0x0003;

    static const int AUDIO_LANG1_LANG2 = 0x0004;

    /**PAL B*/
    static const int STD_PAL_B     = 0x00000001;
    /**PAL B1*/
    static const int STD_PAL_B1    = 0x00000002;
    /**PAL G*/
    static const int STD_PAL_G     = 0x00000004;
    /**PAL H*/
    static const int STD_PAL_H     = 0x00000008;
    /**PAL I*/
    static const int STD_PAL_I     = 0x00000010;
    /**PAL D*/
    static const int STD_PAL_D     = 0x00000020;
    /**PAL D1*/
    static const int STD_PAL_D1    = 0x00000040;
    /**PAL K*/
    static const int STD_PAL_K     = 0x00000080;
    /**PAL M*/
    static const int STD_PAL_M     = 0x00000100;
    /**PAL N*/
    static const int STD_PAL_N     = 0x00000200;
    /**PAL Nc*/
    static const int STD_PAL_Nc    = 0x00000400;
    /**PAL 60*/
    static const int STD_PAL_60    = 0x00000800;
    /**NTSC M*/
    static const int STD_NTSC_M    = 0x00001000;
    /**NTSC M JP*/
    static const int STD_NTSC_M_JP = 0x00002000;
    /**NTSC 443*/
    static const int STD_NTSC_443  = 0x00004000;
    /**NTSC M KR*/
    static const int STD_NTSC_M_KR = 0x00008000;
    /**SECAM B*/
    static const int STD_SECAM_B   = 0x00010000;
    /**SECAM D*/
    static const int STD_SECAM_D   = 0x00020000;
    /**SECAM G*/
    static const int STD_SECAM_G   = 0x00040000;
    /**SECAM H*/
    static const int STD_SECAM_H   = 0x00080000;
    /**SECAM K*/
    static const int STD_SECAM_K   = 0x00100000;
    /**SECAM K1*/
    static const int STD_SECAM_K1  = 0x00200000;
    /**SECAM L*/
    static const int STD_SECAM_L   = 0x00400000;
    /**SECAM LC*/
    static const int STD_SECAM_LC  = 0x00800000;
    /**ATSC VSB8*/
    static const int STD_ATSC_8_VSB  = 0x01000000;
    /**ATSC VSB16*/
    static const int STD_ATSC_16_VSB = 0x02000000;
    /**NTSC*/
    static const int STD_NTSC      = STD_NTSC_M | STD_NTSC_M_JP | STD_NTSC_M_KR;
    /**SECAM DK*/
    static const int STD_SECAM_DK  = STD_SECAM_D | STD_SECAM_K | STD_SECAM_K1;
    /**SECAM*/
    static const int STD_SECAM     = STD_SECAM_B | STD_SECAM_G | STD_SECAM_H | STD_SECAM_DK | STD_SECAM_L | STD_SECAM_LC;
    /**PAL BG*/
    static const int STD_PAL_BG    = STD_PAL_B | STD_PAL_B1 | STD_PAL_G;
    /**PAL DK*/
    static const int STD_PAL_DK    = STD_PAL_D | STD_PAL_D1 | STD_PAL_K;
    /**PAL*/
    static const int STD_PAL       = STD_PAL_BG | STD_PAL_DK | STD_PAL_H | STD_PAL_I;

    //static const int TUNER_STD_MN  = STD_PAL_M|STD_PAL_N|STD_PAL_Nc| STD_NTSC
    static const int STD_B         = STD_PAL_B    | STD_PAL_B1   | STD_SECAM_B;
    static const int STD_GH        = STD_PAL_G    | STD_PAL_H   | STD_SECAM_G  | STD_SECAM_H;
    static const int STD_DK        = STD_PAL_DK   | STD_SECAM_DK;
    static const int STD_M         = STD_PAL_M    | STD_NTSC_M;
    static const int STD_BG        = STD_PAL_BG   | STD_SECAM_B | STD_SECAM_G ;

    static const int COLOR_AUTO  = 0x02000000;
    static const int COLOR_PAL   = 0x04000000;
    static const int COLOR_NTSC  = 0x08000000;
    static const int COLOR_SECAM = 0x10000000;

    static const int SAT_POLARISATION_H = 0;
    static const int SAT_POLARISATION_V = 1;

public:
    CTvChannel();
    CTvChannel(int dbID, int mode, int freq, int bw, int mod, int sym, int ofdm, int channelNum);
    ~CTvChannel();
    CTvChannel(const CTvChannel&);
    CTvChannel& operator= (const CTvChannel& cTvChannel);
    static Vector<CTvChannel> tvChannelList(int sat_id);
    static int selectByID(int id, CTvChannel &c);
    static int updateByID(int progID, int std, int freq, int fineFreq);
    static  int SelectByFreq(int freq, CTvChannel &channel);
    static int DeleteBetweenFreq(int beginFreq, int endFreq);
    static int CleanAllChannelBySrc(int src);
    static int getChannelListBySrc(int src, Vector< sp<CTvChannel> > &v_channel);
    void tvChannelDel();
    static void tvChannelDelBySatID(int id);
    int  getID()
    {
        return id;
    };
    int getDVBTSID();
    void getDVBOrigNetID();
    void getFrontendID();
    void getTSSourceID();
    void isDVBCMode();
    void setFrequency(int freq);
    int getFrequency()
    {
        return frequency;
    }
    int getSymbolRate()
    {
        return symbolRate;
    }
    int getModulation()
    {
        return modulation;
    }
    int getBandwidth()
    {
        return bandwidth;
    }
    int getMode()
    {
        return mode;
    }

    int getStd()
    {
        return standard;
    };
    int getAfcData()
    {
        return afc_data;
    };
    int getLogicalChannelNum()
    {
        return logicalChannelNum;
    };
    void setSymbolRate(int symbol);
    void setPolarisation();
    void setATVAudio();
    void setATVVideoFormat();
    void setATVAudioFormat();
    void setATVFreq();
    void setATVAfcData();
    void setModulation(int modulation)
    {
        this->modulation = modulation;
    }

    void setPhysicalNumDisplayName(String8 name)
    {
        this->physicalNumDisplayName = name;
    }

    String8 getPhysicalNumDisplayName()
    {
        return this->physicalNumDisplayName;
    }
    //
private:
    friend class LightRefBase<CTvChannel>;
    void createFromCursor(CTvDatabase::Cursor &c);

    //
    int id;
    int dvbTSID;
    int dvbOrigNetID;
    int fendID;
    int tsSourceID;

    int mode;
    int frequency;
    int symbolRate;
    int modulation;
    int bandwidth;
    int audio;
    int standard;
    int afc_data;
    int sat_id;
    int logicalChannelNum;
    //showboz
    //public TVSatelliteParams tv_satparams;
    int sat_polarisation;

    //add dtmb extral channel frequency
    String8 physicalNumDisplayName;
};

#endif  //_CTVCHANNEL_H

