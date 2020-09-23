/*
 * Copyright (c) 2013 - 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "AVUtils"
#include <utils/Log.h>

#include "common/ExtensionsLoader.hpp"
#include "stagefright/AVExtensions.h"
#include <media/stagefright/omx/OMXUtils.h>
#include <media/IOMX.h>

namespace android {

const char *AVUtils::getComponentRole(bool isEncoder, const char *mime) {
    ALOGV("AVUtils::getComponentRole");

    return GetComponentRole(isEncoder,mime);
}

status_t AVUtils::getVideoCodingTypeFromMimeEx(
        const char *, OMX_VIDEO_CODINGTYPE *) {
    return ERROR_UNSUPPORTED;
}

bool AVUtils::isVendorSoftDecoder(const char *) {
    return false;
}

bool AVUtils::isAudioExtendFormat(const char *) {
    return false;
}

bool AVUtils::isExtendFormat(const char *) {
    return false;
}

bool AVUtils::isAudioExtendCoding(int) {
    return false;
}
int AVUtils::getAudioExtendParameter(int, uint32_t ,const sp<IOMXNode> &OMXNode, sp<AMessage> &notify) {
    if (OMXNode == NULL ||notify == NULL)
        ALOGI("AVUtils::getAudioExtendParameter err");
    return -1;
}
int AVUtils::setAudioExtendParameter(const char *,const sp<IOMXNode> &OMXNode, const sp<AMessage> &notify) {
    if (OMXNode == NULL ||notify == NULL)
        ALOGI("AVUtils::setAudioExtendParameter err");
    return -1;
}

int AVUtils::handleExtendParameter(const char *,const sp<IOMXNode> &OMXNode, const sp<AMessage> &notify) {
    if (OMXNode == NULL ||notify == NULL)
        ALOGI("AVUtils::setVideoExtendParameter err");
    return -1;
}

void AVUtils::addExtendXML(MediaCodecsXmlParser*) {
    ALOGI("AVUtils::addExtendXML");
    //addExtendXML(xmlparser);
    return;
}

bool AVUtils::isExtendPlayer(player_type) {
    return false;
}

status_t AVUtils::convertMetaDataToMessage(
        const sp<MetaData> &, sp<AMessage> &) {
    return OK;
}



// ----- NO TRESSPASSING BEYOND THIS LINE ------
AVUtils::AVUtils() {}

AVUtils::~AVUtils() {}

//static
AVUtils *AVUtils::sInst =
        ExtensionsLoader<AVUtils>::createInstance("_ZN7android19createExtendedUtilsEv");

} //namespace android

