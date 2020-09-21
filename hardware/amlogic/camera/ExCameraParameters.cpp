/*
 * Copyright (C) 2011 The Android Open Source Project
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




#define LOG_TAG "CAMHAL_ExCamParameters "
#include <utils/Log.h>

#include <string.h>
#include <stdlib.h>
#include "CameraHal.h"
#include <ExCameraParameters.h>

namespace android {

//extensions to camera mode
const char ExCameraParameters::HIGH_PERFORMANCE_MODE[] = "high-performance";
const char ExCameraParameters::HIGH_QUALITY_MODE[] = "high-quality";
const char ExCameraParameters::HIGH_QUALITY_ZSL_MODE[] = "high-quality-zsl";
const char ExCameraParameters::VIDEO_MODE[] = "video-mode";

//extensions to standard android Parameters
const char ExCameraParameters::KEY_SUPPORTED_CAMERAS[] = "camera-indexes";
const char ExCameraParameters::KEY_CAMERA[] = "camera-index";
const char ExCameraParameters::KEY_SHUTTER_ENABLE[] = "shutter-enable";
const char ExCameraParameters::KEY_CAMERA_NAME[] = "camera-name";
const char ExCameraParameters::KEY_BURST[] = "burst-capture";
const char ExCameraParameters::KEY_CAP_MODE[] = "mode";
const char ExCameraParameters::KEY_VNF[] = "vnf";
const char ExCameraParameters::KEY_SATURATION[] = "saturation";
const char ExCameraParameters::KEY_BRIGHTNESS[] = "brightness";
const char ExCameraParameters::KEY_EXPOSURE_MODE[] = "exposure";
const char ExCameraParameters::KEY_SUPPORTED_EXPOSURE[] = "exposure-mode-values";
const char ExCameraParameters::KEY_CONTRAST[] = "contrast";
const char ExCameraParameters::KEY_SHARPNESS[] = "sharpness";
const char ExCameraParameters::KEY_ISO[] = "iso";
const char ExCameraParameters::KEY_SUPPORTED_ISO_VALUES[] = "iso-mode-values";
const char ExCameraParameters::KEY_SUPPORTED_IPP[] = "ipp-values";
const char ExCameraParameters::KEY_IPP[] = "ipp";
const char ExCameraParameters::KEY_MAN_EXPOSURE[] = "manual-exposure";
const char ExCameraParameters::KEY_METERING_MODE[] = "meter-mode";
const char ExCameraParameters::KEY_PADDED_WIDTH[] = "padded-width";
const char ExCameraParameters::KEY_PADDED_HEIGHT[] = "padded-height";
const char ExCameraParameters::KEY_EXP_BRACKETING_RANGE[] = "exp-bracketing-range";
const char ExCameraParameters::KEY_TEMP_BRACKETING[] = "temporal-bracketing";
const char ExCameraParameters::KEY_TEMP_BRACKETING_RANGE_POS[] = "temporal-bracketing-range-positive";
const char ExCameraParameters::KEY_TEMP_BRACKETING_RANGE_NEG[] = "temporal-bracketing-range-negative";
const char ExCameraParameters::KEY_S3D_SUPPORTED[] = "s3d-supported";
const char ExCameraParameters::KEY_MEASUREMENT_ENABLE[] = "measurement";
const char ExCameraParameters::KEY_GBCE[] = "gbce";
const char ExCameraParameters::KEY_GLBCE[] = "glbce";
const char ExCameraParameters::KEY_CURRENT_ISO[] = "current-iso";
const char ExCameraParameters::KEY_SENSOR_ORIENTATION[] = "sensor-orientation";
const char ExCameraParameters::KEY_SENSOR_ORIENTATION_VALUES[] = "sensor-orientation-values";
const char ExCameraParameters::KEY_MINFRAMERATE[] = "min-framerate";
const char ExCameraParameters::KEY_MAXFRAMERATE[] = "max-framerate";
const char ExCameraParameters::KEY_RECORDING_HINT[] = "internal-recording-hint";
const char ExCameraParameters::KEY_AUTO_FOCUS_LOCK[] = "auto-focus-lock";

//extensions for enabling/disabling GLBCE
const char ExCameraParameters::GLBCE_ENABLE[] = "enable";
const char ExCameraParameters::GLBCE_DISABLE[] = "disable";

//extensions for enabling/disabling GBCE
const char ExCameraParameters::GBCE_ENABLE[] = "enable";
const char ExCameraParameters::GBCE_DISABLE[] = "disable";

//extensions for enabling/disabling measurement
const char ExCameraParameters::MEASUREMENT_ENABLE[] = "enable";
const char ExCameraParameters::MEASUREMENT_DISABLE[] = "disable";

//extensions for zoom
const char ExCameraParameters::ZOOM_SUPPORTED[] = "true";
const char ExCameraParameters::ZOOM_UNSUPPORTED[] = "false";

//extensions for 2D Preview in Stereo Mode
const char ExCameraParameters::KEY_S3D2D_PREVIEW[] = "s3d2d-preview";
const char ExCameraParameters::KEY_S3D2D_PREVIEW_MODE[] = "s3d2d-preview-values";

//extensions for SAC/SMC
const char ExCameraParameters::KEY_AUTOCONVERGENCE[] = "auto-convergence";
const char ExCameraParameters::KEY_AUTOCONVERGENCE_MODE[] = "auto-convergence-mode";
const char ExCameraParameters::KEY_MANUALCONVERGENCE_VALUES[] = "manual-convergence-values";

//extensions for setting EXIF tags
const char ExCameraParameters::KEY_EXIF_MODEL[] = "exif-model";
const char ExCameraParameters::KEY_EXIF_MAKE[] = "exif-make";

//extensions for additiona GPS data
const char ExCameraParameters::KEY_GPS_MAPDATUM[] = "gps-mapdatum";
const char ExCameraParameters::KEY_GPS_VERSION[] = "gps-version";
const char ExCameraParameters::KEY_GPS_DATESTAMP[] = "gps-datestamp";

//extensions for enabling/disabling shutter sound
const char ExCameraParameters::SHUTTER_ENABLE[] = "true";
const char ExCameraParameters::SHUTTER_DISABLE[] = "false";

//extensions for Temporal Bracketing
const char ExCameraParameters::BRACKET_ENABLE[] = "enable";
const char ExCameraParameters::BRACKET_DISABLE[] = "disable";

//extensions to Image post-processing
const char ExCameraParameters::IPP_LDCNSF[] = "ldc-nsf";
const char ExCameraParameters::IPP_LDC[] = "ldc";
const char ExCameraParameters::IPP_NSF[] = "nsf";
const char ExCameraParameters::IPP_NONE[] = "off";

//extensions to standard android pixel formats
const char ExCameraParameters::PIXEL_FORMAT_RAW[] = "raw";
const char ExCameraParameters::PIXEL_FORMAT_JPS[] = "jps";
const char ExCameraParameters::PIXEL_FORMAT_MPO[] = "mpo";
const char ExCameraParameters::PIXEL_FORMAT_RAW_JPEG[] = "raw+jpeg";
const char ExCameraParameters::PIXEL_FORMAT_RAW_MPO[] = "raw+mpo";

//extensions to standard android scene mode settings
const char ExCameraParameters::SCENE_MODE_SPORT[] = "sport";
const char ExCameraParameters::SCENE_MODE_CLOSEUP[] = "closeup";
const char ExCameraParameters::SCENE_MODE_AQUA[] = "aqua";
const char ExCameraParameters::SCENE_MODE_SNOWBEACH[] = "snow-beach";
const char ExCameraParameters::SCENE_MODE_MOOD[] = "mood";
const char ExCameraParameters::SCENE_MODE_NIGHT_INDOOR[] = "night-indoor";
const char ExCameraParameters::SCENE_MODE_DOCUMENT[] = "document";
const char ExCameraParameters::SCENE_MODE_BARCODE[] = "barcode";
const char ExCameraParameters::SCENE_MODE_VIDEO_SUPER_NIGHT[] = "super-night";
const char ExCameraParameters::SCENE_MODE_VIDEO_CINE[] = "cine";
const char ExCameraParameters::SCENE_MODE_VIDEO_OLD_FILM[] = "old-film";

//extensions to standard android white balance values.
const char ExCameraParameters::WHITE_BALANCE_TUNGSTEN[] = "tungsten";
const char ExCameraParameters::WHITE_BALANCE_HORIZON[] = "horizon";
const char ExCameraParameters::WHITE_BALANCE_SUNSET[] = "sunset";
const char ExCameraParameters::WHITE_BALANCE_FACE[] = "face-priority";

//extensions to  standard android focus modes.
const char ExCameraParameters::FOCUS_MODE_PORTRAIT[] = "portrait";
const char ExCameraParameters::FOCUS_MODE_EXTENDED[] = "extended";
const char ExCameraParameters::FOCUS_MODE_FACE[] = "face-priority";

//extensions to add  values for effect settings.
const char ExCameraParameters::EFFECT_NATURAL[] = "natural";
const char ExCameraParameters::EFFECT_VIVID[] = "vivid";
const char ExCameraParameters::EFFECT_COLOR_SWAP[] = "color-swap";
const char ExCameraParameters::EFFECT_BLACKWHITE[] = "blackwhite";

//extensions to add exposure preset modes
const char ExCameraParameters::EXPOSURE_MODE_OFF[] = "off";
const char ExCameraParameters::EXPOSURE_MODE_AUTO[] = "auto";
const char ExCameraParameters::EXPOSURE_MODE_NIGHT[] = "night";
const char ExCameraParameters::EXPOSURE_MODE_BACKLIGHT[] = "backlighting";
const char ExCameraParameters::EXPOSURE_MODE_SPOTLIGHT[] = "spotlight";
const char ExCameraParameters::EXPOSURE_MODE_SPORTS[] = "sports";
const char ExCameraParameters::EXPOSURE_MODE_SNOW[] = "snow";
const char ExCameraParameters::EXPOSURE_MODE_BEACH[] = "beach";
const char ExCameraParameters::EXPOSURE_MODE_APERTURE[] = "aperture";
const char ExCameraParameters::EXPOSURE_MODE_SMALL_APERTURE[] = "small-aperture";
const char ExCameraParameters::EXPOSURE_MODE_FACE[] = "face-priority";

//extensions to add iso values
const char ExCameraParameters::ISO_MODE_AUTO[] = "auto";
const char ExCameraParameters::ISO_MODE_100[] = "100";
const char ExCameraParameters::ISO_MODE_200[] = "200";
const char ExCameraParameters::ISO_MODE_400[] = "400";
const char ExCameraParameters::ISO_MODE_800[] = "800";
const char ExCameraParameters::ISO_MODE_1000[] = "1000";
const char ExCameraParameters::ISO_MODE_1200[] = "1200";
const char ExCameraParameters::ISO_MODE_1600[] = "1600";

//extensions to add auto convergence values
const char ExCameraParameters::AUTOCONVERGENCE_MODE_DISABLE[] = "mode-disable";
const char ExCameraParameters::AUTOCONVERGENCE_MODE_FRAME[] = "mode-frame";
const char ExCameraParameters::AUTOCONVERGENCE_MODE_CENTER[] = "mode-center";
const char ExCameraParameters::AUTOCONVERGENCE_MODE_FFT[] = "mode-fft";
const char ExCameraParameters::AUTOCONVERGENCE_MODE_MANUAL[] = "mode-manual";

//values for camera direction
const char ExCameraParameters::FACING_FRONT[]="front";
const char ExCameraParameters::FACING_BACK[]="back";

//extensions to flash settings
const char ExCameraParameters::FLASH_MODE_FILL_IN[] = "fill-in";

//extensions to add sensor orientation parameters
const char ExCameraParameters::ORIENTATION_SENSOR_NONE[] = "0";
const char ExCameraParameters::ORIENTATION_SENSOR_90[] = "90";
const char ExCameraParameters::ORIENTATION_SENSOR_180[] = "180";
const char ExCameraParameters::ORIENTATION_SENSOR_270[] = "270";
#ifdef METADATA_MODE_FOR_PREVIEW_CALLBACK
 //extensions to add preview callback in metadata mode
const char ExCameraParameters::KEY_PREVEIW_CALLBACK_IN_METADATA_ENABLE[] = "preview-callback-in-metadata-enable";
const char ExCameraParameters::KEY_PREVEIW_CALLBACK_IN_METADATA_LENGTH[] = "preview-callback-in-metadata-length";
const char ExCameraParameters::PREVEIW_CALLBACK_IN_METADATA_ENABLE[] = "1";
const char ExCameraParameters::PREVEIW_CALLBACK_IN_METADATA_DISABLE[] = "0";
const char ExCameraParameters::PREVEIW_CALLBACK_IN_METADATA_LENGTH_NONE[] = "0";
const char ExCameraParameters::PREVEIW_CALLBACK_IN_METADATA_LENGTH[] = "16";
#endif
};

