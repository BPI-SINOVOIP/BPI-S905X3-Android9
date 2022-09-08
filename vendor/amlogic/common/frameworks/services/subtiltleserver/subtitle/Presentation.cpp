#define LOG_TAG "Presentation"

#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <utils/Looper.h>

#include <utils/Timers.h>
#include <time.h>

#include "ParserFactory.h"
#include "AndroidHidlRemoteRender.h"
#include "Presentation.h"

static const int DVB_TIME_MULTI = 90;
static inline int64_t convertDvbTime2Ns(int64_t dvbMillis) {
    return ms2ns(dvbMillis)/DVB_TIME_MULTI; // dvbTime is multi 90.
}

static inline int convertNs2DvbTime(int64_t ns) {
    return ns2ms(ns)*DVB_TIME_MULTI;
}

static bool cmpSpu(std::shared_ptr<AML_SPUVAR> a, std::shared_ptr<AML_SPUVAR> b) {
    return a->pts < b->pts;
}

static const int64_t FIVE_SECONDS_NS = 5*1000*1000*1000LL;
static const int64_t ONE_SECONDS_NS = 1*1000*1000*1000LL;

static const int64_t ADDJUST_VERY_SMALL_PTS_MS = 5*1000LL;
static const int64_t ADDJUST_NO_PTS_MS = 10*1000LL;

Presentation::Presentation(std::shared_ptr<Display> disp) :
    mCurrentPresentRelativeTime(0),
    mStartPresentMonoTimeNs(0),
    mStartTimeModifier(0)
{
    std::unique_lock<std::mutex> autolock(mMutex);
    mDisplay = disp;
    mRender = std::shared_ptr<Render>(new AndroidHidlRemoteRender/*SkiaRenderI*/(disp));
    mMsgProcess = nullptr; // only access in threadloop.
}

Presentation::~Presentation() {
    //TODO: do we need poke thread exit immediately? by post a message?
    //      then need add a lock, for protect access mLooper in multi-thread
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        //delete mMsgProcess;
        // We do not delete here, let sp pointer do this!
        mMsgProcess->join();
        mMsgProcess = nullptr; // late sp delete it.
    }
}

bool Presentation::notifyStartTimeStamp(int64_t startTime) {
    mStartTimeModifier = convertDvbTime2Ns(startTime);

    ALOGD("notifyStartTimeStamp: %lld", startTime);
    return true;
}


bool Presentation::syncCurrentPresentTime(int64_t pts) {
    mCurrentPresentRelativeTime = convertDvbTime2Ns(pts);

    // the time
    mStartPresentMonoTimeNs = systemTime(SYSTEM_TIME_MONOTONIC) - convertDvbTime2Ns(pts);

    // Log information, do not rush out too much, throttle to 1/300.
    static int i = 0;
    if (i++%300 == 0) {
        ALOGD("pts:%lld mCurrentPresentRelativeTime:%lld  current:%lld",
            pts, ns2ms(mCurrentPresentRelativeTime), ns2ms(systemTime(SYSTEM_TIME_MONOTONIC)));
    }

    return true;
}


bool Presentation::startPresent(std::shared_ptr<Parser> parser) {
    std::unique_lock<std::mutex> autolock(mMutex);
    mParser = parser;
    //mLooperThread = std::thread(&MessageProcess::looperLoop, this);

    mMsgProcess = new MessageProcess(this);

    // hold a reference for RefBase object
    mMsgProcess->incStrong(nullptr);
    return true;
}

bool Presentation::stopPresent() {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        //delete mMsgProcess;
        // We do not delete here, let sp pointer do this!
        mMsgProcess->join();
        mMsgProcess->decStrong(nullptr);
        mMsgProcess = nullptr; // late sp delete it.
    }
    return true;
}

bool Presentation::resetForSeek() {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        mMsgProcess->notifyMessage(MessageProcess::MSG_RESET_MESSAGE_QUEUE);
    }
    return true;
}

void Presentation::notifySubdataAdded() {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        mMsgProcess->notifyMessage(MessageProcess::MSG_PTS_TIME_CHECK_SPU);
    }
}

void Presentation::dump(int fd, const char *prefix) {
    dprintf(fd, "%s Presentation:\n", prefix);
    dprintf(fd, "%s   CurrentPresentRelativeTime[dvb time]: %d\n",
            prefix, convertNs2DvbTime(mCurrentPresentRelativeTime));
    dprintf(fd, "%s   StartPresentMonoTime[dvb time]: %d\n",
            prefix, convertNs2DvbTime(mStartPresentMonoTimeNs));
    dprintf(fd, "%s   StartTimeModifier[dvb time]: %d\n",
            prefix, convertNs2DvbTime(mStartTimeModifier));
    dprintf(fd, "\n");
    if (mParser != nullptr) {
        dprintf(fd, "%s  Parser : %p\n", prefix, mParser.get());
    }
    if (mDisplay != nullptr) {
        dprintf(fd, "%s  Display: %p\n", prefix, mDisplay.get());
    }
    if (mRender != nullptr) {
        dprintf(fd, "%s  Render : %p\n", prefix, mRender.get());
    }
    dprintf(fd, "\n");

    {
        dprintf(fd, "%s   Ready for showing SPUs:\n", prefix);
        auto it = mEmittedShowingSpu.begin();
        for (; it != mEmittedShowingSpu.end();  ++it) {
            if ((*it) != nullptr) {
                    (*it)->dump(fd, "      ");
            }
        }
    }
    dprintf(fd, "\n");

    {
        dprintf(fd, "%s   Showed but not deleted SPUs:\n", prefix);
        auto it = mEmittedFaddingSpu.begin();
        for (; it != mEmittedFaddingSpu.end();  ++it) {
            if ((*it) != nullptr) {
                    (*it)->dump(fd, "      ");
            }
        }
    }
}

Presentation::MessageProcess::MessageProcess(Presentation *present) {
    mRequestThreadExit = false;
    mPresent = present;
    mLooperThread = std::shared_ptr<std::thread>(new std::thread(&MessageProcess::looperLoop, this));
    mCv.notify_all();
}

Presentation::MessageProcess::~MessageProcess() {
    mPresent = nullptr;
}

void Presentation::MessageProcess::join() {
    mRequestThreadExit = true;
    if (mLooper) {
        mLooper->removeMessages(this, MSG_PTS_TIME_CHECK_SPU);
        mLooper->wake();
    }
    mLooperThread->join();
    mLooper = nullptr;
}

bool Presentation::MessageProcess::notifyMessage(int what) {
    if (mLooper) {
        mLooper->sendMessage(this, Message(what));
    }
    return true;
}

void Presentation::MessageProcess::handleMessage(const Message& message) {
    switch (message.what) {
        case MSG_PTS_TIME_CHECK_SPU: {
            mLooper->removeMessages(this, MSG_PTS_TIME_CHECK_SPU);
            std::shared_ptr<AML_SPUVAR> spu = mPresent->mParser->tryConsumeDecodedItem();

            if (spu != nullptr) {
                // has subtitle to show! Post to render list
                ALOGD("Got  SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%u(%u) duration:%d(%u) data:%p(%p)",
                        ns2ms(mPresent->mCurrentPresentRelativeTime),
                        ns2ms(mPresent->mStartTimeModifier),
                        spu->pts, spu->pts/DVB_TIME_MULTI,
                        spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                        spu->spu_data, spu->spu_data);
                mPresent->mEmittedShowingSpu.push_back(spu);
                mPresent->mEmittedShowingSpu.sort(cmpSpu);
            }

            // handle presentation showing.
            if (mPresent->mEmittedShowingSpu.size() > 0) {
                spu = mPresent->mEmittedShowingSpu.front();
                uint64_t timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;

                //in case seek done, then show out-of-date subtitle
                while (spu != nullptr && spu->m_delay > 0 && (ns2ms(timestamp) > spu->m_delay/DVB_TIME_MULTI) && spu->isExtSub) {
                    mPresent->mEmittedShowingSpu.pop_front();
                    spu = mPresent->mEmittedShowingSpu.front();
                    timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;
                }

                if (spu != nullptr) {
                    uint64_t pts = convertDvbTime2Ns(spu->pts);
                    timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;

                    uint64_t tolerance = 33*1000*1000LL; // 33ms tolerance
                    uint64_t ptsDiff = (pts>timestamp) ? (pts-timestamp) : (timestamp-pts);
                    // The subtitle pts ahead more than 100S of video...maybe aheam more 20s
                    if ((ptsDiff >= 200*1000*1000*1000LL) && !(spu->isExtSub)) {
                        // we cannot check it's valid or not, so delay 1s(common case) and show
                        spu->pts = (unsigned int)convertNs2DvbTime(timestamp+1*1000*1000*1000LL);
                        spu->m_delay = spu->pts + 10*1000*DVB_TIME_MULTI;
                        pts = convertDvbTime2Ns(spu->pts);
                    }

                    if (spu->m_delay <= 0) {
                        spu->m_delay = spu->pts + (ADDJUST_NO_PTS_MS * DVB_TIME_MULTI);
                    }

                    if (spu->isImmediatePresent || (pts <= (timestamp+tolerance))) {
                        mPresent->mEmittedShowingSpu.pop_front();
                        ALOGD("Show SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%u(%u) duration:%d(%u) data:%p(%p)",
                                ns2ms(mPresent->mCurrentPresentRelativeTime),
                                ns2ms(mPresent->mStartTimeModifier),
                                spu->pts, spu->pts/DVB_TIME_MULTI,
                                spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                                spu->spu_data, spu->spu_data);
                        mPresent->mRender->showSubtitleItem(spu, mPresent->mParser->getParseType());

                        // fix fadding time, if not valid.
                        if (spu->isImmediatePresent) {
                            // immediatePresent no pts, this may affect the fading calculate.
                            // apply the real pts and fading delay.
                            spu->pts = convertNs2DvbTime(timestamp);
                            // translate to dvb time. add 10s time
                            spu->m_delay = spu->pts + (ADDJUST_NO_PTS_MS * DVB_TIME_MULTI);
                        } else if ((spu->m_delay == 0) || (spu->m_delay < spu->pts)
                                || (spu->m_delay-spu->pts) < 1000*DVB_TIME_MULTI) {
                            spu->m_delay = spu->pts + (ADDJUST_VERY_SMALL_PTS_MS * DVB_TIME_MULTI);
                        }

                        // add to fadding list. post for fadding
                        std::shared_ptr<AML_SPUVAR> cachedSpu = mPresent->mEmittedFaddingSpu.front();
                        if (cachedSpu != nullptr) {
                            mPresent->mEmittedFaddingSpu.pop_front();
                            mPresent->mRender->removeSubtitleItem(cachedSpu);
                        }
                        mPresent->mEmittedFaddingSpu.push_back(spu);
                    } else {
                        mLooper->sendMessageDelayed(pts-timestamp, this, Message(MSG_PTS_TIME_CHECK_SPU));
                    }
                } else {
                    ALOGE("Error! should not nullptr here!");
                }
            }

            // handle presentation fadding
            if (mPresent->mEmittedFaddingSpu.size() > 0) {
                spu = mPresent->mEmittedFaddingSpu.front();

                if (spu != nullptr) {
                    uint64_t delayed = convertDvbTime2Ns(spu->m_delay);
                    uint64_t timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;
                    uint64_t ahead_delay_tor = ((spu->isExtSub)?5:100)*1000*1000*1000LL;
                    if ((delayed <= timestamp) && (delayed*5 > timestamp)) {
                        mPresent->mEmittedFaddingSpu.pop_front();
                        ALOGD("1 fade SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%u(%u) duration:%d(%u) data:%p(%p)",
                                ns2ms(mPresent->mCurrentPresentRelativeTime),
                                ns2ms(mPresent->mStartTimeModifier),
                                spu->pts, spu->pts/DVB_TIME_MULTI,
                                spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                                spu->spu_data, spu->spu_data);

                        if (spu->isKeepShowing == false) {
                            mPresent->mRender->hideSubtitleItem(spu);
                        } else {
                            mPresent->mRender->removeSubtitleItem(spu);
                        }
                   } else if  ((timestamp != 0) && ((delayed - timestamp) > ahead_delay_tor)) { //when the video gets to begin,to get rid of the sutitle data to avoid the memory leak
                        //because when pull out the cable , the video pts became zero. And the timestamp became zero.
                        //And then it would clear the subtitle data queue which may be used by the dtvkit.It may cause crash as the "bad file description".
                        //so add the "(timestamp != 0)"  condition check.
                        mPresent->mEmittedFaddingSpu.pop_front();
                        ALOGD("2 fade SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%u(%u) duration:%d(%u) data:%p(%p)",
                                ns2ms(mPresent->mCurrentPresentRelativeTime),
                                ns2ms(mPresent->mStartTimeModifier),
                                spu->pts, spu->pts/DVB_TIME_MULTI,
                                spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                                spu->spu_data, spu->spu_data);
                        if (spu->isKeepShowing == false) {
                            mPresent->mRender->hideSubtitleItem(spu);
                        } else {
                            mPresent->mRender->removeSubtitleItem(spu);
                        }
                    }

                   // fire this. can be more than required
                   mLooper->sendMessageDelayed(ms2ns(100), this, Message(MSG_PTS_TIME_CHECK_SPU));
                } else {
                    ALOGE("Error! should not nullptr here!");
                }
            }
        }
        break;

        case MSG_RESET_MESSAGE_QUEUE:
            mPresent->mEmittedShowingSpu.clear();
            mPresent->mEmittedFaddingSpu.clear();
            mPresent->mRender->resetSubtitleItem();
            break;

        default:
        break;
    }
}

void Presentation::MessageProcess::looperLoop() {
    {  // wait until construct finish and run.
        std::unique_lock<std::mutex> autolock(mLock);
        mCv.wait_for(autolock, std::chrono::milliseconds(1000));
    }

    mLooper = new Looper(false);
    mLooper->sendMessageDelayed(100LL, this, Message(MSG_PTS_TIME_CHECK_SPU));
    mLooper->incStrong(nullptr);

    mPresent->mEmittedShowingSpu.clear();
    mPresent->mEmittedFaddingSpu.clear();


    while (!mRequestThreadExit) {
        int32_t ret = mLooper->pollAll(2000);
        switch (ret) {
            case -1:
                ALOGD("ALOOPER_POLL_WAKE\n");
                break;
            case -3: // timeout
                break;
            default:
                ALOGD("default ret=%d", ret);
                break;
        }
    }

    mLooper->decStrong(nullptr);
}

