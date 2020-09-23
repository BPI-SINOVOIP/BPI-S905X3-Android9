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
#ifndef _AV_EXTENSIONS_H_
#define _AV_EXTENSIONS_H_

#include <common/AVExtensionsCommon.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/xmlparser/MediaCodecsXmlParser.h>
#include <OMX_Video.h>
#include <media/IOMX.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/MediaPlayerInterface.h>


namespace android {

/*
 * Common delegate to the classes in libstagefright
 */
struct AVUtils {

    virtual const char *getComponentRole(bool isEncoder, const char *mime);

    virtual status_t getVideoCodingTypeFromMimeEx(
        const char *, OMX_VIDEO_CODINGTYPE *);

    virtual bool isVendorSoftDecoder(const char *);

    virtual bool isAudioExtendFormat(const char *);

    virtual bool isAudioExtendCoding(int);

    virtual int getAudioExtendParameter(int, uint32_t ,const sp<IOMXNode> &OMXNode, sp<AMessage> &notify);

    virtual int setAudioExtendParameter(const char *,const sp<IOMXNode> &OMXNode,const sp<AMessage> &notify);

    virtual bool isExtendFormat(const char *);

    virtual int handleExtendParameter(const char *,const sp<IOMXNode> &OMXNode,const sp<AMessage> &notify);

    virtual void addExtendXML(MediaCodecsXmlParser*);

    virtual bool isExtendPlayer(player_type);

    virtual status_t convertMetaDataToMessage(
        const sp<MetaData> &, sp<AMessage> &);

    // ----- NO TRESSPASSING BEYOND THIS LINE ------
    DECLARE_LOADABLE_SINGLETON(AVUtils);
};

}

#endif // _AV_EXTENSIONS__H_
