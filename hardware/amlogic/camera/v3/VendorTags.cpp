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

#include <system/camera_metadata.h>
//#include "Metadata.h"

//#define LOG_NDEBUG 0
#define LOG_TAG "VendorTags"
#include <android/log.h>

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <utils/Log.h>

#include "VendorTags.h"

namespace default_camera_hal {

// Internal representations of vendor tags for convenience.
// Other classes must access this data via public interfaces.
// Structured to be easy to extend and contain complexity.
namespace {
// Describes a single vendor tag entry
struct Entry {
    const char* name;
    uint8_t     type;
};
// Describes a vendor tag section
struct Section {
    const char* name;
    uint32_t start;
    uint32_t end;
    const Entry* tags;
};

enum vendor_section {
    ANDROID_LENS = VENDOR_SECTION,
	ANDROID_LENS_INFO,
    ANDROID_SECTION_END
};

const int FAKEVENDOR_SECTION_COUNT = ANDROID_SECTION_END - VENDOR_SECTION;

enum vendor_section_ranges {
    ANDROID_LENS_START          = ANDROID_LENS << 16,
	ANDROID_LENS_INFO_START          = ANDROID_LENS_INFO << 16
};


enum vendor_tags {
    ANDROID_LENS_APERTURE =                           // float        | public
            ANDROID_LENS_START,
    ANDROID_LENS_FILTER_DENSITY,                      // float        | public
    ANDROID_LENS_FOCAL_LENGTH,                        // float        | public
    ANDROID_LENS_FOCUS_DISTANCE,                      // float        | public
    ANDROID_LENS_OPTICAL_STABILIZATION_MODE,          // enum         | public
    ANDROID_LENS_FACING,                              // enum         | public
#if PLATFORM_SDK_VERSION <= 22
    ANDROID_LENS_POSITION,                            // float[]      | system
#endif
    ANDROID_LENS_FOCUS_RANGE,                         // float[]      | public
    ANDROID_LENS_STATE,                               // enum         | public
    ANDROID_LENS_END,
};


enum vendor_info_tags {
    ANDROID_LENS_INFO_AVAILABLE_APERTURES =           // float[]      | public
            ANDROID_LENS_INFO_START,
    ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,     // float[]      | public
    ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,        // float[]      | public
    ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,// byte[]       | public
    ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,            // float        | public
    ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,         // float        | public
    ANDROID_LENS_INFO_SHADING_MAP_SIZE,               // int32[]      | hidden
    ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,     // enum         | public
    ANDROID_LENS_INFO_END,
};


const static char *fakevendor_section_names[FAKEVENDOR_SECTION_COUNT] = {
	"android.lens",
	"android.lens.info"
};

static uint32_t fakevendor_section_bounds[FAKEVENDOR_SECTION_COUNT][2] = {
	
	{ (uint32_t) ANDROID_LENS_START,        (uint32_t) ANDROID_LENS_END },
    { (uint32_t) ANDROID_LENS_INFO_START,        (uint32_t) ANDROID_LENS_INFO_END },
    
};


vendor_tag_info_t fakevendor_lens[ANDROID_LENS_END -
        ANDROID_LENS_START] = {
    [ ANDROID_LENS_APERTURE - ANDROID_LENS_START ] =
    { "aperture",                      TYPE_FLOAT  },
    [ ANDROID_LENS_FILTER_DENSITY - ANDROID_LENS_START ] =
    { "filterDensity",                 TYPE_FLOAT  },
    [ ANDROID_LENS_FOCAL_LENGTH - ANDROID_LENS_START ] =
    { "focalLength",                   TYPE_FLOAT  },
    [ ANDROID_LENS_FOCUS_DISTANCE - ANDROID_LENS_START ] =
    { "focusDistance",                 TYPE_FLOAT  },
    [ ANDROID_LENS_OPTICAL_STABILIZATION_MODE - ANDROID_LENS_START ] =
    { "opticalStabilizationMode",      TYPE_BYTE   },
    [ ANDROID_LENS_FACING - ANDROID_LENS_START ] =
    { "facing",                        TYPE_BYTE   },
#if PLATFORM_SDK_VERSION <= 22
    [ ANDROID_LENS_POSITION - ANDROID_LENS_START ] =
    { "position",                      TYPE_FLOAT  },
#endif
    [ ANDROID_LENS_FOCUS_RANGE - ANDROID_LENS_START ] =
    { "focusRange",                    TYPE_FLOAT  },
    [ ANDROID_LENS_STATE - ANDROID_LENS_START ] =
    { "state",                         TYPE_BYTE   },

};


vendor_tag_info_t fakevendor_lens_info[ANDROID_LENS_INFO_END -
        ANDROID_LENS_INFO_START] = {
    [ ANDROID_LENS_INFO_AVAILABLE_APERTURES - ANDROID_LENS_INFO_START ] =
    { "availableApertures",            TYPE_FLOAT  },
    [ ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES - ANDROID_LENS_INFO_START ] =
    { "availableFilterDensities",      TYPE_FLOAT  },
    [ ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS - ANDROID_LENS_INFO_START ] =
    { "availableFocalLengths",         TYPE_FLOAT  },
    [ ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION - ANDROID_LENS_INFO_START ] =
    { "availableOpticalStabilization", TYPE_BYTE   },
    [ ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE - ANDROID_LENS_INFO_START ] =
    { "hyperfocalDistance",            TYPE_FLOAT  },
    [ ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE - ANDROID_LENS_INFO_START ] =
    { "minimumFocusDistance",          TYPE_FLOAT  },
    [ ANDROID_LENS_INFO_SHADING_MAP_SIZE - ANDROID_LENS_INFO_START ] =
    { "shadingMapSize",                TYPE_INT32  },
    [ ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION - ANDROID_LENS_INFO_START ] =
    { "focusDistanceCalibration",      TYPE_BYTE   },

};

vendor_tag_info_t *fakevendor_tag_info[FAKEVENDOR_SECTION_COUNT] = {
    fakevendor_lens,
	fakevendor_lens_info
};

} // namespace

VendorTags::VendorTags()
  : mTagCount(0)
{	
	int section;
    unsigned int start, end;
	for (section = 0; section < FAKEVENDOR_SECTION_COUNT; section++) {
        start = fakevendor_section_bounds[section][0];
        end = fakevendor_section_bounds[section][1];
        mTagCount += end - start;
    }
}

VendorTags::~VendorTags()
{
}

int VendorTags::getTagCount(const vendor_tag_ops_t* ops)
{
	ALOGV("%s ,mTagCount =%d",__func__,mTagCount);
    return mTagCount;
}

void VendorTags::getAllTags(const vendor_tag_ops_t* ops, uint32_t* tag_array)
{
	ALOGV("%s",__func__);
    if (tag_array == NULL) {
        ALOGE("%s: NULL tag_array", __func__);
        return;
    }
	int section;
    unsigned int start, end, tag;
	for (section = 0; section < FAKEVENDOR_SECTION_COUNT; section++) {
        start = fakevendor_section_bounds[section][0];
        end = fakevendor_section_bounds[section][1];
        for (tag = start; tag < end; tag++) {
            *tag_array++ = tag;
        }
    }
}

const char* VendorTags::getSectionName(const vendor_tag_ops_t* ops, uint32_t tag)
{
	ALOGV("%s",__func__);
	
	int tag_section = (tag >> 16) - VENDOR_SECTION;
    if (tag_section < 0 ||
            tag_section >= FAKEVENDOR_SECTION_COUNT) return NULL;

    return fakevendor_section_names[tag_section];
}

const char* VendorTags::getTagName(const vendor_tag_ops_t* ops, uint32_t tag)
{
	ALOGV("%s",__func__);

	int tag_section = (tag >> 16) - VENDOR_SECTION;
    if (tag_section < 0
            || tag_section >= FAKEVENDOR_SECTION_COUNT
            || tag >= fakevendor_section_bounds[tag_section][1]) return NULL;
    int tag_index = tag & 0xFFFF;
    return fakevendor_tag_info[tag_section][tag_index].tag_name;
}

int VendorTags::getTagType(const vendor_tag_ops_t* ops, uint32_t tag)
{
	ALOGV("%s",__func__);

	int tag_section = (tag >> 16) - VENDOR_SECTION;
    if (tag_section < 0
            || tag_section >= FAKEVENDOR_SECTION_COUNT
            || tag >= fakevendor_section_bounds[tag_section][1]) return -1;
    int tag_index = tag & 0xFFFF;
    return fakevendor_tag_info[tag_section][tag_index].tag_type;
}
} // namespace default_camera_hal
