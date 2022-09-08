#define LOG_TAG "SubtitleService"

#include <stdlib.h>
#include <utils/Log.h>
#include <utils/CallStack.h>
#include "SocketServer.h"
#include "DataSourceFactory.h"


//#include "AndroidDisplay.h"

#include "SubtitleService.h"


SubtitleService::SubtitleService() {
    mIsInfoRetrieved = false;
    ALOGD("%s", __func__);
    mStarted = false;
    mIsInfoRetrieved = false;
    mFmqReceiver = nullptr;
    mDumpMaps = 0;
    mSubtiles = nullptr;
    mDataSource = nullptr;
}

SubtitleService::~SubtitleService() {
    ALOGD("%s", __func__);
    android::CallStack here(LOG_TAG);
    if (mFmqReceiver != nullptr) {
        mFmqReceiver->unregistClient(mDataSource);
    }
}


void SubtitleService::setupDumpRawFlags(int flagMap) {
    mDumpMaps = flagMap;
}

bool SubtitleService::startFmqReceiver(std::unique_ptr<FmqReader> reader) {
    if (mFmqReceiver != nullptr) {
        ALOGD("ALOGE error! why still has a reference?");
        mFmqReceiver = nullptr;
    }

    mFmqReceiver = std::make_unique<FmqReceiver>(std::move(reader));
    ALOGD("add :%p", mFmqReceiver.get());

    if (mDataSource != nullptr) {
        mFmqReceiver->registClient(mDataSource);
    }

    return true;
}
bool SubtitleService::stopFmqReceiver() {
    if (mFmqReceiver != nullptr && mDataSource != nullptr) {
        ALOGD("release :%p", mFmqReceiver.get());
        mFmqReceiver->unregistClient(mDataSource);
        mFmqReceiver = nullptr;
        return true;
    }

    return false;
}

bool SubtitleService::startSubtitle(int fd, SubtitleIOType type, ParserEventNotifier *notifier) {
    ALOGD("%s  fd:%d, type:%d", __func__, fd, type);
        std::unique_lock<std::mutex> autolock(mLock);
        if (mStarted) {
            ALOGD("Already started, exit");
            return false;
        } else {
            mStarted = true;
        }

    std::shared_ptr<Subtitle> subtitle(new Subtitle(fd, notifier));

    std::shared_ptr<DataSource> datasource = DataSourceFactory::create(fd, type);

    if (mDumpMaps & (1<<SUBTITLE_DUMP_SOURCE)) {
        datasource->enableSourceDump(true);
    }
    mDataSource = datasource;
    if (TYPE_SUBTITLE_DTVKIT_DVB == mSubParam.subType) {
           mDataSource->updateParameter(mSubParam.subType, &mSubParam.dtvkitDvbParam);
    } else if (TYPE_SUBTITLE_DTVKIT_TELETEXT == mSubParam.subType) {
           mDataSource->updateParameter(mSubParam.subType, &mSubParam.ttParam);
    } else if (TYPE_SUBTITLE_DTVKIT_SCTE27== mSubParam.subType) {
           mDataSource->updateParameter(mSubParam.subType, &mSubParam.scteParam);
    }
   // schedule subtitle
    subtitle->scheduleStart();

   // schedule subtitle
    //subtitle.attach input source
    subtitle->attachDataSource(datasource, subtitle);

    // subtitle.attach display

   // schedule subtitle
//    subtitle->scheduleStart();
    //we only update params for DTV. because we do not wait dtv and
    // CC parser run the same time
    if (mSubParam.isValidDtvParams()) {
        subtitle->setParameter(&mSubParam);
    }

    mSubtiles = subtitle;
    mDataSource = datasource;
    if (mFmqReceiver != nullptr) {
        mFmqReceiver->registClient(mDataSource);
    }
    return true;
}

bool SubtitleService::resetForSeek() {

    if (mSubtiles != nullptr) {
        return mSubtiles->resetForSeek();
    }

    return false;
}



int SubtitleService::updateVideoPosAt(int timeMills) {
    static int test = 0;
    if (test++ %100 == 0)
        ALOGD("%s： %d(called %d times)", __func__, timeMills, test);

    if (mSubtiles) {
        return mSubtiles->onMediaCurrentPresentationTime(timeMills);
    }

    return -1;
;
}


//when play by ctc, this operation is  before startSubtitle,
//so keep struct mSub_param_t and when startSubtitle setParameter
void SubtitleService::setSubType(int type) {
    mSubParam.dtvSubType = (DtvSubtitleType)type;
    mSubParam.update();
    //return 0;
}

void SubtitleService::setDemuxId(int demuxId) {
    switch (mSubParam.dtvSubType) {
        case  DTV_SUB_DTVKIT_SCTE27:
            mSubParam.scteParam.demuxId = demuxId;
        break;
        case DTV_SUB_DTVKIT_DVB:
            mSubParam.dtvkitDvbParam.demuxId = demuxId;
        break;
        case DTV_SUB_DTVKIT_TELETEXT:
            mSubParam.ttParam.demuxId = demuxId;
        break;
        default:
        break;
    }
    if (NULL == mDataSource )
      return;
    if (mSubParam.subType == TYPE_SUBTITLE_DTVKIT_DVB)  {
        mDataSource->updateParameter(mSubParam.subType, &mSubParam.dtvkitDvbParam);
    } else if (mSubParam.subType == TYPE_SUBTITLE_DTVKIT_TELETEXT) {
        mDataSource->updateParameter(mSubParam.subType, &mSubParam.ttParam);
    } else if (mSubParam.subType == TYPE_SUBTITLE_DTVKIT_SCTE27) {
        mDataSource->updateParameter(mSubParam.subType, &mSubParam.scteParam);
    }
}
void SubtitleService::setSubPid(int pid) {
    switch (mSubParam.dtvSubType) {
        case  DTV_SUB_SCTE27:
            [[fallthrough]];
        case  DTV_SUB_DTVKIT_SCTE27:
            mSubParam.scteParam.SCTE27_PID = pid;
        break;
        case DTV_SUB_DTVKIT_DVB:
            mSubParam.dtvkitDvbParam.pid = pid;
        break;
        case DTV_SUB_DTVKIT_TELETEXT:
            mSubParam.ttParam.pid = pid;
        break;
        default:
        break;
    }
    if (NULL == mDataSource )
      return;
    if (mSubParam.subType == TYPE_SUBTITLE_DTVKIT_DVB)  {
           mDataSource->updateParameter(mSubParam.subType, &mSubParam.dtvkitDvbParam);
    } else if (mSubParam.subType == TYPE_SUBTITLE_DTVKIT_TELETEXT) {
           mDataSource->updateParameter(mSubParam.subType, &mSubParam.ttParam);
    }
}

void SubtitleService::setSubPageId(int pageId) {
    switch (mSubParam.dtvSubType) {
        case DTV_SUB_DTVKIT_DVB:
            mSubParam.dtvkitDvbParam.compositionId = pageId;
        break;
        case DTV_SUB_DTVKIT_TELETEXT:
            mSubParam.ttParam.magazine = pageId;
        break;
        default:
        break;
    }
}
void SubtitleService::setSubAncPageId(int ancPageId) {
    switch (mSubParam.dtvSubType) {
        case DTV_SUB_DTVKIT_DVB:
            mSubParam.dtvkitDvbParam.ancillaryId = ancPageId;
        break;
        case DTV_SUB_DTVKIT_TELETEXT:
            mSubParam.ttParam.page = ancPageId;
        break;
        default:
        break;
    }
}

void SubtitleService::setChannelId(int channelId) {
    mSubParam.ccParam.ChannelID = channelId;
}


void SubtitleService::setClosedCaptionVfmt(int vfmt) {
     mSubParam.ccParam.vfmt = vfmt;
}

bool SubtitleService::ttControl(int cmd, int pageNo, int subPageNo, int pageDir, int subPageDir) {
    // DO NOT update the entire parameter.
    ALOGD(" ttControl cmd:%d, pageNo=%d, subPageNo:%d",cmd, pageNo, subPageNo);

    switch (cmd) {
        case TT_EVENT_GO_TO_PAGE:
        case TT_EVENT_GO_TO_SUBTITLE:
            mSubParam.ttParam.pageNo = pageNo;
            mSubParam.ttParam.subPageNo = subPageNo;
            break;
    }
    mSubParam.ttParam.event = (TeletextEvent)cmd;
    mSubParam.subType = TYPE_SUBTITLE_DVB_TELETEXT;
    ALOGD("event=%d", mSubParam.ttParam.event);
    if (mSubtiles != nullptr) {
        return mSubtiles->setParameter(&mSubParam);
    }
    ALOGD("%s mSubtiles null", __func__);

    return false;
}

int SubtitleService::ttGoHome() {
    ALOGD("%s ", __func__);
    return mSubtiles->setParameter(&mSubParam);
}

int SubtitleService::ttGotoPage(int pageNo, int subPageNo) {
    ALOGD("debug##%s ", __func__);
    mSubParam.ttParam.ctrlCmd = CMD_GO_TO_PAGE;
    mSubParam.ttParam.pageNo = pageNo;
    mSubParam.ttParam.subPageNo = subPageNo;
    mSubParam.subType = TYPE_SUBTITLE_DVB_TELETEXT;
    return mSubtiles->setParameter(&mSubParam);

}

int SubtitleService::ttNextPage(int dir) {
    ALOGD("debug##%s ", __func__);
    mSubParam.ttParam.ctrlCmd = CMD_NEXT_PAGE;
    mSubParam.subType = TYPE_SUBTITLE_DVB_TELETEXT;
    mSubParam.ttParam.pageDir = dir;
    return mSubtiles->setParameter(&mSubParam);
}


int SubtitleService::ttNextSubPage(int dir) {
    mSubParam.ttParam.ctrlCmd = CMD_NEXT_SUB_PAGE;
    mSubParam.subType = TYPE_SUBTITLE_DVB_TELETEXT;
    mSubParam.ttParam.subPageDir = dir;
    return mSubtiles->setParameter(&mSubParam);
}

bool SubtitleService::userDataOpen(ParserEventNotifier *notifier) {
    ALOGD("%s ", __func__);
    //default start userdata monitor afd
    if (mUserDataAfd == nullptr) {
        mUserDataAfd = std::shared_ptr<UserDataAfd>(new UserDataAfd());
        mUserDataAfd->start(notifier);
    }
    return true;
}

bool SubtitleService::userDataClose() {
    ALOGD("%s ", __func__);
    if (mUserDataAfd != nullptr) {
        mUserDataAfd->stop();
        mUserDataAfd = nullptr;
    }

    return true;
}

bool SubtitleService::stopSubtitle() {
    ALOGD("%s", __func__);
    std::unique_lock<std::mutex> autolock(mLock);
    if (!mStarted) {
        ALOGD("Already stopped, exit");
        return false;
    } else {
        mStarted = false;
    }

    if (mSubtiles != nullptr) {
        mSubtiles->dettachDataSource(mSubtiles);
    }

    if (mFmqReceiver != nullptr && mDataSource != nullptr) {
        mFmqReceiver->unregistClient(mDataSource);
        mFmqReceiver = nullptr;
    }

    mDataSource = nullptr;
    mSubtiles = nullptr;
    mSubParam.subType = TYPE_SUBTITLE_INVALID;
    mSubParam.dtvSubType = DTV_SUB_INVALID;
    return true;
}

int SubtitleService::totalSubtitles() {
    if (mSubtiles == nullptr) {
        ALOGE("Not ready or exited, ignore request!");
        return 0;
    }
    // TODO: impl a state of subtitle, throw error when call in wrong state
    // currently we simply wait a while and return
    if (mIsInfoRetrieved) {
        return mSubtiles->getTotalSubtitles();
    }
    for (int i=0; i<5; i++) {
        if ((mSubtiles != nullptr) && (mSubtiles->getTotalSubtitles() >= 0)) {
            return mSubtiles->getTotalSubtitles();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    mIsInfoRetrieved = true;
    return 0;
}

int SubtitleService::subtitleType() {
    // TODO: impl a state of subtitle, throw error when call in wrong state
    return mSubtiles->getSubtitleType();
}

std::string SubtitleService::currentLanguage() {
    if (mSubtiles != nullptr) {
        return mSubtiles->currentLanguage();
    }

    return std::string("N/A");
}


void SubtitleService::dump(int fd) {
    dprintf(fd, "\n\n SubtitleService:\n");
    dprintf(fd, "--------------------------------------------------------------------------------------\n");

    dprintf(fd, "SubParams:\n");
    dprintf(fd, "    isDtvSub: %d", mSubParam.isValidDtvParams());
    mSubParam.dump(fd, "    ");

    if (mFmqReceiver != nullptr) {
        dprintf(fd, "\nFastMessageQueue: %p\n", mFmqReceiver.get());
        mFmqReceiver->dump(fd, "    ");
    }

    dprintf(fd, "\nDataSource: %p\n", mDataSource.get());
    if (mDataSource != nullptr) {
        mDataSource->dump(fd, "    ");
    }

    if (mSubtiles != nullptr) {
        return mSubtiles->dump(fd);
    }
}

