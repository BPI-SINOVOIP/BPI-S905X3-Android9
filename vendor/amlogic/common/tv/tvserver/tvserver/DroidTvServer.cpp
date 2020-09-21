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

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "HIDLServer"

#include <inttypes.h>
#include <string>
#include <binder/Parcel.h>
#include <cutils/properties.h>

#include "CTvLog.h"
#include "DroidTvServer.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace tvserver {
namespace V1_0 {
namespace implementation {

DroidTvServer::DroidTvServer() : mDeathRecipient(new DeathRecipient(this)) {
    mTvServiceIntf = new DroidTvServiceIntf();
    mTvServiceIntf->setListener(this);
}

DroidTvServer::~DroidTvServer() {
    delete mTvServiceIntf;
}

void DroidTvServer::onEvent(const TvHidlParcel &hidlParcel) {
    AutoMutex _l(mLock);
    int clientSize = mClients.size();

    LOGI("onEvent event:%d, client size:%d", hidlParcel.msgType, clientSize);

#if 0
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator == nullptr) {
        LOGE("can not get ashmem service");
        return;
    }

    size_t size = p.dataSize();
    android::hardware::hidl_memory hidlMemory;
    auto res = ashmemAllocator->allocate(size, [&](bool success, const android::hardware::hidl_memory& memory) {
                if (!success) {
                    LOGE("ashmem allocate size:%d fail", size);
                }
                hidlMemory = memory;
            });

    if (!res.isOk()) {
        LOGE("ashmem allocate result fail");
        return;
    }

    sp<IMemory> memory = android::hardware::mapMemory(hidlMemory);
    void* data = memory->getPointer();
    memory->update();
    // update memory however you wish after calling update and before calling commit
    memcpy(data, p.data(), size);
    memory->commit();
#endif
    for (auto it = mClients.begin(); it != mClients.end();) {
         if (it->second == nullptr) {
             it = mClients.erase(it);
             continue;
         }
         auto ret = (it->second)->notifyCallback(hidlParcel);
         if (!ret.isOk() && ret.isDeadObject()) {
             it = mClients.erase(it);
         } else {
             ++it;
         }
     }

}

Return<void> DroidTvServer::lock() {
    return Void();
}

Return<void> DroidTvServer::unlock() {
    return Void();
}

Return<void> DroidTvServer::disconnect() {
    return Void();
}

Return<int32_t> DroidTvServer::processCmd(int32_t type, int32_t size) {
    #if 0
    Parcel p;

    sp<IMemory> memory = android::hardware::mapMemory(parcelMem);
    void* data = memory->getPointer();
    //memory->update();
    // update memory however you wish after calling update and before calling commit
    p.write(data, size);
    int ret = mTvServiceIntf->processCmd(p);
    //memory->commit();
    return ret;
    #endif
    return 0;
}

Return<int32_t> DroidTvServer::startTv() {
    return mTvServiceIntf->startTv();
}

Return<int32_t> DroidTvServer::stopTv() {
    return mTvServiceIntf->stopTv();
}

Return<int32_t> DroidTvServer::switchInputSrc(int32_t inputSrc) {
    return mTvServiceIntf->switchInputSrc(inputSrc);
}

Return<int32_t> DroidTvServer::getInputSrcConnectStatus(int32_t inputSrc) {
    return mTvServiceIntf->getInputSrcConnectStatus(inputSrc);
}

Return<int32_t> DroidTvServer::getCurrentInputSrc() {
    return mTvServiceIntf->getCurrentInputSrc();
}

Return<int32_t> DroidTvServer::getHdmiAvHotplugStatus() {
    return mTvServiceIntf->getHdmiAvHotplugStatus();
}

Return<void> DroidTvServer::getSupportInputDevices(getSupportInputDevices_cb _hidl_cb) {
    std::string devices = mTvServiceIntf->getSupportInputDevices();
    _hidl_cb(0/*don't use*/, devices);
    return Void();
}

Return<void> DroidTvServer::getCurSignalInfo(getCurSignalInfo_cb _hidl_cb) {
    SignalInfo info;
    mTvServiceIntf->getCurSignalInfo(info.fmt, info.transFmt, info.status, info.frameRate);

    _hidl_cb(info);
    return Void();
}

Return<int32_t> DroidTvServer::setMiscCfg(const hidl_string& key, const hidl_string& val) {
    return mTvServiceIntf->setMiscCfg(key, val);
}

Return<void> DroidTvServer::getMiscCfg(const hidl_string& key, const hidl_string& def, getMiscCfg_cb _hidl_cb) {
    std::string cfg = mTvServiceIntf->getMiscCfg(key, def);

    LOGI("%s, key:%s def:%s, cfg:%s", __FUNCTION__, key.c_str(), def.c_str(), cfg.c_str());
    _hidl_cb(cfg);
    return Void();
}

Return<int32_t> DroidTvServer::getHdmiPorts(int32_t inputSrc) {
    return mTvServiceIntf->getHdmiPorts(inputSrc);
}

Return<int32_t> DroidTvServer::isDviSIgnal() {
     return mTvServiceIntf->isDviSIgnal();
}

Return<int32_t> DroidTvServer::isVgaTimingInHdmi() {
    return mTvServiceIntf->isVgaTimingInHdmi();
}

Return<int32_t> DroidTvServer::setHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServiceIntf->setHdmiEdidVersion(port_id, ver);
}

Return<int32_t> DroidTvServer::getHdmiEdidVersion(int32_t port_id) {
    return mTvServiceIntf->getHdmiEdidVersion(port_id);
}

Return<int32_t> DroidTvServer::saveHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServiceIntf->saveHdmiEdidVersion(port_id, ver);
}

Return<int32_t> DroidTvServer::setHdmiColorRangeMode(int32_t range_mode) {
    return mTvServiceIntf->setHdmiColorRangeMode(range_mode);
}

Return<int32_t> DroidTvServer::getHdmiColorRangeMode() {
    return mTvServiceIntf->getHdmiColorRangeMode();
}

Return<int32_t> DroidTvServer::handleGPIO(const hidl_string& key, int32_t is_out, int32_t edge) {
    return mTvServiceIntf->handleGPIO(key, is_out, edge);
}
Return<int32_t> DroidTvServer::setSourceInput(int32_t inputSrc) {
    return mTvServiceIntf->setSourceInput(inputSrc);
}

Return<int32_t> DroidTvServer::setSourceInputExt(int32_t inputSrc, int32_t vInputSrc) {
    return mTvServiceIntf->setSourceInput(inputSrc, vInputSrc);
}

Return<int32_t> DroidTvServer::setBlackoutEnable(int32_t status, int32_t is_save) {
    return mTvServiceIntf->setBlackoutEnable(status, is_save);
}

Return<int32_t> DroidTvServer::getBlackoutEnable() {
    return mTvServiceIntf->getBlackoutEnable();
}

Return<void> DroidTvServer::getATVMinMaxFreq(getATVMinMaxFreq_cb _hidl_cb) {
    int32_t scanMinFreq = 0;
    int32_t scanMaxFreq = 0;
    int32_t ret = mTvServiceIntf->getATVMinMaxFreq(scanMinFreq, scanMaxFreq);
    _hidl_cb(ret, scanMinFreq, scanMaxFreq);

    return Void();
}

Return<int32_t> DroidTvServer::setAmAudioPreMute(int32_t mute) {
    return mTvServiceIntf->setAmAudioPreMute(mute);
}

Return<int32_t> DroidTvServer::setDvbTextCoding(const hidl_string& coding) {
    return mTvServiceIntf->setDvbTextCoding(coding);
}

Return<int32_t> DroidTvServer::operateDeviceForScan(int32_t type) {
    return mTvServiceIntf->operateDeviceForScan(type);
}

Return<int32_t> DroidTvServer::atvAutoScan(int32_t videoStd, int32_t audioStd, int32_t searchType, int32_t procMode) {
    return mTvServiceIntf->atvAutoScan(videoStd, audioStd, searchType, procMode);
}

Return<int32_t> DroidTvServer::atvMunualScan(int32_t startFreq, int32_t endFreq, int32_t videoStd, int32_t audioStd) {
    return mTvServiceIntf->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
}

Return<int32_t> DroidTvServer::Scan(const hidl_string& feparas, const hidl_string& scanparas) {
    return mTvServiceIntf->Scan(feparas, scanparas);
}

Return<int32_t> DroidTvServer::dtvScan(int32_t mode, int32_t scan_mode, int32_t beginFreq, int32_t endFreq, int32_t para1, int32_t para2) {
    return mTvServiceIntf->dtvScan(mode, scan_mode, beginFreq, endFreq, para1, para2);
}

Return<int32_t> DroidTvServer::pauseScan() {
    return mTvServiceIntf->pauseScan();
}

Return<int32_t> DroidTvServer::resumeScan() {
    return mTvServiceIntf->resumeScan();
}

Return<int32_t> DroidTvServer::dtvStopScan() {
    return mTvServiceIntf->dtvStopScan();
}

Return<int32_t> DroidTvServer::dtvGetSignalStrength() {
    return mTvServiceIntf->dtvGetSignalStrength();
}

Return<int32_t> DroidTvServer::tvSetFrontEnd(const hidl_string& feparas, int32_t force) {
    return mTvServiceIntf->tvSetFrontEnd(feparas, force);
}

Return<int32_t> DroidTvServer::tvSetFrontendParms(int32_t feType, int32_t freq, int32_t vStd, int32_t aStd, int32_t vfmt, int32_t soundsys, int32_t p1, int32_t p2) {
    return mTvServiceIntf->tvSetFrontendParms(feType, freq, vStd, aStd, vfmt, soundsys, p1, p2);
}

Return<int32_t> DroidTvServer::sendPlayCmd(int32_t cmd, const hidl_string& id, const hidl_string& param) {
    return mTvServiceIntf->sendPlayCmd(cmd, id, param);
}

Return<int32_t> DroidTvServer::getCurrentSourceInput() {
    return mTvServiceIntf->getCurrentSourceInput();
}

Return<int32_t> DroidTvServer::getCurrentVirtualSourceInput() {
    return mTvServiceIntf->getCurrentVirtualSourceInput();
}

Return<int32_t> DroidTvServer::dtvSetAudioChannleMod(int32_t audioChannelIdx) {
    return mTvServiceIntf->dtvSetAudioChannleMod(audioChannelIdx);
}

Return<void> DroidTvServer::dtvGetVideoFormatInfo(dtvGetVideoFormatInfo_cb _hidl_cb) {
    FormatInfo info;
    mTvServiceIntf->dtvGetVideoFormatInfo(info.width, info.height, info.fps, info.interlace);

    _hidl_cb(info);
    return Void();
}

Return<void> DroidTvServer::dtvGetScanFreqListMode(int32_t mode, dtvGetScanFreqListMode_cb _hidl_cb) {
    std::vector<FreqList> freqlist;
    mTvServiceIntf->dtvGetScanFreqListMode(mode, freqlist);

    _hidl_cb(freqlist);
    return Void();
}

Return<int32_t> DroidTvServer::atvdtvGetScanStatus() {
    return mTvServiceIntf->atvdtvGetScanStatus();
}

Return<int32_t> DroidTvServer::SSMInitDevice() {
    return mTvServiceIntf->SSMInitDevice();
}

Return<int32_t> DroidTvServer::FactoryCleanAllTableForProgram() {
    return mTvServiceIntf->FactoryCleanAllTableForProgram();
}

Return<void> DroidTvServer::getTvSupportCountries(getTvSupportCountries_cb _hidl_cb) {
    std::string Countries = mTvServiceIntf->getTvSupportCountries();
    _hidl_cb(Countries);
    return Void();
}

Return<void> DroidTvServer::getTvDefaultCountry(getTvDefaultCountry_cb _hidl_cb) {
    std::string Country = mTvServiceIntf->getTvDefaultCountry();
    _hidl_cb(Country);
    return Void();
}

Return<void> DroidTvServer::getTvCountryName(const hidl_string& country_code, getTvCountryName_cb _hidl_cb) {
    std::string str = mTvServiceIntf->getTvCountryName(country_code);
    _hidl_cb(str);
    return Void();
}

Return<void> DroidTvServer::getTvSearchMode(const hidl_string& country_code, getTvSearchMode_cb _hidl_cb) {
    std::string str = mTvServiceIntf->getTvSearchMode(country_code);
    _hidl_cb(str);
    return Void();
}

Return<bool> DroidTvServer::getTvDtvSupport(const hidl_string& country_code) {
    return mTvServiceIntf->getTvDtvSupport(country_code);
}

Return<void> DroidTvServer::getTvDtvSystem(const hidl_string& country_code, getTvDtvSystem_cb _hidl_cb) {
    std::string str = mTvServiceIntf->getTvDtvSystem(country_code);
    _hidl_cb(str);
    return Void();
}

Return<bool> DroidTvServer::getTvAtvSupport(const hidl_string& country_code) {
    return mTvServiceIntf->getTvAtvSupport(country_code);
}

Return<void> DroidTvServer::getTvAtvColorSystem(const hidl_string& country_code, getTvAtvColorSystem_cb _hidl_cb) {
    std::string str = mTvServiceIntf->getTvAtvColorSystem(country_code);
    _hidl_cb(str);
    return Void();
}

Return<void> DroidTvServer::getTvAtvSoundSystem(const hidl_string& country_code, getTvAtvSoundSystem_cb _hidl_cb) {
    std::string str = mTvServiceIntf->getTvAtvSoundSystem(country_code);
    _hidl_cb(str);
    return Void();
}

Return<void> DroidTvServer::getTvAtvMinMaxFreq(const hidl_string& country_code, getTvAtvMinMaxFreq_cb _hidl_cb) {
    std::string str = mTvServiceIntf->getTvAtvMinMaxFreq(country_code);
    _hidl_cb(str);
    return Void();
}

Return<bool> DroidTvServer::getTvAtvStepScan(const hidl_string& country_code) {
    return mTvServiceIntf->getTvAtvStepScan(country_code);
}

Return<void> DroidTvServer::setTvCountry(const hidl_string& country) {
    mTvServiceIntf->setTvCountry(country);
    return Void();
}

Return<void> DroidTvServer::setCurrentLanguage(const hidl_string& lang) {
    mTvServiceIntf->setCurrentLanguage(lang);
    return Void();
}

Return<int32_t> DroidTvServer::setAudioOutmode(int32_t mode) {
    return mTvServiceIntf->setAudioOutmode(mode);
}

Return<int32_t> DroidTvServer::getAudioOutmode() {
    return mTvServiceIntf->getAudioOutmode();
}

Return<int32_t> DroidTvServer::getAudioStreamOutmode() {
    return mTvServiceIntf->getAudioStreamOutmode();
}

Return<int32_t> DroidTvServer::getAtvAutoScanMode() {
    return mTvServiceIntf->getAtvAutoScanMode();
}

Return<int32_t> DroidTvServer::vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag) {
    return mTvServiceIntf->vdinUpdateForPQ(gameStatus, pcStatus, autoSwitchFlag);
}
Return<int32_t> DroidTvServer::DtvSetAudioAD(int32_t enable, int32_t audio_pid, int32_t audio_format) {

    return mTvServiceIntf->DtvSetAudioAD(enable, audio_pid, audio_format);
}

Return<int32_t> DroidTvServer::DtvSwitchAudioTrack(int32_t prog_id, int32_t audio_track_id) {

    return mTvServiceIntf->DtvSwitchAudioTrack(prog_id, audio_track_id);
}

Return<int32_t> DroidTvServer::DtvSwitchAudioTrack3(int32_t audio_pid, int32_t audio_format,int32_t audio_param) {

    return mTvServiceIntf->DtvSwitchAudioTrack(audio_pid, audio_format,audio_param);
}

Return<int32_t> DroidTvServer::setWssStatus(int32_t status) {
    return mTvServiceIntf->setWssStatus(status);
}

Return<int32_t> DroidTvServer::sendRecordingCmd(int32_t cmd, const hidl_string& id, const hidl_string& param) {
    return mTvServiceIntf->sendRecordingCmd(cmd, id, param);
}

Return<void> DroidTvServer::searchRrtInfo(int32_t rating_region_id, int32_t dimension_id, int32_t value_id, int32_t program_id, searchRrtInfo_cb _hidl_cb) {
    RRTSearchInfo info;
    rrt_select_info_t tempInfo = mTvServiceIntf->searchRrtInfo(rating_region_id, dimension_id, value_id, program_id);
    info.RatingRegionName = tempInfo.rating_region_name;
    info.DimensionsName = tempInfo.dimensions_name;
    info.RatingValueText = tempInfo.rating_value_text;
    info.status = tempInfo.status;
    _hidl_cb(info);
    return Void();
}

Return<int32_t> DroidTvServer::updateRRT(int32_t freq, int32_t moudle, int32_t mode) {
    return mTvServiceIntf->updateRRT(freq, moudle, mode);
}

Return<int32_t> DroidTvServer::updateEAS(int32_t freq, int32_t moudle, int32_t mode) {
    return mTvServiceIntf->updateEAS(freq, moudle, mode);
}

Return<int32_t> DroidTvServer::setDeviceIdForCec(int32_t DeviceId) {
    return mTvServiceIntf->setDeviceIdForCec(DeviceId);
}

Return<int32_t> DroidTvServer::getTvRunStatus(void) {
    return mTvServiceIntf->getTvRunStatus();
}

Return<int32_t> DroidTvServer::getTvAction(void) {
    return mTvServiceIntf->getTvAction();
}


Return<int32_t> DroidTvServer::setLcdEnable(int32_t enable) {
    return mTvServiceIntf->setLcdEnable(enable);
}

Return<void> DroidTvServer::readMacAddress(readMacAddress_cb _hidl_cb)  {
    unsigned char value[6] = {0};
    int ret = mTvServiceIntf->readMacAddress(value);
    if (ret >= 0) {
        int i;
        hidl_array<int32_t, 6> result;
        for (i=0;i<6;i++) {
            result[i] = value[i];
        }
        _hidl_cb(Result::OK, result);
    }

    return Void();
}

Return<int32_t> DroidTvServer::saveMacAddress(const hidl_array<int32_t, 6>& data_buf) {
    unsigned char tempBuf[6] = {0};
    int i = 0;
    for (i=0;i<6;i++) {
        tempBuf[i] = data_buf[i];
    }
    return mTvServiceIntf->saveMacAddress(tempBuf);
}

Return<int32_t> DroidTvServer::getIwattRegs() {
    return mTvServiceIntf->getIwattRegs();
}

Return<int32_t> DroidTvServer::setSameSourceEnable(int32_t isEnable) {
    int ret = -1;
    if (isEnable != 0) {
        ret = mTvServiceIntf->setSameSourceEnable(true);
    } else {
        ret = mTvServiceIntf->setSameSourceEnable(false);
    }

    return ret;
}

Return<int32_t> DroidTvServer::setPreviewWindow(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    return mTvServiceIntf->setPreviewWindow(x1, y1, x2, y2);
}

Return<int32_t> DroidTvServer::setPreviewWindowMode(int32_t enable) {
    return mTvServiceIntf->setPreviewWindowMode(enable == 1 ? true : false);
}


Return<void> DroidTvServer::setCallback(const sp<ITvServerCallback>& callback, ConnectType type) {
    AutoMutex _l(mLock);
    if ((int)type > (int)ConnectType::TYPE_TOTAL - 1) {
        LOGE("%s don't support type:%d", __FUNCTION__, (int)type);
        return Void();
    }

    if (callback != nullptr) {
        int cookie = -1;
        int clientSize = mClients.size();
        for (int i = 0; i < clientSize; i++) {
            if (mClients[i] == nullptr) {
                LOGI("%s, client index:%d had died, this id give the new client", __FUNCTION__, i);
                cookie = i;
                mClients[i] = callback;
                break;
            }
        }

        if (cookie < 0) {
            cookie = clientSize;
            mClients[clientSize] = callback;
        }
        Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, cookie);
        bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
        if (!linkSuccess) {
            LOGW("Couldn't link death recipient for type: %s, client: %d", getConnectTypeStr(type), cookie);
        }
        LOGI("%s client type:%s, client size:%d, cookie:%d", __FUNCTION__, getConnectTypeStr(type), (int)mClients.size(), cookie);
   }

    if (!mListenerStarted) {
        mTvServiceIntf->startListener();
        mListenerStarted= true;
    }

    return Void();
}

const char* DroidTvServer::getConnectTypeStr(ConnectType type) {
    switch (type) {
        case ConnectType::TYPE_HAL:
            return "HAL";
        case ConnectType::TYPE_EXTEND:
            return "EXTEND";
        default:
            return "unknown type";
    }
}

void DroidTvServer::handleServiceDeath(uint32_t cookie) {
    LOGI("tvserver daemon client:%d died", cookie);
    AutoMutex _l(mLock);
    if (mClients[cookie] != nullptr) {
        mClients[cookie]->unlinkToDeath(mDeathRecipient);
        mClients[cookie].clear();
        mClients[cookie] = nullptr;
    }
    LOGI("%s, client size:%d", __FUNCTION__,(int)mClients.size());
}

DroidTvServer::DeathRecipient::DeathRecipient(sp<DroidTvServer> server)
        : droidTvServer(server) {}

void DroidTvServer::DeathRecipient::serviceDied(
        uint64_t cookie,
        const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    LOGE("%s, droid tvserver daemon a client died cookie:%d", __FUNCTION__, (int)cookie);

    uint32_t client = static_cast<uint32_t>(cookie);
    droidTvServer->handleServiceDeath(client);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tvserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
