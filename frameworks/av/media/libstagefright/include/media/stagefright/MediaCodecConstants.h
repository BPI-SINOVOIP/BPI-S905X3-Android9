/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef MEDIA_CODEC_CONSTANTS_H_
#define MEDIA_CODEC_CONSTANTS_H_

namespace {

// from MediaCodecInfo.java
constexpr int32_t AVCProfileBaseline = 0x01;
constexpr int32_t AVCProfileMain     = 0x02;
constexpr int32_t AVCProfileExtended = 0x04;
constexpr int32_t AVCProfileHigh     = 0x08;
constexpr int32_t AVCProfileHigh10   = 0x10;
constexpr int32_t AVCProfileHigh422  = 0x20;
constexpr int32_t AVCProfileHigh444  = 0x40;
constexpr int32_t AVCProfileConstrainedBaseline = 0x10000;
constexpr int32_t AVCProfileConstrainedHigh     = 0x80000;

constexpr int32_t AVCLevel1       = 0x01;
constexpr int32_t AVCLevel1b      = 0x02;
constexpr int32_t AVCLevel11      = 0x04;
constexpr int32_t AVCLevel12      = 0x08;
constexpr int32_t AVCLevel13      = 0x10;
constexpr int32_t AVCLevel2       = 0x20;
constexpr int32_t AVCLevel21      = 0x40;
constexpr int32_t AVCLevel22      = 0x80;
constexpr int32_t AVCLevel3       = 0x100;
constexpr int32_t AVCLevel31      = 0x200;
constexpr int32_t AVCLevel32      = 0x400;
constexpr int32_t AVCLevel4       = 0x800;
constexpr int32_t AVCLevel41      = 0x1000;
constexpr int32_t AVCLevel42      = 0x2000;
constexpr int32_t AVCLevel5       = 0x4000;
constexpr int32_t AVCLevel51      = 0x8000;
constexpr int32_t AVCLevel52      = 0x10000;

constexpr int32_t H263ProfileBaseline             = 0x01;
constexpr int32_t H263ProfileH320Coding           = 0x02;
constexpr int32_t H263ProfileBackwardCompatible   = 0x04;
constexpr int32_t H263ProfileISWV2                = 0x08;
constexpr int32_t H263ProfileISWV3                = 0x10;
constexpr int32_t H263ProfileHighCompression      = 0x20;
constexpr int32_t H263ProfileInternet             = 0x40;
constexpr int32_t H263ProfileInterlace            = 0x80;
constexpr int32_t H263ProfileHighLatency          = 0x100;

constexpr int32_t H263Level10      = 0x01;
constexpr int32_t H263Level20      = 0x02;
constexpr int32_t H263Level30      = 0x04;
constexpr int32_t H263Level40      = 0x08;
constexpr int32_t H263Level45      = 0x10;
constexpr int32_t H263Level50      = 0x20;
constexpr int32_t H263Level60      = 0x40;
constexpr int32_t H263Level70      = 0x80;

constexpr int32_t MPEG4ProfileSimple              = 0x01;
constexpr int32_t MPEG4ProfileSimpleScalable      = 0x02;
constexpr int32_t MPEG4ProfileCore                = 0x04;
constexpr int32_t MPEG4ProfileMain                = 0x08;
constexpr int32_t MPEG4ProfileNbit                = 0x10;
constexpr int32_t MPEG4ProfileScalableTexture     = 0x20;
constexpr int32_t MPEG4ProfileSimpleFace          = 0x40;
constexpr int32_t MPEG4ProfileSimpleFBA           = 0x80;
constexpr int32_t MPEG4ProfileBasicAnimated       = 0x100;
constexpr int32_t MPEG4ProfileHybrid              = 0x200;
constexpr int32_t MPEG4ProfileAdvancedRealTime    = 0x400;
constexpr int32_t MPEG4ProfileCoreScalable        = 0x800;
constexpr int32_t MPEG4ProfileAdvancedCoding      = 0x1000;
constexpr int32_t MPEG4ProfileAdvancedCore        = 0x2000;
constexpr int32_t MPEG4ProfileAdvancedScalable    = 0x4000;
constexpr int32_t MPEG4ProfileAdvancedSimple      = 0x8000;

constexpr int32_t MPEG4Level0      = 0x01;
constexpr int32_t MPEG4Level0b     = 0x02;
constexpr int32_t MPEG4Level1      = 0x04;
constexpr int32_t MPEG4Level2      = 0x08;
constexpr int32_t MPEG4Level3      = 0x10;
constexpr int32_t MPEG4Level3b     = 0x18;
constexpr int32_t MPEG4Level4      = 0x20;
constexpr int32_t MPEG4Level4a     = 0x40;
constexpr int32_t MPEG4Level5      = 0x80;
constexpr int32_t MPEG4Level6      = 0x100;

constexpr int32_t MPEG2ProfileSimple              = 0x00;
constexpr int32_t MPEG2ProfileMain                = 0x01;
constexpr int32_t MPEG2Profile422                 = 0x02;
constexpr int32_t MPEG2ProfileSNR                 = 0x03;
constexpr int32_t MPEG2ProfileSpatial             = 0x04;
constexpr int32_t MPEG2ProfileHigh                = 0x05;

constexpr int32_t MPEG2LevelLL     = 0x00;
constexpr int32_t MPEG2LevelML     = 0x01;
constexpr int32_t MPEG2LevelH14    = 0x02;
constexpr int32_t MPEG2LevelHL     = 0x03;
constexpr int32_t MPEG2LevelHP     = 0x04;

constexpr int32_t AACObjectMain       = 1;
constexpr int32_t AACObjectLC         = 2;
constexpr int32_t AACObjectSSR        = 3;
constexpr int32_t AACObjectLTP        = 4;
constexpr int32_t AACObjectHE         = 5;
constexpr int32_t AACObjectScalable   = 6;
constexpr int32_t AACObjectERLC       = 17;
constexpr int32_t AACObjectERScalable = 20;
constexpr int32_t AACObjectLD         = 23;
constexpr int32_t AACObjectHE_PS      = 29;
constexpr int32_t AACObjectELD        = 39;
constexpr int32_t AACObjectXHE        = 42;

constexpr int32_t VP8Level_Version0 = 0x01;
constexpr int32_t VP8Level_Version1 = 0x02;
constexpr int32_t VP8Level_Version2 = 0x04;
constexpr int32_t VP8Level_Version3 = 0x08;

constexpr int32_t VP8ProfileMain = 0x01;

constexpr int32_t VP9Profile0 = 0x01;
constexpr int32_t VP9Profile1 = 0x02;
constexpr int32_t VP9Profile2 = 0x04;
constexpr int32_t VP9Profile3 = 0x08;
constexpr int32_t VP9Profile2HDR = 0x1000;
constexpr int32_t VP9Profile3HDR = 0x2000;

constexpr int32_t VP9Level1  = 0x1;
constexpr int32_t VP9Level11 = 0x2;
constexpr int32_t VP9Level2  = 0x4;
constexpr int32_t VP9Level21 = 0x8;
constexpr int32_t VP9Level3  = 0x10;
constexpr int32_t VP9Level31 = 0x20;
constexpr int32_t VP9Level4  = 0x40;
constexpr int32_t VP9Level41 = 0x80;
constexpr int32_t VP9Level5  = 0x100;
constexpr int32_t VP9Level51 = 0x200;
constexpr int32_t VP9Level52 = 0x400;
constexpr int32_t VP9Level6  = 0x800;
constexpr int32_t VP9Level61 = 0x1000;
constexpr int32_t VP9Level62 = 0x2000;

constexpr int32_t HEVCProfileMain        = 0x01;
constexpr int32_t HEVCProfileMain10      = 0x02;
constexpr int32_t HEVCProfileMainStill   = 0x04;
constexpr int32_t HEVCProfileMain10HDR10 = 0x1000;

constexpr int32_t HEVCMainTierLevel1  = 0x1;
constexpr int32_t HEVCHighTierLevel1  = 0x2;
constexpr int32_t HEVCMainTierLevel2  = 0x4;
constexpr int32_t HEVCHighTierLevel2  = 0x8;
constexpr int32_t HEVCMainTierLevel21 = 0x10;
constexpr int32_t HEVCHighTierLevel21 = 0x20;
constexpr int32_t HEVCMainTierLevel3  = 0x40;
constexpr int32_t HEVCHighTierLevel3  = 0x80;
constexpr int32_t HEVCMainTierLevel31 = 0x100;
constexpr int32_t HEVCHighTierLevel31 = 0x200;
constexpr int32_t HEVCMainTierLevel4  = 0x400;
constexpr int32_t HEVCHighTierLevel4  = 0x800;
constexpr int32_t HEVCMainTierLevel41 = 0x1000;
constexpr int32_t HEVCHighTierLevel41 = 0x2000;
constexpr int32_t HEVCMainTierLevel5  = 0x4000;
constexpr int32_t HEVCHighTierLevel5  = 0x8000;
constexpr int32_t HEVCMainTierLevel51 = 0x10000;
constexpr int32_t HEVCHighTierLevel51 = 0x20000;
constexpr int32_t HEVCMainTierLevel52 = 0x40000;
constexpr int32_t HEVCHighTierLevel52 = 0x80000;
constexpr int32_t HEVCMainTierLevel6  = 0x100000;
constexpr int32_t HEVCHighTierLevel6  = 0x200000;
constexpr int32_t HEVCMainTierLevel61 = 0x400000;
constexpr int32_t HEVCHighTierLevel61 = 0x800000;
constexpr int32_t HEVCMainTierLevel62 = 0x1000000;
constexpr int32_t HEVCHighTierLevel62 = 0x2000000;

constexpr int32_t DolbyVisionProfileDvavPer = 0x1;
constexpr int32_t DolbyVisionProfileDvavPen = 0x2;
constexpr int32_t DolbyVisionProfileDvheDer = 0x4;
constexpr int32_t DolbyVisionProfileDvheDen = 0x8;
constexpr int32_t DolbyVisionProfileDvheDtr = 0x10;
constexpr int32_t DolbyVisionProfileDvheStn = 0x20;
constexpr int32_t DolbyVisionProfileDvheDth = 0x40;
constexpr int32_t DolbyVisionProfileDvheDtb = 0x80;
constexpr int32_t DolbyVisionProfileDvheSt = 0x100;
constexpr int32_t DolbyVisionProfileDvavSe = 0x200;

constexpr int32_t DolbyVisionLevelHd24    = 0x1;
constexpr int32_t DolbyVisionLevelHd30    = 0x2;
constexpr int32_t DolbyVisionLevelFhd24   = 0x4;
constexpr int32_t DolbyVisionLevelFhd30   = 0x8;
constexpr int32_t DolbyVisionLevelFhd60   = 0x10;
constexpr int32_t DolbyVisionLevelUhd24   = 0x20;
constexpr int32_t DolbyVisionLevelUhd30   = 0x40;
constexpr int32_t DolbyVisionLevelUhd48   = 0x80;
constexpr int32_t DolbyVisionLevelUhd60   = 0x100;

constexpr int32_t BITRATE_MODE_CBR = 2;
constexpr int32_t BITRATE_MODE_CQ = 0;
constexpr int32_t BITRATE_MODE_VBR = 1;

constexpr int32_t COLOR_Format12bitRGB444             = 3;
constexpr int32_t COLOR_Format16bitARGB1555           = 5;
constexpr int32_t COLOR_Format16bitARGB4444           = 4;
constexpr int32_t COLOR_Format16bitBGR565             = 7;
constexpr int32_t COLOR_Format16bitRGB565             = 6;
constexpr int32_t COLOR_Format18bitARGB1665           = 9;
constexpr int32_t COLOR_Format18BitBGR666             = 41;
constexpr int32_t COLOR_Format18bitRGB666             = 8;
constexpr int32_t COLOR_Format19bitARGB1666           = 10;
constexpr int32_t COLOR_Format24BitABGR6666           = 43;
constexpr int32_t COLOR_Format24bitARGB1887           = 13;
constexpr int32_t COLOR_Format24BitARGB6666           = 42;
constexpr int32_t COLOR_Format24bitBGR888             = 12;
constexpr int32_t COLOR_Format24bitRGB888             = 11;
constexpr int32_t COLOR_Format25bitARGB1888           = 14;
constexpr int32_t COLOR_Format32bitABGR8888           = 0x7F00A000;
constexpr int32_t COLOR_Format32bitARGB8888           = 16;
constexpr int32_t COLOR_Format32bitBGRA8888           = 15;
constexpr int32_t COLOR_Format8bitRGB332              = 2;
constexpr int32_t COLOR_FormatCbYCrY                  = 27;
constexpr int32_t COLOR_FormatCrYCbY                  = 28;
constexpr int32_t COLOR_FormatL16                     = 36;
constexpr int32_t COLOR_FormatL2                      = 33;
constexpr int32_t COLOR_FormatL24                     = 37;
constexpr int32_t COLOR_FormatL32                     = 38;
constexpr int32_t COLOR_FormatL4                      = 34;
constexpr int32_t COLOR_FormatL8                      = 35;
constexpr int32_t COLOR_FormatMonochrome              = 1;
constexpr int32_t COLOR_FormatRawBayer10bit           = 31;
constexpr int32_t COLOR_FormatRawBayer8bit            = 30;
constexpr int32_t COLOR_FormatRawBayer8bitcompressed  = 32;
constexpr int32_t COLOR_FormatRGBAFlexible            = 0x7F36A888;
constexpr int32_t COLOR_FormatRGBFlexible             = 0x7F36B888;
constexpr int32_t COLOR_FormatSurface                 = 0x7F000789;
constexpr int32_t COLOR_FormatYCbYCr                  = 25;
constexpr int32_t COLOR_FormatYCrYCb                  = 26;
constexpr int32_t COLOR_FormatYUV411PackedPlanar      = 18;
constexpr int32_t COLOR_FormatYUV411Planar            = 17;
constexpr int32_t COLOR_FormatYUV420Flexible          = 0x7F420888;
constexpr int32_t COLOR_FormatYUV420PackedPlanar      = 20;
constexpr int32_t COLOR_FormatYUV420PackedSemiPlanar  = 39;
constexpr int32_t COLOR_FormatYUV420Planar            = 19;
constexpr int32_t COLOR_FormatYUV420SemiPlanar        = 21;
constexpr int32_t COLOR_FormatYUV422Flexible          = 0x7F422888;
constexpr int32_t COLOR_FormatYUV422PackedPlanar      = 23;
constexpr int32_t COLOR_FormatYUV422PackedSemiPlanar  = 40;
constexpr int32_t COLOR_FormatYUV422Planar            = 22;
constexpr int32_t COLOR_FormatYUV422SemiPlanar        = 24;
constexpr int32_t COLOR_FormatYUV444Flexible          = 0x7F444888;
constexpr int32_t COLOR_FormatYUV444Interleaved       = 29;
constexpr int32_t COLOR_QCOM_FormatYUV420SemiPlanar   = 0x7fa30c00;
constexpr int32_t COLOR_TI_FormatYUV420PackedSemiPlanar = 0x7f000100;

constexpr char FEATURE_AdaptivePlayback[]       = "adaptive-playback";
constexpr char FEATURE_IntraRefresh[] = "intra-refresh";
constexpr char FEATURE_PartialFrame[] = "partial-frame";
constexpr char FEATURE_SecurePlayback[]         = "secure-playback";
constexpr char FEATURE_TunneledPlayback[]       = "tunneled-playback";

// from MediaFormat.java
constexpr char MIMETYPE_VIDEO_VP8[] = "video/x-vnd.on2.vp8";
constexpr char MIMETYPE_VIDEO_VP9[] = "video/x-vnd.on2.vp9";
constexpr char MIMETYPE_VIDEO_AVC[] = "video/avc";
constexpr char MIMETYPE_VIDEO_HEVC[] = "video/hevc";
constexpr char MIMETYPE_VIDEO_MPEG4[] = "video/mp4v-es";
constexpr char MIMETYPE_VIDEO_H263[] = "video/3gpp";
constexpr char MIMETYPE_VIDEO_MPEG2[] = "video/mpeg2";
constexpr char MIMETYPE_VIDEO_RAW[] = "video/raw";
constexpr char MIMETYPE_VIDEO_DOLBY_VISION[] = "video/dolby-vision";
constexpr char MIMETYPE_VIDEO_SCRAMBLED[] = "video/scrambled";

constexpr char MIMETYPE_AUDIO_AMR_NB[] = "audio/3gpp";
constexpr char MIMETYPE_AUDIO_AMR_WB[] = "audio/amr-wb";
constexpr char MIMETYPE_AUDIO_MPEG[] = "audio/mpeg";
constexpr char MIMETYPE_AUDIO_AAC[] = "audio/mp4a-latm";
constexpr char MIMETYPE_AUDIO_QCELP[] = "audio/qcelp";
constexpr char MIMETYPE_AUDIO_VORBIS[] = "audio/vorbis";
constexpr char MIMETYPE_AUDIO_OPUS[] = "audio/opus";
constexpr char MIMETYPE_AUDIO_G711_ALAW[] = "audio/g711-alaw";
constexpr char MIMETYPE_AUDIO_G711_MLAW[] = "audio/g711-mlaw";
constexpr char MIMETYPE_AUDIO_RAW[] = "audio/raw";
constexpr char MIMETYPE_AUDIO_FLAC[] = "audio/flac";
constexpr char MIMETYPE_AUDIO_MSGSM[] = "audio/gsm";
constexpr char MIMETYPE_AUDIO_AC3[] = "audio/ac3";
constexpr char MIMETYPE_AUDIO_EAC3[] = "audio/eac3";
constexpr char MIMETYPE_AUDIO_SCRAMBLED[] = "audio/scrambled";

constexpr char MIMETYPE_IMAGE_ANDROID_HEIC[] = "image/vnd.android.heic";

constexpr char MIMETYPE_TEXT_CEA_608[] = "text/cea-608";
constexpr char MIMETYPE_TEXT_CEA_708[] = "text/cea-708";
constexpr char MIMETYPE_TEXT_SUBRIP[] = "application/x-subrip";
constexpr char MIMETYPE_TEXT_VTT[] = "text/vtt";

constexpr int32_t COLOR_RANGE_FULL = 1;
constexpr int32_t COLOR_RANGE_LIMITED = 2;
constexpr int32_t COLOR_STANDARD_BT2020 = 6;
constexpr int32_t COLOR_STANDARD_BT601_NTSC = 4;
constexpr int32_t COLOR_STANDARD_BT601_PAL = 2;
constexpr int32_t COLOR_STANDARD_BT709 = 1;
constexpr int32_t COLOR_TRANSFER_HLG = 7;
constexpr int32_t COLOR_TRANSFER_LINEAR = 1;
constexpr int32_t COLOR_TRANSFER_SDR_VIDEO = 3;
constexpr int32_t COLOR_TRANSFER_ST2084 = 6;

constexpr char KEY_AAC_DRC_ATTENUATION_FACTOR[] = "aac-drc-cut-level";
constexpr char KEY_AAC_DRC_BOOST_FACTOR[] = "aac-drc-boost-level";
constexpr char KEY_AAC_DRC_EFFECT_TYPE[] = "aac-drc-effect-type";
constexpr char KEY_AAC_DRC_HEAVY_COMPRESSION[] = "aac-drc-heavy-compression";
constexpr char KEY_AAC_DRC_TARGET_REFERENCE_LEVEL[] = "aac-target-ref-level";
constexpr char KEY_AAC_ENCODED_TARGET_LEVEL[] = "aac-encoded-target-level";
constexpr char KEY_AAC_MAX_OUTPUT_CHANNEL_COUNT[] = "aac-max-output-channel_count";
constexpr char KEY_AAC_PROFILE[] = "aac-profile";
constexpr char KEY_AAC_SBR_MODE[] = "aac-sbr-mode";
constexpr char KEY_AUDIO_SESSION_ID[] = "audio-session-id";
constexpr char KEY_BIT_RATE[] = "bitrate";
constexpr char KEY_BITRATE_MODE[] = "bitrate-mode";
constexpr char KEY_CA_SESSION_ID[] = "ca-session-id";
constexpr char KEY_CA_SYSTEM_ID[] = "ca-system-id";
constexpr char KEY_CAPTURE_RATE[] = "capture-rate";
constexpr char KEY_CHANNEL_COUNT[] = "channel-count";
constexpr char KEY_CHANNEL_MASK[] = "channel-mask";
constexpr char KEY_COLOR_FORMAT[] = "color-format";
constexpr char KEY_COLOR_RANGE[] = "color-range";
constexpr char KEY_COLOR_STANDARD[] = "color-standard";
constexpr char KEY_COLOR_TRANSFER[] = "color-transfer";
constexpr char KEY_COMPLEXITY[] = "complexity";
constexpr char KEY_DURATION[] = "durationUs";
constexpr char KEY_FEATURE_[] = "feature-";
constexpr char KEY_FLAC_COMPRESSION_LEVEL[] = "flac-compression-level";
constexpr char KEY_FRAME_RATE[] = "frame-rate";
constexpr char KEY_GRID_COLUMNS[] = "grid-cols";
constexpr char KEY_GRID_ROWS[] = "grid-rows";
constexpr char KEY_HDR_STATIC_INFO[] = "hdr-static-info";
constexpr char KEY_HEIGHT[] = "height";
constexpr char KEY_I_FRAME_INTERVAL[] = "i-frame-interval";
constexpr char KEY_INTRA_REFRESH_PERIOD[] = "intra-refresh-period";
constexpr char KEY_IS_ADTS[] = "is-adts";
constexpr char KEY_IS_AUTOSELECT[] = "is-autoselect";
constexpr char KEY_IS_DEFAULT[] = "is-default";
constexpr char KEY_IS_FORCED_SUBTITLE[] = "is-forced-subtitle";
constexpr char KEY_IS_TIMED_TEXT[] = "is-timed-text";
constexpr char KEY_LANGUAGE[] = "language";
constexpr char KEY_LATENCY[] = "latency";
constexpr char KEY_LEVEL[] = "level";
constexpr char KEY_MAX_BIT_RATE[] = "max-bitrate";
constexpr char KEY_MAX_HEIGHT[] = "max-height";
constexpr char KEY_MAX_INPUT_SIZE[] = "max-input-size";
constexpr char KEY_MAX_WIDTH[] = "max-width";
constexpr char KEY_MIME[] = "mime";
constexpr char KEY_OPERATING_RATE[] = "operating-rate";
constexpr char KEY_OUTPUT_REORDER_DEPTH[] = "output-reorder-depth";
constexpr char KEY_PCM_ENCODING[] = "pcm-encoding";
constexpr char KEY_PRIORITY[] = "priority";
constexpr char KEY_PROFILE[] = "profile";
constexpr char KEY_PUSH_BLANK_BUFFERS_ON_STOP[] = "push-blank-buffers-on-shutdown";
constexpr char KEY_QUALITY[] = "quality";
constexpr char KEY_REPEAT_PREVIOUS_FRAME_AFTER[] = "repeat-previous-frame-after";
constexpr char KEY_ROTATION[] = "rotation-degrees";
constexpr char KEY_SAMPLE_RATE[] = "sample-rate";
constexpr char KEY_SLICE_HEIGHT[] = "slice-height";
constexpr char KEY_STRIDE[] = "stride";
constexpr char KEY_TEMPORAL_LAYERING[] = "ts-schema";
constexpr char KEY_TILE_HEIGHT[] = "tile-height";
constexpr char KEY_TILE_WIDTH[] = "tile-width";
constexpr char KEY_TRACK_ID[] = "track-id";
constexpr char KEY_WIDTH[] = "width";

// from MediaCodec.java
constexpr int32_t ERROR_INSUFFICIENT_OUTPUT_PROTECTION = 4;
constexpr int32_t ERROR_INSUFFICIENT_RESOURCE = 1100;
constexpr int32_t ERROR_KEY_EXPIRED = 2;
constexpr int32_t ERROR_NO_KEY = 1;
constexpr int32_t ERROR_RECLAIMED = 1101;
constexpr int32_t ERROR_RESOURCE_BUSY = 3;
constexpr int32_t ERROR_SESSION_NOT_OPENED = 5;
constexpr int32_t ERROR_UNSUPPORTED_OPERATION = 6;
constexpr char CODEC[] = "android.media.mediacodec.codec";
constexpr char ENCODER[] = "android.media.mediacodec.encoder";
constexpr char HEIGHT[] = "android.media.mediacodec.height";
constexpr char MIME_TYPE[] = "android.media.mediacodec.mime";
constexpr char MODE[] = "android.media.mediacodec.mode";
constexpr char MODE_AUDIO[] = "audio";
constexpr char MODE_VIDEO[] = "video";
constexpr char ROTATION[] = "android.media.mediacodec.rotation";
constexpr char SECURE[] = "android.media.mediacodec.secure";
constexpr char WIDTH[] = "android.media.mediacodec.width";

constexpr int32_t BUFFER_FLAG_CODEC_CONFIG = 2;
constexpr int32_t BUFFER_FLAG_END_OF_STREAM = 4;
constexpr int32_t BUFFER_FLAG_KEY_FRAME = 1;
constexpr int32_t BUFFER_FLAG_PARTIAL_FRAME = 8;
constexpr int32_t BUFFER_FLAG_SYNC_FRAME = 1;
constexpr int32_t CONFIGURE_FLAG_ENCODE = 1;
constexpr int32_t CRYPTO_MODE_AES_CBC     = 2;
constexpr int32_t CRYPTO_MODE_AES_CTR     = 1;
constexpr int32_t CRYPTO_MODE_UNENCRYPTED = 0;
constexpr int32_t INFO_OUTPUT_BUFFERS_CHANGED = -3;
constexpr int32_t INFO_OUTPUT_FORMAT_CHANGED  = -2;
constexpr int32_t INFO_TRY_AGAIN_LATER        = -1;
constexpr int32_t VIDEO_SCALING_MODE_SCALE_TO_FIT               = 1;
constexpr int32_t VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING = 2;
constexpr char PARAMETER_KEY_REQUEST_SYNC_FRAME[] = "request-sync";
constexpr char PARAMETER_KEY_SUSPEND[] = "drop-input-frames";
constexpr char PARAMETER_KEY_VIDEO_BITRATE[] = "video-bitrate";

}

#endif  // MEDIA_CODEC_CONSTANTS_H_

