/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef VENDOR_TAGS_H_
#define VENDOR_TAGS_H_

#include <hardware/camera_common.h>
#include <system/camera_metadata.h>

namespace default_camera_hal {

// VendorTags contains all vendor-specific metadata tag functionality
class VendorTags {
    public:
        VendorTags();
        ~VendorTags();

        // Vendor Tags Operations (see <hardware/camera_common.h>)
        int getTagCount(const vendor_tag_ops_t* ops);
        void getAllTags(const vendor_tag_ops_t* ops, uint32_t* tag_array);
        const char* getSectionName(const vendor_tag_ops_t* ops, uint32_t tag);
        const char* getTagName(const vendor_tag_ops_t* ops, uint32_t tag);
        int getTagType(const vendor_tag_ops_t* ops, uint32_t tag);

    private:
        // Total number of vendor tags
        int mTagCount;
};

// Tag sections start at the beginning of vendor tags (0x8000_0000)
// See <system/camera_metadata.h>

typedef struct vendor_tag_info {
    const char *tag_name;
    uint8_t     tag_type;
} vendor_tag_info_t;

} // namespace default_camera_hal

#endif // VENDOR_TAGS_H_
