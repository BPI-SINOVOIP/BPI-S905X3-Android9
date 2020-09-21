/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "VorbisComment"
#include <utils/Log.h>

#include "media/VorbisComment.h"

#include <media/stagefright/foundation/base64.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ByteUtils.h>
#include <media/stagefright/MetaDataBase.h>

namespace android {

static void extractAlbumArt(
        MetaDataBase *fileMeta, const void *data, size_t size) {
    ALOGV("extractAlbumArt from '%s'", (const char *)data);

    sp<ABuffer> flacBuffer = decodeBase64(AString((const char *)data, size));
    if (flacBuffer == NULL) {
        ALOGE("malformed base64 encoded data.");
        return;
    }

    size_t flacSize = flacBuffer->size();
    uint8_t *flac = flacBuffer->data();
    ALOGV("got flac of size %zu", flacSize);

    uint32_t picType;
    uint32_t typeLen;
    uint32_t descLen;
    uint32_t dataLen;
    char type[128];

    if (flacSize < 8) {
        return;
    }

    picType = U32_AT(flac);

    if (picType != 3) {
        // This is not a front cover.
        return;
    }

    typeLen = U32_AT(&flac[4]);
    if (typeLen > sizeof(type) - 1) {
        return;
    }

    // we've already checked above that flacSize >= 8
    if (flacSize - 8 < typeLen) {
        return;
    }

    memcpy(type, &flac[8], typeLen);
    type[typeLen] = '\0';

    ALOGV("picType = %d, type = '%s'", picType, type);

    if (!strcmp(type, "-->")) {
        // This is not inline cover art, but an external url instead.
        return;
    }

    if (flacSize < 32 || flacSize - 32 < typeLen) {
        return;
    }

    descLen = U32_AT(&flac[8 + typeLen]);
    if (flacSize - 32 - typeLen < descLen) {
        return;
    }

    dataLen = U32_AT(&flac[8 + typeLen + 4 + descLen + 16]);

    // we've already checked above that (flacSize - 32 - typeLen - descLen) >= 0
    if (flacSize - 32 - typeLen - descLen < dataLen) {
        return;
    }

    ALOGV("got image data, %zu trailing bytes",
         flacSize - 32 - typeLen - descLen - dataLen);

    fileMeta->setData(
            kKeyAlbumArt, 0, &flac[8 + typeLen + 4 + descLen + 20], dataLen);

    fileMeta->setCString(kKeyAlbumArtMIME, type);
}

void parseVorbisComment(
        MetaDataBase *fileMeta, const char *comment, size_t commentLength)
{
    struct {
        const char *const mTag;
        uint32_t mKey;
    } kMap[] = {
        { "TITLE", kKeyTitle },
        { "ARTIST", kKeyArtist },
        { "ALBUMARTIST", kKeyAlbumArtist },
        { "ALBUM ARTIST", kKeyAlbumArtist },
        { "COMPILATION", kKeyCompilation },
        { "ALBUM", kKeyAlbum },
        { "COMPOSER", kKeyComposer },
        { "GENRE", kKeyGenre },
        { "AUTHOR", kKeyAuthor },
        { "TRACKNUMBER", kKeyCDTrackNumber },
        { "DISCNUMBER", kKeyDiscNumber },
        { "DATE", kKeyDate },
        { "YEAR", kKeyYear },
        { "LYRICIST", kKeyWriter },
        { "METADATA_BLOCK_PICTURE", kKeyAlbumArt },
        { "ANDROID_LOOP", kKeyAutoLoop },
    };

        for (size_t j = 0; j < sizeof(kMap) / sizeof(kMap[0]); ++j) {
            size_t tagLen = strlen(kMap[j].mTag);
            if (!strncasecmp(kMap[j].mTag, comment, tagLen)
                    && comment[tagLen] == '=') {
                if (kMap[j].mKey == kKeyAlbumArt) {
                    extractAlbumArt(
                            fileMeta,
                            &comment[tagLen + 1],
                            commentLength - tagLen - 1);
                } else if (kMap[j].mKey == kKeyAutoLoop) {
                    if (!strcasecmp(&comment[tagLen + 1], "true")) {
                        fileMeta->setInt32(kKeyAutoLoop, true);
                    }
                } else {
                    fileMeta->setCString(kMap[j].mKey, &comment[tagLen + 1]);
                }
            }
        }

}

}  // namespace android
