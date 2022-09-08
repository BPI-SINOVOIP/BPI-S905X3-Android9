#define LOG_TAG "Subtitle"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <utils/CallStack.h>
#include <utils/Log.h>

#include <chrono>
#include <thread>

#include "Subtitle.h"
#include "Parser.h"
#include "ParserFactory.h"

//#include "BitmapDisplay.h"

Subtitle::Subtitle() :
    mSubtitleTracks(0),
    mCurrentSubtitleType(-1),
    mRenderTime(-1),
    mParserNotifier(nullptr),
    mExitRequested(false),
    mThread(nullptr),
    mPendingAction(-1),
    mFd(-1)
 {
    ALOGD("%s", __func__);
}

Subtitle::Subtitle(int fd, ParserEventNotifier *notifier) :
        mSubtitleTracks(0),
        mCurrentSubtitleType(-1),
        mRenderTime(-1),
        mExitRequested(false),
        mThread(nullptr),
        mPendingAction(-1)
{
    ALOGD("%s", __func__);
    mParserNotifier = notifier;
    mFd = fd;

    mSubPrams = std::shared_ptr<SubtitleParamType>(new SubtitleParamType());

    mPresentation = std::shared_ptr<Presentation>(new Presentation(nullptr));
}


Subtitle::~Subtitle() {
    ALOGD("%s", __func__);
    //android::CallStack(LOG_TAG);

    mExitRequested = true;
    mCv.notify_all();
    if (mThread != nullptr) {

        mThread->join();
    }

    if (mDataSource != nullptr) {
        mDataSource->stop();
    }

    if (mParser != nullptr) {
        mParser->stopParse();
        mPresentation->stopPresent();
        mParser = nullptr;
    }
}

void Subtitle::attachDataSource(std::shared_ptr<DataSource> source, std::shared_ptr<InfoChangeListener>listener) {
     ALOGD("%s", __func__);
    mDataSource = source;
    mDataSource->registerInfoListener(listener);
    mDataSource->start();
}

void Subtitle::dettachDataSource(std::shared_ptr<InfoChangeListener>listener) {
    mDataSource->unregisterInfoListener(listener);
}


void Subtitle::onSubtitleChanged(int newTotal) {
    if (newTotal == mSubtitleTracks) return;
    ALOGD("onSubtitleChanged:%d", newTotal);
    mSubtitleTracks = newTotal;
}

void Subtitle::onRenderStartTimestamp(int64_t startTime) {
    //mRenderTime = renderTime;
    if (mPresentation != nullptr) {
        mPresentation->notifyStartTimeStamp(startTime);
    }
}

void Subtitle::onRenderTimeChanged(int64_t renderTime) {
    //ALOGD("onRenderTimeChanged:%lld", renderTime);
    mRenderTime = renderTime;
    if (mPresentation != nullptr) {
        mPresentation->syncCurrentPresentTime(mRenderTime);
    }
}

void Subtitle::onGetLanguage(std::string &lang) {
    mCurrentLanguage = lang;
}

void Subtitle::onTypeChanged(int newType) {

    std::unique_lock<std::mutex> autolock(mMutex);

    if (newType == mCurrentSubtitleType) return;

    ALOGD("onTypeChanged:%d", newType);
    if (newType <= TYPE_SUBTITLE_INVALID || newType >= TYPE_SUBTITLE_MAX) {
        ALOGD("Error! invalid type!%d", newType);
        return;
    }
    if (newType == TYPE_SUBTITLE_DTVKIT_SCTE27) {
        return;
    }
    mCurrentSubtitleType = newType;
    mSubPrams->subType = static_cast<SubtitleType>(newType);

    // need handle
    mPendingAction = ACTION_SUBTITLE_RECEIVED_SUBTYPE;
    mCv.notify_all(); // let handle it
}

int Subtitle::onMediaCurrentPresentationTime(int ptsMills) {
    unsigned int pts = (unsigned int)ptsMills;

    if (mPresentation != nullptr) {
        mPresentation->syncCurrentPresentTime(pts);
    }
    return 0;
}

int Subtitle::getTotalSubtitles() {
    return mSubtitleTracks;
}


int Subtitle::getSubtitleType() {
    return mCurrentSubtitleType;
}
std::string Subtitle::currentLanguage() {
    return mCurrentLanguage;
}

/* currently, we  */
bool Subtitle::setParameter(void *params) {
    std::unique_lock<std::mutex> autolock(mMutex);
    SubtitleParamType *p = new SubtitleParamType();
    *p = *(static_cast<SubtitleParamType*>(params));

    //android::CallStack here("here");

    mSubPrams = std::shared_ptr<SubtitleParamType>(p);
    mSubPrams->update();// need update before use

    //process ttx skip page func.
    if  ((mSubPrams->subType == TYPE_SUBTITLE_DVB_TELETEXT) || (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_TELETEXT)) {
        mPendingAction = ACTION_SUBTITLE_SET_PARAM;
        mCv.notify_all();
        return true;
    }

    mPendingAction = ACTION_SUBTITLE_RECEIVED_SUBTYPE;
    mCv.notify_all();
    return true;
}

bool Subtitle::resetForSeek() {

    mPendingAction = ACTION_SUBTITLE_RESET_FOR_SEEK;
    mCv.notify_all();
    return true;
}

// TODO: actually, not used now
void Subtitle::scheduleStart() {
    ALOGD("scheduleStart:%d", mSubPrams->subType);
    if (nullptr != mDataSource) {
        mDataSource->start();
    }

    mThread = std::shared_ptr<std::thread>(new std::thread(&Subtitle::run, this));
}

// Run in a new thread. any access to this object's member field need protected by lock
void Subtitle::run() {
    // check exit
    ALOGD("run mExitRequested:%d, mSubPrams->subType:%d", mExitRequested, mSubPrams->subType);

    while (!mExitRequested) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::unique_lock<std::mutex> autolock(mMutex);
        //mCv.wait(autolock);
        mCv.wait_for(autolock, std::chrono::milliseconds(100));

        switch (mPendingAction) {
            case ACTION_SUBTITLE_SET_PARAM: {
                if (mParser == nullptr) {
                    mParser = ParserFactory::create(mSubPrams, mDataSource);
                    mParser->startParse(mParserNotifier, mPresentation.get());
                    mPresentation->startPresent(mParser);
                }
                if (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_DVB) {
                    mParser->updateParameter(TYPE_SUBTITLE_DTVKIT_DVB, &mSubPrams->dtvkitDvbParam);
                } else if (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_TELETEXT
                         || mSubPrams->subType == TYPE_SUBTITLE_DVB_TELETEXT) {
                    mParser->updateParameter(TYPE_SUBTITLE_DVB_TELETEXT, &mSubPrams->ttParam);
                } else if (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_SCTE27) {
                    mParser->updateParameter(TYPE_SUBTITLE_DTVKIT_SCTE27, &mSubPrams->scteParam);
                }
            }
            break;
            case ACTION_SUBTITLE_RECEIVED_SUBTYPE: {
                if (mParser != nullptr) {
                    mParser->stopParse();
                    mPresentation->stopPresent();
                    mParser = nullptr;
                }
                mParser = ParserFactory::create(mSubPrams, mDataSource);
                mParser->startParse(mParserNotifier, mPresentation.get());
                mPresentation->startPresent(mParser);
                if (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_DVB) {
                    mParser->updateParameter(TYPE_SUBTITLE_DTVKIT_DVB, &mSubPrams->dtvkitDvbParam);
                } else if (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_TELETEXT) {
                    mParser->updateParameter(TYPE_SUBTITLE_DVB_TELETEXT, &mSubPrams->ttParam);
                } else if (mSubPrams->subType == TYPE_SUBTITLE_DTVKIT_SCTE27) {
                    mParser->updateParameter(TYPE_SUBTITLE_DTVKIT_SCTE27, &mSubPrams->scteParam);
                }
            }
            break;

            case ACTION_SUBTITLE_RESET_FOR_SEEK:
                if (mParser != nullptr) {
                    mParser->resetForSeek();
                }

                if (mPresentation != nullptr) {
                    mPresentation->resetForSeek();
                }
            break;
        }
        // handled
        mPendingAction = -1;

        // wait100ms, still no parser, then start default CC
        if (mParser == nullptr) {
            ALOGD("No parser found, create default!");
            if (mFd > 0) {
                mSubPrams->subType = TYPE_SUBTITLE_EXTERNAL;// if mFd > 0 is Ext sub
            }
            // start default parser, normally, this is CC
            mParser = ParserFactory::create(mSubPrams, mDataSource);
            mParser->startParse(mParserNotifier, mPresentation.get());
            mPresentation->startPresent(mParser);
        }

    }
}

void Subtitle::dump(int fd) {
    dprintf(fd, "\n\n Subtitle:\n");
    dprintf(fd, "--------------------------------------------------------------------------------------\n");
    dprintf(fd, "isExitedRequested? %s\n", mExitRequested?"Yes":"No");
    dprintf(fd, "PendingAction: %d\n", mPendingAction);
    dprintf(fd, "\n");
    dprintf(fd, "DataSource  : %p\n", mDataSource.get());
    dprintf(fd, "Presentation: %p\n", mPresentation.get());
    dprintf(fd, "Parser      : %p\n", mParser.get());
    dprintf(fd, "\n");
    dprintf(fd, "Total Subtitle Tracks: %d\n", mSubtitleTracks);
    dprintf(fd, "Current Subtitle type: %d\n", mCurrentSubtitleType);
    dprintf(fd, "Current Video PTS    : %lld\n", mRenderTime);

    if (mSubPrams != nullptr) {
        dprintf(fd, "\nSubParams:\n");
        mSubPrams->dump(fd, "  ");
    }

    if (mPresentation != nullptr) {
        dprintf(fd, "\nPresentation: %p\n", mPresentation.get());
        mPresentation->dump(fd, "   ");
    }

   // std::shared_ptr<Parser> mParser;
    if (mParser != nullptr) {
        dprintf(fd, "\nParser: %p\n", mParser.get());
        mParser->dump(fd, "   ");
    }


}

