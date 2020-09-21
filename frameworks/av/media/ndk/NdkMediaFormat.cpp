/*
 * Copyright (C) 2014 The Android Open Source Project
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
#define LOG_TAG "NdkMediaFormat"

#include <inttypes.h>

#include <media/NdkMediaFormat.h>

#include <utils/Log.h>
#include <utils/StrongPointer.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <android_runtime/AndroidRuntime.h>
#include <android_util_Binder.h>

#include <jni.h>

using namespace android;

struct AMediaFormat {
    sp<AMessage> mFormat;
    String8 mDebug;
    KeyedVector<String8, String8> mStringCache;
};

extern "C" {

// private functions for conversion to/from AMessage
AMediaFormat* AMediaFormat_fromMsg(const void* data) {
    ALOGV("private ctor");
    AMediaFormat* mData = new AMediaFormat();
    mData->mFormat = *((sp<AMessage>*)data);
    if (mData->mFormat == NULL) {
        ALOGW("got NULL format");
        mData->mFormat = new AMessage;
    }
    return mData;
}

void AMediaFormat_getFormat(const AMediaFormat* mData, void* dest) {
    *((sp<AMessage>*)dest) = mData->mFormat;
}


/*
 * public function follow
 */
EXPORT
AMediaFormat *AMediaFormat_new() {
    ALOGV("ctor");
    sp<AMessage> msg = new AMessage();
    return AMediaFormat_fromMsg(&msg);
}

EXPORT
media_status_t AMediaFormat_delete(AMediaFormat *mData) {
    ALOGV("dtor");
    delete mData;
    return AMEDIA_OK;
}


EXPORT
const char* AMediaFormat_toString(AMediaFormat *mData) {
    sp<AMessage> f = mData->mFormat;
    String8 ret;
    int num = f->countEntries();
    for (int i = 0; i < num; i++) {
        if (i != 0) {
            ret.append(", ");
        }
        AMessage::Type t;
        const char *name = f->getEntryNameAt(i, &t);
        ret.append(name);
        ret.append(": ");
        switch (t) {
            case AMessage::kTypeInt32:
            {
                int32_t val;
                f->findInt32(name, &val);
                ret.appendFormat("int32(%" PRId32 ")", val);
                break;
            }
            case AMessage::kTypeInt64:
            {
                int64_t val;
                f->findInt64(name, &val);
                ret.appendFormat("int64(%" PRId64 ")", val);
                break;
            }
            case AMessage::kTypeSize:
            {
                size_t val;
                f->findSize(name, &val);
                ret.appendFormat("size_t(%zu)", val);
                break;
            }
            case AMessage::kTypeFloat:
            {
                float val;
                f->findFloat(name, &val);
                ret.appendFormat("float(%f)", val);
                break;
            }
            case AMessage::kTypeDouble:
            {
                double val;
                f->findDouble(name, &val);
                ret.appendFormat("double(%f)", val);
                break;
            }
            case AMessage::kTypeRect:
            {
                int32_t left, top, right, bottom;
                f->findRect(name, &left, &top, &right, &bottom);
                ret.appendFormat("Rect(%" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 ")",
                                 left, top, right, bottom);
                break;
            }
            case AMessage::kTypeString:
            {
                AString val;
                f->findString(name, &val);
                ret.appendFormat("string(%s)", val.c_str());
                break;
            }
            case AMessage::kTypeBuffer:
            {
                ret.appendFormat("data");
                break;
            }
            default:
            {
                ret.appendFormat("unknown(%d)", t);
                break;
            }
        }
    }
    ret.append("}");
    mData->mDebug = ret;
    return mData->mDebug.string();
}

EXPORT
bool AMediaFormat_getInt32(AMediaFormat* format, const char *name, int32_t *out) {
    return format->mFormat->findInt32(name, out);
}

EXPORT
bool AMediaFormat_getInt64(AMediaFormat* format, const char *name, int64_t *out) {
    return format->mFormat->findInt64(name, out);
}

EXPORT
bool AMediaFormat_getFloat(AMediaFormat* format, const char *name, float *out) {
    return format->mFormat->findFloat(name, out);
}

EXPORT
bool AMediaFormat_getDouble(AMediaFormat* format, const char *name, double *out) {
    return format->mFormat->findDouble(name, out);
}

EXPORT
bool AMediaFormat_getSize(AMediaFormat* format, const char *name, size_t *out) {
    return format->mFormat->findSize(name, out);
}

EXPORT
bool AMediaFormat_getRect(AMediaFormat* format, const char *name,
                          int32_t *left, int32_t *top, int32_t *right, int32_t *bottom) {
    return format->mFormat->findRect(name, left, top, right, bottom);
}

EXPORT
bool AMediaFormat_getBuffer(AMediaFormat* format, const char *name, void** data, size_t *outsize) {
    sp<ABuffer> buf;
    if (format->mFormat->findBuffer(name, &buf)) {
        *data = buf->data() + buf->offset();
        *outsize = buf->size();
        return true;
    }
    return false;
}

EXPORT
bool AMediaFormat_getString(AMediaFormat* mData, const char *name, const char **out) {

    for (size_t i = 0; i < mData->mStringCache.size(); i++) {
        if (strcmp(mData->mStringCache.keyAt(i).string(), name) == 0) {
            mData->mStringCache.removeItemsAt(i, 1);
            break;
        }
    }

    AString tmp;
    if (mData->mFormat->findString(name, &tmp)) {
        String8 ret(tmp.c_str());
        mData->mStringCache.add(String8(name), ret);
        *out = ret.string();
        return true;
    }
    return false;
}

EXPORT
void AMediaFormat_setInt32(AMediaFormat* format, const char *name, int32_t value) {
    format->mFormat->setInt32(name, value);
}

EXPORT
void AMediaFormat_setInt64(AMediaFormat* format, const char *name, int64_t value) {
    format->mFormat->setInt64(name, value);
}

EXPORT
void AMediaFormat_setFloat(AMediaFormat* format, const char* name, float value) {
    format->mFormat->setFloat(name, value);
}

EXPORT
void AMediaFormat_setDouble(AMediaFormat* format, const char* name, double value) {
    format->mFormat->setDouble(name, value);
}

EXPORT
void AMediaFormat_setSize(AMediaFormat* format, const char* name, size_t value) {
    format->mFormat->setSize(name, value);
}

EXPORT
void AMediaFormat_setRect(AMediaFormat* format, const char *name,
                          int32_t left, int32_t top, int32_t right, int32_t bottom) {
    format->mFormat->setRect(name, left, top, right, bottom);
}

EXPORT
void AMediaFormat_setString(AMediaFormat* format, const char* name, const char* value) {
    // AMessage::setString() makes a copy of the string
    format->mFormat->setString(name, value, strlen(value));
}

EXPORT
void AMediaFormat_setBuffer(AMediaFormat* format, const char* name, void* data, size_t size) {
    // the ABuffer(void*, size_t) constructor doesn't take ownership of the data, so create
    // a new buffer and copy the data into it
    sp<ABuffer> buf = new ABuffer(size);
    memcpy(buf->data(), data, size);
    buf->setRange(0, size);
    // AMessage::setBuffer() increases the refcount of the buffer
    format->mFormat->setBuffer(name, buf);
}


EXPORT const char* AMEDIAFORMAT_KEY_AAC_DRC_ATTENUATION_FACTOR = "aac-drc-cut-level";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_DRC_BOOST_FACTOR = "aac-drc-boost-level";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_DRC_HEAVY_COMPRESSION = "aac-drc-heavy-compression";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_DRC_TARGET_REFERENCE_LEVEL = "aac-target-ref-level";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_ENCODED_TARGET_LEVEL = "aac-encoded-target-level";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_MAX_OUTPUT_CHANNEL_COUNT = "aac-max-output-channel_count";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_PROFILE = "aac-profile";
EXPORT const char* AMEDIAFORMAT_KEY_AAC_SBR_MODE = "aac-sbr-mode";
EXPORT const char* AMEDIAFORMAT_KEY_AUDIO_SESSION_ID = "audio-session-id";
EXPORT const char* AMEDIAFORMAT_KEY_BITRATE_MODE = "bitrate-mode";
EXPORT const char* AMEDIAFORMAT_KEY_BIT_RATE = "bitrate";
EXPORT const char* AMEDIAFORMAT_KEY_CAPTURE_RATE = "capture-rate";
EXPORT const char* AMEDIAFORMAT_KEY_CHANNEL_COUNT = "channel-count";
EXPORT const char* AMEDIAFORMAT_KEY_CHANNEL_MASK = "channel-mask";
EXPORT const char* AMEDIAFORMAT_KEY_COLOR_FORMAT = "color-format";
EXPORT const char* AMEDIAFORMAT_KEY_COLOR_RANGE = "color-range";
EXPORT const char* AMEDIAFORMAT_KEY_COLOR_STANDARD = "color-standard";
EXPORT const char* AMEDIAFORMAT_KEY_COLOR_TRANSFER = "color-transfer";
EXPORT const char* AMEDIAFORMAT_KEY_COMPLEXITY = "complexity";
EXPORT const char* AMEDIAFORMAT_KEY_CSD = "csd";
EXPORT const char* AMEDIAFORMAT_KEY_CSD_0 = "csd-0";
EXPORT const char* AMEDIAFORMAT_KEY_CSD_1 = "csd-1";
EXPORT const char* AMEDIAFORMAT_KEY_CSD_2 = "csd-2";
EXPORT const char* AMEDIAFORMAT_KEY_DISPLAY_CROP = "crop";
EXPORT const char* AMEDIAFORMAT_KEY_DISPLAY_HEIGHT = "display-height";
EXPORT const char* AMEDIAFORMAT_KEY_DISPLAY_WIDTH = "display-width";
EXPORT const char* AMEDIAFORMAT_KEY_DURATION = "durationUs";
EXPORT const char* AMEDIAFORMAT_KEY_FLAC_COMPRESSION_LEVEL = "flac-compression-level";
EXPORT const char* AMEDIAFORMAT_KEY_FRAME_RATE = "frame-rate";
EXPORT const char* AMEDIAFORMAT_KEY_GRID_COLUMNS = "grid-cols";
EXPORT const char* AMEDIAFORMAT_KEY_GRID_ROWS = "grid-rows";
EXPORT const char* AMEDIAFORMAT_KEY_HDR_STATIC_INFO = "hdr-static-info";
EXPORT const char* AMEDIAFORMAT_KEY_HEIGHT = "height";
EXPORT const char* AMEDIAFORMAT_KEY_INTRA_REFRESH_PERIOD = "intra-refresh-period";
EXPORT const char* AMEDIAFORMAT_KEY_IS_ADTS = "is-adts";
EXPORT const char* AMEDIAFORMAT_KEY_IS_AUTOSELECT = "is-autoselect";
EXPORT const char* AMEDIAFORMAT_KEY_IS_DEFAULT = "is-default";
EXPORT const char* AMEDIAFORMAT_KEY_IS_FORCED_SUBTITLE = "is-forced-subtitle";
EXPORT const char* AMEDIAFORMAT_KEY_I_FRAME_INTERVAL = "i-frame-interval";
EXPORT const char* AMEDIAFORMAT_KEY_LANGUAGE = "language";
EXPORT const char* AMEDIAFORMAT_KEY_LATENCY = "latency";
EXPORT const char* AMEDIAFORMAT_KEY_LEVEL = "level";
EXPORT const char* AMEDIAFORMAT_KEY_MAX_HEIGHT = "max-height";
EXPORT const char* AMEDIAFORMAT_KEY_MAX_INPUT_SIZE = "max-input-size";
EXPORT const char* AMEDIAFORMAT_KEY_MAX_WIDTH = "max-width";
EXPORT const char* AMEDIAFORMAT_KEY_MIME = "mime";
EXPORT const char* AMEDIAFORMAT_KEY_MPEG_USER_DATA = "mpeg-user-data";
EXPORT const char* AMEDIAFORMAT_KEY_OPERATING_RATE = "operating-rate";
EXPORT const char* AMEDIAFORMAT_KEY_PCM_ENCODING = "pcm-encoding";
EXPORT const char* AMEDIAFORMAT_KEY_PRIORITY = "priority";
EXPORT const char* AMEDIAFORMAT_KEY_PROFILE = "profile";
EXPORT const char* AMEDIAFORMAT_KEY_PUSH_BLANK_BUFFERS_ON_STOP = "push-blank-buffers-on-shutdown";
EXPORT const char* AMEDIAFORMAT_KEY_REPEAT_PREVIOUS_FRAME_AFTER = "repeat-previous-frame-after";
EXPORT const char* AMEDIAFORMAT_KEY_ROTATION = "rotation-degrees";
EXPORT const char* AMEDIAFORMAT_KEY_SAMPLE_RATE = "sample-rate";
EXPORT const char* AMEDIAFORMAT_KEY_SEI = "sei";
EXPORT const char* AMEDIAFORMAT_KEY_SLICE_HEIGHT = "slice-height";
EXPORT const char* AMEDIAFORMAT_KEY_STRIDE = "stride";
EXPORT const char* AMEDIAFORMAT_KEY_TEMPORAL_LAYER_ID = "temporal-layer-id";
EXPORT const char* AMEDIAFORMAT_KEY_TEMPORAL_LAYERING = "ts-schema";
EXPORT const char* AMEDIAFORMAT_KEY_TILE_HEIGHT = "tile-height";
EXPORT const char* AMEDIAFORMAT_KEY_TILE_WIDTH = "tile-width";
EXPORT const char* AMEDIAFORMAT_KEY_TIME_US = "timeUs";
EXPORT const char* AMEDIAFORMAT_KEY_TRACK_ID = "track-id";
EXPORT const char* AMEDIAFORMAT_KEY_TRACK_INDEX = "track-index";
EXPORT const char* AMEDIAFORMAT_KEY_WIDTH = "width";


} // extern "C"


