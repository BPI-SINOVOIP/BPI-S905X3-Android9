//#define LOG_NDEBUG 0
#define LOG_TAG "testCtsPlayer"
#include <utils/Log.h>

#include <utils/RefBase.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ADebug.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
//#include <gui/Surface.h>
//#include <gui/SurfaceComposerClient.h>
#include <iostream>
#include <libgen.h>
#include <math.h>
#include <binder/ProcessState.h>

#include <CTC_MediaProcessor.h>
#include "../media_processor/CTC_Utils.h"


using namespace android;

static bool g_quit = false;

/////////////////////////////////////////

struct Env
{
    Env(const char* file, int vpid = 0, int apid = 0, int spid = 0);
    ~Env();

    CTC_MediaInfo mMediaInfo;
    VIDEO_PARA_T mVideoPara;
    AUDIO_PARA_T mAudioPara;
    SUBTITLE_PARA_T mSubtitlePara;
    int64_t mBitrate = 0;
    int mVpid = 0;
    int mApid = 0;
    int mSpid = 0;
};

Env::Env(const char* file, int vpid, int apid, int spid)
: mVpid(vpid)
, mApid(apid)
, mSpid(spid)
{
    memset(&mVideoPara, 0, sizeof(VIDEO_PARA_T));
    memset(&mAudioPara, 0, sizeof(AUDIO_PARA_T));
    memset(&mSubtitlePara, 0, sizeof(SUBTITLE_PARA_T));

    getMediaInfo(file, &mMediaInfo);

    if (vpid == 0 || mMediaInfo.vPara.pid == vpid) {
        mVideoPara = mMediaInfo.vPara;
    }

    for (size_t i = 0; i < mMediaInfo.aParaCount; ++i) {
        if (apid == 0 || mMediaInfo.aParas[i].pid == apid) {
            mAudioPara = mMediaInfo.aParas[i];
            
            break;
        }
    }

    for (size_t i = 0; i < mMediaInfo.sParaCcount; ++i) {
        if (spid == 0 || mMediaInfo.aParas[i].pid == spid) {
            mSubtitlePara = mMediaInfo.sParas[i];

            break;
        }
    }

    mBitrate = mMediaInfo.bitrate;

}

Env::~Env()
{
    freeMediaInfo(&mMediaInfo);
}

///////////////////////////////////////////////////
struct SourceFeeder : public AHandler
{
    enum FeedMode {
        FEED_MODE_FAST,
        FEED_MODE_CONSTANT,
    };

    explicit SourceFeeder(aml::CTC_MediaProcessor*& player, int fd, int64_t bitrate, FeedMode feedMode);
    void init();
    int start();
    int stop();

protected:
    ~SourceFeeder();
    void onMessageReceived(const sp<AMessage>& msg);

private:
    enum {
        kWhatFeedData = 'fDat',
    };

    static constexpr int kFeedChunkSize = 13160;

    void onFeedData(const sp<AMessage>& msg);
    int64_t calcFeedDelay();

    aml::CTC_MediaProcessor*& mPlayer;
    int mFd = -1;
    int64_t mBitrate = -1;
    FeedMode mFeedMode;
    int64_t mFeedDurationUs = -1;
    int64_t mFeedBeginTimeUs = -1;

    sp<ALooper> mLooper;
    bool mStarted = false;

    uint8_t* mFeedBuffer = nullptr;

    DISALLOW_EVIL_CONSTRUCTORS(SourceFeeder);
};

SourceFeeder::SourceFeeder(aml::CTC_MediaProcessor*& player, int fd, int64_t bitrate, FeedMode feedMode)
: mPlayer(player)
, mFd(::dup(fd))
, mBitrate(bitrate)
, mFeedMode(feedMode)
{
    if (mBitrate > 0) {

        mBitrate *= 1.04;
        
        int mFeedFreq = std::max(mBitrate / (kFeedChunkSize * 8), (int64_t)1);
        mFeedDurationUs = 1000000 / mFeedFreq;
    }
    ALOGI("mFd:%d, bitrate:%lld, mFeedDurationUs:%lld, %.2fms", 
            mFd, mBitrate, mFeedDurationUs, mFeedDurationUs/1e3);

    mLooper = new ALooper();
    mLooper->setName("SourceFeeder");

    mFeedBuffer = new uint8_t[kFeedChunkSize];
}

SourceFeeder::~SourceFeeder()
{
    stop();

    delete[] mFeedBuffer;

    if (mFd > 0) {
        ::close(mFd);
        mFd = -1;
    }
}

void SourceFeeder::init()
{
    mLooper->registerHandler(this);
}

int SourceFeeder::start()
{
    mLooper->start();
    mStarted = true;

    sp<AMessage> msg = new AMessage(kWhatFeedData, this);
    msg->post();
    
    return OK;
}

int SourceFeeder::stop()
{
    if (!mStarted)
        return OK;

    if (mLooper != nullptr) {
        mLooper->unregisterHandler(this->id());
        mLooper->stop();
    }

    mStarted = false;

    return OK;
}

void SourceFeeder::onMessageReceived(const sp<AMessage>& msg)
{
    switch (msg->what()) {
    case kWhatFeedData:
        onFeedData(msg);
        break;

    default:
        TRESPASS();
        break;
    }
}

void SourceFeeder::onFeedData(const sp<AMessage>& msg)
{
    mFeedBeginTimeUs = ALooper::GetNowUs();

doRead:
    int ret = ::read(mFd, mFeedBuffer, kFeedChunkSize);
    if (ret < 0) {
        ALOGE("read source failed: %s", strerror(errno));
        return;
    } else if (ret == 0) {
        ALOGI("read source EOF!");
#if 0
        return;
#endif

        ALOGI("seek to begin and feed again...");
        ret = ::lseek(mFd, SEEK_SET, 0);
        if (ret < 0) {
            ALOGE("lseek failed: %s", strerror(errno));
            return;
        }

        goto doRead;
    }


    while (ret > 0) {
        if (g_quit) 
            break;

        int len = mPlayer->WriteData(mFeedBuffer, ret);
        if (len < 0) {
            usleep(10000);
            continue;
        }

        if (len == 0) 
            break;

        ret -= len;
    }

    int64_t delayUs = calcFeedDelay();
    msg->post(delayUs);
}

int64_t SourceFeeder::calcFeedDelay()
{
    if (mFeedMode == FEED_MODE_FAST)
        return 0;

    int64_t feedCostUs = ALooper::GetNowUs() - mFeedBeginTimeUs;

    int64_t delayUs = std::max(mFeedDurationUs - feedCostUs, (int64_t)0);
    ALOGV("delayUs:%.2f ms", delayUs/1e3);

    return delayUs;
}


///////////////////////////////////////////////////
static void set_sysfs_str(const char* path, const char* value)
{
    int fd;

    fd = ::open(path, O_WRONLY);
    if (fd < 0) {
        printf("open %s failed, %s\n", path, strerror(errno));
        return;
    }

    if (::write(fd, value, strlen(value)+1) < 0) {
        printf("write %s failed, %s\n", path, strerror(errno));
    }

    ::close(fd);
}


int main(int argc, char *argv[])
{
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    const char* file = "/data/a.ts";
    int vpid = 0;
    int apid = 0;
    int spid = 0;
    if (argc < 2) {
        printf("Usage: %s [filepath] -v <pid> -a <pid> -s <pid>\n", basename(argv[0]));
        return 0;
    }

    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'v':
                vpid = atoi(argv[i+1]);
                break;

            case 'a':
                apid = atoi(argv[i+1]);
                break;

            case 's':
                spid = atoi(argv[i+1]);
                break;
            } 

            ++i;
        } else {
            file = argv[i];
        }
    }

    printf("test_file: %s\n", file);

    signal(SIGINT, [](int sig) {
        printf("%s\n", strsignal(sig));
        g_quit = true;
    });

    Env env(file, vpid, apid, spid);

#if 1
    int ret = atexit([]() {
#if ANDROID_PLATFORM_SDK_VERSION >= 28
        set_sysfs_str("/sys/class/graphics/fb0/osd_display_debug", "0");
#endif
        set_sysfs_str("/sys/class/graphics/fb0/blank", "0");
    });

    if (ret < 0) {
        printf("call atexit failed! %s\n", strerror(errno));
    }

start:
    int fd = -1;
    fd = ::open(file, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed: %s\n", file, strerror(errno));
        return -1;
    }

    aml::CTC_MediaProcessor* tsplayer = aml::GetMediaProcessor(0);
    tsplayer->InitVideo(&env.mVideoPara);
    tsplayer->InitAudio(&env.mAudioPara);
    tsplayer->InitSubtitle(&env.mSubtitlePara);
    if (!tsplayer->StartPlay()) {
        printf("start play failed!\n");
        return -1;
    }

#if ANDROID_PLATFORM_SDK_VERSION >= 28
    set_sysfs_str("/sys/class/graphics/fb0/osd_display_debug", "1");
#endif
    set_sysfs_str("/sys/class/graphics/fb0/blank", "1");
#endif

    sp<SourceFeeder> sourceFeeder = new SourceFeeder(
            tsplayer, fd, env.mBitrate, SourceFeeder::FEED_MODE_CONSTANT);
    sourceFeeder->init();
    sourceFeeder->start();

    while (!g_quit) {
        if (tcgetpgrp(0) != getpid()) {
            usleep(10*1000);
            continue;
        }

        struct pollfd fds;
        fds.fd = STDIN_FILENO;
        fds.events = POLL_IN;
        fds.revents = 0;
        int ret = ::poll(&fds, 1, 1000);
        if (ret < 0) {
            //printf("poll STDIN_FILENO failed! %d\n", -errno);
        } else if (ret > 0) {
            if (fds.revents & POLL_ERR) {
                printf("poll error!\n");
                continue;
            } else if (!(fds.revents & POLL_IN)) {
                continue;
            }

            char buffer[10];
            int len = ::read(STDIN_FILENO, buffer, sizeof(buffer));
            if (len < 0) {
                printf("read failed! %d", -errno);
                continue;
            }
            //printf("read buf:%s, size:%d\n", buffer, len);
            std::string buf(buffer, len);
            auto b = buf.find_first_not_of(' ');
            auto e = buf.find_last_not_of(' ');
            if (b != std::string::npos && e != std::string::npos)
                buf = buf.substr(b, e-b+1);

            if (buf.empty())
                continue;

            //printf("%s, buf[0]:%c", buf.c_str(), buf[0]);
            switch (buf[0]) {
            case 'f':
                printf("flush\n");
                tsplayer->Seek();
                break;

            case 'r':
            {
                float rate = 1.0f;
                buf = buf.substr(1);
                auto b = buf.find_first_not_of(' ');
                buf = buf.substr(b);
                rate = atof(buf.c_str());
                printf("rate = %.2f\n", rate);

                //if (rate > 0.1f && rate < 2.0f)
                    //player->setPlaybackRate(rate);
            }
            break;

            case 'l':
            {
                printf("loop play test\n");
                sourceFeeder->stop();
                sourceFeeder.clear();

                tsplayer->Stop();
                //player->destroy();
                delete tsplayer;

                ::close(fd);
                fd = -1;
                printf("pld player stopepd, create new one!\n");
                goto start;
            }
            break;

            case 't':
            {
                //allAudio ^= 1;
                //printf("allAudio:%d\n", allAudio);
                //player->change(allAudio);
            }
            break;

            case 's':
            {
                float speed = 1.5f;
                printf("set speed:%.2f\n", speed);
                //player->setSpeed(speed);
            }
            break;

            //open
            case 'o':
            {
                switch (buf[1]) {
                case 'a':
                    printf("open audio\n");
                    //tsplayer->StartAudio();
                    break;

                case 'v':
                    printf("open video\n");
                    //tsplayer->StartVideo();
                    break;

                case 's':
                    printf("open subtitle\n");
                    //tsplayer->StartSubtitle();
                    break;
                }
            }
            break;

            //close
            case 'c':
            {
                switch (buf[1]) {
                case 'a':
                    printf("close audio\n");
                    //tsplayer->StopAudio();
                    break;

                case 'v':
                    printf("close video\n");
                    //tsplayer->StopVideo();
                    break;

                case 's':
                    printf("close subtitle\n");
                    //tsplayer->StopSubtitle();
                    break;
                }
            }
            break;

            //enable
            case 'e':
            {
                switch (buf[1]) {
                case 'a':
                    printf("enable audio output\n");
                    //tsplayer->EnableAudioOutput();
                    break;

                case 'v':
                    printf("enable video output\n");
                    //tsplayer->VideoShow();
                    break;

                case 's':
                    printf("enable subtitle output\n");
                    //tsplayer->SubtitleShowHide(true);
                    break;
                }
            }
            break;

            //disable
            case 'd':
            {
                switch (buf[1]) {
                case 'a':
                    printf("disable audio output\n");
                    //tsplayer->DisableAudioOutput();
                    break;

                case 'v':
                    printf("disable video output\n");
                    //tsplayer->VideoHide();
                    break;

                case 's':
                    printf("disable subtitle output\n");
                    //tsplayer->SubtitleShowHide(false);
                    break;
                }
            }
            break;

            default:
                //printf("unknown command: %c\n", buf[0]);
                break;
            }
        }
    }

    printf("exiting...");
    sourceFeeder->stop();
    sourceFeeder.clear();


    ::close(fd);
    fd = -1;

    printf("exited!\n");
    
    return 0;
}
