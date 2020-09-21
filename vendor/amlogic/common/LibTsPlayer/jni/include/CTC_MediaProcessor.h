/*
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _CTC_MEDIAPROCESSOR_H_
#define _CTC_MEDIAPROCESSOR_H_

#include "CTC_Common.h"

namespace android {
class Surface;
}
struct ANativeWindow;
class ITsPlayer;

namespace aml {
class CTC_MediaProcessor;

/**
 * \brief GetMediaProcessor 
 *
 * \param channelNo the channel number, start from zero
 * \param initialParam initialize parameter
 *
 * \return a pointer of CTC_MediaProcessor object, need delete by user
 */
CTC_MediaProcessor* GetMediaProcessor(int channelNo, CTC_InitialParameter* initialParam = nullptr);

/**
 * \brief GetMediaProcessorVersion 
 *
 * \return CTC_MediaProcessor version number
 */
int GetMediaProcessorVersion();


class CTC_MediaProcessor
{
public:
    virtual ~CTC_MediaProcessor();

public:
    int GetPlayMode();

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
    int  SetVideoWindow(int x,int y,int width,int height);

    /**
     * \brief show video 
     *
     * \return 
     */
    int VideoShow(void);

    /**
     * \brief hide video 
     *
     * \return 
     */
    int VideoHide(void);

    /**
     * \brief initialize video parameter 
     *
     * \param pVideoPara
     */
    void InitVideo(PVIDEO_PARA_T pVideoPara);

    /**
     * \brief initialize audio parameter
     *
     * \param pAudioPara
     */
    void InitAudio(PAUDIO_PARA_T pAudioPara);

    /**
     * \brief initialize subtitle parameter 
     *
     * \param sParam
     */
    void InitSubtitle(PSUBTITLE_PARA_T pSubtitlePara);

    /**
     * \brief StartPlay 
     *
     * \return true if success, otherwise false.
     */
    bool StartPlay();

    /**
     * \brief WriteData 
     *
     * \param pBuffer
     * \param nSize
     *
     * \return 0: success
     *        -1: failure
     */
    int WriteData(unsigned char* pBuffer, unsigned int nSize);

    /**
     * \brief SoftWriteData 
     *
     * \param type
     * \param pBuffer
     * \param nSize
     * \param timestamp 90KHZ unit
     *
     * \return 
     */
	int SoftWriteData(PLAYER_STREAMTYPE_E type, uint8_t *pBuffer, uint32_t nSize, uint64_t timestamp);

    /**
     * \brief Pause 
     *
     * \return 
     */
    bool Pause(); 

    /**
     * \brief resume from paused state
     *
     * \return 
     */
    bool Resume();

    /**
     * \brief request decoder enter into fast forward or fast backward state
     *
     * \return 
     */
    bool Fast();

    /**
     * \brief return to normal playing state 
     *
     * \return 
     */
    bool StopFast(); 

    /**
     * \brief Stop 
     *
     * \return 
     */
    bool Stop();

    /**
     * \brief flush all buffers and kepp last frame 
     *
     * \return 
     */
    bool Seek();

    /**
     * \brief SetVolume 
     *
     * \param volume
     *
     * \return 
     */
    int SetVolume( float volume);

    /**
     * \brief GetVolume 
     *
     * \return 
     */
    float GetVolume();

    /**
     * \brief SetRatio 
     *
     * \param contentMode
     *
     * \return 
     */
    int SetRatio(CONTENTMODE_E contentMode);

    /**
     * \brief GetAudioBalance 
     *
     * \return 
     */
    int GetAudioBalance();

    /**
     * \brief SetAudioBalance 
     *
     * \param nAudioBalance
     *
     * \return 
     */
    bool SetAudioBalance(ABALANCE_E nAudioBalance);

    /**
     * \brief GetVideoPixels 
     *
     * \param width
     * \param height
     */
    void GetVideoPixels(int& width, int& height);

    /**
     * \brief IsSoftFit 
     *
     * \return 
     */
    bool IsSoftFit();

    /**
     * \brief SetEPGSize 
     *
     * \param w
     * \param h
     */
    void SetEPGSize(int w, int h);

    /**
     * \brief SetSurface 
     *
     * \param pSurface CTC standard requires a pointer to ANativeWindow
     */
    void SetSurface(ANativeWindow *pSurface);

    /**
     * \brief SetSurface 
     *
     * \param pSurface pointer to Surface
     */
    void SetSurface(android::Surface *pSurface);

    /**
     * \brief SwitchAudioTrack 
     *
     * \param pid set pid if stream type is TS
     * \param pAudioPara set audio parameter if stream type is ES
     */
    void SwitchAudioTrack(uint16_t pid, PAUDIO_PARA_T pAudioPara);

    /**
     * \brief SwitchSubtitle 
     *
     * \param pid
     */
    void SwitchSubtitle(int pid);

    /**
     * \brief SetProperty 
     *
     * \param nType
     * \param nSub
     * \param nValue
     */
    void SetProperty(int nType, int nSub, int nValue);

    /**
     * \brief GetCurrentPlayTime 
     *
     * \return 
     */
    int64_t GetCurrentPlayTime();

    /**
     * \brief leaveChannel 
     */
    void leaveChannel();

    /**
     * \brief playerback_register_evt_cb 
     *
     * \param pfunc
     * \param handler
     */
    void playerback_register_evt_cb(IPTV_PLAYER_EVT_CB pfunc, void* handler);

    //aml extension begin
    /**
     * \brief playerback_getStatusInfo 
     *
     * \param enAttrType
     * \param value
     *
     * \return 
     */
    int playerback_getStatusInfo(IPTV_ATTR_TYPE_e enAttrType, int *value);

    /**
     * \brief ClearLastFrame 
     */
    void ClearLastFrame();

    /**
     * \brief BlackOut
     *
     * \param EarseLastFrame
     */
    void BlackOut(int EarseLastFrame);

    /**
     * \brief SetErrorRecovery 
     *
     * \param mode
     *
     * \return 
     */
    bool SetErrorRecovery(int mode);

    /**
     * \brief GetAvBufStatus 
     *
     * \param pstatus
     */
    int GetAvBufStatus(PAVBUF_STATUS pstatus);

    /**
     * \brief GetRealTimeFrameRate 
     *
     * \return 
     */
    int GetRealTimeFrameRate();

    /**
     * \brief GetVideoFrameRate
     *
     * \return 
     */
    int GetVideoFrameRate();

    /**
     * \brief GetVideoDropNumber 
     *
     * \return 
     */
    int GetVideoDropNumber();

    /**
     * \brief GetVideoTotalNumber 
     *
     * \return 
     */
    int GetVideoTotalNumber();

    /**
     * \brief GetVideoResolution 
     */
    void GetVideoResolution();

    /**
     * \brief SubtitleShowHide 
     *
     * \param bShow
     *
     * \return 
     */
    bool SubtitleShowHide(bool bShow);

    /**
     * \brief SetVideoHole 
     *
     * \param x
     * \param y
     * \param w
     * \param h
     */
    void SetVideoHole(int x,int y,int w,int h);

    /**
     * \brief writeScaleValue 
     */
    void writeScaleValue();

    /**
     * \brief GetCurrentVidPTS 
     *
     * \param pPTS
     *
     * \return 
     */
	int GetCurrentVidPTS(unsigned long long *pPTS);

    /**
     * \brief GetVideoInfo 
     *
     * \param width
     * \param height
     * \param ratio
     */
	int GetVideoInfo(int *width, int *height, int *ratio);

    /**
     * \brief GetPlayerInstanceNo 
     *
     * \return 
     */
	int GetPlayerInstanceNo();

    /**
     * \brief ExecuteCmd 
     *
     * \param cmd_str
     */
	void ExecuteCmd(const char* cmd_str);

    /**
     * \brief RegisterCallBack 
     *
     * \param hander
     * \param enEvt
     * \param pfunc
     *
     * \return 
     */
    int RegisterCallBack(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc);

    //aml extension end

    /**
     * \brief for mediaplayer::SetParameter
     *
     * \param key
     * \param request
     */
    void SetParameter(int32_t key, void* request);

    /**
     * \brief GetParameter 
     *
     * \param key
     * \param reply
     */
    void GetParameter(int32_t key, void* reply);

    /**
     * \brief Invoke 
     *
     * \param hander
     * \param type
     * \param inptr
     * \param outptr
     *
     * \return 
     */
    int Invoke(void *hander, int type, void * inptr, void * outptr);

    // the follow interfaces is defined by CTC standard
    /**
     * \brief check whether is end of stream 
     *
     * \return 
     */
    bool GetIsEos();

    /**
     * \brief GetCurrentPts, millisecond unit
     *
     * \return 
     */
    int64_t GetCurrentPts();

    /**
     * \brief SetStopMode 
     *
     * \param bHoldLastPic
     */
    void SetStopMode(bool bHoldLastPic);
    
    /**
     * \brief GetBufferStatus 
     *
     * \param totalsiz
     * \param datasize
     *
     * \return 
     */
    int GetBufferStatus(int *totalsiz, int* datasize);

    /**
     * \brief GetStreamInfo 
     *
     * \param pVideoInfo
     * \param pAudioInfo
     *
     * \return 
     */
    int GetStreamInfo(PVIDEO_INFO_T pVideoInfo, PAUDIO_INFO_T pAudioInfo);

    /**
     * \brief SetDrmInfo 
     *
     * \param pDrmPara
     *
     * \return 
     */
    bool SetDrmInfo(PDRM_PARA_T pDrmPara);

    /**
     * \brief Set3DMode 
     *
     * \param mode
     *
     * \return 
     */
    int Set3DMode(DISPLAY_3DMODE_E mode);

    /**
     * \brief SetSurfaceOrder 
     *
     * \param pOrder
     */
    void SetSurfaceOrder(SURFACE_ORDER_E pOrder);
    // end

protected:
    friend CTC_MediaProcessor* GetMediaProcessor(int channelNo, CTC_InitialParameter* initialParam);
	CTC_MediaProcessor(int channelNo, CTC_InitialParameter* initialParam);
	
    ITsPlayer* mTsPlayer = nullptr;
    void* opaque = nullptr;

private:
    CTC_MediaProcessor(const CTC_MediaProcessor&);
    CTC_MediaProcessor& operator=(const CTC_MediaProcessor&);
};

}


///////////////////////////////////////////////////////////////////////////////
//
// The Following is just for backward compatibility
//
///////////////////////////////////////////////////////////////////////////////

// 获取CTC_MediaProcessor 派生类的实例对象。在CTC_MediaProcessor () 这个接口的实现中，需要创建一个
// CTC_MediaProcessor 派生类的实例，然后返回这个实例的指针

typedef enum {
    PLAYER_TYPE_NORMAL = 0,
    PLAYER_TYPE_OMX,
    PLAYER_TYPE_HWOMX,
    PLAYER_TYPE_NORMAL_MULTI,
    PLAYER_TYPE_USE_PARAM = 0x10000000,
} player_type_t;

ITsPlayer* GetMediaProcessor();  // { return NULL; }
ITsPlayer* GetMediaProcessor(player_type_t type);  // { return NULL; }

#ifdef USE_OPTEEOS
ITsPlayer* GetMediaProcessor(bool DRMMode);
ITsPlayer* GetMediaProcessor(player_type_t type, bool DRMMode);
#endif

// 获取底层模块实现的接口版本号。将来如果有多个底层接口定义，使得上层与底层之间能够匹配。本版本定义
// 返回为1
int GetMediaProcessorVersion();  //  { return 0; }
















///////////////////////////////////////////////////////////////////////////////
#endif  // _CTC_MEDIAPROCESSOR_H_
