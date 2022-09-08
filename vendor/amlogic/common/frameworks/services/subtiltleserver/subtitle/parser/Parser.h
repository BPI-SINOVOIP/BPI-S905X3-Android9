#ifndef __SUBTITLE_PARSER_H__
#define __SUBTITLE_PARSER_H__

#include <list>
#include <vector>
#include <thread>
#include "DataSource.h"
#include "sub_types.h"
#include "sub_types2.h"   //for cc scte27
#include "cutils/properties.h"
#include "ParserEventNotifier.h"
#include <sys/types.h>
#include "VideoInfo.h"


#define SUB_INIT        0
#define SUB_PLAYING     1
#define SUB_STOP        2
#define SUB_EXIT        3

#define VOB_SUB_WIDTH 1920
#define VOB_SUB_HEIGHT 1280
#define VOB_SUB_SIZE VOB_SUB_WIDTH*VOB_SUB_HEIGHT/4
#define DVB_SUB_SIZE VOB_SUB_WIDTH*VOB_SUB_HEIGHT*4
#define SCTE27_SUB_SIZE 1920*1080*4

#define INTERNAL_MAX_NUMBER_SPU_ITEM 20
#define EXTERNAL_MAX_NUMBER_SPU_ITEM 500



static const int AML_PARSER_SYNC_WORD = 'AMLU';


class Parser {
public:
    Parser() {
        mParseType = -1;
        mNotifier = nullptr;
        mDataNotifier = nullptr;
        mThreadExitRequested = false;
        mState = -1;
        mMaxSpuItems = INTERNAL_MAX_NUMBER_SPU_ITEM;
    }
    virtual ~Parser() {
        mState = SUB_EXIT;
    };

    virtual int parse(/*params*/) = 0;

    /* default impl not used. for some parser to override */
    virtual bool updateParameter(int type, void *data) {
        (void) type;
        (void) data;
        return false;
    }

   virtual void resetForSeek() {
        std::unique_lock<std::mutex> autolock(mMutex);
        mDecodedSpu.clear();
    }

    bool startParse(ParserEventNotifier *notify, ParserSubdataNotifier *dataNofity) {
        mState = SUB_INIT;
        mNotifier = notify;
        mDataNotifier = dataNofity;
        mThread = std::thread(&Parser::_parserEntry, this);
        return true;
    }

    int getParseType(){
        return mParseType;
    }

    bool stopParse() {
        mState = SUB_STOP;
        if (mThreadExitRequested)
            return false;

        mThreadExitRequested = true;
        mThread.join();
        return true;
    }

    // TODO: split to a spu manager class
    std::shared_ptr<AML_SPUVAR> consumeDecodedItem() {
        std::shared_ptr<AML_SPUVAR> item;
        std::unique_lock<std::mutex> autolock(mMutex);
        while (mDecodedSpu.size() == 0) {
            mCv.wait(autolock);
        }
        item = mDecodedSpu.front();
        mDecodedSpu.pop_front();
        return item;
    }

   virtual std::shared_ptr<AML_SPUVAR> tryConsumeDecodedItem() {
        std::shared_ptr<AML_SPUVAR> item;
        std::unique_lock<std::mutex> autolock(mMutex);
        if (mDecodedSpu.size() == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "Parser", "in parser item->spu_data null");
            return nullptr;
        }
        item = mDecodedSpu.front();
        mDecodedSpu.pop_front();

        return item;
    }

    virtual void addDecodedItem(std::shared_ptr<AML_SPUVAR> item) {
        std::unique_lock<std::mutex> autolock(mMutex);

        // Do not add when exited, it's useless.
        if (mState == SUB_STOP || mState == SUB_EXIT) {
            return;
        }
        while (mDecodedSpu.size() >= mMaxSpuItems) {
            mDecodedSpu.pop_front();
        }

        // validate the item: do not sent empty items due to parser error.
        if (item == nullptr || item->spu_data == nullptr) {
            __android_log_print(ANDROID_LOG_ERROR, "Parser", "add invalid empty spu!");
            return;
        }

        mDecodedSpu.push_back(item);
        if (mDataNotifier != nullptr) mDataNotifier->notifySubdataAdded();
        mCv.notify_all();
    }
    virtual void dump(int fd, const char *prefix) = 0;
    std::shared_ptr<DataSource> mDataSource;


protected:
    ParserEventNotifier *mNotifier;
    // TODO: included in  spu manager class
    ParserSubdataNotifier *mDataNotifier;
    bool mThreadExitRequested;
    int mState;
    int mParseType;
    unsigned int mMaxSpuItems;


    void dumpCommon(int fd, const char *prefix) {
        dprintf(fd, "%s DataSource=%p\n", prefix, mDataSource.get());
        dprintf(fd, "%s ExitRequested ? %s\n", prefix, mThreadExitRequested?"yes":"no");
        dprintf(fd, "%s State=%d\n", prefix, mState);
        dprintf(fd, "%s Parser Type=%d\n", prefix, mParseType);
        dprintf(fd, "%s Max allowed SPUs (%d)\n", prefix, mMaxSpuItems);
    }
    std::list<std::shared_ptr<AML_SPUVAR>> mDecodedSpu;
    std::thread mThread;

private:

    std::mutex mMutex;
    std::condition_variable mCv;

    void _parserEntry() {
        while (!mThreadExitRequested && mState != SUB_STOP && mState != SUB_EXIT) {
            parse();
        }
    }

};

#endif
