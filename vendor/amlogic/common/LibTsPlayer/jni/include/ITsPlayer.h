#ifndef _ITSPLAYER_H_
#define _ITSPLAYER_H_

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <CTC_Common.h>

namespace android {
class Surface;
};

using android::RefBase;
using android::status_t;
using android::KeyedVector;
using android::String8;
using android::Surface;


///////////////////////////////////////////////////////////////////////////////
//
// we can NOT change memory layout of ITsPlayer, it we want to keep backward compatibility
//
///////////////////////////////////////////////////////////////////////////////
class ITsPlayer : public RefBase
{
public:
    ITsPlayer() {}
    virtual ~ITsPlayer() {}

    virtual int  GetPlayMode()=0; //what does this mean?

    /**
     * \brief SetVideoWindow 
     *
     * \param x
     * \param y
     * \param width
     * \param height
     *
     * \return 
     */
    virtual int  SetVideoWindow(int x,int y,int width,int height)=0;

    /**
     * \brief VideoShow 
     *
     * \return 
     */
    virtual int  VideoShow(void)=0;

    /**
     * \brief VideoHide 
     *
     * \return 
     */
    virtual int  VideoHide(void)=0;

    /**
     * \brief InitVideo 
     *
     * \param pVideoPara
     */
    virtual void InitVideo(PVIDEO_PARA_T pVideoPara)=0;

    /**
     * \brief InitAudio 
     *
     * \param pAudioPara
     */
    virtual void InitAudio(PAUDIO_PARA_T pAudioPara)=0;

    /**
     * \brief StartPlay 
     *
     * \return 
     */
    virtual bool StartPlay()=0;

    /**
     * \brief WriteData 
     *
     * \param pBuffer
     * \param nSize
     *
     * \return 
     */
    virtual int WriteData(unsigned char* pBuffer, unsigned int nSize)=0;

    /**
     * \brief Pause 
     *
     * \return 
     */
    virtual bool Pause()=0;

    /**
     * \brief Resume 
     *
     * \return 
     */
    virtual bool Resume()=0;

    /**
     * \brief Fast 
     *
     * \return 
     */
    virtual bool Fast()=0;

    /**
     * \brief StopFast 
     *
     * \return 
     */
    virtual bool StopFast()=0;

    /**
     * \brief Stop 
     *
     * \return 
     */
    virtual bool Stop()=0;

    /**
     * \brief Seek 
     *
     * \return 
     */
    virtual bool Seek()=0;

    /**
     * \brief SetVolume 
     *
     * \param volume
     *
     * \return 
     */
    virtual bool SetVolume(int volume)=0;

    /**
     * \brief GetVolume 
     *
     * \return 
     */
    virtual int GetVolume()=0;

    /**
     * \brief SetRatio 
     *
     * \param nRatio
     *
     * \return 
     */
    virtual bool SetRatio(int nRatio)=0;

    /**
     * \brief GetAudioBalance 
     *
     * \return 
     */
    virtual int GetAudioBalance()=0;

    /**
     * \brief SetAudioBalance 
     *
     * \param nAudioBalance
     *
     * \return 
     */
    virtual bool SetAudioBalance(int nAudioBalance)=0;

    /**
     * \brief GetVideoPixels 
     *
     * \param width
     * \param height
     */
    virtual void GetVideoPixels(int& width, int& height)=0;

    /**
     * \brief IsSoftFit 
     *
     * \return 
     */
    virtual bool IsSoftFit()=0;

    /**
     * \brief SetEPGSize 
     *
     * \param w
     * \param h
     */
    virtual void SetEPGSize(int w, int h)=0;

    /**
     * \brief SetSurface 
     *
     * \param pSurface
     */
    virtual void SetSurface(Surface* pSurface)=0;

    /**
     * \brief SwitchAudioTrack 
     *
     * \param pid
     */
    virtual void SwitchAudioTrack(int pid) = 0;

    /**
     * \brief SwitchSubtitle 
     *
     * \param pid
     */
    virtual void SwitchSubtitle(int pid) = 0;

    /**
     * \brief SetProperty 
     *
     * \param nType
     * \param nSub
     * \param nValue
     */
    virtual void SetProperty(int nType, int nSub, int nValue) = 0;

    /**
     * \brief GetCurrentPlayTime 
     *
     * \return 
     */
    virtual int64_t GetCurrentPlayTime() = 0;

    /**
     * \brief leaveChannel 
     */
    virtual void leaveChannel() = 0;

    /**
     * \brief playerback_register_evt_cb 
     *
     * \param pfunc
     * \param hander
     */
    virtual void playerback_register_evt_cb(IPTV_PLAYER_EVT_CB pfunc, void *hander) = 0;

#ifdef TELECOM_QOS_SUPPORT
        virtual void RegisterParamEvtCb(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc) = 0;
#endif

    //aml extension begin
    virtual int playerback_getStatusInfo(IPTV_ATTR_TYPE_e enAttrType, int *value)=0;


    virtual void ClearLastFrame() = 0;
    virtual void BlackOut(int EarseLastFrame)= 0;


    virtual bool SetErrorRecovery(int mode) = 0;


    virtual void GetAvbufStatus(PAVBUF_STATUS pstatus) = 0;



    virtual int GetRealTimeFrameRate() = 0;


    virtual int GetVideoFrameRate() = 0;
    

    virtual int GetVideoDropNumber() = 0;


    virtual int GetVideoTotalNumber() = 0;


    virtual void GetVideoResolution() = 0;

    /**
     * \brief InitSubtitle 
     *
     * \param pSubtitlePara
     */
	virtual void InitSubtitle(PSUBTITLE_PARA_T pSubtitlePara)=0;


    virtual bool SubtitleShowHide(bool bShow) = 0;


    virtual void SetVideoHole(int x,int y,int w,int h) = 0;


    virtual void writeScaleValue() = 0;


    virtual void SwitchAudioTrack_ZTE(PAUDIO_PARA_T pAudioPara)= 0;


	virtual int GetCurrentVidPTS(unsigned long long *pPTS)=0;


	virtual void GetVideoInfo(int *width, int *height, int *ratio)=0;


	virtual int GetPlayerInstanceNo() = 0;


	virtual void ExecuteCmd(const char* cmd_str) = 0;

    /**
     * \brief SoftWriteData 
     *
     * \param type
     * \param pBuffer
     * \param nSize
     * \param timestamp
     *
     * \return 
     */
	virtual int SoftWriteData(PLAYER_STREAMTYPE_E type, uint8_t *pBuffer, uint32_t nSize, uint64_t timestamp) = 0;


    virtual status_t setDataSource(const char *path, const KeyedVector<String8, String8> *headers = NULL) = 0;


    virtual int RegisterCallBack(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc) = 0;
    //aml extension end

    /**
     * \brief SetParameter 
     *
     * \param hander
     * \param type
     * \param ptr
     *
     * \return 
     */
    virtual int SetParameter(void* handler, int key, void * request) = 0;

    /**
     * \brief GetParameter 
     *
     * \param hander
     * \param type
     * \param ptr
     *
     * \return 
     */
    virtual int GetParameter(void* handler, int key, void * reply) = 0;

    virtual int Invoke(void *hander, int type, void * inptr, void * outptr) = 0;

    //aml ITsPlayer backward compability keeping END


    // the follow interfaces is defined by CTC standard
#if 0
    virtual bool GetIsEos() = 0;
    virtual uint32_t GetCurrentPts() = 0;
    virtual void SetStopMode(bool bHoldLastPic) = 0;
    virtual int GetBufferStatus(int *totalsiz, int* datasize) = 0;
    virtual int GetStreamInfo(PVIDEO_INFO_T pVideoInfo, PAUDIO_INFO_T pAudioInfo) = 0;
    virtual bool SetDrmInfo(PDRM_PARA_T pDrmPara) = 0;
    virtual int Set3DMode(DISPLAY_3DMODE_E mode) = 0;
    virtual void SetSurfaceOrder(SURFACE_ORDER_E pOrder) = 0;
#endif
    // end
};

///////////////////////////////////////////////////////////////////////////////
class ITsPlayerFactory
{
public:
    static ITsPlayerFactory& instance();
    ITsPlayer* createITsPlayer(int channelNo, CTC_InitialParameter* initialParam);

private:
    ITsPlayerFactory();
    ~ITsPlayerFactory();

private:
    static ITsPlayerFactory* sITsPlayerFactory;

    ITsPlayerFactory(const ITsPlayerFactory&) = delete;
    ITsPlayerFactory& operator=(const ITsPlayerFactory&) = delete;
};


#endif /* ifndef _ITSPLAYER_H_*/

