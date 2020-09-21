/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_AV_H
#define _C_AV_H

#ifdef SUPPORT_ADTV
#include "am_av.h"
#include "am_aout.h"
#endif
#include "CTvEv.h"
#include "CTvLog.h"
#include "../tvin/CTvin.h"

static const char *PATH_FRAME_COUNT_DI  = "/sys/module/di/parameters/frame_count";
static const char *PATH_FRAME_COUNT_VIDEO = "/sys/module/amvideo/parameters/new_frame_count";
static const char *PATH_FRAME_COUNT = PATH_FRAME_COUNT_VIDEO;

static const char *PATH_VIDEO_AMVIDEO = "/dev/amvideo";
static const char *PATH_MEPG_DTMB_LOOKUP_PTS_FLAG = "/sys/module/amvdec_mpeg12/parameters/dtmb_flag";

#define VIDEO_RGB_SCREEN    "/sys/class/video/rgb_screen"
#define VIDEO_TEST_SCREEN   "/sys/class/video/test_screen"
#define VIDEO_DISABLE_VIDEO "/sys/class/video/disable_video"
#define VIDEO_SCREEN_MODE   "/sys/class/video/screen_mode"
#define VIDEO_AXIS          "/sys/class/video/axis"
#define VIDEO_DEVICE_RESOLUTION "/sys/class/video/device_resolution"
#define VIDEO_SYNC_ENABLE   "/sys/class/tsync/enable"
#define VIDEO_SYNC_MODE     "/sys/class/tsync/mode"

//must sync with am_aout.h
enum
{
    TV_AOUT_OUTPUT_STEREO,     /**< Stereo output*/
    TV_AOUT_OUTPUT_DUAL_LEFT,  /**< Left audio output to dual channel*/
    TV_AOUT_OUTPUT_DUAL_RIGHT, /**< Right audio output to dual channel*/
    TV_AOUT_OUTPUT_SWAP        /**< Swap left and right channel*/
};

enum
{
    TV_AV_TS_SRC_TS0,                    /**< TS input port 0*/
    TV_AV_TS_SRC_TS1,                    /**< TS input port 1*/
    TV_AV_TS_SRC_TS2,                    /**< TS input port 2*/
    TV_AV_TS_SRC_HIU,                    /**< HIU port (file input)*/
    TV_AV_TS_SRC_DMX0,                   /**< Demux 0*/
    TV_AV_TS_SRC_DMX1,                   /**< Demux 1*/
    TV_AV_TS_SRC_DMX2                    /**< Demux 2*/
};
//end

enum {
    VIDEO_LAYER_NONE                    = -1,
    VIDEO_LAYER_ENABLE                  = 0,
    VIDEO_LAYER_DISABLE_BLACK           = 1,
    VIDEO_LAYER_DISABLE_BLUE            = 2
};

enum {
    ENABLE_VIDEO_LAYER,
    DISABLE_VIDEO_LAYER,
    ENABLE_AND_CLEAR_VIDEO_LAYER
};

typedef enum video_display_resolution_e {
    VPP_DISPLAY_RESOLUTION_1366X768,
    VPP_DISPLAY_RESOLUTION_1920X1080,
    VPP_DISPLAY_RESOLUTION_3840X2160,
    VPP_DISPLAY_RESOLUTION_MAX,
} video_display_resolution_t;


class CAv {
public:
    CAv();
    ~CAv();
    //video screen_mode
    static const int VIDEO_WIDEOPTION_NORMAL           = 0;
    static const int VIDEO_WIDEOPTION_FULL_STRETCH     = 1;
    static const int VIDEO_WIDEOPTION_4_3              = 2;
    static const int VIDEO_WIDEOPTION_16_9             = 3;
    static const int VIDEO_WIDEOPTION_NONLINEAR        = 4;
    static const int VIDEO_WIDEOPTION_NORMAL_NOSCALEUP = 5;
    static const int VIDEO_WIDEOPTION_CROP_FULL        = 6;
    static const int VIDEO_WIDEOPTION_CROP             = 7;
    //
    class AVEvent : public CTvEv {
    public:
        AVEvent(): CTvEv(CTvEv::TV_EVENT_AV)
        {
            type = 0;
            param = 0;
            param1 = 0;
            status = 0;
        };
        ~AVEvent()
        {};
        static const int EVENT_AV_STOP             = 1;
        static const int EVENT_AV_RESUEM           = 2;
        static const int EVENT_AV_SCAMBLED         = 3;
        static const int EVENT_AV_UNSUPPORT        = 4;
        static const int EVENT_AV_VIDEO_AVAILABLE  = 5;
        static const int EVENT_PLAY_UPDATE  = 6;
        static const int EVENT_AUDIO_CB  = 7;
        std::string player;
        int type;
        long param;
        long param1;
        int status;
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const AVEvent &ev) = 0;
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

    int Open();
    int Close();
    int SetVideoWindow(int x, int y, int w, int h);
    int GetVideoStatus(int *w, int *h, int *fps, int *interlace);
    int GetAudioStatus( int fmt[2], int sample_rate[2], int resolution[2], int channels[2],
    int lfepresent[2], int *frames, int *ab_size, int *ab_data, int *ab_free);
    int SwitchTSAudio(int apid, int afmt);
    int ResetAudioDecoder();
    int SetADAudio(unsigned int enable, int apid, int afmt);
    int SetTSSource(int ts_source);
    int StartTS(unsigned short vpid, unsigned short apid, unsigned short pcrid, int vfmt, int afmt);
    int StopTS();
    int AudioGetOutputMode(int *mode);
    int AudioSetOutputMode(int mode);

    /*TimeShifting*/
    int startTimeShift(void *para);
    int stopTimeShift();
    int playTimeShift();
    int pauseTimeShift();
    int resumeTimeShift();
    int seekTimeShift(int pos, bool start);
    int setTimeShiftSpeed(int speed);
    int switchTimeShiftAudio(int apid, int afmt);

    int AudioSetPreGain(float pre_gain);
    int AudioGetPreGain(float *gain);
    int AudioSetPreMute(unsigned int mute);
    int AudioGetPreMute(unsigned int *mute);
    int SetVideoScreenColor (int vdin_blending_mask, int y, int u, int v);
    int DisableVideoWithBlueColor();
    int DisableVideoWithBlackColor();
    int EnableVideoAuto();
    int EnableVideoNow(bool IsShowTestScreen);
    int EnableVideoWhenVideoPlaying(int minFrameCount = 8, int waitTime = 5000);
    int WaittingVideoPlaying(int minFrameCount = 8, int waitTime = 5000);
    int EnableVideoBlackout();
    int DisableVideoBlackout();
    int SetVideoLayerStatus ( int value );
    int ClearVideoBuffer();
    bool videoIsPlaying(int minFrameCount = 8);
    int setVideoScreenMode ( int value );
    int getVideoScreenMode();
    int setVideoAxis ( int h, int v, int width, int height );
    video_display_resolution_t getVideoDisplayResolution();
    int setRGBScreen(int r, int g, int b);
    int getRGBScreen();

    int setLookupPtsForDtmb(int enable);
    tvin_sig_fmt_t getVideoResolutionToFmt();
    int getVideoFrameCount();
private:
    static void av_evt_callback ( long dev_no, int event_type, void *param, void *user_data );
#ifdef SUPPORT_ADTV
    static void av_audio_callback(int event_type, AudioParms* param, void *user_data);
#endif
    int mTvPlayDevId;
    IObserver *mpObserver;
    AVEvent mCurAvEvent;
    int mVideoLayerState;

    int mFdAmVideo;
};
#endif
