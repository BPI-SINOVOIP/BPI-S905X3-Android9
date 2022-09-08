#ifndef __SUBTILE_SUBTITLE_H__
#define __SUBTILE_SUBTITLE_H__

#include <thread>
#include "InfoChangeListener.h"
#include "Parser.h"
#include "Presentation.h"
#include "ParserFactory.h"
#include <UserDataAfd.h>



class Subtitle : public InfoChangeListener {

public:
    Subtitle();
    Subtitle(int fd, ParserEventNotifier *notifier);
    virtual ~Subtitle();

    void attachDataSource(std::shared_ptr<DataSource> source, std::shared_ptr<InfoChangeListener>listener);
    void dettachDataSource(std::shared_ptr<InfoChangeListener>listener);

    int getTotalSubtitles();
    int getSubtitleType();
    std::string currentLanguage();

    void scheduleStart();
    bool resetForSeek();


// from Info Listener:
    void onSubtitleChanged(int newTotal);
    void onRenderStartTimestamp(int64_t startTime);
    void onRenderTimeChanged(int64_t renderTime);
    void onTypeChanged(int newType);
    void onGetLanguage(std::string &lang);

    //ttx control api
    int ttGoHome();
    int ttGotoPage(int pageNo, int subPageNo);
    int ttNextPage(int dir);
    int ttNextSubPage(int dir);


    int onMediaCurrentPresentationTime(int ptsMills);

    bool setParameter(void *params);

    void dump(int fd);

private:
    static const int ACTION_SUBTITLE_RECEIVED_SUBTYPE = 1;
    static const int ACTION_SUBTITLE_RESET_FOR_SEEK = 2;
    static const int ACTION_SUBTITLE_SET_PARAM = 3;
    void run();

    std::shared_ptr<DataSource> mDataSource;

    int mSubtitleTracks;
    int mCurrentSubtitleType;
    std::string mCurrentLanguage;

    int64_t mRenderTime;

    std::shared_ptr<Presentation> mPresentation;

    // TODO: create a manager for this. also, not use this
    std::shared_ptr<Parser> mParser;
    std::shared_ptr<UserDataAfd> mUserDataAfd;

    ParserEventNotifier *mParserNotifier;

    bool mExitRequested;
    std::shared_ptr<std::thread> mThread;

    std::shared_ptr<SubtitleParamType> mSubPrams;

    int mPendingAction;
    std::mutex mMutex;
    std::condition_variable mCv;

    int mFd;


};

#endif
