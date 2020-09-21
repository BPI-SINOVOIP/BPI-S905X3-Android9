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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2017/09/20
 *  @par function description:
 *  - 1 system control interface
 */

#ifndef ANDROID_SYSTEM_CONTROL_SERVICE_H
#define ANDROID_SYSTEM_CONTROL_SERVICE_H

#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Mutex.h>
#include <ISystemControlService.h>

#include "SysWrite.h"
#include "common.h"
#include "DisplayMode.h"
#include "Dimension.h"
#include <string>
#include <vector>

#include "CPQControl.h"
#include "ubootenv/Ubootenv.h"
#include "CFbcCommunication.h"

#include <vendor/amlogic/hardware/droidvold/1.0/IDroidVold.h>
using ::vendor::amlogic::hardware::droidvold::V1_0::IDroidVold;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::Return;

extern "C" int vdc_loop(int argc, char **argv);

namespace android {
// ----------------------------------------------------------------------------

class SystemControlService
{
public:
    SystemControlService(const char *path);
    virtual ~SystemControlService();

    bool getSupportDispModeList(std::vector<std::string> *supportDispModes);
    bool getActiveDispMode(std::string *activeDispMode);
    bool setActiveDispMode(std::string& activeDispMode);
    //read write property and sysfs
    bool getProperty(const std::string &key, std::string *value);
    bool getPropertyString(const std::string &key, std::string *value, const std::string &def);
    int32_t getPropertyInt(const std::string &key, int32_t def);
    int64_t getPropertyLong(const std::string &key, int64_t def);
    bool getPropertyBoolean(const std::string& key, bool def);
    void setProperty(const std::string& key, const std::string& value);
    bool readSysfs(const std::string& path, std::string& value);
    bool writeSysfs(const std::string& path, const std::string& value);
    bool writeSysfs(const std::string& path, const char *value, const int size);
    int32_t readHdcpRX22Key(char *value __attribute__((unused)), int size __attribute__((unused)));
    bool writeHdcpRX22Key(const char *value, const int size);
    int32_t readHdcpRX14Key(char *value __attribute__((unused)), int size __attribute__((unused)));
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHdcpRXImg(const std::string& path);
    bool readUnifyKey(const std::string& key, std::string& value);
    bool writeUnifyKey(const std::string& key, const std::string& value);
    int32_t readPlayreadyKey(const std::string& key, char *value, int size);
    bool writePlayreadyKey(const std::string& key, const char *buff, const int size);

    int32_t readAttestationKey(const std::string& node, const std::string& name, char *value, int size);
    bool writeAttestationKey(const std::string& node, const std::string& name, const char *buff, const int size);
    bool checkAttestationKey();
    bool getModeSupportDeepColorAttr(const std::string& mode,const std::string& color);
    //set or get uboot env
    bool getBootEnv(const std::string& key, std::string& value);
    void setBootEnv(const std::string& key, const std::string& value);
    void getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip);
    void loopMountUnmount(int isMount, const std::string& path);
    void setSourceOutputMode(const std::string& mode);
    void setSinkOutputMode(const std::string& mode);
    void setDigitalMode(const std::string& mode);
    void setOsdMouseMode(const std::string& mode);
    void setOsdMousePara(int x, int y, int w, int h);
    void setPosition(int left, int top, int width, int height);
    void getPosition(const std::string& mode, int &x, int &y, int &w, int &h);
    void initDolbyVision(int state);
    void setDolbyVisionEnable(int state);
    bool isTvSupportDolbyVision(std::string& mode);
    int32_t getDolbyVisionType();
    void setGraphicsPriority(const std::string& mode);
    void getGraphicsPriority(std::string& mode);
    void isHDCPTxAuthSuccess(int &status);
    void saveDeepColorAttr(const std::string& mode, const std::string& dcValue);
    void getDeepColorAttr(const std::string& mode, std::string& value);
    void setHdrMode(const std::string& mode);
    void setSdrMode(const std::string& mode);
    int64_t resolveResolutionValue(const std::string& mode);
    void setListener(const sp<SystemControlNotify>& listener);
    void setAppInfo(const std::string& pkg, const std::string& cls, const std::vector<std::string>& procList);
    bool getPrefHdmiDispMode(std::string *prefDispMode);

    //3D
    int32_t set3DMode(const std::string& mode3d);
    void init3DSetting(void);
    int32_t getVideo3DFormat(void);
    int32_t getDisplay3DTo2DFormat(void);
    bool setDisplay3DTo2DFormat(int format);
    bool setDisplay3DFormat(int format);
    int32_t getDisplay3DFormat(void);
    bool setOsd3DFormat(int format);
    bool switch3DTo2D(int format);
    bool switch2DTo3D(int format);
    void autoDetect3DForMbox();
    //3D end

    //PQ
    int loadPQSettings(source_input_param_t srcInputParam);
    int setPQmode(int mode, int isSave, int is_autoswitch);
    int getPQmode(void);
    int savePQmode(int mode);
    int setColorTemperature(int mode, int isSave);
    int getColorTemperature(void);
    int saveColorTemperature(int mode);
    int setColorTemperatureUserParam(int mode, int isSave, int param_type, int value);
    tcon_rgb_ogo_t getColorTemperatureUserParam(void);
    int setBrightness(int value, int isSave);
    int getBrightness(void);
    int saveBrightness(int value);
    int setContrast(int value, int isSave);
    int getContrast(void);
    int saveContrast(int value);
    int setSaturation(int value, int isSave);
    int getSaturation(void);
    int saveSaturation(int value);
    int setHue(int value, int isSave );
    int getHue(void);
    int saveHue(int value);
    int setSharpness(int value, int is_enable, int isSave);
    int getSharpness(void);
    int saveSharpness(int value);
    int setNoiseReductionMode(int nr_mode, int isSave);
    int getNoiseReductionMode(void);
    int saveNoiseReductionMode(int nr_mode);
    int setEyeProtectionMode(int source_input, int enable, int isSave);
    int getEyeProtectionMode(int source_input);
    int setGammaValue(int gamma_curve, int isSave);
    int getGammaValue(void);
    int setDisplayMode(int source_input, int mode, int isSave);
    int getDisplayMode(int source_input);
    int saveDisplayMode(int source_input, int mode);
    int setBacklight(int value, int isSave);
    int getBacklight(void);
    int saveBacklight(int value);
    int setDynamicBacklight(int mode, int isSave);
    int getDynamicBacklight(void);
    int setLocalContrastMode(int mode, int isSave);
    int getLocalContrastMode();
    int setColorBaseMode(int mode, int isSave);
    int getColorBaseMode();
    bool checkLdimExist(void);
    int factorySetPQMode_Brightness(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value);
    int factoryGetPQMode_Brightness(int inputSrc, int sigFmt, int transFmt, int pq_mode);
    int factorySetPQMode_Contrast(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value);
    int factoryGetPQMode_Contrast(int inputSrc, int sigFmt, int transFmt, int pq_mode);
    int factorySetPQMode_Saturation(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value);
    int factoryGetPQMode_Saturation(int inputSrc, int sigFmt, int transFmt, int pq_mode);
    int factorySetPQMode_Hue(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value);
    int factoryGetPQMode_Hue(int inputSrc, int sigFmt, int transFmt, int pq_mode);
    int factorySetPQMode_Sharpness(int inputSrc, int sigFmt, int transFmt, int pq_mode, int value);
    int factoryGetPQMode_Sharpness(int inputSrc, int sigFmt, int transFmt, int pq_mode);
    int factoryResetPQMode(void);
    int factoryResetColorTemp(void);
    int factorySetParamsDefault(void);
    int factorySetOverscan(int inputSrc, int sigFmt, int transFmt, int he_value, int hs_value, int ve_value, int vs_value);
    tvin_cutwin_t factoryGetOverscan(int inputSrc, int sigFmt, int transFmt);
    int factorySetNolineParams(int inputSrc, int sigFmt, int transFmt, int type, int osd0_value, int osd25_value, int osd50_value, int osd75_value, int osd100_value);
    noline_params_t factoryGetNolineParams(int inputSrc, int sigFmt, int transFmt, int type);
    int factoryGetColorTemperatureParams(int colorTemp_mode);
    int factorySSMRestore(void);
    int factoryResetNonlinear(void);
    int factorySetGamma(int gamma_r, int gamma_g, int gamma_b);
    int factorySetHdrMode(int mode);
    int factoryGetHdrMode(void);
    int sysSSMReadNTypes(int id, int data_len, int offset);
    int sysSSMWriteNTypes(int id, int data_len, int data_buf, int offset);
    int getActualAddr(int id);
    int getActualSize(int id);
    int SSMRecovery(void);
    int setPLLValues(source_input_param_t srcInputParam);
    int setCVD2Values(void);
    int getSSMStatus(void);
    int setCurrentSourceInfo(int sourceInput, int sigFmt, int transFmt);
    source_input_param_t getCurrentSourceInfo(void);
    int setCurrentHdrInfo(int hdrInfo);
    int setwhiteBalanceGainRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value);
    int setwhiteBalanceGainGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value);
    int setwhiteBalanceGainBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value);
    int setwhiteBalanceOffsetRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value);
    int setwhiteBalanceOffsetGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value);
    int setwhiteBalanceOffsetBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode, int value);
    int getwhiteBalanceGainRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode);
    int getwhiteBalanceGainGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode);
    int getwhiteBalanceGainBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode);
    int getwhiteBalanceOffsetRed(int inputSrc, int sigFmt, int transFmt, int colortemp_mode);
    int getwhiteBalanceOffsetGreen(int inputSrc, int sigFmt, int transFmt, int colortemp_mode);
    int getwhiteBalanceOffsetBlue(int inputSrc, int sigFmt, int transFmt, int colortemp_mode);
    int saveWhiteBalancePara(int inputSrc, int sigFmt, int transFmt, int colorTemp_mode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset);
    int getRGBPattern();
    int setRGBPattern(int r, int g, int b);
    int factorySetDDRSSC (int step);
    int factoryGetDDRSSC(void);
    int factorySetLVDSSSC (int step);
    int factoryGetLVDSSSC(void);
    int setLVDSSSC(int step);
    int whiteBalanceGrayPatternOpen();
    int whiteBalanceGrayPatternClose();
    int whiteBalanceGrayPatternSet(int value);
    int whiteBalanceGrayPatternGet();
    int setDnlpParams(int inputSrc, int sigFmt, int transFmt, int level);
    int getDnlpParams(int inputSrc, int sigFmt, int transFmt);
    int factorySetDnlpParams(int inputSrc, int sigFmt, int transFmt, int level, int final_gain);
    int factoryGetDnlpParams(int inputSrc, int sigFmt, int transFmt, int level);
    int factorySetBlackExtRegParams(int inputSrc, int sigFmt, int transFmt, int val);
    int factoryGetBlackExtRegParams(int inputSrc, int sigFmt, int transFmt);
    int factorySetColorParams(int inputSrc, int sigFmt, int transFmt, int color_type, int color_param, int val);
    int factoryGetColorParams(int inputSrc, int sigFmt, int transFmt, int color_type, int color_param);
    int factorySetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type, int val);
    int factoryGetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type);
    int factorySetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val);
    int factoryGetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type);
    int factorySetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val);
    int factoryGetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type);
    int factorySetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD, int param_type, int val);
    int factoryGetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD,int param_type);
    tvpq_databaseinfo_t getPQDatabaseInfo(int dataBaseName);
    int setDtvKitSourceEnable(int isEnable);
    void setHdrStrategy(const std::string& value);
    //PQ end

    //FBC
    void setFBCUpgradeListener(const sp<SystemControlNotify>& listener);
    int StartUpgradeFBC(const std::string&file_name, int mode, int upgrade_blk_size);
    int UpdateFBCUpgradeStatus(int status, int param);

    void setDisplayModeListener(const sp<SystemControlNotify>& listener);
    void SendDisplayMode(int mode);

    static SystemControlService* instantiate(const char *cfgpath);
    virtual status_t dump(int fd, const Vector<String16>& args);

    int getLogLevel();

private:
    int permissionCheck();
    void setLogLevel(int level);
    void traceValue(const std::string& type, const std::string& key, const std::string& value);
    void traceValue(const std::string& type, const std::string& key, const int size);
    int getProcName(pid_t pid, String16& procName);

    mutable Mutex mLock;

    int mLogLevel;

    SysWrite *pSysWrite;
    DisplayMode *pDisplayMode;
    CPQControl *pCPQControl;
    Dimension *pDimension;
    Ubootenv *pUbootenv;
    CConfigFile *mTvhalConfigFile;

    struct DroidVoldDeathRecipient : public android::hardware::hidl_death_recipient  {
        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    };
    sp<DroidVoldDeathRecipient> mDeathRecipient = nullptr;

    sp<IDroidVold> mDroidVold;
    sp<SystemControlNotify> mNotifyListener;
    sp<SystemControlNotify> mDisplayListener;
};

// ----------------------------------------------------------------------------

} // namespace android
#endif // ANDROID_SYSTEM_CONTROL_SERVICE_H
