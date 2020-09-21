/*
 * Copyright 2018 The Android Open Source Project
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
#define LOG_TAG "MetaDataUtils"

#include <media/stagefright/foundation/avc_utils.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaDataUtils.h>

namespace android {

bool MakeAVCCodecSpecificData(MetaDataBase &meta, const uint8_t *data, size_t size) {
    int32_t width;
    int32_t height;
    int32_t sarWidth;
    int32_t sarHeight;
    sp<ABuffer> accessUnit = new ABuffer((void*)data,  size);
    sp<ABuffer> csd = MakeAVCCodecSpecificData(accessUnit, &width, &height, &sarWidth, &sarHeight);
    if (csd == nullptr) {
        return false;
    }
    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);

    meta.setData(kKeyAVCC, kTypeAVCC, csd->data(), csd->size());
    meta.setInt32(kKeyWidth, width);
    meta.setInt32(kKeyHeight, height);
    if (sarWidth > 0 && sarHeight > 0) {
        meta.setInt32(kKeySARWidth, sarWidth);
        meta.setInt32(kKeySARHeight, sarHeight);
    }
    return true;
}

bool MakeAACCodecSpecificData(
        MetaDataBase &meta,
        unsigned profile, unsigned sampling_freq_index,
        unsigned channel_configuration) {
    if(sampling_freq_index > 11u) {
        return false;
    }
    int32_t sampleRate;
    int32_t channelCount;
    static const int32_t kSamplingFreq[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000
    };
    sampleRate = kSamplingFreq[sampling_freq_index];
    channelCount = channel_configuration;

    static const uint8_t kStaticESDS[] = {
        0x03, 22,
        0x00, 0x00,     // ES_ID
        0x00,           // streamDependenceFlag, URL_Flag, OCRstreamFlag

        0x04, 17,
        0x40,                       // Audio ISO/IEC 14496-3
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,

        0x05, 2,
        // AudioSpecificInfo follows

        // oooo offf fccc c000
        // o - audioObjectType
        // f - samplingFreqIndex
        // c - channelConfig
    };

    size_t csdSize = sizeof(kStaticESDS) + 2;
    uint8_t *csd = new uint8_t[csdSize];
    memcpy(csd, kStaticESDS, sizeof(kStaticESDS));

    csd[sizeof(kStaticESDS)] =
        ((profile + 1) << 3) | (sampling_freq_index >> 1);

    csd[sizeof(kStaticESDS) + 1] =
        ((sampling_freq_index << 7) & 0x80) | (channel_configuration << 3);

    meta.setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);

    meta.setInt32(kKeySampleRate, sampleRate);
    meta.setInt32(kKeyChannelCount, channelCount);

    meta.setData(kKeyESDS, 0, csd, csdSize);
    delete [] csd;
    return true;
}

}  // namespace android
