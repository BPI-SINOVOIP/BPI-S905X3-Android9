/*
 * author: jintao.xu@amlogic.com
 * date: 2015-07-10
 * wrap original source code for CTC usage
 */

#ifndef _CTC_OMXPLAYER_H_
#define _CTC_OMXPLAYER_H_

#include <gui/Surface.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/AMessage.h>
#include <gui/SurfaceComposerClient.h>
#include <media/stagefright/foundation/ADebug.h>
#include <gui/Surface.h>
#include <system/window.h>
#include <gui/IGraphicBufferProducer.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/ICrypto.h>
#include <media/stagefright/foundation/AString.h>
#include <utils/Log.h>
#include <media/stagefright/MetaData.h>
#include <SoftwareRenderer.h>
#include <media/stagefright/MediaBuffer.h>
#include "CTsPlayer.h"
#include "ffextractor.h"


const int kPTSProbeSize = 30;

using namespace android;

struct input_buffer_t {
	int index;
	void *ptr;
    int offset;
    int buffer_size;
    int data_size;
    bool have_data;
};

struct output_buffer_t {
	int index;
    int64_t nTimeStamp;
};

struct extract_buffer_t {
	void *ptr;
    int offset;
    int buffer_size;
    int data_size;
    bool have_data;
};

class CTsOmxPlayer : public CTsPlayer {
public:
    CTsOmxPlayer();
    virtual ~CTsOmxPlayer();
    virtual void InitVideo(PVIDEO_PARA_T pVideoPara);
	virtual void InitAudio(PAUDIO_PARA_T pAudioPara);
    virtual void InitSubtitle(PSUBTITLE_PARA_T pSubtitlePara);
    virtual bool StartPlay();
    virtual bool Stop();
    virtual int WriteData(unsigned char* pBuffer, unsigned int nSize);
    virtual void SetEPGSize(int w, int h);
    virtual int  SetVideoWindow(int x,int y,int width,int height);
    //virtual void writeDecoderInBuffer();
    virtual void readExtractor();
    virtual void readDecoder();
    virtual bool createWindowSurface();
    virtual bool createOmxDecoder();
	virtual void renderFrame();
	virtual void releaseYUVFrames();
    virtual void ClearLastFrame() ;
    virtual void leaveChannel() ;
    virtual int  VideoHide(void);
    virtual int  VideoShow(void);

private:
    FILE* mFp;
    bool	mSoftDecoder;
    VIDEO_PARA_T mSoftVideoPara;
    int mSoftNativeX;
    int mSoftNativeY;
    int mSoftNativeWidth;
    int mSoftNativeHeight;

	int mSoftRenderWidth;
	int mSoftRenderHeight;

    //sp<ALooper> mSoftLooper;
    sp<SurfaceComposerClient> mSoftComposerClient;
    sp<MediaCodec> mSoftCodec;
    Vector<sp<ABuffer> > mSoftInBuffers;
    Vector<sp<ABuffer> > mSoftOutBuffers;
    int mSoftInputBufferCount;
    input_buffer_t * mInputBuffer;
    int mInputBufferSize;
    List<int> mInputBufferQueue;
    List<output_buffer_t> mOutputBufferQueue;
    Mutex mInputQueueLock;
    Mutex mOutputQueueLock;
    bool mIsPlaying;
    int mApkUiWidth;
    int mApkUiHeight;

    class RunWorker: public Thread {
	public:
        enum dt_statue_t {
            STATUE_STOPPED,
            STATUE_RUNNING,
            STATUE_PAUSE
        };
        enum dt_module_t{
            MODULE_DECODER,
            MODULE_EXTRACT,
			MODULE_VSYNC
        };
        dt_module_t mTaskModule;
		RunWorker(CTsOmxPlayer* player, dt_module_t module);
		int32_t  start ();
		int32_t  stop ();
		int32_t  pause ();
		virtual bool threadLoop();
		dt_statue_t mTaskStatus;
	private:
		CTsOmxPlayer* mPlayer;
	};
    sp<RunWorker> mRunWorkerDecoder;
    sp<RunWorker> mRunWorkerExtract;
	sp<RunWorker> mRunWorkerVsync;
	sp<RunWorker> mRunWorkerWatchdog;

    class CTCTSExtractor : public RefBase {
    public:
        CTCTSExtractor(CTsOmxPlayer* player);
        sp<MetaData> getFormat();
        status_t read(MediaBuffer **buffer);
    private:
        mutable Mutex mLock;
        CTsOmxPlayer* mPlayer;
        void init();
        status_t feedMore();
    };

    sp<CTCTSExtractor> mExtract;
    sp<MetaData> mMeta;
    extract_buffer_t  mExtractBuffer;
    //sp<ALooper> mSoftLooper;
    sp<SurfaceControl> mSoftControl;
    sp<Surface> mSoftSurface;
    int64_t mFirstDisplayTime;
    int64_t mFirst_Pts;
    List<MediaBuffer* > mExtractOutBufferQueue;
    Mutex mFFExtractOutBufferQueueLock;
    List<MediaPacket*> mFFExtractOutBufferQueue;
	List<YUVFrame*> mYUVFrameQueue;
    Mutex mExtractOutQueueLock;
	Mutex mYUVFrameQueueLock;
	SoftwareRenderer *mSoftRenderer;

	int64_t mFirstIDRPTS;
	int64_t mSecondIDRPTS;
	int mIDRInterval;
	bool mIDRStart;
	int64_t mFrameDuration;
	int64_t mLastRenderPTS;
	int64_t mLastRenderTime;
	
	int64_t mAccumPTS;
	
	bool mPTSDiscontinue;
	int mPTSDrift;
	int mFPSProbeSize;
	bool mForceStop;
	bool mKeepLastFrame;
    bool mOpenLog;
	size_t mInputQueueSize;
	int mYuvNo;
	sp<AMessage> mFormatMsg;
};

#endif  //  _CTC_OMXPLAYER_H_

