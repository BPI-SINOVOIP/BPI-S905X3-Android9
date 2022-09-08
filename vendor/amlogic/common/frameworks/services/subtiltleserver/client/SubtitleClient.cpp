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

#define LOG_NDEBUG  0
#define LOG_TAG "SubtitleClient"

#include <utils/Log.h>
#include "SubtitleClient.h"
#include "SubtitleClientPrivate.h"

using namespace android;
using namespace subtitle;

SubtitleClient::SubtitleClient()
: mImpl(new SubtitleClientPrivate())
{
    ALOGD("ctor SubtitleClient");
}

SubtitleClient::~SubtitleClient()
{
    ALOGD("dtor SubtitleClient");
}

status_t SubtitleClient::connect(bool attachMode)
{
    if (mImpl == nullptr) {
        return INVALID_OPERATION;
    }

    return mImpl->connect(attachMode);
}

void SubtitleClient::disconnect()
{
    return mImpl->disconnect();
}

status_t SubtitleClient::setViewWindow(int x, int y, int width, int height)
{
    return mImpl->setViewWindow(x, y, width, height);
}

status_t SubtitleClient::setVisible(bool visible)
{
    return mImpl->setVisible(visible);
}

bool SubtitleClient::isVisible() const
{
    return mImpl->isVisible();
}

status_t SubtitleClient::setViewAttribute(const ViewAttribute& attr)
{
    return mImpl->setViewAttribute(attr);
}

status_t SubtitleClient::init(const Subtitle_Param& param)
{
    return mImpl->init(param);
}

status_t SubtitleClient::getSourceAttribute(Subtitle_Param* param)
{
    return mImpl->getSourceAttribute(param);
}

void SubtitleClient::registerCallback(const CallbackParam& cbParam)
{
    return mImpl->registerCallback(cbParam);
}

void SubtitleClient::setInputSource(InputSource source)
{
    return mImpl->setInputSource(source);
}

SubtitleClient::InputSource SubtitleClient::getInputSource() const
{
    return mImpl->getInputSource();
}

SubtitleClient::TransferType SubtitleClient::getDefaultTransferType() const
{
    return mImpl->getDefaultTransferType();
}

status_t SubtitleClient::openTransferChannel(TransferType transferType)
{
    return mImpl->openTransferChannel(transferType);
}

void SubtitleClient::closeTransferChannel()
{
    return mImpl->closeTransferChannel();
}

bool SubtitleClient::isTransferChannelOpened() const
{
    return mImpl->isTransferChannelOpened();
}

size_t SubtitleClient::getHeaderSize() const
{
    return mImpl->getHeaderSize();
}

status_t SubtitleClient::constructPacketHeader(void* header, size_t headerBufferSize, const PackHeaderAttribute& attr) const
{
    return mImpl->constructPacketHeader(header, headerBufferSize, attr);
}

ssize_t SubtitleClient::send(const void* buf, size_t len)
{
    return mImpl->send(buf, len);
}

status_t SubtitleClient::flush()
{
    return mImpl->flush();
}

status_t SubtitleClient::start()
{
    return mImpl->start();
}

status_t SubtitleClient::stop()
{
    return mImpl->stop();
}

status_t SubtitleClient::goHome()
{
    return mImpl->goHome();
}

status_t SubtitleClient::gotoPage(int pageNo, int subPageNo)
{
    return mImpl->gotoPage(pageNo, subPageNo);
}

status_t SubtitleClient::nextPage(int direction)
{
    return mImpl->nextPage(direction);
}

status_t SubtitleClient::nextSubPage(int direction)
{
    return mImpl->nextSubPage(direction);
}

bool SubtitleClient::isPlaying() const
{
    return mImpl->isPlaying();
}

