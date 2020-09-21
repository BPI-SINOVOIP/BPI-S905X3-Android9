/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#ifdef SUPPORT_ADTV
#include <am_epg.h>
#endif
#include "CTvEv.h"
#include "CTvLog.h"
#include <tvutils.h>

#if !defined(_CDTVRRT_H)
#define _CDTVRRT_H

#if ANDROID_PLATFORM_SDK_VERSION >= 28
#define TV_RRT_DEFINE_PARAM_PATH          "/mnt/vendor/param/tv_rrt_define.xml"
#else
#define TV_RRT_DEFINE_PARAM_PATH          "/param/tv_rrt_define.xml"
#endif

typedef struct rrt_info
{
    unsigned short rating_region;
    int  dimensions_id;
    int  rating_value_id;
    char rating_region_name[2048];
    char dimensions_name[2048];
    char abbrev_rating_value_text[2048];
    char rating_value_text[2048];
} rrt_info_t;

typedef struct rrt_select_info_s
{
    int  status;
    int  rating_region_name_count;
    char rating_region_name[2048];
    int  dimensions_name_count;
    char dimensions_name[2048];
    int  rating_value_text_count;
    char rating_value_text[2048];
} rrt_select_info_t;

typedef enum rrt_search_mode_e
{
    RRT_AUTO_SEARCH = 0,
    RRT_MANU_SEARCH,
} rrt_search_mode_t;

class CTvRrt
{
public:
    static const int MODE_ADD            = 0;
    static const int MODE_REMOVE         = 1;

    static const int SCAN_RRT            = 0x2000;
    static const int INVALID_ID          = -1;

    class RrtEvent : public CTvEv {
        public:
            RrtEvent(): CTvEv(CTvEv::TV_EVENT_RRT)
            {
                satus = 0;
            };
            ~RrtEvent()
            {
            };

            static const int EVENT_RRT_SCAN_START          = 1;
            static const int EVENT_RRT_SCAN_SCANING        = 2;
            static const int EVENT_RRT_SCAN_END            = 3;

            int satus;
        };

        class IObserver {
        public:
            IObserver() {};
            virtual ~IObserver() {};
            virtual void onEvent(const RrtEvent &ev) = 0;
        };

        int setObserver(IObserver *ob)
        {
            mpObserver = ob;
            return 0;
        }

public:
    static CTvRrt *getInstance();
    CTvRrt();
    ~CTvRrt();
    int StartRrtUpdate(rrt_search_mode_t mode);
    int StopRrtUpdate(void);
    int GetRRTRating(int rating_region_id, int dimension_id, int value_id, int program_id, rrt_select_info_t *ret);

    int mRrtScanStatus;
    int mScanResult;
    int mDmx_id ;
    int mLastRatingRegion;
    int mLastDimensionsDefined;
    int mLastVersion;
    void * mRrtScanHandle = nullptr;

private:
    int RrtCreate(int fend_id, int dmx_id, int src, char * textLangs);
    int RrtDestroy();
    int RrtChangeMode(int op, int mode);
    int RrtScanStart(void);
    int RrtScanStop(void);

    static void RrtTableCallback(void * dev_no, int event_type, void *param, void *user_data);
    void RrtDataUpdate(void * dev_no, int event_type, void *param, void *user_data);

#ifdef SUPPORT_ADTV
    void MultipleStringParser(atsc_multiple_string_t &atsc_multiple_string, char *ret);
#endif

    bool RrtUpdataCheck(int rating_region, int dimensions_defined, int version_number);
    static CTvRrt *mInstance;
    IObserver *mpObserver;
    RrtEvent mCurRrtEv;
};


#endif //_CDTVRRT_H

