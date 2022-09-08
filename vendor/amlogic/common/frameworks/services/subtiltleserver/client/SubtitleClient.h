/*
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _SUBTITLE_CLIENT_H_
#define _SUBTITLE_CLIENT_H_

#include <utils/RefBase.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <utils/Errors.h>
#include <functional>
#include <amports/vformat.h>
#include <stdint.h>


using android::RefBase;
using android::sp;
using android::status_t;
using android::ABuffer;

class SubtitleClientPrivate;

namespace subtitle {

//keep sync with SUB_TYPE  defined in hidl client
enum SubtitleType : uint32_t {
    CC              = 2,
    SCTE27          = 3,
    DVB_SUBTITLE    = 4,
    TELETEXT        = 5,
    UNKNOWN         = UINT32_MAX,
};

typedef struct {
    int Channel_ID;
    vformat_t vfmt;  //Video format
} CC_Param;

typedef struct {
    int SCTE27_PID;
} SCTE27_Param;

typedef struct {
    int Sub_PID;
    int Composition_Page;
    int Ancillary_Page;
} Dvb_Subtitle_Param;

typedef struct {
    int Teletext_PID;
    int Magzine_No;
    int Page_No;
    int SubPage_No;
} TELETEXT_Param;

typedef struct {
    SubtitleType sub_type;
    union {
        CC_Param cc_param;
        Dvb_Subtitle_Param dvb_sub_param;
        SCTE27_Param scte27_param;
        TELETEXT_Param tt_param;
    };
} Subtitle_Param;

enum {
    Event_None = 0,
    Subtitle_Available,
    Subtitle_Unavailable,
    CC_Removed,
    CC_Added,
    CC_Displayed,
    Subtitle_Displayed,
} AM_Sub_Event;

}

/**
 * \brief
 * don't need report Available and Unavailable event
 */
class SubtitleClient : public RefBase
{
public:
    enum Event : uint32_t{
        INFO,
        Subtitle_Available,
        Subtitle_Unavailable,
        CC_Removed,
        CC_Added,
        ERROR,
    };

    enum InfoType : uint32_t{
        DISPLAYED,
        UNDERFLOW,
        OVERFLOW,
    };

    enum ErrorType : uint32_t{
        UNKNOWN_ERROR,
        LOSE_DATA,
        INVALID_DATA,
        PTS_ERROR,
        DECODER_EXIT,
    };

    enum InputSource : uint32_t{
        EXTERNAL_SOURCE,
        INTERNAL_SOURCE,
    };

    enum TransferType : uint32_t{
        TRANSFER_BY_SOCKET, // No by socket now! only FMQ! later remove it.
        TRANSFER_BY_BINDER,
    };

    struct ViewAttribute {
        bool        opaque;
    };

    struct PackHeaderAttribute {
        int             subtitleCount;
        subtitle::SubtitleType    type;
        size_t          rawDataSize;
        int64_t         pts;
        int             duration;
    };

    typedef int SeSessionId;
    typedef android::RefBase Handler;

    using ClockCallback = std::function<status_t(int64_t* timeUs)>;
    using EventCallback = std::function<void(Event, int32_t, int32_t)>;

    struct CallbackParam {
        ClockCallback clockCb;
        EventCallback eventCb;
    };

public:
    SubtitleClient();
    ~SubtitleClient();

    /**
     * \brief getVersion
     *
     * \return
     */
    uint32_t getVersion() const;

    /**
     * \brief connect to subtitle server
     * should be asynchronous
     *
     * \param attachMode
     *
     * \return 0, success,
     *      -EAGAIN, need try again,
     *      other errors.
     */
    status_t connect(bool attachMode);
    void disconnect();

    /**
     * \brief set subtitle view show position
     *
     * \param x
     * \param y
     * \param width
     * \param height
     *
     * \return
     */
    status_t setViewWindow(int x, int y, int width, int height);

    status_t setVisible(bool visible);
    bool isVisible() const;

    /**
     * \brief set subtitle view attribute
     *
     * \param attr
     *          opaque: let view become opaque if true.
     *
     * \return
     */
    status_t setViewAttribute(const ViewAttribute& attr);

    status_t init(const subtitle::Subtitle_Param& param);

    /**
     * \brief get subtitle's attribute
     *
     * \param param
     *
     * \return
     */
    status_t getSourceAttribute(subtitle::Subtitle_Param* param);

    /**
     * \brief registerCallback
     *
     * \param cbParam
     */
    void registerCallback(const CallbackParam& cbParam);

    /**
     * \brief setInputSource
     * subtitle data provided by user or polled by ourself
     *
     * \param source
     */
    void setInputSource(InputSource source);
    InputSource getInputSource() const;

    /**
     * \brief get subtitle raw data's default transfer type
     *
     * \return TransferType, default is TRANSFER_BY_SOCKET.
     */
    TransferType getDefaultTransferType() const;

    status_t openTransferChannel(TransferType transferType);
    void closeTransferChannel();
    bool isTransferChannelOpened() const;

    /**
     * \brief get smallest buffer size needed for header
     *
     * \return
     */
    size_t getHeaderSize() const;

    /**
     * \brief constructPacketHeader
     *
     * \param header
     * \param headerBufferSize
     * \param attr
     *
     * \return
     */
    status_t constructPacketHeader(void* header, size_t headerBufferSize, const PackHeaderAttribute& attr) const;

    ssize_t send(const void* buf, size_t len);

    /**
     * \brief flush
     * need be called when switch subtitle
     *
     * \return
     */
    status_t flush();

    /**
     * \brief start decode and displaying
     *
     * \return
     */
    status_t start();

    /**
     * \brief stop decode and displaying
     *
     * \return
     */
    status_t stop();

    status_t goHome();
    status_t gotoPage(int pageNo, int subPageNo);
    status_t nextPage(int direction);
    status_t nextSubPage(int direction);

    /**
     * \brief whether subtitle is decoding and displaying
     *      don't care subtitle view is visible or not.
     *
     * \return
     */
    bool isPlaying() const;

private:
    sp<SubtitleClientPrivate> mImpl;

    DISALLOW_EVIL_CONSTRUCTORS(SubtitleClient);
};

#endif
