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
 *  @date     2017/10/18
 *  @par function description:
 *  - 1 system control apis for other vendor process
 */

#ifndef ANDROID_SYSTEMCONTROLCLIENT_H
#define ANDROID_SYSTEMCONTROLCLIENT_H

#include <utils/Errors.h>
#include <string>
#include <vector>
#include <utils/Mutex.h>

#include "PQType.h"
#include <vendor/amlogic/hardware/systemcontrol/1.1/ISystemControl.h>
using ::vendor::amlogic::hardware::systemcontrol::V1_1::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControlCallback;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::DroidDisplayInfo;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::SourceInputParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::NolineParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::OverScanParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::WhiteBalanceParam;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::PQDatabaseInfo;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::Return;
using ::android::hardware::Void;


namespace android {

class SysCtrlListener : virtual public RefBase {
public:
    virtual void notify(int event) = 0;
    virtual void notifyFBCUpgrade(int state, int param) = 0;
    virtual void onSetDisplayMode(int mode) = 0;
};

class SystemControlClient  : virtual public RefBase {
public:
    SystemControlClient();
    ~SystemControlClient();

    bool getProperty(const std::string& key, std::string& value);
    bool getPropertyString(const std::string& key, std::string& value, const std::string& def);
    int32_t getPropertyInt(const std::string& key, int32_t def);
    int64_t getPropertyLong(const std::string& key, int64_t def);

    bool getPropertyBoolean(const std::string& key, bool def);
    void setProperty(const std::string& key, const std::string& value);

    bool readSysfs(const std::string& path, std::string& value);
    bool writeSysfs(const std::string& path, const std::string& value);
    bool writeSysfs(const std::string& path, const char *value, const int size);

    int32_t readHdcpRX22Key(char *value, int size);
    bool writeHdcpRX22Key(const char *value, const int size);
    int32_t readHdcpRX14Key(char *value, int size);
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHdcpRXImg(const std::string& path);

    bool readUnifyKey(const std::string& key, std::string& value);
    bool writeUnifyKey(const std::string& key, const std::string& value);
    int32_t readPlayreadyKey(const std::string& key, char *value, int size);
    bool writePlayreadyKey(const std::string& key, const char *buff, const int size);

    int32_t readAttestationKey(const std::string& node, const std::string& name, char *value, int size);
    bool writeAttestationKey(const std::string& node, const std::string& name, const char *buff, const int size);
    bool checkAttestationKey();

    void setBootEnv(const std::string& key, const std::string& value);
    bool getBootEnv(const std::string& key, std::string& value);
    void getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip);

    void loopMountUnmount(const bool &isMount, const std::string& path);

    void setMboxOutputMode(const std::string& mode);
    void setSinkOutputMode(const std::string& mode);

    void setDigitalMode(const std::string& mode);
    //void setListener(const sp<ISystemControlCallback> callback);
    void setOsdMouseMode(const std::string& mode);
    void setOsdMousePara(int x, int y, int w, int h);
    void setPosition(int left, int top, int width, int height);
    void getPosition(const std::string& mode, int &x, int &y, int &w, int &h);
    void getDeepColorAttr(const std::string& mode, std::string& value);
    void saveDeepColorAttr(const std::string& mode, const std::string& dcValue);
    int64_t resolveResolutionValue(const std::string& mode);
    void initDolbyVision(int state);
    void setDolbyVisionEnable(int state);
    bool isTvSupportDolbyVision(std::string& mode);
    int32_t getDolbyVisionType();
    void setGraphicsPriority(const std::string& mode);
    void getGraphicsPriority(std::string& mode);
    void setHdrMode(const std::string& mode);
    void setSdrMode(const std::string& mode);
    void setAppInfo(const std::string& pkg, const std::string& cls, const std::vector<std::string> &proc);

    int32_t set3DMode(const std::string& mode3d);
    void init3DSetting(void);
    int getVideo3DFormat(void);
    int getDisplay3DTo2DFormat(void);
    bool setDisplay3DTo2DFormat(int format);
    bool setDisplay3DFormat(int format);
    int getDisplay3DFormat(void);
    bool setOsd3DFormat(int format);
    bool switch3DTo2D(int format);
    bool switch2DTo3D(int format);
    void autoDetect3DForMbox(void);
    bool getSupportDispModeList(std::vector<std::string>& supportDispModes);
    bool getActiveDispMode(std::string& activeDispMode);
    bool setActiveDispMode(std::string& activeDispMode);

    void isHDCPTxAuthSuccess(int &status);
    bool getModeSupportDeepColorAttr(const std::string& mode, const std::string& color);
    void setHdrStrategy(const std::string& value);
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
    int factoryResetPQMode(void);
    int factorySetPQMode_Brightness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Brightness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Contrast(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Contrast(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Saturation(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Saturation(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Hue(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Hue(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factorySetPQMode_Sharpness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode, int value);
    int factoryGetPQMode_Sharpness(int inputSrc, int sig_fmt, int trans_fmt, int pq_mode);
    int factoryResetColorTemp(void);
    int factorySetOverscan(int inputSrc, int sigFmt, int transFmt, int he_value, int hs_value, int ve_value, int vs_value);
    tvin_cutwin_t factoryGetOverscan(int inputSrc, int sigFmt, int transFmt);
    int factorySetNolineParams(int inputSrc, int sigFmt, int transFmt, int type, int osd0_value, int osd25_value,
                                        int osd50_value, int osd75_value, int osd100_value);
    noline_params_t factoryGetNolineParams(int inputSrc, int sigFmt, int transFmt, int type);
    int factoryfactoryGetColorTemperatureParams(int colorTemp_mode);
    int factorySetParamsDefault(void);
    int factorySSMRestore(void);
    int factoryResetNonlinear(void);
    int factorySetGamma(int gamma_r, int gamma_g, int gamma_b);
    int sysSSMReadNTypes(int id, int data_len, int offset);
    int sysSSMWriteNTypes(int id, int data_len, int data_buf, int offset);
    int getActualAddr(int id);
    int getActualSize(int id);
    int SSMRecovery(void);
    int setPLLValues(source_input_param_t srcInputParam);
    int setCVD2Values(void);
    int getSSMStatus(void);
    int setCurrentHdrInfo(int32_t hdrInfo);
    int setCurrentSourceInfo(int32_t sourceInput, int32_t sigFmt, int32_t transFmt);
    void getCurrentSourceInfo(int32_t &sourceInput, int32_t &sigFmt, int32_t &transFmt);
    int setwhiteBalanceGainRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceGainGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceGainBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceOffsetRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceOffsetGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int setwhiteBalanceOffsetBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode, int32_t value);
    int getwhiteBalanceGainRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceGainGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceGainBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceOffsetRed(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceOffsetGreen(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int getwhiteBalanceOffsetBlue(int32_t inputSrc, int sig_fmt, int trans_fmt, int32_t colortemp_mode);
    int saveWhiteBalancePara(int32_t sourceType, int sig_fmt, int trans_fmt, int32_t colorTemp_mode, int32_t r_gain, int32_t g_gain, int32_t b_gain, int32_t r_offset, int32_t g_offset, int32_t b_offset);
    int getRGBPattern();
    int setRGBPattern(int32_t r, int32_t g, int32_t b);
    int factorySetDDRSSC(int32_t step);
    int factoryGetDDRSSC();
    int factorySetLVDSSSC(int32_t step);
    int factoryGetLVDSSSC();
    int whiteBalanceGrayPatternClose();
    int whiteBalanceGrayPatternOpen();
    int whiteBalanceGrayPatternSet(int32_t value);
    int whiteBalanceGrayPatternGet();
    int factorySetHdrMode(int mode);
    int factoryGetHdrMode(void);
    int setDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level);
    int getDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt);
    int factorySetDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level, int final_gain);
    int factoryGetDnlpParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int level);
    int factorySetBlackExtRegParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int val);
    int factoryGetBlackExtRegParams(int inputSrc, int32_t sigFmt, int32_t transFmt);
    int factorySetColorParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int color_type, int color_param, int val);
    int factoryGetColorParams(int inputSrc, int32_t sigFmt, int32_t transFmt, int color_type, int color_param);
    int factorySetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type, int val);
    int factoryGetNoiseReductionParams(int inputSrc, int sig_fmt, int trans_fmt, int nr_mode, int param_type);
    int factorySetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val);
    int factoryGetCTIParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type);
    int factorySetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type, int val);
    int factoryGetDecodeLumaParams(int inputSrc, int sig_fmt, int trans_fmt, int param_type);
    int factorySetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD, int param_type, int val);
    int factoryGetSharpnessParams(int inputSrc, int sig_fmt, int trans_fmt, int isHD,int param_type);
    PQDatabaseInfo getPQDatabaseInfo(int32_t dataBaseName);
    int setDtvKitSourceEnable(int isEnable);
    //PQ end

    //FBC
    int StartUpgradeFBC(const std::string&file_name, int mode, int upgrade_blk_size);
    int UpdateFBCUpgradeStatus(int state, int param);

    void setListener(const sp<SysCtrlListener> &listener);
    void setDisplayModeListener(const sp<SysCtrlListener> &listener);

 private:
     class SystemControlHidlCallback : public ISystemControlCallback {
     public:
         SystemControlHidlCallback(SystemControlClient *client): SysCtrlClient(client) {};
         Return<void> notifyCallback(const int event) override;
         Return<void> notifyFBCUpgradeCallback(int state, int param) override;
         Return<void> notifySetDisplayModeCallback(int mode) override;
     private:
         SystemControlClient *SysCtrlClient;
     };

    class SystemControlDeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        SystemControlDeathRecipient(SystemControlClient* client): mClient(client) {};
        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    private:
        SystemControlClient* mClient;
    };

    void connect();

    sp<SystemControlDeathRecipient> mDeathRecipient = nullptr;
    static SystemControlClient *mInstance;

    sp<ISystemControl> mSysCtrl;
    static Mutex mLock;
    sp<SysCtrlListener> mListener;
    sp<SysCtrlListener> mDisplayListener;
    sp<SystemControlHidlCallback> mSystemControlHidlCallback = nullptr;

};

}; // namespace android

#endif // ANDROID_SYSTEMCONTROLCLIENT_H
