#ifndef _CTSPLAYER_IMPL_H_
#define _CTSPLAYER_IMPL_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <gui/Surface.h>
#include <utils/KeyedVector.h>

extern "C" {
#include <amports/vformat.h>
#include <amports/aformat.h>
#include <amports/amstream.h>
#include <codec.h>
#include <codec_info.h>
#include <list.h>
#include <codec_videoinfo.h>
}
#include <string.h>
#include <utils/Timers.h>
#include <ITsPlayer.h>


using namespace android;

#define lock_t          pthread_mutex_t
#define lp_lock_init(x,v)   pthread_mutex_init(x,v)
#define lp_lock_deinit(x)   pthread_mutex_destroy(x)
#define lp_lock(x)      pthread_mutex_lock(x)
#define lp_trylock(x)   pthread_mutex_trylock(x)
#define lp_unlock(x)    pthread_mutex_unlock(x)



#define IN
#define OUT

#define TS_PACKET_SIZE 188
#define PROCESS_NAME 64
typedef struct{
	void *(*init)(IN int flags);
  int (*read)(IN void *handle, IN unsigned char *buf, IN int size);
  int (*ready)(IN void *handle);
  int (*exit)(IN void *handle);
}wiptv_callback;


#define TRICKMODE_NONE       0x00
#define TRICKMODE_I          0x01
#define TRICKMODE_I_HEVC     0x07
#define TRICKMODE_FFFB       0x02


typedef struct CTsParameter {
    int mMultiSupport;
};

typedef struct {
    struct list_head list;
    unsigned char* tmpbuffer;
    unsigned int size;
}V_HEADER_T, *PV_HEADER_T;

#ifndef AVCODEC_AVCODEC_H
#endif

typedef struct ST_LPbuffer{
    unsigned char *rp;
    unsigned char *wp;
    unsigned char *buffer;
    unsigned char *bufferend;
    int valid_can_read;
    bool enlpflag;
}LPBUFFER_T;

typedef struct ctsplayer_state {
    int valid;

    // setting
    int64_t last_update_time;
    /*for caton info*/
    struct codec_quality_info quality_info;
    int catoning;
    int caton_start_underflow;
    int64_t caton_start_time;
    /*for calc avg stream bitrate*/
    int64_t bytes_record_starttime_ms;
    int64_t bytes_record_start;
    int64_t bytes_record_cur;

    // player info
    int first_picture_comming;
    int64_t first_frame_pts;
    int64_t avdiff;
    int ff_mode;
    int ts_error;
    int ts_sync_loss;
    int ecm_error;

    // video info
    int64_t vpts;
    int video_ratio;
    int video_rWH = 0;
    int Video_frame_format = 0;
    int video_width;
    int video_height;
    int vbuf_size;
    int vbuf_used;
    int vdec_total;
    int vdec_error;
    int vdec_drop;
    int vdec_underflow;
    int vpts_error;
    int frame_rate;
    int current_fps;

    // audio info
    int64_t apts;
    int abuf_size;
    int abuf_used;
    int adec_error;
    int adec_drop;
    int adec_underflow;
    int apts_error;

    //audio decode info
    int samplate;
    int channel;
    int bitrate;
    int audio_bps;
    int audio_type;

    //other info
    int stream_bitrate;//avg from writedata, duration 1 sec
    int caton_times; //the num of carton
    int caton_time;
    int stream_bps;//avg dev video bitrate
};





typedef enum {
    VIDEO_FRAME_TYPE_UNKNOWN = 0,
    VIDEO_FRAME_TYPE_I,
    VIDEO_FRAME_TYPE_P,
    VIDEO_FRAME_TYPE_B,
    VIDEO_FRAME_TYPE_IDR,
    VIDEO_FRAME_TYPE_BUTT,
}VID_FRAME_TYPE_e;

typedef struct VIDEO_FRM_STATUS_INFO {
    VID_FRAME_TYPE_e enVidFrmType;
    int  nVidFrmSize;
    int  nMinQP;
    int  nMaxQP;
    int  nAvgQP;
    int  nMaxMV;
    int  nMinMV;
    int  nAvgMV;
    int  SkipRatio;
    int  nUnderflow;
} VIDEO_FRM_STATUS_INFO_T;




int enable_gl_2xscale(const char *);

int Active_osd_viewport(int , int );

class CTsPlayer : public ITsPlayer
{
public:
    CTsPlayer();
    CTsPlayer(CTsParameter p);
#ifdef USE_OPTEEOS
    CTsPlayer(bool DRMMode);
    CTsPlayer(bool DRMMode, bool omx_player);
#else
    CTsPlayer(bool omx_player);
#endif
    virtual ~CTsPlayer();
public:
    virtual int  GetPlayMode();
    virtual int  SetVideoWindow(int x,int y,int width,int height);
    virtual int  VideoShow(void);
    virtual int  VideoHide(void);
    virtual void InitVideo(PVIDEO_PARA_T pVideoPara);
    virtual void InitAudio(PAUDIO_PARA_T pAudioPara);
    virtual void InitSubtitle(PSUBTITLE_PARA_T pSubtitlePara);
    virtual bool StartPlay();

    bool StartRender();  //?

    virtual int WriteData(unsigned char* pBuffer, unsigned int nSize);
    virtual int SoftWriteData(PLAYER_STREAMTYPE_E type, uint8_t *pBuffer, uint32_t nSize, uint64_t timestamp);
    virtual bool Pause();
    virtual bool Resume();
    virtual bool Fast();
    virtual bool StopFast();
    virtual bool Stop();
    virtual bool Seek();
    virtual bool SetVolume(int volume);
    virtual int GetVolume();
    virtual bool SetRatio(int nRatio);
    virtual int GetAudioBalance();
    virtual bool SetAudioBalance(int nAudioBalance);
    virtual void GetVideoPixels(int& width, int& height);
    virtual bool IsSoftFit();
    virtual void SetEPGSize(int w, int h);
    virtual void SetSurface(Surface* pSurface);
    virtual void SwitchAudioTrack(int pid);
    virtual void SwitchSubtitle(int pid);
    virtual void SetProperty(int nType, int nSub, int nValue);
    virtual int64_t GetCurrentPlayTime();
    virtual void leaveChannel();
    virtual void playerback_register_evt_cb(IPTV_PLAYER_EVT_CB pfunc, void *hander);
#ifdef TELECOM_QOS_SUPPORT
    virtual void RegisterParamEvtCb(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc);
#endif
    virtual int playerback_getStatusInfo(IPTV_ATTR_TYPE_e enAttrType, int *value);
    virtual void ClearLastFrame();
    virtual void BlackOut(int EarseLastFrame);
    virtual bool SetErrorRecovery(int mode);
    virtual void GetAvbufStatus(PAVBUF_STATUS pstatus);
    virtual int GetRealTimeFrameRate();
    virtual int GetVideoFrameRate();
    virtual int GetVideoDropNumber();
    virtual int GetVideoTotalNumber();
    virtual void GetVideoResolution();
    virtual bool SubtitleShowHide(bool bShow);
    virtual void SetVideoHole(int x,int y,int w,int h);
    virtual void writeScaleValue();

    virtual void SwitchAudioTrack_ZTE(PAUDIO_PARA_T pAudioPara);

	virtual int GetCurrentVidPTS(unsigned long long *pPTS);
	virtual void GetVideoInfo(int *width, int *height, int *ratio);
	virtual int GetPlayerInstanceNo();
	virtual void ExecuteCmd(const char* cmd_str);
    virtual status_t setDataSource(const char *path, const KeyedVector<String8, String8> *headers = NULL) {return 0;}
    virtual int RegisterCallBack(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc);
    virtual int SetParameter(void* handler, int key, void * request);
    virtual int GetParameter(void* handler, int key, void * reply);
    virtual int Invoke(void *hander, int type, void * inptr, void * outptr);


    // the follow interfaces is defined by CTC standard
    virtual bool GetIsEos();
    virtual uint32_t GetCurrentPts();
    virtual void SetStopMode(bool bHoldLastPic);
    virtual int GetBufferStatus(int *totalsiz, int* datasize);
    virtual int GetStreamInfo(PVIDEO_INFO_T pVideoInfo, PAUDIO_INFO_T pAudioInfo);
    virtual bool SetDrmInfo(PDRM_PARA_T pDrmPara);
    virtual int Set3DMode(DISPLAY_3DMODE_E mode);
    virtual void SetSurfaceOrder(SURFACE_ORDER_E pOrder);
    // end



    void updateinfo_to_middleware(struct av_param_info_t av_param_info,struct av_param_qosinfo_t av_param_qosinfo);

    /*end add*/
    bool mIsOmxPlayer;
    void update_nativewindow();
    int updateCTCInfo();
    void ShowFrameInfo(struct vid_frame_info* frameinfo);

#if ANDROID_PLATFORM_SDK_VERSION <= 27
    void readExtractor();
#endif
protected:
    int		m_bLeaveChannel;

private:
    AUDIO_PARA_T a_aPara[MAX_AUDIO_PARAM_SIZE];
    SUBTITLE_PARA_T sPara[MAX_SUBTITLE_PARAM_SIZE];
    VIDEO_PARA_T vPara;
    PV_HEADER_T tsheader;
    int player_pid;
    codec_para_t  codec;
    codec_para_t  *pcodec;
    codec_para_t  *vcodec;
    codec_para_t  *acodec;
    codec_para_t  *scodec;
    bool		  m_bIsPlay;
    bool          m_bIsSeek;
    int			  m_nOsdBpp;
    int			  m_nAudioBalance;
    int			  m_nVolume;
    int           m_nEPGWidth;
    int           m_nEPGHeight;
    bool          m_bFast;
    bool          m_bSetEPGSize;
    bool          m_bWrFirstPkg;
    int	          m_nMode;
    int mWinAis[4];
    sp<ANativeWindow> mNativeWindow;
    int width_old,width_new;
    int height_old,height_new;

    int frame_rate_ctc;
    int threshold_value;
    int threshold_ctl_flag;
    int underflow_ctc;
    int underflow_kernel;
    int underflow_tmp;
    int underflow_count;
    int qos_count;
    int prev_vread_buffer;
    int vrp_is_buffer_changed;
    int last_data_len;
    int last_data_len_statistics;
    int mIsTSdata;

    IPTV_PLAYER_EVT_CB pfunc_player_evt;
    void *player_evt_hander;

    void *m_player_evt_hander_regitstercallback;
    IPTV_PLAYER_PARAM_EVENT_CB  m_pfunc_player_param_evt_registercallback;

#ifdef TELECOM_QOS_SUPPORT
    IPTV_PLAYER_PARAM_EVENT_CB  pfunc_player_param_evt;
    void *player_evt_param_handler;
#endif
    unsigned int writecount ;
    int64_t m_StartPlayTimePoint;
    /*+[SE] [BUG][BUG-170677][yinli.xia] added:2s later
        to statistics video frame when start to play*/
    int m_Frame_StartTime_Ctl;
    int64_t m_Frame_StartPlayTimePoint;
    bool    m_isSoftFit;
    FILE*	  m_fp;
    lock_t  mutex;
    pthread_t mThread[2];
    pthread_cond_t m_pthread_cond;
    pthread_cond_t s_pthread_cond;

    pthread_t readThread;
    pthread_t tsheaderThread;
    unsigned char header_buffer[TS_PACKET_SIZE];
    virtual void checkAbend();
    virtual void checkBuffLevel();
    virtual void checkBuffstate();
    static void *threadCheckAbend(void *pthis);
    static void *threadReadPacket(void *pthis);
    static void *Get_TsHeader_thread(void *pthis);

    bool    m_isBlackoutPolicy;
    bool    m_bchangeH264to4k;
    lock_t  mutex_lp;
    lock_t  mutex_session;
    lock_t  mutex_header;
    void checkVdecstate();
    bool    m_bIsPause;
    virtual bool iStartPlay( );
    virtual bool iStop( );
    int64_t m_PreviousOverflowTime;
    size_t  mInputQueueSize;
    ctsplayer_state m_sCtsplayerState;
    pthread_t mInfoThread;
    int mLastVdecInfoNum;
    char CallingPidName[PROCESS_NAME];
    static void * threadReportInfo(void *pthis);
    void update_caton_info(struct av_param_info_t * info);
    void update_stream_bitrate();
    int SetVideoWindowImpl(int x,int y,int width,int height);
    bool CheckMultiSupported(int video_type);
	void * stop_thread(void );
    static void * init_thread(void *pthis);
    void thread_wait_timeUs(int microseconds);
    void thread_wake_up();
    virtual void init_params();
    int is_use_double_write();
    void check_use_double_write();
    void stop_double_write();
    int parser_header(unsigned char* pBuffer, unsigned int size, unsigned char *buffer);
    int get_hevc_csd_packet(unsigned char* buf, int size, unsigned char *buffer);
    int Add_Packet_ToList(unsigned char* pBuffer, unsigned int nSize);
};






#endif


