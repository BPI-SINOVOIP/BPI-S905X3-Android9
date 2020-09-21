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
#define LOG_TAG "Metadata"

#include <errno.h>

#include <log/log.h>

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <utils/Trace.h>

#include <system/camera_metadata.h>

#include "Metadata.h"

namespace usb_camera_hal {

Metadata::Metadata():
    mData(NULL) {
}

Metadata::~Metadata() {
    replace(NULL);
}

void Metadata::replace(camera_metadata_t *m) {
    if (m == mData) {
        return;
    }
    if (mData)
        free_camera_metadata(mData);
    mData = m;
}

int Metadata::init(const camera_metadata_t *metadata) {
    camera_metadata_t* tmp;

    if (!validate_camera_metadata_structure(metadata, NULL))
        return -EINVAL;

    tmp = clone_camera_metadata(metadata);
    if (tmp == NULL)
        return -EINVAL;

    replace(tmp);
    return 0;
}

int Metadata::addUInt8(uint32_t tag, int count, const uint8_t *data) {
    if (!validate(tag, TYPE_BYTE, count)) return -EINVAL;
    return add(tag, count, data);
}

int Metadata::add1UInt8(uint32_t tag, const uint8_t data) {
    return addUInt8(tag, 1, &data);
}

int Metadata::addInt32(uint32_t tag, int count, const int32_t *data) {
    if (!validate(tag, TYPE_INT32, count)) return -EINVAL;
    return add(tag, count, data);
}

int Metadata::addFloat(uint32_t tag, int count, const float *data) {
    if (!validate(tag, TYPE_FLOAT, count)) return -EINVAL;
    return add(tag, count, data);
}

int Metadata::addInt64(uint32_t tag, int count, const int64_t *data) {
    if (!validate(tag, TYPE_INT64, count)) return -EINVAL;
    return add(tag, count, data);
}

int Metadata::addDouble(uint32_t tag, int count, const double *data) {
    if (!validate(tag, TYPE_DOUBLE, count)) return -EINVAL;
    return add(tag, count, data);
}

int Metadata::addRational(uint32_t tag, int count,
        const camera_metadata_rational_t *data) {
    if (!validate(tag, TYPE_RATIONAL, count)) return -EINVAL;
    return add(tag, count, data);
}

bool Metadata::validate(uint32_t tag, int tag_type, int count) {
    if (get_camera_metadata_tag_type(tag) < 0) {
        ALOGE("%s: Invalid metadata entry tag: %d", __func__, tag);
        return false;
    }
    if (tag_type < 0 || tag_type >= NUM_TYPES) {
        ALOGE("%s: Invalid metadata entry tag type: %d", __func__, tag_type);
        return false;
    }
    if (tag_type != get_camera_metadata_tag_type(tag)) {
        ALOGE("%s: Tag %d called with incorrect type: %s(%d)", __func__, tag,
                camera_metadata_type_names[tag_type], tag_type);
        return false;
    }
    if (count < 1) {
        ALOGE("%s: Invalid metadata entry count: %d", __func__, count);
        return false;
    }
    return true;
}

int Metadata::add(uint32_t tag, int count, const void *tag_data) {
    // Opportunistically attempt to add if metadata has room for it
    if (!add_camera_metadata_entry(mData, tag, tag_data, count)) {
        return 0;
    }

    int res;
    camera_metadata_t* tmp;
    int tag_type = get_camera_metadata_tag_type(tag);
    size_t size = calculate_camera_metadata_entry_data_size(tag_type, count);
    size_t entry_capacity = get_camera_metadata_entry_count(mData) + 1;
    size_t data_capacity = get_camera_metadata_data_count(mData) + size;

    // Double new dimensions to minimize future reallocations
    tmp = allocate_camera_metadata(entry_capacity * 2, data_capacity * 2);
    if (tmp == NULL) {
        ALOGE("%s: Failed to allocate new metadata with %zu entries, %zu data",
                __func__, entry_capacity, data_capacity);
        return -ENOMEM;
    }
    // Append the current metadata to the new (empty) metadata
    res = append_camera_metadata(tmp, mData);
    if (res) {
        ALOGE("%s: Failed to append old metadata %p to new %p",
                __func__, mData, tmp);
        return res;
    }
    // Add the remaining new item
    res = add_camera_metadata_entry(tmp, tag, tag_data, count);
    if (res) {
        ALOGE("%s: Failed to add new entry (%d, %p, %d) to metadata %p",
                __func__, tag, tag_data, count, tmp);
        return res;
    }

    replace(tmp);
    return 0;
}

camera_metadata_t* Metadata::get() {
    return mData;
}

} // namespace usb_camera_hal
