/*
 * Copyright (C) 2015 The Android Open Source Project
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
#define LOG_TAG "ACameraMetadata"

#include "ACameraMetadata.h"
#include <utils/Vector.h>
#include <system/graphics.h>
#include <media/NdkImage.h>

using namespace android;

/**
 * ACameraMetadata Implementation
 */
ACameraMetadata::ACameraMetadata(camera_metadata_t* buffer, ACAMERA_METADATA_TYPE type) :
        mData(buffer), mType(type) {
    if (mType == ACM_CHARACTERISTICS) {
        filterUnsupportedFeatures();
        filterStreamConfigurations();
    }
    // TODO: filter request/result keys
}

bool
ACameraMetadata::isNdkSupportedCapability(int32_t capability) {
    switch (capability) {
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT:
            return true;
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING:
        case ANDROID_REQUEST_AVAILABLE_CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO:
            return false;
        default:
            // Newly defined capabilities will be unsupported by default (blacklist)
            // TODO: Should we do whitelist or blacklist here?
            ALOGE("%s: Unknonwn capability %d", __FUNCTION__, capability);
            return false;
    }
}

void
ACameraMetadata::filterUnsupportedFeatures() {
    // Hide unsupported capabilities (reprocessing)
    camera_metadata_entry entry = mData.find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES);
    if (entry.count == 0 || entry.type != TYPE_BYTE) {
        ALOGE("%s: malformed available capability key! count %zu, type %d",
                __FUNCTION__, entry.count, entry.type);
        return;
    }

    Vector<uint8_t> capabilities;
    capabilities.setCapacity(entry.count);
    for (size_t i = 0; i < entry.count; i++) {
        uint8_t capability = entry.data.u8[i];
        if (isNdkSupportedCapability(capability)) {
            capabilities.push(capability);
        }
    }
    mData.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES, capabilities);
}


void
ACameraMetadata::filterStreamConfigurations() {
    const int STREAM_CONFIGURATION_SIZE = 4;
    const int STREAM_FORMAT_OFFSET = 0;
    const int STREAM_WIDTH_OFFSET = 1;
    const int STREAM_HEIGHT_OFFSET = 2;
    const int STREAM_IS_INPUT_OFFSET = 3;
    camera_metadata_entry entry = mData.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    if (entry.count == 0 || entry.count % 4 || entry.type != TYPE_INT32) {
        ALOGE("%s: malformed available stream configuration key! count %zu, type %d",
                __FUNCTION__, entry.count, entry.type);
        return;
    }

    Vector<int32_t> filteredStreamConfigs;
    filteredStreamConfigs.setCapacity(entry.count);

    for (size_t i=0; i < entry.count; i += STREAM_CONFIGURATION_SIZE) {
        int32_t format = entry.data.i32[i + STREAM_FORMAT_OFFSET];
        int32_t width = entry.data.i32[i + STREAM_WIDTH_OFFSET];
        int32_t height = entry.data.i32[i + STREAM_HEIGHT_OFFSET];
        int32_t isInput = entry.data.i32[i + STREAM_IS_INPUT_OFFSET];
        if (isInput == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT) {
            // Hide input streams
            continue;
        }
        // Translate HAL formats to NDK format
        if (format == HAL_PIXEL_FORMAT_BLOB) {
            format = AIMAGE_FORMAT_JPEG;
        }
        filteredStreamConfigs.push_back(format);
        filteredStreamConfigs.push_back(width);
        filteredStreamConfigs.push_back(height);
        filteredStreamConfigs.push_back(isInput);
    }

    mData.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, filteredStreamConfigs);

    entry = mData.find(ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS);
    Vector<int32_t> filteredDepthStreamConfigs;
    filteredDepthStreamConfigs.setCapacity(entry.count);

    for (size_t i=0; i < entry.count; i += STREAM_CONFIGURATION_SIZE) {
        int32_t format = entry.data.i32[i + STREAM_FORMAT_OFFSET];
        int32_t width = entry.data.i32[i + STREAM_WIDTH_OFFSET];
        int32_t height = entry.data.i32[i + STREAM_HEIGHT_OFFSET];
        int32_t isInput = entry.data.i32[i + STREAM_IS_INPUT_OFFSET];
        if (isInput == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT) {
            // Hide input streams
            continue;
        }
        // Translate HAL formats to NDK format
        if (format == HAL_PIXEL_FORMAT_BLOB) {
            format = AIMAGE_FORMAT_DEPTH_POINT_CLOUD;
        } else if (format == HAL_PIXEL_FORMAT_Y16) {
            format = AIMAGE_FORMAT_DEPTH16;
        }

        filteredDepthStreamConfigs.push_back(format);
        filteredDepthStreamConfigs.push_back(width);
        filteredDepthStreamConfigs.push_back(height);
        filteredDepthStreamConfigs.push_back(isInput);
    }
    mData.update(ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS, filteredDepthStreamConfigs);
}

bool
ACameraMetadata::isVendorTag(const uint32_t tag) {
    uint32_t tag_section = tag >> 16;
    if (tag_section >= VENDOR_SECTION) {
        return true;
    }
    return false;
}

camera_status_t
ACameraMetadata::getConstEntry(uint32_t tag, ACameraMetadata_const_entry* entry) const {
    if (entry == nullptr) {
        return ACAMERA_ERROR_INVALID_PARAMETER;
    }

    Mutex::Autolock _l(mLock);

    camera_metadata_ro_entry rawEntry = mData.find(tag);
    if (rawEntry.count == 0) {
        ALOGE("%s: cannot find metadata tag %d", __FUNCTION__, tag);
        return ACAMERA_ERROR_METADATA_NOT_FOUND;
    }
    entry->tag = tag;
    entry->type = rawEntry.type;
    entry->count = rawEntry.count;
    entry->data.u8 = rawEntry.data.u8;
    return ACAMERA_OK;
}

camera_status_t
ACameraMetadata::update(uint32_t tag, uint32_t count, const uint8_t* data) {
    return updateImpl<uint8_t>(tag, count, data);
}

camera_status_t
ACameraMetadata::update(uint32_t tag, uint32_t count, const int32_t* data) {
    return updateImpl<int32_t>(tag, count, data);
}

camera_status_t
ACameraMetadata::update(uint32_t tag, uint32_t count, const float* data) {
    return updateImpl<float>(tag, count, data);
}

camera_status_t
ACameraMetadata::update(uint32_t tag, uint32_t count, const double* data) {
    return updateImpl<double>(tag, count, data);
}

camera_status_t
ACameraMetadata::update(uint32_t tag, uint32_t count, const int64_t* data) {
    return updateImpl<int64_t>(tag, count, data);
}

camera_status_t
ACameraMetadata::update(uint32_t tag, uint32_t count, const ACameraMetadata_rational* data) {
    return updateImpl<camera_metadata_rational_t>(tag, count, data);
}

camera_status_t
ACameraMetadata::getTags(/*out*/int32_t* numTags,
                         /*out*/const uint32_t** tags) const {
    Mutex::Autolock _l(mLock);
    if (mTags.size() == 0) {
        size_t entry_count = mData.entryCount();
        mTags.setCapacity(entry_count);
        const camera_metadata_t* rawMetadata = mData.getAndLock();
        for (size_t i = 0; i < entry_count; i++) {
            camera_metadata_ro_entry_t entry;
            int ret = get_camera_metadata_ro_entry(rawMetadata, i, &entry);
            if (ret != 0) {
                ALOGE("%s: error reading metadata index %zu", __FUNCTION__, i);
                return ACAMERA_ERROR_UNKNOWN;
            }
            // Hide system key from users
            if (sSystemTags.count(entry.tag) == 0) {
                mTags.push_back(entry.tag);
            }
        }
        mData.unlock(rawMetadata);
    }

    *numTags = mTags.size();
    *tags = mTags.array();
    return ACAMERA_OK;
}

const CameraMetadata&
ACameraMetadata::getInternalData() const {
    return mData;
}

// TODO: some of key below should be hidden from user
// ex: ACAMERA_REQUEST_ID and ACAMERA_REPROCESS_EFFECTIVE_EXPOSURE_FACTOR
/*@O~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~
 * The key entries below this point are generated from metadata
 * definitions in /system/media/camera/docs. Do not modify by hand or
 * modify the comment blocks at the start or end.
 *~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~*/

bool
ACameraMetadata::isCaptureRequestTag(const uint32_t tag) {
    // Skip check for vendor keys
    if (isVendorTag(tag)) {
        return true;
    }

    switch (tag) {
        case ACAMERA_COLOR_CORRECTION_MODE:
        case ACAMERA_COLOR_CORRECTION_TRANSFORM:
        case ACAMERA_COLOR_CORRECTION_GAINS:
        case ACAMERA_COLOR_CORRECTION_ABERRATION_MODE:
        case ACAMERA_CONTROL_AE_ANTIBANDING_MODE:
        case ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION:
        case ACAMERA_CONTROL_AE_LOCK:
        case ACAMERA_CONTROL_AE_MODE:
        case ACAMERA_CONTROL_AE_REGIONS:
        case ACAMERA_CONTROL_AE_TARGET_FPS_RANGE:
        case ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER:
        case ACAMERA_CONTROL_AF_MODE:
        case ACAMERA_CONTROL_AF_REGIONS:
        case ACAMERA_CONTROL_AF_TRIGGER:
        case ACAMERA_CONTROL_AWB_LOCK:
        case ACAMERA_CONTROL_AWB_MODE:
        case ACAMERA_CONTROL_AWB_REGIONS:
        case ACAMERA_CONTROL_CAPTURE_INTENT:
        case ACAMERA_CONTROL_EFFECT_MODE:
        case ACAMERA_CONTROL_MODE:
        case ACAMERA_CONTROL_SCENE_MODE:
        case ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE:
        case ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST:
        case ACAMERA_CONTROL_ENABLE_ZSL:
        case ACAMERA_EDGE_MODE:
        case ACAMERA_FLASH_MODE:
        case ACAMERA_HOT_PIXEL_MODE:
        case ACAMERA_JPEG_GPS_COORDINATES:
        case ACAMERA_JPEG_GPS_PROCESSING_METHOD:
        case ACAMERA_JPEG_GPS_TIMESTAMP:
        case ACAMERA_JPEG_ORIENTATION:
        case ACAMERA_JPEG_QUALITY:
        case ACAMERA_JPEG_THUMBNAIL_QUALITY:
        case ACAMERA_JPEG_THUMBNAIL_SIZE:
        case ACAMERA_LENS_APERTURE:
        case ACAMERA_LENS_FILTER_DENSITY:
        case ACAMERA_LENS_FOCAL_LENGTH:
        case ACAMERA_LENS_FOCUS_DISTANCE:
        case ACAMERA_LENS_OPTICAL_STABILIZATION_MODE:
        case ACAMERA_NOISE_REDUCTION_MODE:
        case ACAMERA_SCALER_CROP_REGION:
        case ACAMERA_SENSOR_EXPOSURE_TIME:
        case ACAMERA_SENSOR_FRAME_DURATION:
        case ACAMERA_SENSOR_SENSITIVITY:
        case ACAMERA_SENSOR_TEST_PATTERN_DATA:
        case ACAMERA_SENSOR_TEST_PATTERN_MODE:
        case ACAMERA_SHADING_MODE:
        case ACAMERA_STATISTICS_FACE_DETECT_MODE:
        case ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE:
        case ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE:
        case ACAMERA_STATISTICS_OIS_DATA_MODE:
        case ACAMERA_TONEMAP_CURVE_BLUE:
        case ACAMERA_TONEMAP_CURVE_GREEN:
        case ACAMERA_TONEMAP_CURVE_RED:
        case ACAMERA_TONEMAP_MODE:
        case ACAMERA_TONEMAP_GAMMA:
        case ACAMERA_TONEMAP_PRESET_CURVE:
        case ACAMERA_BLACK_LEVEL_LOCK:
        case ACAMERA_DISTORTION_CORRECTION_MODE:
            return true;
        default:
            return false;
    }
}

// System tags that should be hidden from users
std::unordered_set<uint32_t> ACameraMetadata::sSystemTags ({
    ANDROID_CONTROL_SCENE_MODE_OVERRIDES,
    ANDROID_CONTROL_AE_PRECAPTURE_ID,
    ANDROID_CONTROL_AF_TRIGGER_ID,
    ANDROID_DEMOSAIC_MODE,
    ANDROID_EDGE_STRENGTH,
    ANDROID_FLASH_FIRING_POWER,
    ANDROID_FLASH_FIRING_TIME,
    ANDROID_FLASH_COLOR_TEMPERATURE,
    ANDROID_FLASH_MAX_ENERGY,
    ANDROID_FLASH_INFO_CHARGE_DURATION,
    ANDROID_JPEG_MAX_SIZE,
    ANDROID_JPEG_SIZE,
    ANDROID_NOISE_REDUCTION_STRENGTH,
    ANDROID_QUIRKS_METERING_CROP_REGION,
    ANDROID_QUIRKS_TRIGGER_AF_WITH_AUTO,
    ANDROID_QUIRKS_USE_ZSL_FORMAT,
    ANDROID_REQUEST_INPUT_STREAMS,
    ANDROID_REQUEST_METADATA_MODE,
    ANDROID_REQUEST_OUTPUT_STREAMS,
    ANDROID_REQUEST_TYPE,
    ANDROID_REQUEST_MAX_NUM_REPROCESS_STREAMS,
    ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
    ANDROID_SCALER_AVAILABLE_RAW_SIZES,
    ANDROID_SENSOR_BASE_GAIN_FACTOR,
    ANDROID_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS,
    ANDROID_SENSOR_TEMPERATURE,
    ANDROID_SENSOR_PROFILE_HUE_SAT_MAP,
    ANDROID_SENSOR_PROFILE_TONE_CURVE,
    ANDROID_SENSOR_OPAQUE_RAW_SIZE,
    ANDROID_SHADING_STRENGTH,
    ANDROID_STATISTICS_HISTOGRAM_MODE,
    ANDROID_STATISTICS_SHARPNESS_MAP_MODE,
    ANDROID_STATISTICS_HISTOGRAM,
    ANDROID_STATISTICS_SHARPNESS_MAP,
    ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
    ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
    ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
    ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
    ANDROID_DEPTH_MAX_DEPTH_SAMPLES,
});

/*~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~
 * End generated code
 *~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~O@*/
