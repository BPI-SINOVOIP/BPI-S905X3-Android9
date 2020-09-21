/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifdef SUPPORT_ADTV
#include <am_scan.h>
#include <am_check_scramb.h>
#include <am_epg.h>
#include <am_mem.h>
#include <am_db.h>
#include "am_cc.h"
#endif
#include "CTvChannel.h"
#include "CTvLog.h"
#include "CTvEv.h"
#include "tvin/CTvin.h"
#include "tv/CFrontEnd.h"

#include <list>

#ifndef SUPPORT_ADTV
typedef uint8_t        AM_Bool_t;
#define AM_TRUE        (1)
#define AM_FALSE       (0)
#endif

//must sync with dvb am_scan.h
enum
{
	TV_SCAN_ATVMODE_AUTO	= 0x01,	/**< auto scan mode*/
	TV_SCAN_ATVMODE_MANUAL	= 0x02,	/**< manual scan mode*/
	TV_SCAN_ATVMODE_FREQ	= 0x03,	/**< all band scan mode*/
	TV_SCAN_ATVMODE_NONE	= 0x07,	/**< none*/
};

enum
{
	TV_SCAN_DTVMODE_AUTO 			= 0x01,	/**< auto scan mode*/
	TV_SCAN_DTVMODE_MANUAL 			= 0x02,	/**< manual scan mode*/
	TV_SCAN_DTVMODE_ALLBAND 		= 0x03, /**< all band scan mode*/
	TV_SCAN_DTVMODE_SAT_BLIND		= 0x04,	/**< Satellite blind scan mode*/
	TV_SCAN_DTVMODE_NONE			= 0x07,	/**< none*/
	/* OR option(s)*/
	TV_SCAN_DTVMODE_SEARCHBAT		= 0x08, /**< Whether to search BAT table*/
	TV_SCAN_DTVMODE_SAT_UNICABLE	= 0x10,	/**< Satellite Unicable mode*/
	TV_SCAN_DTVMODE_FTA				= 0x20,	/**< Only scan free programs*/
	TV_SCAN_DTVMODE_NOTV			= 0x40, /**< Donot store tv programs*/
	TV_SCAN_DTVMODE_NORADIO			= 0x80, /**< Donot store radio programs*/
	TV_SCAN_DTVMODE_ISDBT_ONESEG	= 0x100, /**< Scan ISDBT oneseg in layer A*/
	TV_SCAN_DTVMODE_ISDBT_FULLSEG	= 0x200, /**< Scan ISDBT fullseg*/
	TV_SCAN_DTVMODE_SCRAMB_TSHEAD	= 0x400, /**< is check scramb by ts head*/
	TV_SCAN_DTVMODE_NOVCT			= 0x800, /**< Donot store in vct but not in pmt programs*/
	TV_SCAN_DTVMODE_NOVCTHIDE		= 0x1000, /**< Donot store in vct hide flag is set 1*/
};

enum
{
	TV_SCAN_DTV_STD_DVB		= 0x00,	/**< DVB standard*/
	TV_SCAN_DTV_STD_ATSC	= 0x01,	/**< ATSC standard*/
	TV_SCAN_DTV_STD_ISDB	= 0x02,	/**< ISDB standard*/
	TV_SCAN_DTV_STD_MAX,
};

enum {
	TV_FE_QPSK,
	TV_FE_QAM,
	TV_FE_OFDM,
	TV_FE_ATSC,
	TV_FE_ANALOG,
	TV_FE_DTMB,
	TV_FE_ISDBT
};

enum
{
	TV_SCAN_MODE_ATV_DTV,	/**< First search all ATV, then search all DTV*/
	TV_SCAN_MODE_DTV_ATV,	/**< First search all DTV, then search all ATV*/
	TV_SCAN_MODE_ADTV,		/**< A/DTV Use the same frequency table, search the A/DTV one by one In a frequency*/
};
//end

#if !defined(_CTVSCANNER_H)
#define _CTVSCANNER_H
class CTvScanner {
public:
    /** ATSC Attenna type */
    static const int AM_ATSC_ATTENNA_TYPE_AIR = 1;
    static const int AM_ATSC_ATTENNA_TYPE_CABLE_STD = 2;
    static const int AM_ATSC_ATTENNA_TYPE_CABLE_IRC = 3;
    static const int AM_ATSC_ATTENNA_TYPE_CABLE_HRC = 4;
    CTvScanner();
    ~CTvScanner();

    /*deprecated{{*/
    int ATVManualScan(int min_freq, int max_freq, int std, int store_Type = 0, int channel_num = 0);
    int autoAtvScan(int min_freq, int max_freq, int std, int search_type);
    int autoAtvScan(int min_freq, int max_freq, int std, int search_type, int proc_mode);
    int autoDtmbScan();
    int manualDtmbScan(int beginFreq, int endFreq, int modulation = -1);
    int manualDtmbScan(int freq);
    int unsubscribeEvent();
    int dtvAutoScan(int mode);
    int dtvManualScan(int mode, int beginFreq, int endFreq, int para1, int para2);
    int dtvScan(int mode, int scan_mode, int beginFreq, int endFreq, int para1, int para2);
    int dtvScan(char *feparas, int scan_mode, int beginFreq, int endFreq);
    /*}}deprecated*/

    int Scan(char *feparas, char *scanparas);
    int stopScan();
    int pauseScan();
    int resumeScan();
    int getScanStatus(int *status);

    static const char *getDtvScanListName(int mode);
    static CTvScanner *getInstance();

    void SetCurrentLanguage(std::string lang)
    {
        mCurrentSystemLang = lang;
    }

    std::string GetCurrentLanguage()
    {
        return mCurrentSystemLang;
    }

    struct ScannerLcnInfo {

        public:
        #define MAX_LCN 4
        int net_id;
        int ts_id;
        int service_id;
        int visible[MAX_LCN];
        int lcn[MAX_LCN];
        int valid[MAX_LCN];
    };

    typedef std::list<ScannerLcnInfo*> lcn_list_t;


    class ScannerEvent: public CTvEv {
    public:
        static const int EVENT_SCAN_PROGRESS = 0;
        static const int EVENT_STORE_BEGIN   = 1;
        static const int EVENT_STORE_END     = 2;
        static const int EVENT_SCAN_END     = 3;
        static const int EVENT_BLINDSCAN_PROGRESS = 4;
        static const int EVENT_BLINDSCAN_NEWCHANNEL = 5;
        static const int EVENT_BLINDSCAN_END    = 6;
        static const int EVENT_ATV_PROG_DATA = 7;
        static const int EVENT_DTV_PROG_DATA = 8;
        static const int EVENT_SCAN_EXIT     = 9;
        static const int EVENT_SCAN_BEGIN    = 10;
        static const int EVENT_LCN_INFO_DATA = 11;

        ScannerEvent(): CTvEv(CTvEv::TV_EVENT_SCANNER)
        {
            reset();
            mPercent           = 0;
            mTotalChannelCount = 0;
            mLockedStatus      = 0;
            mChannelIndex      = 0;
            mMode              = 0;
            mFrequency         = 0;
            mSymbolRate        = 0;
        }
        void reset()
        {
            clear();
            mFEParas.clear();
        }
        void clear()
        {
            mType = -1;
            mProgramName[0] = '\0';
            mParas[0] = '\0';
            mAcnt = 0;
            mScnt = 0;
            mVctType = 0;
            mVct[0] = '\0';

            memset(&mLcnInfo, 0, sizeof(ScannerLcnInfo));

            mMajorChannelNumber = -1;
            mMinorChannelNumber = -1;

            mModulation = 0;
            mBandwidth = 0;
            mReserved = 0;
            mSat_polarisation = 0;
            mStrength = 0;
            mSnr = 0;
            mprogramType = 0;
            mVideoStd = 0;
            mAudioStd = 0;
            mIsAutoStd = 0;
            mTsId = 0;
            mONetId = 0;
            mServiceId = 0;
            mVid = 0;
            mVfmt = 0;
            mPcr = 0;
            mFree_ca = 0;
            mScrambled = 0;
            mScanMode = 0;
            mSdtVer = 0;
            mSortMode = 0;
            mSourceId = 0;
            mAccessControlled = 0;
            mHidden = 0;
            mHideGuide = 0;
        }
        ~ScannerEvent()
        {
        }

        //common
        int mType;
        int mPercent;
        int mTotalChannelCount;
        int mLockedStatus;
        int mChannelIndex;

        int mMode;
        long mFrequency;
        int mSymbolRate;
        int mModulation;
        int mBandwidth;
        int mReserved;

        int mSat_polarisation;
        int mStrength;
        int mSnr;

        char mProgramName[1024];
        int mprogramType;
        char mParas[128];

        //for atv
        int mVideoStd;
        int mAudioStd;
        int mIsAutoStd;

        //for DTV
        int mTsId;
        int mONetId;
        int mServiceId;
        int mVid;
        int mVfmt;
        int mAcnt;
        int mAid[32];
        int mAfmt[32];
        char mAlang[32][10];
        int mAtype[32];
        int mAExt[32];
        int mPcr;

        int mScnt;
        int mStype[32];
        int mSid[32];
        int mSstype[32];
        int mSid1[32];
        int mSid2[32];
        char mSlang[32][10];

        int mFree_ca;
        int mScrambled;

        int mScanMode;

        int mSdtVer;

        int mSortMode;

        ScannerLcnInfo  mLcnInfo;

        CFrontEnd::FEParas mFEParas;

        int mMajorChannelNumber;
        int mMinorChannelNumber;
        int mSourceId;
        char mAccessControlled;
        char mHidden;
        char mHideGuide;
        char mVctType;
        char mVct[1024];
        int mProgramsInPat;
        int mPatTsId;
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const ScannerEvent &ev) = 0;
    };
    // 1 VS n
    //int addObserver(IObserver* ob);
    //int removeObserver(IObserver* ob);

    // 1 VS 1
    int setObserver(IObserver *ob)
    {
        mpObserver = ob;
        return 0;
    }

    class ScanParas : public Paras {

    public:
        ScanParas() : Paras() { }
        ScanParas(const ScanParas &sp) : Paras(sp.mparas) { }
        ScanParas(const char *paras) : Paras(paras) { }
        ScanParas& operator = (const ScanParas &spp);

        int getMode() const { return getInt(SCP_MODE, 0); }
        ScanParas& setMode(int m) { setInt(SCP_MODE, m); return *this; }
        int getAtvMode() const { return getInt(SCP_ATVMODE, 0); }
        ScanParas& setAtvMode(int a) { setInt(SCP_ATVMODE, a); return *this; }
        int getDtvMode() const { return getInt(SCP_DTVMODE, 0); }
        ScanParas& setDtvMode(int d) { setInt(SCP_DTVMODE, d); return *this; }
        ScanParas& setAtvFrequency1(int f) { setInt(SCP_ATVFREQ1, f); return *this; }
        int getAtvFrequency1() const { return getInt(SCP_ATVFREQ1, 0); }
        ScanParas& setAtvFrequency2(int f) { setInt(SCP_ATVFREQ2, f); return *this; }
        int getAtvFrequency2() const { return getInt(SCP_ATVFREQ2, 0); }
        ScanParas& setDtvFrequency1(int f) { setInt(SCP_DTVFREQ1, f); return *this; }
        int getDtvFrequency1() const { return getInt(SCP_DTVFREQ1, 0); }
        ScanParas& setDtvFrequency2(int f) { setInt(SCP_DTVFREQ2, f); return *this; }
        int getDtvFrequency2() const { return getInt(SCP_DTVFREQ2, 0); }
        ScanParas& setProc(int p) { setInt(SCP_PROC, p); return *this; }
        int getProc() const { return getInt(SCP_PROC, 0); }
        ScanParas& setDtvStandard(int s) { setInt(SCP_DTVSTD, s); return *this; }
        int getDtvStandard() const { return getInt(SCP_DTVSTD, -1); }

        ScanParas&  setAtvModifier(const char* name, int m) { char n[32] = {0}; snprintf(n, 32, "%s_amod", name); setInt(n, m); return *this; }
        int getAtvModifier(const char* name, int def) { char n[32] = {0}; snprintf(n, 32, "%s_amod", name); return getInt(n, def); }
        ScanParas&  setDtvModifier(const char* name, int m) { char n[32] = {0}; snprintf(n, 32, "%s_dmod", name); setInt(n, m); return *this; }
        int getDtvModifier(const char* name, int def) { char n[32] = {0}; snprintf(n, 32, "%s_dmod", name); return getInt(n, def); }

    public:
        static const char* SCP_MODE;
        static const char* SCP_ATVMODE;
        static const char* SCP_DTVMODE;
        static const char* SCP_ATVFREQ1;
        static const char* SCP_ATVFREQ2;
        static const char* SCP_DTVFREQ1;
        static const char* SCP_DTVFREQ2;
        static const char* SCP_PROC;
        static const char* SCP_DTVSTD;
    };

    int Scan(CFrontEnd::FEParas &fp, ScanParas &sp);

private:
    AM_Bool_t checkAtvCvbsLock(unsigned long  *colorStd);
    static AM_Bool_t checkAtvCvbsLockHelper(void *);

    static void evtCallback(long dev_no, int event_type, void *param, void *data);

    void reconnectDmxToFend(int dmx_no, int fend_no);
    int getAtscChannelPara(int attennaType, Vector<sp<CTvChannel> > &vcp);
    void sendEvent(ScannerEvent &evt);
    //
    void* mScanHandle;
    volatile bool mbScanStart;

    //scan para info
    /** General TV Scan Mode */
    static const int TV_MODE_ATV = 0;   // Only search ATV
    static const int TV_MODE_DTV = 1;   // Only search DTV
    static const int TV_MODE_ADTV = 2;  // A/DTV will share a same frequency list, like ATSC
    /** DTV scan mode */
    static const int DTV_MODE_AUTO   = 1;
    static const int DTV_MODE_MANUAL = 2;
    static const int DTV_MODE_ALLBAND = 3;
    static const int DTV_MODE_BLIND  = 4;

    /** DTV scan options, DONOT channge */
    static const int DTV_OPTION_UNICABLE = 0x10;      //Satellite unicable mode
    static const int DTV_OPTION_FTA      = 0x20;      //Only store free programs
    static const int DTV_OPTION_NO_TV    = 0x40;      //Only store tv programs
    static const int DTV_OPTION_NO_RADIO = 0x80;      //Only store radio programs

    /** ATV scan mode */
    static const int ATV_MODE_AUTO   = 1;
    static const int ATV_MODE_MANUAL = 2;

    //
    /*subtitle*/
    static const int TYPE_DVB_SUBTITLE = 1;
    static const int TYPE_DTV_TELETEXT = 2;
    static const int TYPE_ATV_TELETEXT = 3;
    static const int TYPE_DTV_CC = 4;
    static const int TYPE_ATV_CC = 5;
    static const int TYPE_DTV_TELETEXT_IMG = 6;
    static const int TYPE_ISDB = 7;
    static const int TYPE_SCTE27 = 8;

    typedef struct {
        int nid;
        int tsid;
        int pat_ts_id;
        CFrontEnd::FEParas fe;
        int dtvstd;
        char vct[1024];
    } SCAN_TsInfo_t;

    typedef std::list<SCAN_TsInfo_t*> ts_list_t;

#define AM_SCAN_MAX_SRV_NAME_LANG 4
#define AM_DB_MAX_SRV_NAME_LEN 64

    typedef struct {
        uint8_t srv_type, eit_sche, eit_pf, rs, free_ca;
        uint8_t access_controlled, hidden, hide_guide;
        uint8_t plp_id, vct_type;
        int vid, vfmt, srv_id, pmt_pid, pcr_pid, src;
        int chan_num, scrambled_flag;
        int major_chan_num, minor_chan_num, source_id;
        char name[(AM_DB_MAX_SRV_NAME_LEN + 4)*AM_SCAN_MAX_SRV_NAME_LANG + 1];
    #ifdef SUPPORT_ADTV
        AM_SI_AudioInfo_t aud_info;
        AM_SI_SubtitleInfo_t sub_info;
        AM_SI_TeletextInfo_t ttx_info;
        AM_SI_CaptionInfo_t cap_info;
        AM_SI_Scte27SubtitleInfo_t scte27_info;
    #endif
        int sdt_version;
        SCAN_TsInfo_t *tsinfo;
        int programs_in_pat;
    } SCAN_ServiceInfo_t;

    typedef std::list<SCAN_ServiceInfo_t*> service_list_t;

#ifdef SUPPORT_ADTV
    dvbpsi_pat_t *getValidPats(AM_SCAN_TS_t *ts);
    int getPmtPid(dvbpsi_pat_t *pats, int pn);
    void extractCaScrambledFlag(dvbpsi_descriptor_t *p_first_descriptor, int *flag);
    void extractSrvInfoFromSdt(AM_SCAN_Result_t *result, dvbpsi_sdt_t *sdts, SCAN_ServiceInfo_t *service);
    void extractSrvInfoFromVc(AM_SCAN_Result_t *result, dvbpsi_atsc_vct_channel_t *vcinfo, SCAN_ServiceInfo_t *service);
    void updateServiceInfo(AM_SCAN_Result_t *result, SCAN_ServiceInfo_t *service);
    void getLcnInfo(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, lcn_list_t &llist);
    void addFixedATSCCaption(AM_SI_CaptionInfo_t *cap_info, int service, int cc, int text, int is_digital_cc);

    int checkIsSkipProgram(SCAN_ServiceInfo_t *srv_info, int mode);
    void processDvbTs(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, service_list_t &slist);
    void processAnalogTs(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, SCAN_TsInfo_t *tsinfo, service_list_t &slist);
    void processAtscTs(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, SCAN_TsInfo_t *tsinfo, service_list_t &slist);
    void processTsInfo(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, SCAN_TsInfo_t *info);
    void storeNewDvb(AM_SCAN_Result_t *result, AM_SCAN_TS_t *newts);
    void storeNewAnalog(AM_SCAN_Result_t *result, AM_SCAN_TS_t *newts);
    void storeNewAtsc(AM_SCAN_Result_t *result, AM_SCAN_TS_t *newts);
    void storeScan(AM_SCAN_Result_t *result, AM_SCAN_TS_t *curr_ts);


    int createAtvParas(AM_SCAN_ATVCreatePara_t &atv_para, CFrontEnd::FEParas &fp, ScanParas &sp);
    int freeAtvParas(AM_SCAN_ATVCreatePara_t &atv_para);
    int createDtvParas(AM_SCAN_DTVCreatePara_t &dtv_para, CFrontEnd::FEParas &fp, ScanParas &sp);
    int freeDtvParas(AM_SCAN_DTVCreatePara_t &dtv_para);
    void CC_VBINetworkCb(void* handle, vbi_network *n);
    static void storeScanHelper(AM_SCAN_Result_t *result);
    static void CC_VBINetworkCbHelper(void* handle, vbi_network *n);
#endif
    SCAN_ServiceInfo_t* getServiceInfo();
    void notifyService(SCAN_ServiceInfo_t *service);
    void notifyLcn(ScannerLcnInfo *lcn);
    int insertLcnList(lcn_list_t &llist, ScannerLcnInfo *lcn, int idx);
    int getParamOption(const char *para);
    int getScanDtvStandard(ScanParas &scp);
    bool needVbiAssist();
    bool checkVbiDataReady(int freq);
    int startVBI();
    void stopVBI();
    void resetVBI();
    int FETypeHelperCB(int id, void *para, void *user);
    static int FETypeHelperCBHelper(int id, void *para, void *user);


private:
    static CTvScanner *mInstance;

    IObserver *mpObserver;
    //
    CTvin *mpTvin;
    int mMode;
    int mFendID;
    /** DTV parameters */
    int mTvMode;
    int mTvOptions;
    int mSat_id;
    int mSource;

    //showboz
    //TVSatelliteParams tv_satparams;
    int mTsSourceID;
    CTvChannel mStartChannel;
    Vector<CTvChannel> mvChooseListChannels;
    /** ATV parameters */
    int mAtvMode;
    int mStartFreq;
    int mDirection;
    int mChannelID;

    //extern for scanner
    //int channelID; //can be used for manual scan
    /** Atv set */
    int mMinFreq;
    int mMaxFreq;
    long long mCurScanStartFreq;
    long long mCurScanEndFreq;
    int tunerStd;
    /** Tv set */
    int demuxID;//default 0
    String8 defaultTextLang;
    String8 orderedTextLangs;
    //showboz
    //Vector<CTvChannel> ChannelList;//VS mvChooseListChannels

    /** Dtv-Sx set Unicable settings*/
    int user_band;
    int ub_freq;//!< kHz

    CFrontEnd::FEParas mFEParas;
    ScanParas mScanParas;

    std::string mCurrentSystemLang;
    int mFEType;

    static ScannerEvent mCurEv;

    static service_list_t service_list_dummy;

    void* mVbi;
    int mVbiTsId;
    int mAtvIsAtsc;

    typedef struct {
        int freq;
        int tsid;
    }SCAN_TsIdInfo_t;
    typedef std::list<SCAN_TsIdInfo_t*> tsid_list_t;
    tsid_list_t tsid_list;

};
#endif //CTVSCANNER_H
