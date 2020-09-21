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
 *  @date     2018/1/15
 *  @par function description:
 *  - 1 droidlogic tvserver daemon, hwbiner implematation
 */

#ifndef ANDROID_DROIDLOGIC_HDMI_CEC_V1_0_H
#define ANDROID_DROIDLOGIC_HDMI_CEC_V1_0_H

#include <binder/IBinder.h>
#include <utils/Mutex.h>
#include <vector>
#include <map>

#include "DroidTvServiceIntf.h"
#include <vendor/amlogic/hardware/tvserver/1.0/ITvServer.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace tvserver {
namespace V1_0 {
namespace implementation {

using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServer;
using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServerCallback;
using ::vendor::amlogic::hardware::tvserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::tvserver::V1_0::SignalInfo;
using ::vendor::amlogic::hardware::tvserver::V1_0::FormatInfo;
using ::vendor::amlogic::hardware::tvserver::V1_0::RRTSearchInfo;
using ::vendor::amlogic::hardware::tvserver::V1_0::TvHidlParcel;
using ::vendor::amlogic::hardware::tvserver::V1_0::Result;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::hardware::hidl_array;

using namespace android;

class DroidTvServer : public ITvServer, public TvServiceNotify {
public:
    DroidTvServer();
    virtual ~DroidTvServer();

    Return<void> disconnect() override;

    Return<void> lock() override;

    Return<void> unlock() override;

    Return<int32_t> processCmd(int32_t type, int32_t size) override;

    Return<int32_t> startTv() override;
    Return<int32_t> stopTv() override;
    Return<int32_t> switchInputSrc(int32_t inputSrc) override;
    Return<int32_t> getInputSrcConnectStatus(int32_t inputSrc) override;
    Return<int32_t> getCurrentInputSrc() override;
    Return<int32_t> getHdmiAvHotplugStatus() override;
    Return<void> getSupportInputDevices(getSupportInputDevices_cb _hidl_cb) override;
    Return<int32_t> getHdmiPorts(int32_t inputSrc) override;

    Return<void> getCurSignalInfo(getCurSignalInfo_cb _hidl_cb) override;
    Return<int32_t> setMiscCfg(const hidl_string& key, const hidl_string& val) override;
    Return<void> getMiscCfg(const hidl_string& key, const hidl_string& def, getMiscCfg_cb _hidl_cb) override;

    Return<int32_t> isDviSIgnal() override;
    Return<int32_t> isVgaTimingInHdmi() override;
    Return<int32_t> setHdmiEdidVersion(int32_t port_id, int32_t ver) override;
    Return<int32_t> getHdmiEdidVersion(int32_t port_id) override;
    Return<int32_t> saveHdmiEdidVersion(int32_t port_id, int32_t ver) override;
    Return<int32_t> setHdmiColorRangeMode(int32_t range_mode) override;
    Return<int32_t> getHdmiColorRangeMode() override;
    Return<int32_t> handleGPIO(const hidl_string& key, int32_t is_out, int32_t edge) override;
    Return<int32_t> setSourceInput(int32_t inputSrc) override;
    Return<int32_t> setSourceInputExt(int32_t inputSrc, int32_t vInputSrc) override;
    Return<int32_t> setBlackoutEnable(int32_t satus, int32_t is_save) override;
    Return<int32_t> getBlackoutEnable() override;
    Return<void> getATVMinMaxFreq(getATVMinMaxFreq_cb _hidl_cb) override;
    Return<int32_t> setAmAudioPreMute(int32_t mute) override;
    Return<int32_t> setDvbTextCoding(const hidl_string& coding) override;
    Return<int32_t> operateDeviceForScan(int32_t type) override;
    Return<int32_t> atvAutoScan(int32_t videoStd, int32_t audioStd, int32_t searchType, int32_t procMode) override;
    Return<int32_t> atvMunualScan(int32_t startFreq, int32_t endFreq, int32_t videoStd, int32_t audioStd) override;
    Return<int32_t> Scan(const hidl_string& feparas, const hidl_string& scanparas) override;
    Return<int32_t> dtvScan(int32_t mode, int32_t scan_mode, int32_t beginFreq, int32_t endFreq, int32_t para1, int32_t para2) override;
    Return<int32_t> pauseScan() override;
    Return<int32_t> resumeScan() override;
    Return<int32_t> dtvStopScan() override;
    Return<int32_t> tvSetFrontEnd(const hidl_string& feparas, int32_t force) override;
    Return<int32_t> dtvGetSignalStrength() override;
    Return<int32_t> tvSetFrontendParms(int32_t feType, int32_t freq, int32_t vStd, int32_t aStd, int32_t vfmt, int32_t soundsys, int32_t p1, int32_t p2) override;
    Return<int32_t> sendPlayCmd(int32_t cmd, const hidl_string& id, const hidl_string& param) override;
    Return<int32_t> getCurrentSourceInput() override;
    Return<int32_t> getCurrentVirtualSourceInput() override;
    Return<int32_t> dtvSetAudioChannleMod(int32_t audioChannelIdx) override;
    Return<void> dtvGetVideoFormatInfo(dtvGetVideoFormatInfo_cb _hidl_cb) override;
    Return<void> dtvGetScanFreqListMode(int32_t mode, dtvGetScanFreqListMode_cb _hidl_cb) override;
    Return<int32_t> atvdtvGetScanStatus() override;
    Return<int32_t> SSMInitDevice() override;
    Return<int32_t> FactoryCleanAllTableForProgram() override;
    Return<void> getTvSupportCountries(getTvSupportCountries_cb _hidl_cb) override;
    Return<void> getTvDefaultCountry(getTvDefaultCountry_cb _hidl_cb) override;
    Return<void> getTvCountryName(const hidl_string& country_code, getTvCountryName_cb _hidl_cb) override;
    Return<void> getTvSearchMode(const hidl_string& country_code, getTvSearchMode_cb _hidl_cb) override;
    Return<bool> getTvDtvSupport(const hidl_string& country_code) override;
    Return<void> getTvDtvSystem(const hidl_string& country_code, getTvDtvSystem_cb _hidl_cb) override;
    Return<bool> getTvAtvSupport(const hidl_string& country_code) override;
    Return<void> getTvAtvColorSystem(const hidl_string& country_code, getTvAtvColorSystem_cb _hidl_cb) override;
    Return<void> getTvAtvSoundSystem(const hidl_string& country_code, getTvAtvSoundSystem_cb _hidl_cb) override;
    Return<void> getTvAtvMinMaxFreq(const hidl_string& country_code, getTvAtvMinMaxFreq_cb _hidl_cb) override;
    Return<bool> getTvAtvStepScan(const hidl_string& country_code) override;
    Return<void> setTvCountry(const hidl_string& country) override;
    Return<void> setCurrentLanguage(const hidl_string& lang) override;
    Return<int32_t> setAudioOutmode(int32_t mode) override;
    Return<int32_t> getAudioOutmode() override;
    Return<int32_t> getAudioStreamOutmode() override;
    Return<int32_t> getAtvAutoScanMode() override;
    Return<int32_t> vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag) override;
    Return<int32_t> DtvSetAudioAD(int32_t enable, int32_t audio_pid, int32_t audio_format) override;
    Return<int32_t> DtvSwitchAudioTrack(int32_t prog_id, int32_t audio_track_id) override;
    Return<int32_t> DtvSwitchAudioTrack3(int32_t audio_pid, int32_t audio_format,int32_t audio_param) override;
    Return<int32_t> setWssStatus(int32_t status) override;
    Return<int32_t> sendRecordingCmd(int32_t cmd, const hidl_string& id, const hidl_string& param) override;
    Return<void> searchRrtInfo(int32_t rating_region_id, int32_t dimension_id, int32_t value_id, int32_t program_id, searchRrtInfo_cb _hidl_cb) override;
    Return<int32_t> updateRRT(int32_t freq, int32_t moudle, int32_t mode) override;
    Return<int32_t> updateEAS(int32_t freq, int32_t moudle, int32_t mode) override;
    Return<int32_t> setDeviceIdForCec(int32_t DeviceId) override;
    Return<int32_t> getTvRunStatus(void) override;
    Return<int32_t> getTvAction(void) override;
    Return<int32_t> setLcdEnable(int32_t enable) override;
    Return<void> readMacAddress(readMacAddress_cb _hidl_cb) override;
    Return<int32_t> saveMacAddress(const hidl_array<int32_t, 6>& data_buf) override;
    Return<int32_t> getIwattRegs() override;
    Return<int32_t> setSameSourceEnable(int32_t isEnable) override;
    Return<int32_t> setPreviewWindow(int32_t x1, int32_t y1, int32_t x2, int32_t y2) override;
    Return<int32_t> setPreviewWindowMode(int32_t enable) override;

    Return<void> setCallback(const sp<ITvServerCallback>& callback, ConnectType type) override;

    virtual void onEvent(const TvHidlParcel &hidlParcel);

private:

    const char* getConnectTypeStr(ConnectType type);

    // Handle the case where the callback registered for the given type dies
    void handleServiceDeath(uint32_t type);

    bool mDebug = false;
    bool mListenerStarted = false;
    DroidTvServiceIntf *mTvServiceIntf;
    std::map<uint32_t, sp<ITvServerCallback>> mClients;

    mutable Mutex mLock;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<DroidTvServer> server);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<DroidTvServer> droidTvServer;
    };

    sp<DeathRecipient> mDeathRecipient;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tvserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
#endif /* ANDROID_DROIDLOGIC_HDMI_CEC_V1_0_H */
