/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef AMLOGIC_EXTRATOR_SUPPORT_HEADER
#define AMLOGIC_EXTRATOR_SUPPORT_HEADER

#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>
#include <media/stagefright/MediaExtractor.h>

namespace android {

bool sniffFFmpegFormat(const sp<DataSource> &source, String8 *mimeType,
					float *confidence, sp<AMessage> *meta);
sp<MediaExtractor> createFFmpegExtractor(
					const sp<DataSource> &source, const char *mime);

int registerAmExExtratorSniffers(void);

sp<MediaExtractor> createAmExExtractor(
        const sp<DataSource> &source,const char* mime, const sp<AMessage> &meta);

sp<MediaExtractor> createAmExtractor(
    const sp<DataSource> &source, const char* mime, const sp<AMessage> &meta);

}

#endif
