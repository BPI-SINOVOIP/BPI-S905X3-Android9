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

#ifndef AM_META_DATA_EXT_H_

#define AM_META_DATA_EXT_H_

#include <sys/types.h>

#include <stdint.h>

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>

namespace android {

// The following keys map to int32_t data unless indicated otherwise.
enum {
    // DRM information to implement secure pipeline
    kKeyDrmID             = 'drID',  // raw data

    // kKeyFrameRate already exists, but we want a real number like 29.97.
    kKeyFrameRateQ16      = 'frQF',  // int32_t (video frame rate fps in Q16)
    // Key to store codec specific data.
    kKeyCodecSpecific     = 'cosp',  // raw data
    // Keys for WMA codec initialization parameters.
    kKeyWMABlockAlign     = 'blkA',  // int32_t
    kKeyWMABitsPerSample  = 'btSp',  // int32_t
    kKeyWMAFormatTag      = 'foTg',  // int32_t

    // Key to pass native window to OMX components.
    kKeyNativeWindow      = 'nawi',  // pointer

    // Key to pass CryptoInfo to player.
    kKeyCryptoInfo        = 'cryi',  // pointer

    // Key to adjust the timing of video/audio format change notification.
    kKeyFormatChangeTime = 'fctu',  // int64_t (usecs)

    // Keys to pass the original PTS and DTS to the decoder component.
    kKeyPTSFromContainer = 'ptsC',  // int64_t (usecs)
    kKeyDTSFromContainer = 'dtsC',  // int64_t (usecs)
    kKeyMediaTimeOffset  = 'mOfs',  // int64_t (usecs)

    // Keys to support various PCM formats.
    kKeyPCMBitsPerSample = 'Pbps',  // int32_t
    kKeyPCMDataEndian    = 'Pend',  // int32_t (OMX_ENDIANTYPE)
    kKeyPCMDataSigned    = 'Psgn',  // int32_t (OMX_NUMERICALDATATYPE)

    // Keys to inner subtitle support
    kKeyStreamTimeBaseNum           = 'tiBS',   //int32_t stream time base num
    kKeyStreamTimeBaseDen           = 'tiDS',   //int32_t stream time base den
    kKeyStreamStartTime             = 'stTS',   //int64_t stream start time
    kKeyStreamCodecID               = 'cIDS',   //int32_t stream codec id
    kKeyStreamCodecTag              = 'cTgS',   //int32_t stream codec tag
    kKeyPktSize                     = 'sizP',   // int32_t avPacket size
    kKeyPktPts                      = 'ptsP',   // int64_t avPacket pts
    kKeyPktDts                      = 'dtsP',   // int64_t avPacket dts
    kKeyPktDuration                 = 'durP',   // int32_t avPacket duration
    kKeyPktConvergenceDuration      = 'cduP',   // int64_t avPacket convergence duration
    kKeyPktFirstVPts                = 'VPts',   // int64_t first video pts


    // audio profile
    kKeyAudioProfile	  = 'aprf',  // int32_t
    kKeyExtraData	  = 'exda',
    kKeyExtraDataSize	  = 'edsz',
    kKeyCodecID 	  = 'cdid',

    //amffmpeg extended types
    kKeyProgramName 	  = 'proN', // cstring
    kKeyProgramNum	  = 'PrgN', // int32_t
    kKeyIsMVC		  = 'mvc ', // bool (int32_t)
    kKey4kOSD		  = '4OSD', // bool (int32_t)
    KKeyIsDV		  = 'isDV', // bool (int32_t)
    kKeyIsFLV		  = 'isFV', // bool (int32_t)
    kKeyIsAVI	          = 'iAVI', // bool (int32_t)

    kKeyBlockAlign	  = 'bagn',
    kKeyAudioFlag	  ='aufg',	// audio info reported from decoder to indicate special info
    kKeyDtsDecoderVer	  ='dtsV',
    kKeyDts958Fs	  ='dtsF',
    kKeyDts958PktSize	  ='dtsP',
    kKeyDts958PktType	  ='dtsT',
    kKeyDtsPcmSampsInFrmMaxFs='dtsS',


    // Keys to pass the unstable PTS flag to the decoder component.
    kKeyUnstablePTS = 'upts',

};


namespace media {

typedef int32_t Type;

// Amplayer extended types.
static const int kAmlTypeBase = 8192;  //extend first id.
static const Type kAudioTrackNum		 = kAmlTypeBase + 1; // Integer
static const Type kVideoTrackNum		 = kAmlTypeBase + 2; // Integer
static const Type kInnerSubtitleNum 	 = kAmlTypeBase + 3; // Integer
static const Type kAudioCodecAllInfo	 = kAmlTypeBase + 4; // String
static const Type kVideoCodecAllInfo	 = kAmlTypeBase + 5; // String
static const Type kInnerSubtitleAllInfo  = kAmlTypeBase + 6; // String
static const Type kStreamType			 = kAmlTypeBase + 7; // String
static const Type kPlayerType			 = kAmlTypeBase + 8; // Integer

}  // namespace android::media


}  // namespace android

#endif  // AM_META_DATA_EXT_H_
