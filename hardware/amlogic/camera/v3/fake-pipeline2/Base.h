/*
 * Copyright (C) 2012 The Android Open Source Project
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

/**
 * This file includes various basic structures that are needed by multiple parts
 * of the fake camera 2 implementation.
 */

#ifndef HW_EMULATOR_CAMERA2_BASE_H
#define HW_EMULATOR_CAMERA2_BASE_H

//#include <system/window.h>
#include <hardware/camera2.h>
#include <utils/Vector.h>

namespace android {


/* Internal structure for passing buffers across threads */
struct StreamBuffer {
    // Positive numbers are output streams
    // Negative numbers are input reprocess streams
    // Zero is an auxillary buffer
    int streamId;
    uint32_t width, height;
    uint32_t format;
    uint32_t stride;
    buffer_handle_t *buffer;
    uint8_t *img;
    int     share_fd;
};
typedef Vector<StreamBuffer> Buffers;

struct Stream {
    const camera2_stream_ops_t *ops;
    uint32_t width, height;
    int32_t format;
    uint32_t stride;
};

struct ReprocessStream {
    const camera2_stream_in_ops_t *ops;
    uint32_t width, height;
    int32_t format;
    uint32_t stride;
    // -1 if the reprocessing stream is independent
    int32_t sourceStreamId;
};

struct ExifInfo {
	int mainwidth;
	int mainheight;
	int thumbwidth;
	int thumbheight;
	int64_t gpsTimestamp;
	double longitude;
	double latitude;
	double altitude;
	uint8_t gpsProcessingMethod[128];
	bool has_longitude;
	bool has_latitude;
	bool has_altitude;
	bool has_gpsTimestamp;
	bool has_gpsProcessingMethod;
	bool has_focallen;
	float focallen;
	int orientation;
};
} // namespace android;

#endif
