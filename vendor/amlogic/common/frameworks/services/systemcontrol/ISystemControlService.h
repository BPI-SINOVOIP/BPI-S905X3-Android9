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
 *  @author   tellen
 *  @version  1.0
 *  @date     2013/04/26
 *  @par function description:
 *  - 1 control system sysfs proc env & property
 */

#ifndef ANDROID_ISYSTEMCONTROLSERVICE_H
#define ANDROID_ISYSTEMCONTROLSERVICE_H

#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include "ISystemControlNotify.h"
#include <string>
#include <vector>
#include "PQ/include/PQType.h"

namespace android {

// must be kept in sync with ISystemControlService.java
enum {
    GET_PROPERTY            = IBinder::FIRST_CALL_TRANSACTION,
    GET_PROPERTY_STRING     = IBinder::FIRST_CALL_TRANSACTION + 1,
    GET_PROPERTY_INT        = IBinder::FIRST_CALL_TRANSACTION + 2,
    GET_PROPERTY_LONG       = IBinder::FIRST_CALL_TRANSACTION + 3,
    GET_PROPERTY_BOOL       = IBinder::FIRST_CALL_TRANSACTION + 4,
    SET_PROPERTY            = IBinder::FIRST_CALL_TRANSACTION + 5,

    READ_SYSFS              = IBinder::FIRST_CALL_TRANSACTION + 6,
    WRITE_SYSFS             = IBinder::FIRST_CALL_TRANSACTION + 7,

    GET_BOOT_ENV            = IBinder::FIRST_CALL_TRANSACTION + 8,
    SET_BOOT_ENV            = IBinder::FIRST_CALL_TRANSACTION + 9,
    GET_DISPLAY_INFO        = IBinder::FIRST_CALL_TRANSACTION + 10,
    LOOP_MOUNT_UNMOUNT      = IBinder::FIRST_CALL_TRANSACTION + 11,

    MBOX_OUTPUT_MODE        = IBinder::FIRST_CALL_TRANSACTION + 12,
    OSD_MOUSE_MODE          = IBinder::FIRST_CALL_TRANSACTION + 13,
    OSD_MOUSE_PARA          = IBinder::FIRST_CALL_TRANSACTION + 14,
    SET_POSITION            = IBinder::FIRST_CALL_TRANSACTION + 15,
    GET_POSITION            = IBinder::FIRST_CALL_TRANSACTION + 16,

    REINIT                  = IBinder::FIRST_CALL_TRANSACTION + 17,

    //used by video playback
    SET_NATIVE_WIN_RECT     = IBinder::FIRST_CALL_TRANSACTION + 18,
    SET_VIDEO_PLAYING       = IBinder::FIRST_CALL_TRANSACTION + 19,

    SET_POWER_MODE          = IBinder::FIRST_CALL_TRANSACTION + 20,
    INSTABOOT_RESET_DISPLAY = IBinder::FIRST_CALL_TRANSACTION + 21,
    SET_DIGITAL_MODE        = IBinder::FIRST_CALL_TRANSACTION + 22,
    SET_3D_MODE             = IBinder::FIRST_CALL_TRANSACTION + 23,
    SET_LISTENER            = IBinder::FIRST_CALL_TRANSACTION + 24,

    //add 3D set api for XiaoMi by wxl 20160406
    INIT_3D_SETTING                = IBinder::FIRST_CALL_TRANSACTION + 25,
    GET_VIDEO_3D_FORMAT            = IBinder::FIRST_CALL_TRANSACTION + 26,
    GET_VIDEO_3DTO2D_FORMAT        = IBinder::FIRST_CALL_TRANSACTION + 27,
    SET_VIDEO_3DTO2D_FORMAT        = IBinder::FIRST_CALL_TRANSACTION + 28,
    SET_DISPLAY_3D_FORMAT          = IBinder::FIRST_CALL_TRANSACTION + 29,
    GET_DISPLAY_3D_FORMAT          = IBinder::FIRST_CALL_TRANSACTION + 30,
    SET_OSD_3D_FORMAT_HOLDER       = IBinder::FIRST_CALL_TRANSACTION + 31,
    SET_OSD_3D_FORMAT              = IBinder::FIRST_CALL_TRANSACTION + 32,
    SWITCH_3DTO2D                  = IBinder::FIRST_CALL_TRANSACTION + 33,
    SWITCH_2DTO3D                  = IBinder::FIRST_CALL_TRANSACTION + 34,
    AUTO_DETECT_3D                 = IBinder::FIRST_CALL_TRANSACTION + 35,

    WRITE_SYSFS_BIN                 = IBinder::FIRST_CALL_TRANSACTION + 36,
    READ_HDCPRX22_KEY               = IBinder::FIRST_CALL_TRANSACTION + 37,
    WRITE_HDCPRX22_KEY              = IBinder::FIRST_CALL_TRANSACTION + 38,
    READ_HDCPRX14_KEY               = IBinder::FIRST_CALL_TRANSACTION + 39,
    WRITE_HDCPRX14_KEY              = IBinder::FIRST_CALL_TRANSACTION + 40,
    WRITE_HDCPRX_IMG                = IBinder::FIRST_CALL_TRANSACTION + 41,
    GET_SUPPORTED_DISPLAYMODE_LIST     = IBinder::FIRST_CALL_TRANSACTION + 42,
    GET_ACTIVE_DISPLAYMODE          = IBinder::FIRST_CALL_TRANSACTION + 43,
    SET_ACTIVE_DISPLAYMODE          = IBinder::FIRST_CALL_TRANSACTION + 44,
    IS_AUTHSUCCESS                  =IBinder::FIRST_CALL_TRANSACTION + 45,

    //add get/save deep color
    SAVE_DEEP_COLOR_ATTR            = IBinder::FIRST_CALL_TRANSACTION + 46,
    GET_DEEP_COLOR_ATTR             = IBinder::FIRST_CALL_TRANSACTION + 47,
    SINK_OUTPUT_MODE                = IBinder::FIRST_CALL_TRANSACTION + 48,

    WRITE_UNIFY_KEY                 = IBinder::FIRST_CALL_TRANSACTION + 49,
    READ_UNIFY_KEY                  = IBinder::FIRST_CALL_TRANSACTION + 50,

    //set dolby vision
    SET_DOLBY_VISION                = IBinder::FIRST_CALL_TRANSACTION + 51,
    TV_SUPPORT_DOLBY_VISION         = IBinder::FIRST_CALL_TRANSACTION + 52,

    RESOLVE_RESOLUTION_VALUE        = IBinder::FIRST_CALL_TRANSACTION + 53,

    //set HDR mode and SDR mode
    SET_HDR_MODE                    = IBinder::FIRST_CALL_TRANSACTION + 54,
    SET_SDR_MODE                    = IBinder::FIRST_CALL_TRANSACTION + 55,
    //add for PQ
    SET_BRIGHTNESS                  = IBinder::FIRST_CALL_TRANSACTION + 56,
    GET_BRIGHTNESS                  = IBinder::FIRST_CALL_TRANSACTION + 57,
    SAVE_BRIGHTNESS                 = IBinder::FIRST_CALL_TRANSACTION + 58,
    SET_CONTRAST                    = IBinder::FIRST_CALL_TRANSACTION + 59,
    GET_CONTRAST                    = IBinder::FIRST_CALL_TRANSACTION + 60,
    SAVE_CONTRAST                   = IBinder::FIRST_CALL_TRANSACTION + 61,
    SET_SATURATION                  = IBinder::FIRST_CALL_TRANSACTION + 62,
    GET_SATURATION                  = IBinder::FIRST_CALL_TRANSACTION + 63,
    SAVE_SATURATION                 = IBinder::FIRST_CALL_TRANSACTION + 64,
    SET_HUE                         = IBinder::FIRST_CALL_TRANSACTION + 65,
    GET_HUE                         = IBinder::FIRST_CALL_TRANSACTION + 66,
    SAVE_HUE                        = IBinder::FIRST_CALL_TRANSACTION + 67,
    SET_PQMODE                      = IBinder::FIRST_CALL_TRANSACTION + 68,
    GET_PQMODE                      = IBinder::FIRST_CALL_TRANSACTION + 69,
    SAVE_PQMODE                     = IBinder::FIRST_CALL_TRANSACTION + 70,
    SET_SHARPNESS                   = IBinder::FIRST_CALL_TRANSACTION + 71,
    GET_SHARPNESS                   = IBinder::FIRST_CALL_TRANSACTION + 72,
    SAVE_SHARPNESS                  = IBinder::FIRST_CALL_TRANSACTION + 73,
    SET_NOISEREDUCTIONMODE          = IBinder::FIRST_CALL_TRANSACTION + 74,
    GET_NOISEREDUCTIONMODE          = IBinder::FIRST_CALL_TRANSACTION + 75,
    SAVE_NOISEREDUCTIONMODE         = IBinder::FIRST_CALL_TRANSACTION + 76,
    SET_COLORTEMPERATURE            = IBinder::FIRST_CALL_TRANSACTION + 77,
    GET_COLORTEMPERATURE            = IBinder::FIRST_CALL_TRANSACTION + 78,
    SAVE_COLORTEMPERATURE           = IBinder::FIRST_CALL_TRANSACTION + 79,
    SET_COLORTEMPERATUREPARAM       = IBinder::FIRST_CALL_TRANSACTION + 80,
    GET_COLORTEMPERATUREPARAM       = IBinder::FIRST_CALL_TRANSACTION + 81,
    SAVE_COLORTEMPERATUREPARAM      = IBinder::FIRST_CALL_TRANSACTION + 82,
    SET_EYEPROTECTIONMODE           = IBinder::FIRST_CALL_TRANSACTION + 83,
    GET_EYEPROTECTIONMODE           = IBinder::FIRST_CALL_TRANSACTION + 84,
    SET_GAMMAVALUE                  = IBinder::FIRST_CALL_TRANSACTION + 85,
    GET_GAMMAVALUE                  = IBinder::FIRST_CALL_TRANSACTION + 86,
    SET_DEMOCOLORMODE               = IBinder::FIRST_CALL_TRANSACTION + 87,
    GET_DISPLAYMODE                 = IBinder::FIRST_CALL_TRANSACTION + 88,
    GET_OVERSCANPARAM               = IBinder::FIRST_CALL_TRANSACTION + 89,
    FACTORY_RESET_PQMODE            = IBinder::FIRST_CALL_TRANSACTION + 90,
    FACTORY_RESET_COLORTEMP         = IBinder::FIRST_CALL_TRANSACTION + 91,
    FACTORY_SET_PQPARAM             = IBinder::FIRST_CALL_TRANSACTION + 92,
    FACTORY_GET_PQPARAM             = IBinder::FIRST_CALL_TRANSACTION + 93,
    FACTORY_SET_COLORTEMPPARAM      = IBinder::FIRST_CALL_TRANSACTION + 94,
    FACTORY_GET_COLORTEMPPARAM      = IBinder::FIRST_CALL_TRANSACTION + 95,
    FACTORY_SAVE_COLORTEMPPARAM     = IBinder::FIRST_CALL_TRANSACTION + 96,
    FACTORY_SET_OVERSCAN            = IBinder::FIRST_CALL_TRANSACTION + 97,
    FACTORY_GET_OVERSCAN            = IBinder::FIRST_CALL_TRANSACTION + 98,
    FACTORY_SET_NOLINEPARAMS        = IBinder::FIRST_CALL_TRANSACTION + 99,
    FACTORY_GET_NOLINEPARAMS        = IBinder::FIRST_CALL_TRANSACTION + 100,
    FACTORY_RSET_PARAMS             = IBinder::FIRST_CALL_TRANSACTION + 101,
    FACTORY_SSM_RESTORE             = IBinder::FIRST_CALL_TRANSACTION + 102,
    FACTORY_RESET_NOLINEAR          = IBinder::FIRST_CALL_TRANSACTION + 103,
    FACTORY_SET_GAMMA               = IBinder::FIRST_CALL_TRANSACTION + 104,
    LOAD_PQSETTING                  = IBinder::FIRST_CALL_TRANSACTION + 105,
    SSMREADNTYPES                   = IBinder::FIRST_CALL_TRANSACTION + 106,
    SSMWRITENTYPES                  = IBinder::FIRST_CALL_TRANSACTION + 107,
    LOAD_LDIM                       = IBinder::FIRST_CALL_TRANSACTION + 108,
    SSM_GET_ADDR                    = IBinder::FIRST_CALL_TRANSACTION + 109,
    SSM_GET_SIZE                    = IBinder::FIRST_CALL_TRANSACTION + 110,
    SSM_RECOVERY                    = IBinder::FIRST_CALL_TRANSACTION + 111,
    GET_PQMODE_PARAM                = IBinder::FIRST_CALL_TRANSACTION + 112,
    SET_PLLVALUES                   = IBinder::FIRST_CALL_TRANSACTION + 113,
    SET_CVD2VALUES                  = IBinder::FIRST_CALL_TRANSACTION + 114,
    SET_PQ_CONFIG                   = IBinder::FIRST_CALL_TRANSACTION + 115,
    GET_SSM_STATUS                  = IBinder::FIRST_CALL_TRANSACTION + 116,
    RESET_LAST_SOURCE               = IBinder::FIRST_CALL_TRANSACTION + 117,
    SET_CUR_SOURCE_INFO             = IBinder::FIRST_CALL_TRANSACTION + 118,
    GET_AUTOSWITCHPCMODEFLAG        = IBinder::FIRST_CALL_TRANSACTION + 119,
    GET_BOOTANIMSTATUS              = IBinder::FIRST_CALL_TRANSACTION + 120,
    SET_DOLBY_VISION_PRIORITY       = IBinder::FIRST_CALL_TRANSACTION + 121,
    GET_DOLBY_VISION_PRIORITY       = IBinder::FIRST_CALL_TRANSACTION + 122,
    GET_DOLBY_VISION_TYPE           = IBinder::FIRST_CALL_TRANSACTION + 123,

    READ_ATTESTATION_KEY            = IBinder::FIRST_CALL_TRANSACTION + 124,
    WRITE_ATTESTATION_KEY           = IBinder::FIRST_CALL_TRANSACTION + 125,
    WRITE_PLAYREADY_KEY                 = IBinder::FIRST_CALL_TRANSACTION + 126,
    READ_PLAYREADY_KEY                  = IBinder::FIRST_CALL_TRANSACTION + 127,

    //init dolby vision
    INIT_DOLBY_VISION                = IBinder::FIRST_CALL_TRANSACTION + 128,
    GET_MODE_SUPPORT_DEEPCOLOR       = IBinder::FIRST_CALL_TRANSACTION + 129,
};

// ----------------------------------------------------------------------------

// must be kept in sync with interface defined in ISystemControlService.aidl
class ISystemControlService : public IInterface
{
public:
    DECLARE_META_INTERFACE(SystemControlService);

    virtual bool getProperty(const String16& key, String16& value) = 0;
    virtual bool getPropertyString(const String16& key, String16& value, String16& def) = 0;
    virtual int32_t getPropertyInt(const String16& key, int32_t def) = 0;
    virtual int64_t getPropertyLong(const String16& key, int64_t def) = 0;

    virtual bool getPropertyBoolean(const String16& key, bool def) = 0;
    virtual void setProperty(const String16& key, const String16& value) = 0;

    virtual bool readSysfs(const String16& path, String16& value) = 0;
    virtual bool writeSysfs(const String16& path, const String16& value) = 0;
    virtual bool writeSysfs(const String16& path, const char *value, const int size) = 0;

    virtual int32_t readHdcpRX22Key(char *value, int size) = 0;
    virtual bool writeHdcpRX22Key(const char *value, const int size) = 0;
    virtual int32_t readHdcpRX14Key(char *value, int size) = 0;
    virtual bool writeHdcpRX14Key(const char *value, const int size) = 0;
    virtual bool writeHdcpRXImg(const String16& path) = 0;
    virtual bool writeUnifyKey(const String16& path, const String16& value) = 0;
    virtual bool readUnifyKey(const String16& path, String16& value) = 0;
    virtual bool writePlayreadyKey(const String16& path, const char *buff, const int size) = 0;
    virtual int32_t readPlayreadyKey(const String16& path, char *value, int size) = 0;

    virtual int32_t readAttestationKey(const String16& node, const String16& name, char *value, int size) = 0;
    virtual bool writeAttestationKey(const String16& node, const String16& name, const char *buff, const int size) = 0;

    virtual void setBootEnv(const String16& key, const String16& value) = 0;
    virtual bool getBootEnv(const String16& key, String16& value) = 0;
    virtual void getDroidDisplayInfo(int &type, String16& socType, String16& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip) = 0;

    virtual void loopMountUnmount(int &isMount, String16& path) = 0;

    virtual void setMboxOutputMode(const String16& mode) = 0;
    virtual int32_t set3DMode(const String16& mode3d) = 0;
    virtual void setDigitalMode(const String16& mode) = 0;
    virtual void setListener(const sp<ISystemControlNotify>& listener) = 0;
    virtual void setOsdMouseMode(const String16& mode) = 0;
    virtual void setOsdMousePara(int x, int y, int w, int h) = 0;
    virtual void setPosition(int left, int top, int width, int height) = 0;
    virtual void getPosition(const String16& mode, int &x, int &y, int &w, int &h) = 0;
    virtual void getDeepColorAttr(const String16& mode, String16& value) = 0;
    virtual void saveDeepColorAttr(const String16& mode, const String16& dcValue) = 0;
    virtual int64_t resolveResolutionValue(const String16& mode) = 0;
    virtual void initDolbyVision(int state) = 0;
    virtual void setDolbyVisionEnable(int state) = 0;
    virtual int32_t getDolbyVisionType() = 0;
    virtual bool isTvSupportDolbyVision(String16& mode) = 0;
    virtual void setHdrMode(const String16& mode) = 0;
    virtual void setSdrMode(const String16& mode) = 0;
    virtual void reInit(void) = 0;
    virtual void instabootResetDisplay(void) = 0;

    virtual void setVideoPlayingAxis(void) = 0;
    //virtual void setPowerMode(int mode) = 0;

    //add 3D set api for XiaoMi by wxl 20160406
    virtual void init3DSetting(void) = 0;
    virtual int getVideo3DFormat(void) = 0;
    virtual int getDisplay3DTo2DFormat(void) = 0;
    virtual bool setDisplay3DTo2DFormat(int format) = 0;
    virtual bool setDisplay3DFormat(int format) = 0;
    virtual int getDisplay3DFormat(void) = 0;
    virtual bool setOsd3DFormat(int format) = 0;
    virtual bool switch3DTo2D(int format) = 0;
    virtual bool switch2DTo3D(int format) = 0;
    virtual void autoDetect3DForMbox(void) = 0;
    virtual bool getSupportDispModeList(std::vector<std::string> *supportDispModes) = 0;
    virtual bool getActiveDispMode(std::string* activeDispMode) = 0;
    virtual bool setActiveDispMode(std::string& activeDispMode) = 0;

    virtual void isHDCPTxAuthSuccess(int &status) = 0;
    virtual void setSinkOutputMode(const String16& mode) = 0;
    virtual void getBootanimStatus(int &status) = 0;

	//set/get dolby vision graphics priority
    virtual void setGraphicsPriority(const String16& mode) = 0;
    virtual void getGraphicsPriority(String16& mode) = 0;
    virtual bool getModeSupportDeepColorAttr(const std::string& mode,const std::string& color) = 0;
};

// ----------------------------------------------------------------------------
class BnISystemControlService: public BnInterface<ISystemControlService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_ISYSTEMCONTROLSERVICE_H
