/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @date     2017/8/30
 *  @par function description:
 *  - 1 system control hwbinder service
 */

#ifndef ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_1_SYSTEM_CONTROL_HAL_H
#define ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_1_SYSTEM_CONTROL_HAL_H

#include <vendor/amlogic/hardware/systemcontrol/1.1/ISystemControl.h>
#include <vendor/amlogic/hardware/systemcontrol/1.0/types.h>

#include <SystemControlNotify.h>
#include <SystemControlService.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace systemcontrol {
namespace V1_1 {
namespace implementation {

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
using ::android::sp;

class SystemControlHal : public ISystemControl, public SystemControlNotify {
  public:
    SystemControlHal(SystemControlService * control);
    ~SystemControlHal();

    Return<void> getSupportDispModeList(getSupportDispModeList_cb _hidl_cb) override;
    Return<void> getActiveDispMode(getActiveDispMode_cb _hidl_cb) override;
    Return<Result> setActiveDispMode(const hidl_string &activeDispMode) override;
    Return<Result> isHDCPTxAuthSuccess() override;
    Return<void> getProperty(const hidl_string &key, getProperty_cb _hidl_cb) override;
    Return<void> getPropertyString(const hidl_string &key, const hidl_string &def, getPropertyString_cb _hidl_cb) override;
    Return<void> getPropertyInt(const hidl_string &key, int32_t def, getPropertyInt_cb _hidl_cb)  override;
    Return<void> getPropertyLong(const hidl_string &key, int64_t def, getPropertyLong_cb _hidl_cb) override;
    Return<void> getPropertyBoolean(const hidl_string &key, bool def, getPropertyBoolean_cb _hidl_cb) override;
    Return<Result> setProperty(const hidl_string &key, const hidl_string &value) override;
    Return<void> readSysfs(const hidl_string &path, readSysfs_cb _hidl_cb) override;
    Return<Result> writeSysfs(const hidl_string &path, const hidl_string &value) override;
    Return<Result> writeSysfsBin(const hidl_string &path, const hidl_array<int32_t, 4096>& key, int32_t size) override;
    Return<void> readHdcpRX22Key(int32_t size, readHdcpRX22Key_cb _hidl_cb) override;
    Return<Result> writeHdcpRX22Key(const hidl_array<int32_t, 4096>& key, int32_t size) override;
    Return<void> readHdcpRX14Key(int32_t size, readHdcpRX14Key_cb _hidl_cb) override;
    Return<Result> writeHdcpRX14Key(const hidl_array<int32_t, 4096>& key, int32_t size) override;
    Return<Result> writeHdcpRXImg(const hidl_string &path) override;
    Return<void> readUnifyKey(const hidl_string &key, readUnifyKey_cb _hidl_cb) override;
    Return<Result> writeUnifyKey(const hidl_string &key, const hidl_string &value) override;
    Return<void> readPlayreadyKey(const hidl_string &key, int32_t size, readHdcpRX14Key_cb _hidl_cb) override;
    Return<Result> writePlayreadyKey(const hidl_string &path, const hidl_array<int32_t, 4096>& key, int32_t size) override;

    Return<void> readAttestationKey(const hidl_string &node, const hidl_string &name, int32_t size, readAttestationKey_cb _hidl_cb) override;
    Return<Result> writeAttestationKey(const hidl_string &node, const hidl_string &name, const hidl_array<int32_t, 10240>& key) override;
    Return<Result> checkAttestationKey() override;

    Return<void> getBootEnv(const hidl_string &key, getBootEnv_cb _hidl_cb) override;
    Return<void> setBootEnv(const hidl_string &key, const hidl_string &value) override;
    Return<void> getDroidDisplayInfo(getDroidDisplayInfo_cb _hidl_cb) override;
    Return<void> loopMountUnmount(int32_t isMount, const hidl_string& path) override;
    Return<void> setSourceOutputMode(const hidl_string& mode) override;
    Return<void> setSinkOutputMode(const hidl_string& mode) override;
    Return<void> setDigitalMode(const hidl_string& mode) override;
    Return<void> setOsdMouseMode(const hidl_string& mode) override;
    Return<void> setOsdMousePara(int32_t x, int32_t y, int32_t w, int32_t h) override;
    Return<void> setPosition(int32_t left, int32_t top, int32_t width, int32_t height) override;
    Return<void> getPosition(const hidl_string& mode, getPosition_cb _hidl_cb) override;
    Return<void> saveDeepColorAttr(const hidl_string& mode, const hidl_string& dcValue) override;
    Return<void> getDeepColorAttr(const hidl_string &mode, getDeepColorAttr_cb _hidl_cb) override;
    Return<void> initDolbyVision(int32_t state) override;
    Return<void> setDolbyVisionState(int32_t state) override;
    Return<void> sinkSupportDolbyVision(sinkSupportDolbyVision_cb _hidl_cb) override;

    Return<void> setGraphicsPriority(const hidl_string& mode) override;
    Return<void> getGraphicsPriority(getGraphicsPriority_cb _hidl_cb) override;
    Return<void> getDolbyVisionType(getDolbyVisionType_cb _hidl_cb) override;
    Return<void> setHdrMode(const hidl_string& mode) override;
    Return<void> setSdrMode(const hidl_string& mode) override;
    Return<void> resolveResolutionValue(const hidl_string& mode, resolveResolutionValue_cb _hidl_cb) override;
    Return<void> setCallback(const sp<ISystemControlCallback>& callback) override;
    Return<Result> setAppInfo(const hidl_string& pkg, const hidl_string& cls, const hidl_vec<hidl_string>& proc) override;
    Return<void> getPrefHdmiDispMode(getPrefHdmiDispMode_cb _hidl_cb) override;

    //for 3D
    Return<void> set3DMode(const hidl_string& mode) override;
    Return<void> init3DSetting() override;
    Return<void> getVideo3DFormat(getVideo3DFormat_cb _hidl_cb) override;
    Return<void> getDisplay3DTo2DFormat(getDisplay3DTo2DFormat_cb _hidl_cb) override;
    Return<void> setDisplay3DTo2DFormat(int32_t format) override;
    Return<void> setDisplay3DFormat(int32_t format) override;
    Return<void> getDisplay3DFormat(getDisplay3DFormat_cb _hidl_cb) override;
    Return<void> setOsd3DFormat(int32_t format) override;
    Return<void> switch3DTo2D(int32_t format) override;
    Return<void> switch2DTo3D(int32_t format) override;
    Return<void> autoDetect3DForMbox() override;

    //PQ
    Return<int32_t> loadPQSettings(const SourceInputParam& srcInputParam) override;
    Return<int32_t> setPQmode(int32_t mode, int32_t isSave, int32_t is_autoswitch)  override;
    Return<int32_t> getPQmode(void) override;
    Return<int32_t> savePQmode(int32_t mode) override;
    Return<int32_t> setColorTemperature(int32_t mode, int32_t isSave) override;
    Return<int32_t> getColorTemperature(void) override;
    Return<int32_t> saveColorTemperature(int32_t mode) override;
    Return<int32_t> setColorTemperatureUserParam(int32_t mode, int32_t isSave, int32_t param_type, int32_t value) override;
    Return<void> getColorTemperatureUserParam(getColorTemperatureUserParam_cb _hidl_cb) override;
    Return<int32_t> setBrightness(int32_t value, int32_t isSave) override;
    Return<int32_t> getBrightness(void) override;
    Return<int32_t> saveBrightness(int32_t value) override;
    Return<int32_t> setContrast(int32_t value, int32_t isSave) override;
    Return<int32_t> getContrast(void) override;
    Return<int32_t> saveContrast(int32_t value) override;
    Return<int32_t> setSaturation(int32_t value, int32_t isSave) override;
    Return<int32_t> getSaturation(void) override;
    Return<int32_t> saveSaturation(int32_t value) override;
    Return<int32_t> setHue(int32_t value, int32_t isSave) override;
    Return<int32_t> getHue(void) override;
    Return<int32_t> saveHue(int32_t value) override;
    Return<int32_t> setSharpness(int32_t value, int32_t is_enable, int32_t isSave) override;
    Return<int32_t> getSharpness(void) override;
    Return<int32_t> saveSharpness(int32_t value) override;
    Return<int32_t> setNoiseReductionMode(int32_t nr_mode, int32_t isSave) override;
    Return<int32_t> getNoiseReductionMode(void) override;
    Return<int32_t> saveNoiseReductionMode(int32_t nr_mode) override;
    Return<int32_t> setEyeProtectionMode(int32_t inputSrc, int32_t enable, int32_t isSave) override;
    Return<int32_t> getEyeProtectionMode(int32_t inputSrc) override;
    Return<int32_t> setGammaValue(int32_t gamma_curve, int32_t isSave) override;
    Return<int32_t> getGammaValue(void) override;
    Return<int32_t> setDisplayMode(int32_t inputSrc, int32_t mode, int32_t isSave) override;
    Return<int32_t> getDisplayMode(int32_t inputSrc) override;
    Return<int32_t> saveDisplayMode(int32_t inputSrc, int32_t mode) override;
    Return<int32_t> setBacklight(int32_t value, int32_t isSave) override;
    Return<int32_t> getBacklight(void) override;
    Return<int32_t> saveBacklight(int32_t value) override;
    Return<int32_t> setDynamicBacklight(int32_t mode, int32_t isSave) override;
    Return<int32_t> getDynamicBacklight(void) override;
    Return<int32_t> setLocalContrastMode(int32_t mode, int32_t isSave) override;
    Return<int32_t> getLocalContrastMode() override;
    Return<int32_t> setColorBaseMode(int32_t mode, int32_t isSave) override;
    Return<int32_t> getColorBaseMode() override;
    Return<int32_t> checkLdimExist(void) override;
    Return<int32_t> factorySetPQMode_Brightness(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode, int32_t value) override;
    Return<int32_t> factoryGetPQMode_Brightness(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode) override;
    Return<int32_t> factorySetPQMode_Contrast(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode, int32_t value) override;
    Return<int32_t> factoryGetPQMode_Contrast(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode) override;
    Return<int32_t> factorySetPQMode_Saturation(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode, int32_t value) override;
    Return<int32_t> factoryGetPQMode_Saturation(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode) override;
    Return<int32_t> factorySetPQMode_Hue(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode, int32_t value) override;
    Return<int32_t> factoryGetPQMode_Hue(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode) override;
    Return<int32_t> factorySetPQMode_Sharpness(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode, int32_t value) override;
    Return<int32_t> factoryGetPQMode_Sharpness(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t pq_mode) override;
    Return<int32_t> factoryResetPQMode(void) override;
    Return<int32_t> factoryResetColorTemp(void) override;
    Return<int32_t> factorySetParamsDefault(void) override;
    Return<int32_t> factorySetNolineParams(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t type, int32_t osd0_value, int32_t osd25_value,
                                                    int32_t osd50_value, int32_t osd75_value, int32_t osd100_value) override;
    Return<void> factoryGetNolineParams(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t type, factoryGetNolineParams_cb _hidl_cb) override;
    Return<int32_t> factoryfactoryGetColorTemperatureParams(int32_t colorTemp_mode) override;
    Return<int32_t> factorySetOverscan(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t he_value, int32_t hs_value, int32_t ve_value, int32_t vs_value) override;
    Return<void> factoryGetOverscan(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, factoryGetOverscan_cb _hidl_cb) override;
    Return<int32_t> factorySSMRestore(void) override;
    Return<int32_t> factoryResetNonlinear(void) override;
    Return<int32_t> factorySetGamma(int32_t gamma_r, int32_t gamma_g, int32_t gamma_b) override;
    Return<int32_t> sysSSMReadNTypes(int32_t id, int32_t data_len, int32_t offset) override;
    Return<int32_t> sysSSMWriteNTypes(int32_t id, int32_t data_len, int32_t data_buf, int32_t offset) override;
    Return<int32_t> getActualAddr(int32_t id) override;
    Return<int32_t> getActualSize(int32_t id) override;
    Return<int32_t> SSMRecovery(void) override;
    Return<int32_t> setPLLValues(const SourceInputParam& srcInputParam) override;
    Return<int32_t> setCVD2Values() override;
    Return<int32_t> getSSMStatus(void) override;
    Return<int32_t> setCurrentSourceInfo(int32_t sourceInput, int32_t sigFmt, int32_t transFmt) override;
    Return<void> getCurrentSourceInfo(getCurrentSourceInfo_cb _hidl_cb) override;
    Return<int32_t> setCurrentHdrInfo(int32_t hdrInfo) override;
    Return<int32_t> setwhiteBalanceGainRed(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode, int32_t value) override;
    Return<int32_t> setwhiteBalanceGainGreen(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode, int32_t value) override;
    Return<int32_t> setwhiteBalanceGainBlue(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode, int32_t value) override;
    Return<int32_t> setwhiteBalanceOffsetRed(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode, int32_t value) override;
    Return<int32_t> setwhiteBalanceOffsetGreen(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode, int32_t value) override;
    Return<int32_t> setwhiteBalanceOffsetBlue(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode, int32_t value) override;
    Return<int32_t> getwhiteBalanceGainRed(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode) override;
    Return<int32_t> getwhiteBalanceGainGreen(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode) override;
    Return<int32_t> getwhiteBalanceGainBlue(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode) override;
    Return<int32_t> getwhiteBalanceOffsetRed(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode) override;
    Return<int32_t> getwhiteBalanceOffsetGreen(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode) override;
    Return<int32_t> getwhiteBalanceOffsetBlue(int32_t inputSrc, int32_t sigFmt, int32_t transFmt, int32_t colortemp_mode) override;
    Return<int32_t> saveWhiteBalancePara(int32_t sourceType, int32_t sigFmt, int32_t transFmt, int32_t colorTemp_mode, int32_t r_gain, int32_t g_gain, int32_t b_gain, int32_t r_offset, int32_t g_offset, int32_t b_offset) override;
    Return<int32_t> getRGBPattern() override;
    Return<int32_t> setRGBPattern(int32_t r, int32_t g, int32_t b) override;
    Return<int32_t> factorySetDDRSSC(int32_t step) override;
    Return<int32_t> factoryGetDDRSSC() override;
    Return<int32_t> factorySetLVDSSSC(int32_t step) override;
    Return<int32_t> factoryGetLVDSSSC() override;
    Return<int32_t> whiteBalanceGrayPatternClose() override;
    Return<int32_t> whiteBalanceGrayPatternOpen() override;
    Return<int32_t> whiteBalanceGrayPatternSet(int32_t value) override;
    Return<int32_t> whiteBalanceGrayPatternGet() override;
    Return<int32_t> factorySetHdrMode(int32_t mode) override;
    Return<int32_t> factoryGetHdrMode(void) override;
    Return<int32_t> setDnlpParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t level) override;
    Return<int32_t> getDnlpParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt) override;
    Return<int32_t> factorySetDnlpParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t level, int32_t final_gain) override;
    Return<int32_t> factoryGetDnlpParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t level) override;
    Return<int32_t> factorySetBlackExtRegParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t val) override;
    Return<int32_t> factoryGetBlackExtRegParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt) override;
    Return<int32_t> factorySetColorParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t color_type, int32_t color_param, int32_t val) override;
    Return<int32_t> factoryGetColorParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t color_type, int32_t color_param) override;
    Return<int32_t> factorySetNoiseReductionParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t nr_mode, int32_t param_type, int32_t val) override;
    Return<int32_t> factoryGetNoiseReductionParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t nr_mode, int32_t param_type) override;
    Return<int32_t> factorySetCTIParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t param_type, int32_t val) override;
    Return<int32_t> factoryGetCTIParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t param_type) override;
    Return<int32_t> factorySetDecodeLumaParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t param_type, int32_t val) override;
    Return<int32_t> factoryGetDecodeLumaParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t param_type) override;
    Return<int32_t> factorySetSharpnessParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t isHD, int32_t param_type, int32_t val) override;
    Return<int32_t> factoryGetSharpnessParams(int32_t inputSrc, int32_t sig_fmt, int32_t trans_fmt, int32_t isHD, int32_t param_type) override;
    Return<void> getPQDatabaseInfo(int32_t dataBaseName, getPQDatabaseInfo_cb _hidl_cb) override;
    Return<int32_t> setDtvKitSourceEnable(int32_t isEnable) override;
    //PQ end

    //FBC
    Return<int32_t> StartUpgradeFBC(const hidl_string& fileName, int32_t mode, int32_t upgrade_blk_size) override;
    Return<int32_t> UpdateFBCUpgradeStatus(int32_t state, int32_t param) override;

    Return<Result> getModeSupportDeepColorAttr(const hidl_string& mode, const hidl_string& color) override;
    Return<void> setHdrStrategy(const hidl_string &value) override;
    virtual void onEvent(int event);
    virtual void onFBCUpgradeEvent(int32_t state, int32_t param);
    virtual void onSetDisplayMode(int mode);

  private:
    void handleServiceDeath(uint32_t cookie);

    SystemControlService *mSysControl;
    mutable Mutex mLock;
    std::map<uint32_t, sp<ISystemControlCallback>> mClients;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<SystemControlHal> sch);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<SystemControlHal> mSystemControlHal;
    };

    sp<DeathRecipient> mDeathRecipient;

};

}  // namespace implementation
}  // namespace V1_1
}  // namespace systemcontrol
}  // namespace hardware
}  // namespace android
}  // namespace vendor

#endif  // ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_0_SYSTEM_CONTROL_HAL_H
