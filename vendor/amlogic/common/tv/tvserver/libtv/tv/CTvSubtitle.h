/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVSUBTITLE_H)
#define _CTVSUBTITLE_H
#include <stdlib.h>
#include <CTvLog.h>
//using namespace android;

#ifdef SUPPORT_ADTV
#include "am_cc.h"
#include "am_sub2.h"
#include "am_pes.h"
#endif
#include "CTvEv.h"
#include <tvutils.h>

enum cc_param_country {
    CC_PARAM_COUNTRY_USA = 0,
    CC_PARAM_COUNTRY_KOREA,
};

enum cc_param_source_type {
    CC_PARAM_SOURCE_VBIDATA = 0,
    CC_PARAM_SOURCE_USERDATA,
};

enum cc_param_caption_type {
    CC_PARAM_ANALOG_CAPTION_TYPE_CC1 = 0,
    CC_PARAM_ANALOG_CAPTION_TYPE_CC2,
    CC_PARAM_ANALOG_CAPTION_TYPE_CC3,
    CC_PARAM_ANALOG_CAPTION_TYPE_CC4,
    CC_PARAM_ANALOG_CAPTION_TYPE_TEXT1,
    CC_PARAM_ANALOG_CAPTION_TYPE_TEXT2,
    CC_PARAM_ANALOG_CAPTION_TYPE_TEXT3,
    CC_PARAM_ANALOG_CAPTION_TYPE_TEXT4,
    //
    CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE1,
    CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE2,
    CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE3,
    CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE4,
    CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE5,
    CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE6,
};

class CTvSubtitle {
public:
    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        //virtual void onEvent(const CloseCaptionEvent &ev);
        virtual void updateSubtitle(int, int) {};
    };
    void* sub_handle;
    void* pes_handle;

    int              dmx_id;
    int              filter_handle;
    int              bmp_w;
    int              bmp_h;
    int              bmp_pitch;
    unsigned char           *buffer;
    int             sub_w;
    int             sub_h;
    pthread_mutex_t  lock;

    IObserver *mpObser;
    CTvSubtitle();
    ~CTvSubtitle();

    class CloseCaptionEvent: public CTvEv {
    public:
        //static const int CC_CMD_LEN  = 128;
        //static const int CC_DATA_LEN = 512;
        CloseCaptionEvent(): CTvEv(CTvEv::TV_EVENT_CC)
        {
            mCmdBufSize = 0;
            mpCmdBuffer = NULL;
            mDataBufSize = 0;
            mpDataBuffer = NULL;
        }
        ~CloseCaptionEvent()
        {
        }
    public:
        int mCmdBufSize;
        int *mpCmdBuffer;
        int mDataBufSize;
        int *mpDataBuffer;
    };

    void setObserver(IObserver *pObser);
    void setBuffer(char *share_mem);
    void stopDecoder();
    void startSub();
    void stop();
    void clear();
    void nextPage();
    void previousPage();
    void gotoPage(int page);
    void goHome();
    void colorLink(int color);
    void setSearchPattern(char *pattern, bool casefold);
    void searchNext();
    void searchPrevious();

    int sub_init(int, int);
    //
    int sub_destroy();
    //
    int sub_lock();
    //
    int sub_unlock();
    //
    int sub_clear();
    //
    int sub_switch_status();
    int sub_start_dvb_sub(int dmx_id, int pid, int page_id, int anc_page_id);
    //
    int sub_start_dtv_tt(int dmx_id, int region_id, int pid, int page, int sub_page, bool is_sub);
    //
    int sub_stop_dvb_sub();
    //
    int sub_stop_dtv_tt();
    //
    int sub_tt_goto(int page);
    //
    int sub_tt_color_link(int color);
    //
    int sub_tt_home_link();
    //
    int sub_tt_next(int dir);
    //
    int sub_tt_set_search_pattern(char *pattern, bool casefold);
    //
    int sub_tt_search(int dir);
    //
    int sub_start_atsc_cc(enum cc_param_country country, enum cc_param_source_type src_type, int channel_num, enum cc_param_caption_type caption_type);
    //
    int sub_stop_atsc_cc();
    static void close_caption_callback(char *str, int cnt, int data_buf[], int cmd_buf[], void *user_data);
    static void atv_vchip_callback(int Is_chg,  void *user_data);
    int IsVchipChange();
    int ResetVchipChgStat();
private:

    struct DVBSubParams {
        int mDmx_id;
        int mPid;
        int mComposition_page_id;
        int mAncillary_page_id;

        DVBSubParams()
        {
        }
        DVBSubParams(int dmx_id, int pid, int page_id, int anc_page_id)
        {
            mDmx_id              = dmx_id;
            mPid                 = pid;
            mComposition_page_id = page_id;
            mAncillary_page_id   = anc_page_id;
        }
    };

    struct DTVTTParams {
        int mDmx_id;
        int mPid;
        int mPage_no;
        int mSub_page_no;
        int mRegion_id;

        DTVTTParams()
        {
        }
        DTVTTParams(int dmx_id, int pid, int page_no, int sub_page_no, int region_id)
        {
            mDmx_id      = dmx_id;
            mPid         = pid;
            mPage_no     = page_no;
            mSub_page_no = sub_page_no;
            mRegion_id   = region_id;
        }
    };

    int mSubType;
    CloseCaptionEvent mCurCCEv;
    int avchip_chg;
    bool isSubOpen;
};
#endif  //_CTVSUBTITLE_H
