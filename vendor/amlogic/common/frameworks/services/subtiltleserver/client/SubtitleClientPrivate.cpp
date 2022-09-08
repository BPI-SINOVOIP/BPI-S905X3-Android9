#define LOG_NDEBUG 0
#define LOG_TAG "SubtitleClientPrivate"
#include <utils/Log.h>
#include <cutils/properties.h>
#include "SubtitleClientPrivate.h"

#define RETURN_VOID_IF(mode)                                                         \
    do {                                                                             \
        if (mode) {                                                                  \
            ALOGD("%s:%d return <- %s", __FUNCTION__, __LINE__, #mode);              \
            return;                                                                  \
        }                                                                            \
    } while (0)

#define RETURN_RET_IF(mode, ret)                                                     \
    do {                                                                             \
        if (mode) {                                                                  \
            ALOGD("%s:%d return <- %s", __FUNCTION__, __LINE__, #mode);              \
            return ret;                                                              \
        }                                                                            \
    } while (0)

#define SOCKET_HEADER_SIZE (30)
SubtitleClient::CallbackParam mCallbackParam={NULL,NULL};

using namespace android;

SubtitleClientPrivate::SubtitleClientPrivate()
{
    mInputSource = SubtitleClient::INTERNAL_SOURCE;
    mSourceHandle = NULL;
}

SubtitleClientPrivate::~SubtitleClientPrivate()
{
    closeTransferChannel();
}

uint32_t SubtitleClientPrivate::getVersion() const
{
    return 0x100000;
}

status_t SubtitleClientPrivate::connect(bool attachMode)
{
#if ANDROID_PLATFORM_SDK_VERSION > 27
    mAttachMode = false;// always use this. because all handled in subtitleserver
    ALOGD("[%s:%d] High SDK_VERSION attachMode=%d", __FUNCTION__, __LINE__, attachMode);

    // if CTC want subtitle data callback, then register this!
    // currently, we support handle subtitle data in 3 ways!
    // 1. in app render: do not create listener here!
    // 2. standalone render: do not create listener here!
    // 3. CTC take care, register
    // TODO: create a new hidl callback for display data
    registerSubtitleMiddleListener();
    if (!mAttachMode) {
        subtitleCreat();
        ALOGD("[%s:%d] ",__FUNCTION__, __LINE__);
    }
#else
    ALOGD("[%s:%d] Low SDK_VERSION", __FUNCTION__, __LINE__);
#endif

    return OK;
}

void SubtitleClientPrivate::disconnect()
{
    if (!mAttachMode) {
        subtitleDestory();
    }
}

status_t SubtitleClientPrivate::setViewWindow(int x, int y, int width, int height)
{
    subtitleSetSurfaceViewParam(x, y, width, height);

    return OK;
}

status_t SubtitleClientPrivate::setVisible(bool visible)
{
    if (visible) {
        subtitleDisplay();
    } else {
        subtitleHide();
    }
    return OK;
}

bool SubtitleClientPrivate::isVisible() const
{
    return true;
}

status_t SubtitleClientPrivate::setViewAttribute(const SubtitleClient::ViewAttribute& attr)
{
    return OK;
}

status_t SubtitleClientPrivate::init(const subtitle::Subtitle_Param& param)
{
#if ANDROID_PLATFORM_SDK_VERSION <= 27
    ALOGD("[%s:%d] init Subtitle", __FUNCTION__, __LINE__);
    return OK;
#endif

    memset(&msubtitleCtx, 0, sizeof(android::SubtitleServerHidlClient::SUB_Para_t));

    switch (param.sub_type)
    {
    case subtitle::CC:
        msubtitleCtx.tvType     = param.cc_param.vfmt;
        msubtitleCtx.vfmt       = param.cc_param.vfmt;
        msubtitleCtx.channelId  = param.cc_param.Channel_ID;
        break;
    case subtitle::SCTE27:
        msubtitleCtx.tvType     = param.sub_type;
        msubtitleCtx.pid        = param.scte27_param.SCTE27_PID;
        break;
    case subtitle::DVB_SUBTITLE:
        msubtitleCtx.tvType     = param.sub_type;
        msubtitleCtx.pid        = param.dvb_sub_param.Sub_PID;
        msubtitleCtx.ancPageId  = param.dvb_sub_param.Ancillary_Page;
        msubtitleCtx.pageId     = param.dvb_sub_param.Composition_Page;
        break;
    case subtitle::TELETEXT:
        ALOGD("[%s:%d] TELETEXT Subtitle type, unsupport now", __FUNCTION__, __LINE__);
        break;
    default:
        ALOGD("[%s:%d] unknow Subtitle type", __FUNCTION__, __LINE__);
        break;
    }

    ALOGD("[%s:%d] type channelId:%d pid=%d", __FUNCTION__, __LINE__, param.sub_type, msubtitleCtx.pid);

    return OK;
}

status_t SubtitleClientPrivate::getSourceAttribute(subtitle::Subtitle_Param* param)
{
    RETURN_RET_IF(param == NULL, NO_MEMORY);

    switch (msubtitleCtx.tvType)
    {
    case subtitle::CC:
        param->sub_type              = subtitle::SubtitleType::CC;
        param->cc_param.vfmt         = (vformat_t)msubtitleCtx.vfmt;
        param->cc_param.Channel_ID   = msubtitleCtx.channelId;
        break;
    case subtitle::SCTE27:
        param->sub_type                 = subtitle::SubtitleType::SCTE27;
        param->scte27_param.SCTE27_PID  = msubtitleCtx.pid;
        break;
    case subtitle::DVB_SUBTITLE:
        param->sub_type                         = subtitle::SubtitleType::DVB_SUBTITLE;
        param->dvb_sub_param.Sub_PID            = msubtitleCtx.pid;
        param->dvb_sub_param.Ancillary_Page     = msubtitleCtx.ancPageId;
        param->dvb_sub_param.Composition_Page   = msubtitleCtx.pageId;
        break;
    case subtitle::TELETEXT:
        ALOGD("[%s:%d] TELETEXT Subtitle type, unsupport now", __FUNCTION__, __LINE__);
        break;
    default:
        ALOGD("[%s:%d] unknow Subtitle type", __FUNCTION__, __LINE__);
        break;
    }

    ALOGD("[%s:%d] type channelId:%d pid=%d", __FUNCTION__, __LINE__, param->sub_type, msubtitleCtx.pid);

    return OK;
}

void SubtitleClientPrivate::subtitle_evt_callback(SUBTITLE_EVENT evt, int index)
{
    ALOGD("[%s:%d] evt=%d index=%d", __FUNCTION__, __LINE__, evt, index);

    RETURN_VOID_IF(mCallbackParam.eventCb == NULL);
    if (evt == SUBTITLE_EVENT_DATA) {
        mCallbackParam.eventCb(SubtitleClient::Event::CC_Added, index, 0);
    } else if (evt == SUBTITLE_EVENT_NONE) {
        mCallbackParam.eventCb(SubtitleClient::Event::CC_Removed, index, 0);
    }
}

void SubtitleClientPrivate::subtitle_available_callback(SUBTITLE_STATE state, int val)
{
    ALOGD("[%s:%d] available=%d val=%d", __FUNCTION__, __LINE__, state, val);

    RETURN_VOID_IF(mCallbackParam.eventCb == NULL);
    if (state == SUBTITLE_UNAVAIABLE) {
        mCallbackParam.eventCb(SubtitleClient::Event::Subtitle_Unavailable, val, 0);
    } else if (state == SUBTITLE_AVAIABLE) {
        mCallbackParam.eventCb(SubtitleClient::Event::Subtitle_Available, val, 0);
    }
}

void SubtitleClientPrivate::registerCallback(const SubtitleClient::CallbackParam& param)
{
    ALOGD("[%s:%d] registerCallback", __FUNCTION__, __LINE__);

    mCallbackParam = param;
    subtitle_register_event(subtitle_evt_callback);
    subtitle_register_available(subtitle_available_callback);
}

void SubtitleClientPrivate::setInputSource(SubtitleClient::InputSource source)
{
    mInputSource = source;

    //TODO
    //if source is EXTERNAL_SOURCE, subtitleservices no need to read data. INTERNAL_SOURCE is default.
}

SubtitleClient::InputSource SubtitleClientPrivate::getInputSource() const
{
    ALOGD("[%s:%d] mInputSource=%d", __FUNCTION__, __LINE__, mInputSource);

    return mInputSource;
}

SubtitleClient::TransferType SubtitleClientPrivate::getDefaultTransferType() const
{
    return SubtitleClient::TransferType::TRANSFER_BY_SOCKET;
}

status_t SubtitleClientPrivate::openTransferChannel(SubtitleClient::TransferType transferType)
{
    status_t ret = OK;

    ALOGD("[%s:%d] openTransferChannel=%d", __FUNCTION__, __LINE__, transferType);

    if (SubtitleClient::TransferType::TRANSFER_BY_SOCKET == transferType) {
        if (mSourceHandle == NULL) {
            // TODO: how to share and get the sessionID?
            mSourceHandle = SubSource_Create(0);
        }
    }

    return ret;
}

void SubtitleClientPrivate::closeTransferChannel()
{
    if (mSourceHandle != nullptr) {
        SubSource_Stop(mSourceHandle);
        SubSource_Destroy(mSourceHandle);
        mSourceHandle = nullptr;
    }
}

bool SubtitleClientPrivate::isTransferChannelOpened() const
{

    return (mSourceHandle != nullptr);
}

size_t SubtitleClientPrivate::getHeaderSize() const
{
    return SOCKET_HEADER_SIZE;
}

status_t SubtitleClientPrivate::convertSubTypeToSubDecodeType(subtitle::SubtitleType type) {

    int decodeSubType = -1;

    switch (type) {
    case subtitle::CC:
        decodeSubType = -1;
        break;
    case subtitle::SCTE27:
        decodeSubType = -1;
        break;
    case subtitle::DVB_SUBTITLE:
        decodeSubType = 5;
        break;
    case subtitle::TELETEXT:
        decodeSubType = -1;
        break;
    default:
        ALOGD("[%s:%d] unknow Subtitle type=%d", __FUNCTION__, __LINE__, type);
        break;
    }

    return decodeSubType;
}

status_t SubtitleClientPrivate::convertSubTypeToFFmpegType(subtitle::SubtitleType type) {

    int ffmpegSubType = -1;

    switch (type) {
    case subtitle::CC:
        ffmpegSubType = -1;
        break;
    case subtitle::SCTE27:
        ffmpegSubType = -1;
        break;
    case subtitle::DVB_SUBTITLE:
        ffmpegSubType = 0x17001;
        break;
    case subtitle::TELETEXT:
        ffmpegSubType = -1;
        break;
    default:
        ALOGD("[%s:%d] unknow Subtitle type=%d", __FUNCTION__, __LINE__, type);
        break;
    }

    return ffmpegSubType;
}


status_t SubtitleClientPrivate::constructPacketHeader(void* header, size_t headerBufferSize, const SubtitleClient::PackHeaderAttribute& attr)
{
    unsigned char *sub_header = (unsigned char *)header;
    int subDecodeType = -1;
    int mSubTotal = -1;
    unsigned int subFFmpegType = 0;

    RETURN_RET_IF(sub_header==NULL, NO_MEMORY);

    subDecodeType = convertSubTypeToSubDecodeType(attr.type);
    subFFmpegType = convertSubTypeToFFmpegType(attr.type);

    sub_header[0] = 0x51;
    sub_header[1] = 0x53;

    sub_header[2] = (subDecodeType >> 24) & 0xff;
    sub_header[3] = (subDecodeType >> 16) & 0xff;
    sub_header[4] = (subDecodeType >> 8) & 0xff;
    sub_header[5] = subDecodeType & 0xff;
    sub_header[6] = (attr.subtitleCount >> 24) & 0xff;
    sub_header[7] = (attr.subtitleCount >> 16) & 0xff;
    sub_header[8] = (attr.subtitleCount >> 8) & 0xff;
    sub_header[9] = attr.subtitleCount & 0xff;

    sub_header[10] = 0x41;
    sub_header[11] = 0x4d;
    sub_header[12] = 0x4c;
    sub_header[13] = 0x55;
    sub_header[14] = 0x77;

    sub_header[15] = (subFFmpegType >> 16) & 0xff;
    sub_header[16] = (subFFmpegType >> 8) & 0xff;
    sub_header[17] = subFFmpegType & 0xff;
    sub_header[18] = (attr.rawDataSize >> 24) & 0xff;
    sub_header[19] = (attr.rawDataSize >> 16) & 0xff;
    sub_header[20] = (attr.rawDataSize >> 8) & 0xff;
    sub_header[21] = attr.rawDataSize & 0xff;
    sub_header[22] = (attr.pts >> 24) & 0xff;
    sub_header[23] = (attr.pts >> 16) & 0xff;
    sub_header[24] = (attr.pts >> 8) & 0xff;
    sub_header[25] = attr.pts & 0xff;
    sub_header[26] = (attr.duration >> 24) & 0xff;
    sub_header[27] = (attr.duration >> 16) & 0xff;
    sub_header[28] = (attr.duration >> 8) & 0xff;
    sub_header[29] = attr.duration & 0xff;

    ALOGD("[%s:%d] subFFmpegType=0x%x, subDecodeType=%d, attr.rawDataSize=%d, attr.pts=%llu ", __FUNCTION__, __LINE__, subFFmpegType, subDecodeType, attr.rawDataSize, attr.pts);

    return OK;
}

ssize_t SubtitleClientPrivate::send(const void* buf, size_t len)
{
    if (mSourceHandle != NULL) {
        //mSocketClient->socketSend((char*)data, len);
        SubSource_SendData(mSourceHandle, (const char*)buf, len);
    }

    return len;
}

status_t SubtitleClientPrivate::flush()
{
    return OK;
}

status_t SubtitleClientPrivate::start()
{
#if ANDROID_PLATFORM_SDK_VERSION > 27
    if (!mAttachMode) {
        subtitleOpen("", nullptr, &msubtitleCtx);
        ALOGD("[%s:%d] ",__FUNCTION__, __LINE__);
        ALOGD("[%s:%d] msubtitleCtx.tvType=%d pid=%d",__FUNCTION__, __LINE__, msubtitleCtx.tvType, msubtitleCtx.pid);
    }
#else
    subtitleSetSurfaceViewParam(s_video_axis[0], s_video_axis[1], s_video_axis[2], s_video_axis[3]);
    subtitleResetForSeek();
    subtitleOpen("", this);// "" fit for api param, no matter what the path is for inner subtitle.
    subtitleShow();
#endif

    return OK;
}

status_t SubtitleClientPrivate::stop()
{
    ALOGD("[%s:%d] mAttachMode=%d ", __FUNCTION__, __LINE__, mAttachMode);
    if (!mAttachMode) {
        subtitleClose();
    }

    return OK;
}

status_t SubtitleClientPrivate::goHome()
{
    return OK;
}

status_t SubtitleClientPrivate::gotoPage(int pageNo, int subPageNo)
{
    return OK;
}

status_t SubtitleClientPrivate::nextPage(int direction)
{
    return OK;
}

status_t SubtitleClientPrivate::nextSubPage(int direction)
{
    return OK;
}

bool SubtitleClientPrivate::isPlaying() const
{
    return true;
}

