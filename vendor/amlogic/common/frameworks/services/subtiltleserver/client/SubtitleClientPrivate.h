#ifndef _SUBTITLE_CLIENT_PRIVATE_H_
#define _SUBTITLE_CLIENT_PRIVATE_H_

#include "SubtitleClient.h"
#include "subtitleServerHidlClient/SubtitleServerHidlClient.h"
#include <media/stagefright/foundation/ADebug.h>
#include "ClientAdapter.h"

#include "SubtitleReportAPI.h"

using namespace android;

class SubtitleClientPrivate : public RefBase
{
public:
    SubtitleClientPrivate();
    ~SubtitleClientPrivate();

    uint32_t getVersion() const;
    status_t connect(bool attachMode);
    void disconnect();

    status_t setViewWindow(int x, int y, int width, int height);
    status_t setVisible(bool visible);
    bool isVisible() const;
    status_t setViewAttribute(const SubtitleClient::ViewAttribute& attr);
    status_t init(const subtitle::Subtitle_Param& param);
    status_t getSourceAttribute(subtitle::Subtitle_Param* param);
    void registerCallback(const SubtitleClient::CallbackParam& cbParam);
    void setInputSource(SubtitleClient::InputSource source);
    SubtitleClient::InputSource getInputSource() const;
    SubtitleClient::TransferType getDefaultTransferType() const;
    status_t openTransferChannel(SubtitleClient::TransferType transferType);
    void closeTransferChannel();
    bool isTransferChannelOpened() const;
    size_t getHeaderSize() const;
    status_t constructPacketHeader(void* header, size_t headerBufferSize, const SubtitleClient::PackHeaderAttribute& attr);
    ssize_t send(const void* buf, size_t len);
    status_t flush();
    status_t start();
    status_t stop();
    status_t goHome();
    status_t gotoPage(int pageNo, int subPageNo);
    status_t nextPage(int direction);
    status_t nextSubPage(int direction);
    bool isPlaying() const;

protected:
    static void subtitle_evt_callback(SUBTITLE_EVENT evt, int index);
    static void subtitle_available_callback(SUBTITLE_STATE state, int val);
    status_t convertSubTypeToSubDecodeType(subtitle::SubtitleType type);
    status_t convertSubTypeToFFmpegType(subtitle::SubtitleType type);

private:
    android::SubtitleServerHidlClient::SUB_Para_t msubtitleCtx;
    SubtitleClient::InputSource mInputSource;

    SubSourceHandle mSourceHandle;


    //attach to ready subtitleserver
    bool mAttachMode = false;

    DISALLOW_EVIL_CONSTRUCTORS(SubtitleClientPrivate);
};



#endif
