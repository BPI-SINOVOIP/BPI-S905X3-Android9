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

#ifndef AM_MEDIA_DEFS_EXT_H_

#define AM_MEDIA_DEFS_EXT_H_

#include <stdint.h>

namespace android {

extern const int64_t kUnknownPTS;

extern const char *MEDIA_MIMETYPE_VIDEO_MSMPEG4;
extern const char *MEDIA_MIMETYPE_VIDEO_MJPEG;
extern const char *MEDIA_MIMETYPE_VIDEO_SORENSON_SPARK;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV;
extern const char *MEDIA_MIMETYPE_VIDEO_VC1;
extern const char *MEDIA_MIMETYPE_VIDEO_WVC1;

extern const char *MEDIA_MIMETYPE_VIDEO_VP6;
extern const char *MEDIA_MIMETYPE_VIDEO_VP6F;
extern const char *MEDIA_MIMETYPE_VIDEO_VP6A;
extern const char *MEDIA_MIMETYPE_VIDEO_HEVC;
extern const char *MEDIA_MIMETYPE_VIDEO_VPX;
extern const char *MEDIA_MIMETYPE_VIDEO_RM10;
extern const char *MEDIA_MIMETYPE_VIDEO_RM20;
extern const char *MEDIA_MIMETYPE_VIDEO_RM30;
extern const char *MEDIA_MIMETYPE_VIDEO_RM40;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV1;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV2;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV3;
extern const char *MEDIA_MIMETYPE_VIDEO_MSWMV3;
extern const char *MEDIA_MIMETYPE_VIDEO_AVS;
extern const char *MEDIA_MIMETYPE_VIDEO_AVS2;

extern const char *MEDIA_MIMETYPE_VIDEO_VC1;
extern const char *MEDIA_MIMETYPE_VIDEO_WVC1;

extern const char *MEDIA_MIMETYPE_AUDIO_DTS;
extern const char *MEDIA_MIMETYPE_AUDIO_MP1;
extern const char *MEDIA_MIMETYPE_AUDIO_MP2;
extern const char *MEDIA_MIMETYPE_AUDIO_ADPCM_IMA;
extern const char *MEDIA_MIMETYPE_AUDIO_ADPCM_MS;
extern const char *MEDIA_MIMETYPE_AUDIO_ALAC;
extern const char *MEDIA_MIMETYPE_AUDIO_AAC_ADIF;
extern const char *MEDIA_MIMETYPE_AUDIO_AAC_LATM;
extern const char *MEDIA_MIMETYPE_AUDIO_ADTS_PROFILE;
extern const char *MEDIA_MIMETYPE_AUDIO_WMA;
extern const char *MEDIA_MIMETYPE_AUDIO_WMAPRO;
extern const char *MEDIA_MIMETYPE_AUDIO_FFMPEG;
extern const char *MEDIA_MIMETYPE_AUDIO_DTSHD;
extern const char *MEDIA_MIMETYPE_AUDIO_APE;
extern const char *MEDIA_MIMETYPE_AUDIO_AC3;
extern const char *MEDIA_MIMETYPE_AUDIO_EC3;
extern const char *MEDIA_MIMETYPE_AUDIO_TRUEHD;
extern const char *MEDIA_MIMETYPE_AUDIO_ADTS_PROFILE;


extern const char *MEDIA_MIMETYPE_TEXT_TTML;

extern const char *MEDIA_MIMETYPE_CONTAINER_ASF;
extern const char *MEDIA_MIMETYPE_CONTAINER_FLV;
extern const char *MEDIA_MIMETYPE_CONTAINER_PMP;
extern const char *MEDIA_MIMETYPE_CONTAINER_DDP;
extern const char *MEDIA_MIMETYPE_CONTAINER_AIFF;
}  // namespace android

#endif  // AM_MEDIA_DEFS_EXT_H_
