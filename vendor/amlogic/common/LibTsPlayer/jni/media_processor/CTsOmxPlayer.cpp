#define LOG_NDEBUG 0
#define LOG_TAG "ctc_omx_player"

#include "CTsOmxPlayer.h"
#include "Util.h"
#include <ui/DisplayInfo.h>
#include <sys/system_properties.h>
#include <cutils/properties.h>

#define _GNU_SOURCE
#define F_SETPIPE_SZ        (F_LINUX_SPECIFIC_BASE + 7)
#define F_GETPIPE_SZ        (F_LINUX_SPECIFIC_BASE + 8)
#include <fcntl.h>
#include "ffextractor.h"

#define LOG_LINE() ALOGV("[%s:%d] >", __FUNCTION__, __LINE__);

using namespace android;

static int s_nDumpTs = 0;
static int pipe_fd[2] = { -1, -1 };
static bool ffextractor_inited = false;
static int first_pic_comming = 0;
static int s_writebytes = 0;
static sp<SurfaceComposerClient> Ctc_SoftComposerClient = NULL;
static sp<SurfaceControl> Ctc_SoftControl = NULL;
static sp<Surface> Ctc_SoftSurface = NULL;
int read_cb(void *opaque, uint8_t *buf, int size) {
    int ret = read(pipe_fd[0], buf, size);
    return ret;
}
uint64_t last_time;

#ifdef USE_OPTEEOS
CTsOmxPlayer::CTsOmxPlayer():CTsPlayer(false, true) {
#else
CTsOmxPlayer::CTsOmxPlayer():CTsPlayer(true) {
#endif
    LOG_LINE();
    mFp = NULL;
    mIsOmxPlayer = true;
    mSoftDecoder = true;
    mExtract = NULL;
    mFirstDisplayTime = -1;
    mFirstIDRPTS = -1;
    mSecondIDRPTS = -1;
    mIDRInterval = 0;
    mIDRStart = false;
    mFrameDuration = 33;
    mFirst_Pts = 0;
    mAccumPTS = 0;
    mPTSDrift = -15;
    ALOGD("mPTSDrift=%d", mPTSDrift);
    mFPSProbeSize = 0;
    mSoftRenderer = NULL;
    mSoftComposerClient = NULL;
    mSoftControl = NULL;
    mSoftSurface = NULL;
    mSoftRenderWidth = 0;
    mSoftRenderHeight = 0;
    mForceStop = false;
    mIsPlaying = false;
    mLastRenderPTS = 0;
    mInputQueueSize = 0;
    mFormatMsg = new AMessage;
    mOpenLog = false;
    mYuvNo = 0;
    mKeepLastFrame = false;

    char value[PROPERTY_VALUE_MAX];
    int open_log = 0;
    if (property_get("iptv.soft.log", value, NULL) > 0)
        sscanf(value, "%d", &open_log);

    if (open_log)
        mOpenLog = true;

    memset(value, 0, PROPERTY_VALUE_MAX);
    int keep_last_frame = 0;
    if (property_get("iptv.soft.keep", value, NULL) > 0)
        sscanf(value, "%d", &keep_last_frame);

    if (keep_last_frame) {
        mKeepLastFrame = true;
        mSoftComposerClient = Ctc_SoftComposerClient;
        mSoftControl = Ctc_SoftControl;
        mSoftSurface = Ctc_SoftSurface;
        ALOGI("CTsOmxPlayer: use last surface");
    }
    ALOGI("CTsOmxPlayer::creat end mOpenLog=%d\n", mOpenLog);

    return;
}

CTsOmxPlayer::~CTsOmxPlayer() {
    LOG_LINE();
    Stop();
    ALOGD("disposing surface");
    if (!mKeepLastFrame) {
        if (mSoftComposerClient != NULL) {
            mSoftComposerClient->dispose();
            mSoftComposerClient = NULL;
        }
        Ctc_SoftSurface = NULL;
        Ctc_SoftControl = NULL;
        Ctc_SoftComposerClient = NULL;
    } else {
        Ctc_SoftSurface = mSoftSurface;
        Ctc_SoftControl = mSoftControl;
        Ctc_SoftComposerClient = mSoftComposerClient;
    }
    return;
}

bool CTsOmxPlayer::StartPlay() {
    LOG_LINE();
    char value[PROPERTY_VALUE_MAX] = {0};

    mYUVFrameQueue.clear();

    mRunWorkerDecoder = new RunWorker(this, RunWorker::MODULE_DECODER);
    mRunWorkerExtract = new RunWorker(this, RunWorker::MODULE_EXTRACT);
    mRunWorkerVsync   = new RunWorker(this, RunWorker::MODULE_VSYNC);

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(1);
    } else {
        fcntl(pipe_fd[0], F_SETPIPE_SZ, 1048576);
        fcntl(pipe_fd[1], F_SETPIPE_SZ, 1048576);
        ALOGD("pipe opened!");
    }

    if (mIsPlaying == true) {
        ALOGD("mIsPlaying is true");
        return true;
    }
    mIsPlaying = true;

    property_get("iptv.omx.dumpfile", value, "0");
    s_nDumpTs = atoi(value);
    if (s_nDumpTs == 1) {
        char tmpfilename[1024]="";
        static int s_nDumpCnt=0;
        sprintf(tmpfilename,"/storage/external_storage/sda1/Live%d.ts",s_nDumpCnt);
        s_nDumpCnt++;
        mFp = fopen(tmpfilename, "wb+");
    }
    if (mSoftComposerClient == NULL) {
        ALOGD("mSoftComposerClient is NULL, create one");
        if (!createWindowSurface())
            return false;
    } else {
        ALOGD("mSoftComposerClient already created");
    }
    // start extractor\decoer\vsync threads
    if (!createOmxDecoder())
        return false;
		
    first_pic_comming = 0;
    s_writebytes = 0;
    return true;
}

int CTsOmxPlayer::SetVideoWindow(int x,int y,int width,int height) {
    LOG_LINE();
    if ((mSoftNativeX != x) || (mSoftNativeY != y)
            || (mSoftNativeWidth != width) || (mSoftNativeHeight != height)) {
        if (mSoftComposerClient != NULL) {
            ALOGI("CTsOmxPlayer::SetVideoWindow: mSoftComposerClient dispose!");
            mSoftComposerClient->dispose();
            mSoftComposerClient = NULL;
        }
        mSoftNativeX = x;
        mSoftNativeY = y;
        mSoftNativeWidth = width;
        mSoftNativeHeight = height;
    }
    ALOGI("CTsOmxPlayer::SetVideoWindow: %d, %d, %d, %d\n", x, y, width, height);
    return 0;
}

bool CTsOmxPlayer::createWindowSurface() {
    LOG_LINE();
    OUTPUT_MODE output_mode = get_display_mode();
    int c_x = 0, c_y = 0, c_w = 0, c_h = 0;
    getPosition(output_mode, &c_x, &c_y, &c_w, &c_h);
    ALOGI("createWindowSurface c_x: %d, c_y: %d, c_w: %d, c_h: %d\n", c_x, c_y, c_w, c_h);
    ALOGI("SoftNative x: %d, y: %d, w: %d, h: %d\n",
            mSoftNativeX, mSoftNativeY, mSoftNativeWidth, mSoftNativeHeight);

    mSoftComposerClient = new SurfaceComposerClient;
    CHECK_EQ(mSoftComposerClient->initCheck(), (status_t)OK);
    mSoftControl = mSoftComposerClient->createSurface(String8("ctc_Surface"),
            mSoftNativeWidth, mSoftNativeHeight, PIXEL_FORMAT_RGBA_8888, 0);

    CHECK(mSoftControl != NULL);
    CHECK(mSoftControl->isValid());
    SurfaceComposerClient::openGlobalTransaction();
    //CHECK_EQ(mSoftControl->setLayer(INT_MAX-100), (status_t)OK);
    CHECK_EQ(mSoftControl->setLayer(0), (status_t)OK);

    sp<IBinder> dpy = mSoftComposerClient->getBuiltInDisplay(0);
    if (dpy == NULL) {
        ALOGE("SurfaceComposer::getBuiltInDisplay failed");
        return false;
    }

    DisplayInfo info;
    status_t err = mSoftComposerClient->getDisplayInfo(dpy, &info);
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposer::getDisplayInfo failed\n");
        return false;
    }

    ALOGI("getDisplayInfo w: %d, h: %d\n", info.w, info.h);
    float scaleX = 1.000f;
    float scaleY = 1.000f;
    scaleX = (float) info.w / c_w;
    scaleY = (float) info.h / c_h;
    ALOGI("scaleX: %f, scaleY: %f\n", scaleX, scaleY);

    int x = mSoftNativeX * scaleX;
    int y = mSoftNativeY * scaleY;
    ALOGI("x: %d, y: %d\n", x, y);

    mSoftControl->setPosition(x, y);
    err = mSoftControl->setMatrix(scaleX, 0.0f, 0.0f, scaleY);
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposer::setMatrix error");
        return false;
    }

    CHECK_EQ(mSoftControl->show(), (status_t)OK);
    SurfaceComposerClient::closeGlobalTransaction();

    mSoftSurface = mSoftControl->getSurface();
    CHECK(mSoftSurface != NULL);
    return true;
}

bool CTsOmxPlayer::createOmxDecoder() {
    LOG_LINE();
    if ((mRunWorkerExtract.get() != NULL)
            && (mRunWorkerExtract.get()->mTaskStatus != RunWorker::STATUE_RUNNING)) {
        mRunWorkerExtract->start();
    }

    if ((mRunWorkerDecoder.get() != NULL)
            && (mRunWorkerDecoder.get()->mTaskStatus != RunWorker::STATUE_RUNNING)) {
        mRunWorkerDecoder->start();
    }

    if ((mRunWorkerVsync.get() != NULL)
            && (mRunWorkerVsync.get()->mTaskStatus != RunWorker::STATUE_RUNNING)) {
        mRunWorkerVsync->start();
    }

    return true;
}

void CTsOmxPlayer::InitVideo(PVIDEO_PARA_T pVideoPara) {
    LOG_LINE();
    mSoftVideoPara = *pVideoPara;
    ALOGI("nVideoWidth =%d, nVideoHeight=%d, pid=%u", mSoftVideoPara.nVideoWidth, mSoftVideoPara.nVideoHeight);
    if (mSoftVideoPara.nVideoWidth == 0 || mSoftVideoPara.nVideoHeight == 0) {
        ALOGE("nVideoWidth =%d, nVideoHeight=%d", mSoftVideoPara.nVideoWidth, mSoftVideoPara.nVideoHeight);
        mSoftVideoPara.nVideoWidth = 1920;
        mSoftVideoPara.nVideoHeight = 1088;
    }
    return;
}

void CTsOmxPlayer::InitAudio(PAUDIO_PARA_T pAudioPara) {
    LOG_LINE();
    return;
}

void CTsOmxPlayer::InitSubtitle(PSUBTITLE_PARA_T pSubtitlePara) {
    LOG_LINE();
    return;
}

int CTsOmxPlayer::WriteData(unsigned char* pBuffer, unsigned int nSize) {
    if(!mIsPlaying)
        return 0;

    int ret = write(pipe_fd[1], pBuffer, nSize);
    if (mFp != NULL) {
        fwrite(pBuffer, 1, nSize, mFp);
    }
    if (mOpenLog/*first_pic_comming == 0*/) {
        s_writebytes += nSize;
        ALOGI("writedata ret: %d, total:%d\n", nSize, s_writebytes);
    }

    return ret;
}

void CTsOmxPlayer::leaveChannel() {
    LOG_LINE();
    Stop();
}

int CTsOmxPlayer::VideoHide(void) {
    LOG_LINE();
    if (mSoftControl != NULL) {
        SurfaceComposerClient::openGlobalTransaction();
        CHECK_EQ(mSoftControl->hide(), (status_t)OK);
        SurfaceComposerClient::closeGlobalTransaction();
    }
    ALOGI("VideoHide been called\n");
    return 0;
}

int CTsOmxPlayer::VideoShow(void) {
    LOG_LINE();
    if (mSoftControl != NULL) {
        SurfaceComposerClient::openGlobalTransaction();
        CHECK_EQ(mSoftControl->show(), (status_t)OK);
        SurfaceComposerClient::closeGlobalTransaction();
    }
    ALOGI("VideoShow been called\n");
    return 0;
}

bool CTsOmxPlayer::Stop() {
    ALOGI("CTsOmxPlayer Stop\n");
    LOG_LINE();
    if (mIsPlaying) {
        if (mFp != NULL) {
            fclose(mFp);
            mFp = NULL;
        }
        uint8_t *tmp_buf = (uint8_t *)malloc(1024*32);
        if (tmp_buf == NULL) {
            ALOGE("malloc tmp_buf failed");
            return false;
        }
        close(pipe_fd[1]);
        while(read(pipe_fd[0], tmp_buf, 1024*32)>0);
        free(tmp_buf);
        close(pipe_fd[0]);
        ALOGD("pipe closed");
        mIsPlaying = false;

        mRunWorkerVsync->stop();
        ALOGD("vsync thread stopped");

        mRunWorkerDecoder->stop();
        ALOGD("decoder thread stopped");

        mRunWorkerExtract->stop();
        ALOGD("extractor thread stopped");

        if (mRunWorkerDecoder.get() != NULL) {
            mRunWorkerDecoder->stop();
            mRunWorkerDecoder = NULL;
        }
        if (mRunWorkerExtract.get() != NULL) {
            mRunWorkerExtract->stop();
            mRunWorkerExtract = NULL;
        }
        if (mRunWorkerVsync.get() != NULL) {
            mRunWorkerVsync->stop();
            mRunWorkerVsync = NULL;
        }
        LOG_LINE();
        delete mSoftRenderer;
        mSoftRenderer = NULL;
        ffextractor_deinit();
        ffextractor_inited = false;
        ALOGD("ffmpeg denited");
        releaseYUVFrames();
        mYUVFrameQueue.clear();
        LOG_LINE();
        ALOGI("CTsOmxPlayer Stop end\n");
    } else
        ALOGD("CTsOmxPlayer has stoped");
    return true;
}

void CTsOmxPlayer::SetEPGSize(int w, int h) {
    LOG_LINE();
    ALOGV("SetEPGSize: w=%d, h=%d\n", w, h);
    mApkUiWidth = w;
    mApkUiHeight = h;
}

void CTsOmxPlayer::readExtractor() {
    if (ffextractor_inited == false) {
        MediaInfo mi;
        int try_count = 0;
        while (try_count++ < 3) {
            ALOGD("try to init ffmepg %d times", try_count);
            int ret = ffextractor_init(read_cb, &mi);
            if (ret == -1) {
                ALOGD("ffextractor_init return -1");
                continue;
            }
            if (mi.width ==0 || mi.height == 0) {
                ALOGD("invalid dimensions: %d:%d", mi.width, mi.height);
            }
            ffextractor_inited = true;
            break;
        }
    }

    if (ffextractor_inited == false) {
        ALOGE("3 Attempts to init ffextractor all failed");
        mForceStop = true;
        return;
    }
    ffextractor_read_packet(&mInputQueueSize);
    ALOGD_IF(mOpenLog, "readExtractor: mInputQueueSize=%d", mInputQueueSize);
}

void CTsOmxPlayer::readDecoder() {
    if (ffextractor_inited == false) {
        return;
    }

    int yuv_count = 0;
    {
        AutoMutex l(mYUVFrameQueueLock);
        yuv_count = mYUVFrameQueue.size();
    }
    ALOGD_IF(mOpenLog, "readDecoder: yuv_count =%d", yuv_count);
    if (yuv_count > 10)
        return;
    {
        YUVFrame *frame = ff_decode_frame();
        if (frame == NULL || frame->data == NULL) {
            return;
        }

        {
            AutoMutex l(mYUVFrameQueueLock);
            mYUVFrameQueue.push_back(frame);
            ALOGD_IF(mOpenLog, "readDecoder: push_back a frame");
        }
    }
}

void CTsOmxPlayer::releaseYUVFrames() {
    AutoMutex l(mYUVFrameQueueLock);
    while (!mYUVFrameQueue.empty()) {
        YUVFrame *frame = *mYUVFrameQueue.begin();
        free(frame->data);
        free(frame);
        mYUVFrameQueue.erase(mYUVFrameQueue.begin());
    }
    mYUVFrameQueue.clear();
}

void CTsOmxPlayer::renderFrame() {
    if (ffextractor_inited == false) {
        return;
    }

    AutoMutex l(mYUVFrameQueueLock);
    if (!mYUVFrameQueue.empty()) {
        YUVFrame *frame = *mYUVFrameQueue.begin();
        if (mSoftRenderer == NULL || mSoftRenderWidth != frame->width
                || mSoftRenderHeight != frame->height) {
            delete mSoftRenderer;
            mSoftRenderer = NULL;
            sp<MetaData> meta = new MetaData;
            ALOGD("before mSoftRenderer, mSoftNativeWidth=%d, mSoftNativeHeight=%d",
                    frame->width, frame->height);
            meta->setInt32(kKeyWidth, frame->width);
            meta->setInt32(kKeyHeight, frame->height);
            meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420Planar);

            mFormatMsg->setInt32("stride",       frame->width);
            mFormatMsg->setInt32("slice-height", frame->height);
            mFormatMsg->setInt32("color-format", OMX_COLOR_FormatYUV420Planar);
#ifdef ANDROID4
            mSoftRenderer = new SoftwareRenderer(mSoftSurface, meta);
#endif

#ifdef ANDROID5
            mSoftRenderer = new SoftwareRenderer(mSoftSurface);
#endif

            mSoftRenderWidth = frame->width;
            mSoftRenderHeight = frame->height;
        }

        mFPSProbeSize++;
        if (mFPSProbeSize == kPTSProbeSize) {
            mFPSProbeSize = 0;
            if (mInputQueueSize < 25) {
                mPTSDrift += 1;
                if (mPTSDrift >= 15)
                    mPTSDrift = 15;
            } else if (mInputQueueSize > 50) {
                mPTSDrift -= 1;
                if (mFrameDuration + mPTSDrift <= 0)
                    mPTSDrift = -mFrameDuration;
            }
        }

        char levels_value[92];
        int soft_sync_mode = 1;
        if (property_get("iptv.soft.sync_mode",levels_value,NULL) > 0)
            sscanf(levels_value, "%d", &soft_sync_mode);

        if (soft_sync_mode == 0) {//sync by pts
            int64_t pts_diff = frame->pts - mLastRenderPTS;
            if (pts_diff < 0)
                pts_diff = 0 - pts_diff;
            ALOGD_IF(mOpenLog, "renderFrame: pts mode: frame->pts =%lld, mLastRenderPTS =%lld, pts_diff=%lld, mYuvNo=%d",
                    frame->pts, mLastRenderPTS, pts_diff, mYuvNo++);
            if (mFirstDisplayTime == -1 || pts_diff > 5000000) {
                mFirstDisplayTime = getCurrentTimeUs();
                mFirst_Pts = frame->pts;
            }

            if ((getCurrentTimeUs() - mFirstDisplayTime) >= (frame->pts - mFirst_Pts)) {
                mLastRenderTime = getCurrentTimeUs();
#ifdef ANDROID4
                mSoftRenderer->render(frame->data, frame->size, NULL);
#endif

#ifdef ANDROID5
                mSoftRenderer->render(frame->data, frame->size, 0,  NULL, mFormatMsg);
#endif
                free(frame->data);
                free(frame);
                mLastRenderPTS = frame->pts;
                mYUVFrameQueue.erase(mYUVFrameQueue.begin());
                ALOGD_IF(mOpenLog, "renderFrame: pts mode: render a frame");
            }
        } else { //sync by duration
            uint64_t start = getCurrentTimeUs();
            if (first_pic_comming == 0) {
                first_pic_comming = 1;
                ALOGE("render first pic pts:0x%llx", frame->pts);
            }
#ifdef ANDROID4
            mSoftRenderer->render(frame->data, frame->size, NULL);
#endif

#ifdef ANDROID5
            mSoftRenderer->render(frame->data, frame->size, 0,  NULL, mFormatMsg);
#endif
            free(frame->data);
            free(frame);
            ALOGD_IF(mOpenLog, "renderFrame: duration mode: frame->pts =%lld, mFrameDuration=%lld, mPTSDrift=%d, mYuvNo=%d",
                    frame->pts, mFrameDuration, mPTSDrift, mYuvNo++);
            while ((getCurrentTimeUs() - start) / 1000 <= mFrameDuration + mPTSDrift) {
                usleep(2 * 1000);
            }
            mYUVFrameQueue.erase(mYUVFrameQueue.begin());
        }
    }
}

void CTsOmxPlayer::ClearLastFrame() {
    int ret = -1;
    ALOGD("Begin CTsOmxPlayer::ClearLastFrame().\n");
    mKeepLastFrame = false;

    ALOGD("End CTsOmxPlayer::ClearLastFrame().\n");
    return;
}

CTsOmxPlayer::RunWorker::RunWorker(CTsOmxPlayer* player, dt_module_t module) :
    Thread(false), mPlayer(player),mTaskStatus(STATUE_STOPPED),mTaskModule(module) {
}

int32_t CTsOmxPlayer::RunWorker::start () {
    if (mTaskStatus == STATUE_STOPPED) {
        Thread::run("RunWorker");
        mTaskStatus = STATUE_RUNNING;
    } else if (mTaskStatus == STATUE_PAUSE) {
        mTaskStatus = STATUE_RUNNING;
    }
    return 0;
}

int32_t CTsOmxPlayer::RunWorker::stop () {
    LOG_LINE();
    requestExitAndWait();
    mTaskStatus = STATUE_STOPPED;
    return 0;
}

bool CTsOmxPlayer::RunWorker::threadLoop() {
    switch (mTaskModule) {
    case MODULE_EXTRACT:
        mPlayer->readExtractor();//  read av packet
        usleep(5*1000);
        break;
    case MODULE_DECODER:
        mPlayer->readDecoder();//get decoded frame
        usleep(5*1000);
        break;
    case MODULE_VSYNC:
        mPlayer->renderFrame();
        usleep(1000);
        break;
    default:
        ALOGE("RunWorker::threadLoop__error");
    }
    return true;
}
