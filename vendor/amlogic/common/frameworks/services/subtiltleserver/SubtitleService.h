
#ifndef __SUBTITLE_SUBTITLESERVICE_H__
#define __SUBTITLE_SUBTITLESERVICE_H__

#include <vector>

#include "SocketServer.h"
#include "FmqReceiver.h"
#include "Subtitle.h"
#include "ParserFactory.h"
#include "ParserEventNotifier.h"

enum {
    SUBTITLE_DUMP_SOURCE = 0,
};

class SubtitleService {
public:
    SubtitleService();
    ~SubtitleService();

    // TODO: revise params and defines
    bool startSubtitle(int fd, SubtitleIOType type, ParserEventNotifier *notifier);
    bool stopSubtitle();
    bool resetForSeek();

    /* return total subtitles associate with current video */
    int totalSubtitles();
    int subtitleType();
    std::string currentLanguage();


    int updateVideoPosAt(int timeAt);
    void setSubType(int type);
    void setSubPid(int pid);
    void setSubPageId(int pageId);
    void setSubAncPageId(int ancPageId);
    void setClosedCaptionVfmt(int vfmt);
    void setChannelId(int channelId);
    void setDemuxId(int demuxId);

    //ttx control api
    bool ttControl(int cmd, int pageNo, int subPageNo, int pageDir, int subPageDir);
    int ttGoHome();
    int ttGotoPage(int pageNo, int subPageNo);
    int ttNextPage(int dir);
    int ttNextSubPage(int dir);

    bool userDataOpen(ParserEventNotifier *notifier);
    bool userDataClose();


    // debug inteface.
    void dump(int fd);
    void setupDumpRawFlags(int flagMap);

    bool startFmqReceiver(std::unique_ptr<FmqReader> reader);
    bool stopFmqReceiver();

private:
    std::shared_ptr<Subtitle> mSubtiles;

    SubtitleParamType mSubParam;

    std::shared_ptr<DataSource> mDataSource;
    std::unique_ptr<FmqReceiver> mFmqReceiver;
    std::shared_ptr<UserDataAfd> mUserDataAfd;

    int mDumpMaps;

    std::mutex mLock;
    bool mStarted;

    // API may request subtitle info immediately, but
    // parser may not parsed the info, so here we delayed a while.
    // but, if the subtitle not have this, may continuing delay.
    // here, this flag, only delay once.
    bool mIsInfoRetrieved;
};

#endif

