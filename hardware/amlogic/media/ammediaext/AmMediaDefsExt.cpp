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

#define __STDINT_LIMITS
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <AmMediaDefsExt.h>

namespace android
{

const int64_t kUnknownPTS = INT64_MIN;

const char *MEDIA_MIMETYPE_VIDEO_MJPEG = "video/mjpeg";
const char *MEDIA_MIMETYPE_VIDEO_MSMPEG4 = "video/x-msmpeg";
const char *MEDIA_MIMETYPE_VIDEO_SORENSON_SPARK = "video/x-sorenson-spark";
const char *MEDIA_MIMETYPE_VIDEO_WMV = "video/x-ms-wmv";
const char *MEDIA_MIMETYPE_VIDEO_VC1 = "video/vc1";
const char *MEDIA_MIMETYPE_VIDEO_WVC1 = "video/wvc1";
const char *MEDIA_MIMETYPE_VIDEO_VPX = "video/x-vnd.on2.vp8";
const char *MEDIA_MIMETYPE_VIDEO_RM10 = "video/rm10";
const char *MEDIA_MIMETYPE_VIDEO_RM20 = "video/rm20";
const char *MEDIA_MIMETYPE_VIDEO_RM30 = "video/rm30";
const char *MEDIA_MIMETYPE_VIDEO_RM40 = "video/rm40";
const char *MEDIA_MIMETYPE_VIDEO_VP6 = "video/x-vnd.on2.vp6";
const char *MEDIA_MIMETYPE_VIDEO_VP6F = "video/x-vnd.on2.vp6f";
const char *MEDIA_MIMETYPE_VIDEO_VP6A = "video/x-vnd.on2.vp6a";
const char *MEDIA_MIMETYPE_VIDEO_WMV1 = "video/wmv1";
const char *MEDIA_MIMETYPE_VIDEO_WMV2 = "video/wmv2";
const char *MEDIA_MIMETYPE_VIDEO_WMV3 = "video/wmv3";
const char *MEDIA_MIMETYPE_VIDEO_MSWMV3 = "video/x-ms-wmv";
const char *MEDIA_MIMETYPE_VIDEO_AVS = "video/avs";
const char *MEDIA_MIMETYPE_VIDEO_AVS2 = "video/avs2";

const char *MEDIA_MIMETYPE_AUDIO_DTS = "audio/dtshd";
const char *MEDIA_MIMETYPE_AUDIO_MP1 = "audio/mp1";
const char *MEDIA_MIMETYPE_AUDIO_MP2 = "audio/mp2";
const char *MEDIA_MIMETYPE_AUDIO_ADPCM_IMA = "audio/adpcm-ima";
const char *MEDIA_MIMETYPE_AUDIO_ADPCM_MS = "audio/adpcm-ms";
const char *MEDIA_MIMETYPE_AUDIO_ALAC = "audio/alac";
const char *MEDIA_MIMETYPE_AUDIO_AAC_ADIF = "audio/aac-adif";
const char *MEDIA_MIMETYPE_AUDIO_AAC_LATM = "audio/aac-latm";
const char *MEDIA_MIMETYPE_AUDIO_ADTS_PROFILE = "audio/adts";
const char *MEDIA_MIMETYPE_AUDIO_WMA = "audio/wma";
const char *MEDIA_MIMETYPE_AUDIO_WMAPRO = "audio/wmapro";
const char *MEDIA_MIMETYPE_AUDIO_DTSHD  = "audio/dtshd";
const char *MEDIA_MIMETYPE_AUDIO_TRUEHD = "audio/truehd";
const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
const char *MEDIA_MIMETYPE_AUDIO_EC3 = "audio/eac3";

const char *MEDIA_MIMETYPE_AUDIO_APE = "audio/ape";
const char *MEDIA_MIMETYPE_AUDIO_FFMPEG = "audio/ffmpeg";

const char *MEDIA_MIMETYPE_TEXT_TTML = "application/ttml+xml";

const char *MEDIA_MIMETYPE_CONTAINER_ASF = "video/x-ms-asf";
const char *MEDIA_MIMETYPE_CONTAINER_FLV = "video/x-flv";
const char *MEDIA_MIMETYPE_CONTAINER_PMP = "video/pmp";
const char *MEDIA_MIMETYPE_CONTAINER_AIFF = "audio/x-aiff";
const char *MEDIA_MIMETYPE_CONTAINER_DDP = "audio/ddp";
}  // namespace android
