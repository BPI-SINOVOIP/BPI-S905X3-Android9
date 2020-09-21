/*
 * Copyright (C) 2014 The Android Open Source Project
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

/*
 * Contains implementation of a class EmulatedFakeCamera3 that encapsulates
 * functionality of an advanced fake camera.
 */

#define LOG_TAG "EmulatedCamera_FakeCamera3Info"
#include "EmulatedFakeCamera3.h"
#include "inc/DebugUtils.h"

namespace android {

//level: legacy:0-4, limited:1-5, full:2-6
const struct EmulatedFakeCamera3::KeyInfo_s EmulatedFakeCamera3::sKeyInfo[] = {
    {ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES     , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES          , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AE_AVAILABLE_MODES                      , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES          , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AE_COMPENSATION_RANGE                   , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AE_COMPENSATION_STEP                    , LEGACY   ,   BC                   },
#if 1
    {ANDROID_CONTROL_AF_AVAILABLE_MODES                      , LEGACY   ,   BC                   },
#endif
    {ANDROID_CONTROL_AVAILABLE_EFFECTS                       , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AVAILABLE_SCENE_MODES                   , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES     , LEGACY   ,   BC                   },
    {ANDROID_CONTROL_AWB_AVAILABLE_MODES                     , LEGACY   ,   BC                   },
//    {ANDROID_CONTROL_MAX_REGIONS_AE                          , LEGACY   ,   BC                   },
//    {ANDROID_CONTROL_MAX_REGIONS_AF                          , LEGACY   ,   BC                   },
//    {ANDROID_CONTROL_MAX_REGIONS_AWB                         , LEGACY   ,   BC                   },
    {ANDROID_EDGE_AVAILABLE_EDGE_MODES                       , FULL     ,   NONE                 },
    {ANDROID_FLASH_INFO_AVAILABLE                            , LEGACY   ,   BC                   },
    {ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES             , OPT      ,   RAW                  },
    {ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL                   , LEGACY   ,   BC                   },
    {ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES                  , LEGACY   ,   BC                   },
    {ANDROID_LENS_FACING                                     , LEGACY   ,   BC                   },
    {ANDROID_LENS_INFO_AVAILABLE_APERTURES                   , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES            , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS               , LEGACY   ,   BC                   },
    {ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION       , LIMITED  ,   MANUAL_SENSOR        },
    {ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION            , LIMITED  ,   MANUAL_SENSOR        },
    {ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE                   , LIMITED  ,   MANUAL_SENSOR        },
    {ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE                , LIMITED  ,   NONE                 },
    {ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES , LEGACY   ,   BC                   },
    {ANDROID_REQUEST_AVAILABLE_CAPABILITIES                  , LEGACY   ,   BC                   },
//    {ANDROID_REQUEST_MAX_NUM_OUTPUT_PROC                     , LEGACY   ,   BC                   },
//    {ANDROID_REQUEST_MAX_NUM_OUTPUT_PROC_STALLING            , LEGACY   ,   BC                   },
//    {ANDROID_REQUEST_MAX_NUM_OUTPUT_RAW                      , LEGACY   ,   BC                   },
    {ANDROID_REQUEST_PARTIAL_RESULT_COUNT                    , LEGACY   ,   BC                   },
    {ANDROID_REQUEST_PIPELINE_MAX_DEPTH                      , LEGACY   ,   BC                   },
    {ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM               , LEGACY   ,   BC                   },
//    {ANDROID_SCALER_STREAM_CONFIGURATION_MAP                 , LEGACY   ,   BC                   },
    {ANDROID_SCALER_CROPPING_TYPE                            , LEGACY   ,   BC                   },
    {ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES             , LEGACY   ,   NONE                 },
    {ANDROID_SENSOR_BLACK_LEVEL_PATTERN                      , FULL     ,   MANUAL_SENSOR | RAW  },
    {ANDROID_SENSOR_CALIBRATION_TRANSFORM1                   , OPT      ,   RAW                  },
    {ANDROID_SENSOR_CALIBRATION_TRANSFORM2                   , OPT      ,   RAW                  },
    {ANDROID_SENSOR_COLOR_TRANSFORM1                         , OPT      ,   RAW                  },
    {ANDROID_SENSOR_COLOR_TRANSFORM2                         , OPT      ,   RAW                  },
    {ANDROID_SENSOR_FORWARD_MATRIX1                          , OPT      ,   RAW                  },
    {ANDROID_SENSOR_FORWARD_MATRIX2                          , OPT      ,   RAW                  },
    {ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE                   , LEGACY   ,   BC | RAW             },
    {ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT            , FULL     ,   RAW                  },
    {ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE                 , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_SENSOR_INFO_MAX_FRAME_DURATION                  , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_SENSOR_INFO_PHYSICAL_SIZE                       , LEGACY   ,   BC                   },
    {ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE                    , LEGACY   ,   BC                   },
    {ANDROID_SENSOR_INFO_SENSITIVITY_RANGE                   , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_SENSOR_INFO_WHITE_LEVEL                         , OPT      ,   RAW                  },
    {ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE                    , LEGACY   ,   BC                   },
    {ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY                   , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_SENSOR_ORIENTATION                              , LEGACY   ,   BC                   },
    {ANDROID_SENSOR_REFERENCE_ILLUMINANT1                    , OPT      ,   RAW                  },
    {ANDROID_SENSOR_REFERENCE_ILLUMINANT2                    , OPT      ,   RAW                  },
    {ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES     , LEGACY   ,   BC                   },
    {ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES   , OPT      ,   RAW                  },
    {ANDROID_STATISTICS_INFO_MAX_FACE_COUNT                  , LEGACY   ,   BC                   },
    {ANDROID_SYNC_MAX_LATENCY                                , LEGACY   ,   BC                   },
    {ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES                , FULL     ,   MANUAL_SENSOR        },
    {ANDROID_TONEMAP_MAX_CURVE_POINTS                        , FULL     ,   MANUAL_SENSOR        },
    /////////////////////split/////////////////////////
};

const struct EmulatedFakeCamera3::KeyInfo_s EmulatedFakeCamera3::sKeyInfoReq[] = {
//    {ANDROID_CONTROL_AE_ANTIBANDING_MODE, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_AE_LOCK, LIMITED   ,   BC                   },
    {ANDROID_CONTROL_AE_MODE, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_AE_TARGET_FPS_RANGE, LIMITED   ,   BC                   },
    {ANDROID_CONTROL_AF_MODE, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_AF_TRIGGER, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_AWB_LOCK, LIMITED   ,   BC                   },
    {ANDROID_CONTROL_AWB_MODE, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_CAPTURE_INTENT, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_EFFECT_MODE, LIMITED   ,   BC                   },
    {ANDROID_CONTROL_MODE, LIMITED   ,   BC                   },
//    {ANDROID_CONTROL_SCENE_MODE, LIMITED   ,   BC                   },
    {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, LIMITED   ,   BC                   },
//    {ANDROID_FLASH_MODE, LIMITED   ,   BC                   },
//    {ANDROID_JPEG_GPS_LOCATION, LIMITED   ,   BC                   },
//    {ANDROID_JPEG_ORIENTATION, LIMITED   ,   BC                   },
//    {ANDROID_JPEG_QUALITY, LIMITED   ,   BC                   },
//    {ANDROID_JPEG_THUMBNAIL_QUALITY, LIMITED   ,   BC                   },
//    {ANDROID_JPEG_THUMBNAIL_SIZE, LIMITED   ,   BC                   },
//    {ANDROID_SCALER_CROP_REGION, LIMITED   ,   BC                   },
    {ANDROID_STATISTICS_FACE_DETECT_MODE, LIMITED   ,   BC                   },

//    {ANDROID_TONEMAP_MODE, LIMITED   ,  MANUAL_POST_PROCESSING          },
//    {ANDROID_COLOR_CORRECTION_GAINS, LIMITED   ,  MANUAL_POST_PROCESSING          },
//    {ANDROID_COLOR_CORRECTION_TRANSFORM, LIMITED   ,  MANUAL_POST_PROCESSING          },
//    {ANDROID_SHADING_MODE, LIMITED   ,  MANUAL_POST_PROCESSING          },
//    {ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, LIMITED   ,  MANUAL_POST_PROCESSING          },
//    {ANDROID_TONEMAP_CURVE, FULL   ,  MANUAL_POST_PROCESSING          },
//    {ANDROID_COLOR_CORRECTION_ABERRATION_MODE, FULL   ,  MANUAL_POST_PROCESSING          },
    {ANDROID_COLOR_CORRECTION_ABERRATION_MODE, LEGACY   ,  BC},

//    {ANDROID_SENSOR_FRAME_DURATION, LIMITED   ,  MANUAL_SENSOR          },
//    {ANDROID_SENSOR_EXPOSURE_TIME, LIMITED   ,  MANUAL_SENSOR          },
//    {ANDROID_SENSOR_SENSITIVITY, LIMITED   ,  MANUAL_SENSOR          },
//    {ANDROID_LENS_APERTURE, LIMITED   ,  MANUAL_SENSOR          },
//    {ANDROID_LENS_FILTER_DENSITY, LIMITED   ,  MANUAL_SENSOR          },
    {ANDROID_LENS_OPTICAL_STABILIZATION_MODE, LIMITED   ,  MANUAL_SENSOR          },
//    {ANDROID_BLACK_LEVEL_LOCK, LIMITED   ,  MANUAL_SENSOR          },
    {ANDROID_NOISE_REDUCTION_MODE,          LEGACY   ,   BC                   },
};
const struct EmulatedFakeCamera3::KeyInfo_s EmulatedFakeCamera3::sKeyInfoResult[] = {
};

const struct EmulatedFakeCamera3::KeyInfo_s EmulatedFakeCamera3::sKeyBackwardCompat[] = {
    {ANDROID_CONTROL_AE_ANTIBANDING_MODE, 0,},
    {ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, 0,},
    {ANDROID_CONTROL_AE_LOCK, 0,},
    {ANDROID_CONTROL_AE_MODE, 0,},
    {ANDROID_CONTROL_AE_TARGET_FPS_RANGE, 0,},
    {ANDROID_CONTROL_AF_MODE, 0,},
    {ANDROID_CONTROL_AF_TRIGGER, 0,},
    {ANDROID_CONTROL_AWB_LOCK, 0,},
    {ANDROID_CONTROL_AWB_MODE, 0,},
    {ANDROID_CONTROL_CAPTURE_INTENT, 0,},
    {ANDROID_CONTROL_EFFECT_MODE, 0,},
    {ANDROID_CONTROL_MODE, 0,},
    {ANDROID_CONTROL_SCENE_MODE, 0,},
    {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, 0,},
    {ANDROID_FLASH_MODE, 0,},
    //{ANDROID_JPEG_GPS_LOCATION, 0},
    {ANDROID_JPEG_ORIENTATION, 0,},
    {ANDROID_JPEG_QUALITY, 0,},
    {ANDROID_JPEG_THUMBNAIL_QUALITY, 0,},
    {ANDROID_JPEG_THUMBNAIL_SIZE, 0,},
    {ANDROID_SCALER_CROP_REGION, 0,},
    {ANDROID_STATISTICS_FACE_DETECT_MODE, 0,},
};

int EmulatedFakeCamera3::getAvailableChKeys(CameraMetadata *info, uint8_t level){
//actualHwLevel: legacy:0, limited:1, full:2
    enum hardware_level_e actualHwLevel;
    uint8_t availCapMask = NONE;
    int size, sizeReq ,sizeofbckComp;
    int availCount = 0;
    camera_metadata_entry e;
    const struct KeyInfo_s *keyInfo = &EmulatedFakeCamera3::sKeyInfo[0];

    size = sizeof(sKeyInfo)/sizeof(struct KeyInfo_s);
    sizeReq = sizeof(sKeyInfoReq)/sizeof(sKeyInfoReq[0]);
    sizeofbckComp = sizeof(sKeyBackwardCompat)/sizeof(sKeyBackwardCompat[0]);
    int32_t available_keys[size+sizeReq+sizeofbckComp];

    e = info->find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES);
    if (e.count <= 0) {
        CAMHAL_LOGDA("uncertained capabilities!!!\n");
        availCapMask = BC;
    }
    for (size_t i=0; i < e.count; i++) {
        availCapMask |= (1 << e.data.u8[i]);
    }
    CAMHAL_LOGDB("availCapMask=%x\n", availCapMask);

    switch (level) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
            actualHwLevel = LIMITED;
            break;

        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            actualHwLevel = FULL;
            break;

        default:
            CAMHAL_LOGDA("!!!!uncertain hardware level\n");
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            actualHwLevel = LEGACY;
            break;
    }

    for(int i = 0; i < size ; i++){

        if (actualHwLevel >= keyInfo->level) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
        } else if ( availCapMask & keyInfo->capmask) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
#if 0
        } else if ((actualHwLevel != LEGACY) || (keyInfo->level == OPT)) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
#endif
        }

        keyInfo ++;
    }

    info->update(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
            (int32_t *)available_keys, availCount);

    CAMHAL_LOGVB("availableKeySize=%d\n", availCount);

    keyInfo = &EmulatedFakeCamera3::sKeyInfoReq[0];
    for (int i = 0; i < sizeReq; i ++) {
        if (actualHwLevel >= keyInfo->level) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
        } else if ( availCapMask & keyInfo->capmask) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
#if 0
        } else if ((actualHwLevel != LEGACY) || (keyInfo->level == OPT)) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
#endif
        }
        keyInfo ++;
    }
   keyInfo = &EmulatedFakeCamera3::sKeyBackwardCompat[0];
    for (int i = 0; i < sizeofbckComp; i ++){

        if (actualHwLevel >= keyInfo->level) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
        } else if ( availCapMask & keyInfo->capmask) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
#if 0
        } else if ((actualHwLevel != LEGACY) || (keyInfo->level == OPT)) {
            available_keys[availCount] = keyInfo->key;
            availCount ++;
#endif
        }

        keyInfo ++;

        ////////////////

    }

    info->update(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS,
            (int32_t *)available_keys, availCount);

    info->update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS,
            (int32_t *)available_keys, availCount);
    CAMHAL_LOGVB("availableKeySize=%d\n", availCount);
    return 0;
}

} //namespace android
