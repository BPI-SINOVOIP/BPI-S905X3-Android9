/*
 * Copyright (C) 2010 The Android Open Source Project
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
#ifndef OMX_VendorExt_h
#define OMX_VendorExt_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_Core.h>

typedef enum OMX_AUDIO_VENDOR_CODINGEXTTYPE {
    OMX_AUDIO_CodingAndroidDTSHD =   OMX_AUDIO_CodingVendorStartUnused, /**< DTSHD encoded data */
    OMX_AUDIO_CodingAndroidALAC,
    OMX_AUDIO_CodingAndroidTRUEHD,
    OMX_AUDIO_CodingFFMPEG,              /**< ffmpeg audio encoded data */
} OMX_AUDIO_VENDOR_CODINGEXTTYPE;

typedef enum OMX_VIDEO_VENDOR_CODINGEXTTYPE {
    OMX_VIDEO_ExtCodingUnused = OMX_VIDEO_CodingVendorStartUnused,
    OMX_VIDEO_CodingVC1,
    OMX_VIDEO_CodingVP6,        /**< VP6 */
    OMX_VIDEO_CodingMSMPEG4,    /**< MSMPEG4 */
    OMX_VIDEO_CodingSorenson,  /**< Sorenson codec*/
    OMX_VIDEO_CodingWMV3,
    OMX_VIDEO_CodingRV10,
    OMX_VIDEO_CodingRV20,
    OMX_VIDEO_CodingRV30,
    OMX_VIDEO_CodingRV40,
    OMX_VIDEO_CodingAVS,
    OMX_VIDEO_CodingAVS2,
} OMX_VIDEO_CODINGEXTTYPE;

typedef enum OMX_VENDOR_INDEXEXTTYPE {
    OMX_IndexParamAudioAndroidDtshd = OMX_IndexVendorStartUnused + 0x00100000,        /**< reference: OMX_AUDIO_PARAM_ANDROID_DTSHDTYPE */
    OMX_IndexParamAudioAndroidAsf,    /**< reference: OMX_AUDIO_PARAM_ANDROID_ASFTYPE */
    OMX_IndexParamAudioAndroidApe,/**< reference: OMX_AUDIO_PARAM_ANDROID_APETYPE */
    OMX_IndexParamAudioAndroidAlac,               /**< reference: OMX_AUDIO_PARAM_ANDROID_ALACTYPE */
    OMX_IndexParamAudioAndroidTruehd,             /**< reference: OMX_AUDIO_PARAM_ANDROID_TRUEHDTYPE */
    OMX_IndexParamAudioFFmpeg,
    OMX_IndexParamAudioDolbyAudio, /**< reference: OMX_AUDIO_PARAM_DOLBYAUDIOTYPE */

    OMX_IndexParamLowLatencyMode = OMX_IndexVendorStartUnused + 0x00200000,                  /* add by aml */
    OMX_IndexParam4kosd,                           /* add by aml */
    OMX_IndexParamUnstablePTS,                     /* add by aml */
} OMX_VENDOR_INDEXEXTTYPE;

typedef struct OMX_AUDIO_PARAM_DOLBYAUDIOTYPE {
    OMX_U32 nSize;            /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex;       /**< port that this structure applies to */
    OMX_BOOL bExtendFormat;        /**< Using extend format for output  4Bytes PCM size + pcm data + 4Bytes Raw size + Raw data*/
    OMX_U32 nAudioCodec;      /**< AudioCodec.  1.ac3 2.eac3. */
    OMX_U32 nPassThroughEnable;    /**< IEC raw data enable, 0 for disable, 1 for enable*/
} OMX_AUDIO_PARAM_DOLBYAUDIOTYPE;

typedef struct OMX_AUDIO_PARAM_ANDROID_DTSHDTYPE {
    OMX_U32 nSize;            /**< Size of this structure, in Bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex;       /**< Port that this structure applies to */

    OMX_U16 nChannels;
    OMX_U32 nSamplesPerSec;
    OMX_U16 bitwidth;
    OMX_BOOL bExtendFormat;   /**< Using extend format for output 4Bytes PCM size + pcm data + 4Bytes Raw size + Raw data */
    int HwHDPCMoutCap;
    int HwMulChoutCap;
    OMX_U32 nPassThroughEnable;    /**< IEC raw data enable, 0 for disable, 1 for enable*/
} OMX_AUDIO_PARAM_ANDROID_DTSHDTYPE;

typedef struct OMX_AUDIO_PARAM_ANDROID_ASFTYPE {
    OMX_U32 nSize;            /**< Size of this structure, in Bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex;       /**< Port that this structure applies to */

    OMX_U16 wFormatTag;
    OMX_U16 nChannels;
    OMX_U32 nSamplesPerSec;
    OMX_U32 nAvgBitratePerSec;
    OMX_U16 nBlockAlign;
    OMX_U16 wBitsPerSample;
    OMX_U16 extradata_size;
    OMX_U8  extradata[128];
} OMX_AUDIO_PARAM_ASFTYPE;

/** Ffmpeg params */
typedef struct OMX_AUDIO_PARAM_FFMPEGTYPE {
    OMX_U32 nSize;              /**< Size of this structure, in Bytes */
    OMX_VERSIONTYPE nVersion;   /**< OMX specification version information */
    OMX_U32 nPortIndex;         /**< Port that this structure applies to */
    OMX_U32 nChannels;          /**< Number of channels */
    OMX_U32 nBitRate;           /**< Bit rate of the input data.  Use 0 for variable
                                                                  rate or unknown bit rates */
    OMX_U32 nSamplingRate;      /**< is the sampling rate of the source data */
    OMX_U32 nBitPerSample;      /**< Bit per sample */
    OMX_U32 nBlockAlign;        /**< block align */
    OMX_U32 nCodecID;           /**<codec id **/
    OMX_U32 nExtraData_Size;     /** extra data size **/
    OMX_U8 nExtraData[1];          /** extra data point **/
} OMX_AUDIO_PARAM_FFMPEGTYPE;

typedef struct OMX_AUDIO_PARAM_ANDROID_APETYPE {
    OMX_U32 nSize;            /**< Size of this structure, in Bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex;       /**< Port that this structure applies to */

    OMX_U16 nChannels;
    OMX_U32 nSamplesPerSec;
    OMX_U16 wBitsPerSample;
    OMX_U16 extradata_size;
    OMX_U8  *extradata;
} OMX_AUDIO_PARAM_APETYPE;

typedef struct OMX_AUDIO_PARAM__ANDROID_TRUEHDTYPE {
    OMX_U32 nSize; /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex; /**< port that this structure applies to */

    OMX_U16 nChannels;
    OMX_U32 nSamplesPerSec;
    OMX_U16 wBitsPerSample;
    OMX_BOOL bExtendFormat; /**< Using extend format for output 4Bytes PCM size + pcm data + 4Bytes Raw size + Raw data*/
    OMX_U32 nAudioCodec; /**< AudioCodec. 1.ac3 2.eac3. */
    OMX_BOOL is_passthrough_active;
} OMX_AUDIO_PARAM__ANDROID_TRUEHDTYPE;

typedef struct OMX_AUDIO_PARAM_ANDROID_ALACTYPE {
    OMX_U32 nSize;            /**< Size of this structure, in Bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex;       /**< Port that this structure applies to */

    OMX_U16 nChannels;
    OMX_U32 nSamplesPerSec;
    OMX_U16 extradata_size;
    OMX_U8 *extradata;
} OMX_AUDIO_PARAM_ALACTYPE;

typedef struct OMX_VIDEO_FORCESHUTDOWMCOMPONENT{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL isForceShutdowm;
} OMX_VIDEO_FORCESHUTDOWMCOMPONENT;

typedef struct OMX_VIDEO_PARAM_IS_MVC {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bIsMVC;
} OMX_VIDEO_PARAM_IS_MVC;

/**
 * AML RM/WMV2 Video information
 */
typedef struct OMX_VIDEO_INFO {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U8  mExtraData[128];
    OMX_U32 nExtraDataSize;
    OMX_U32 width;
    OMX_U32 height;
} OMX_VIDEO_INFO;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_VindorExt_h */
/* File EOF */
