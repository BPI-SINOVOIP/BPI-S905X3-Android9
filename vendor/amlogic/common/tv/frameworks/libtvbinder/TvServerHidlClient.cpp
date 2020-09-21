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
 *  @date     2018/1/12
 *  @par function description:
 *  - 1 droidlogic tv hwbinder client
 */

#define LOG_TAG "TvServerHidlClient"
#include <utils/Log.h>

#include "include/TvServerHidlClient.h"

namespace android {

Mutex TvServerHidlClient::mLock;

// establish binder interface to tv service
sp<ITvServer> TvServerHidlClient::getTvService()
{
    Mutex::Autolock _l(mLock);

#if 1//PLATFORM_SDK_VERSION >= 26
    sp<ITvServer> tvservice = ITvServer::tryGetService();
    while (tvservice == nullptr) {
         usleep(200*1000);//sleep 200ms
         tvservice = ITvServer::tryGetService();
         ALOGE("tryGet tvserver daemon Service");
    };
    mDeathRecipient = new TvServerDaemonDeathRecipient(this);
    Return<bool> linked = tvservice->linkToDeath(mDeathRecipient, /*cookie*/ 0);
    if (!linked.isOk()) {
        ALOGE("Transaction error in linking to tvserver daemon service death: %s", linked.description().c_str());
    } else if (!linked) {
        ALOGE("Unable to link to tvserver daemon service death notifications");
    } else {
        ALOGI("Link to tvserver daemon service death notification successful");
    }

#else
    Mutex::Autolock _l(mLock);
    if (mTvService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("tvservice"));
            if (binder != 0)
                break;
            ALOGW("TvService not published, waiting...");
            usleep(500000); // 0.5 s
        } while (true);
        if (mDeathNotifier == NULL) {
            mDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(mDeathNotifier);
        mTvService = interface_cast<ITvService>(binder);
    }
    ALOGE_IF(mTvService == 0, "no TvService!?");
    return mTvService;
#endif

    return tvservice;
}

TvServerHidlClient::TvServerHidlClient(tv_connect_type_t type): mType(type)
{
    mTvServer = getTvService();
    if (mTvServer != nullptr) {
        mTvServerHidlCallback = new TvServerHidlCallback(this);
        mTvServer->setCallback(mTvServerHidlCallback, static_cast<ConnectType>(type));
    }
}

TvServerHidlClient::~TvServerHidlClient()
{
    disconnect();
}

sp<TvServerHidlClient> TvServerHidlClient::connect(tv_connect_type_t type)
{
    return new TvServerHidlClient(type);
}

void TvServerHidlClient::reconnect()
{
    ALOGI("tvserver client type:%d reconnect", mType);
    mTvServer.clear();
    //reconnect to server
    mTvServer = getTvService();
    if (mTvServer != nullptr)
        mTvServer->setCallback(mTvServerHidlCallback, static_cast<ConnectType>(mType));
}

void TvServerHidlClient::disconnect()
{
    ALOGD("disconnect");
}

/*
status_t TvServerHidlClient::processCmd(const Parcel &p, Parcel *r __unused)
{
    int cmd = p.readInt32();

    ALOGD("processCmd cmd=%d", cmd);
    return 0;
#if 0
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator == nullptr) {
        ALOGE("can not get ashmem service");
        return -1;
    }

    size_t size = p.dataSize();
    hidl_memory hidlMemory;
    auto res = ashmemAllocator->allocate(size, [&](bool success, const hidl_memory& memory) {
                if (!success) {
                    ALOGE("ashmem allocate size:%d fail", size);
                }
                hidlMemory = memory;
            });

    if (!res.isOk()) {
        ALOGE("ashmem allocate result fail");
        return -1;
    }

    sp<IMemory> memory = hardware::mapMemory(hidlMemory);
    void* data = memory->getPointer();
    memory->update();
    // update memory however you wish after calling update and before calling commit
    memcpy(data, p.data(), size);
    memory->commit();
    Return<int32_t> ret = mTvServer->processCmd(hidlMemory, (int)size);
    if (!ret.isOk()) {
        ALOGE("Failed to processCmd");
    }
    return ret;
#endif
}*/

void TvServerHidlClient::setListener(const sp<TvListener> &listener)
{
    mListener = listener;
}

int TvServerHidlClient::startTv() {
    return mTvServer->startTv();
}

int TvServerHidlClient::stopTv() {
    return mTvServer->stopTv();
}

int TvServerHidlClient::switchInputSrc(int32_t inputSrc) {
    return mTvServer->switchInputSrc(inputSrc);
}

int TvServerHidlClient::getInputSrcConnectStatus(int32_t inputSrc) {
    return mTvServer->getInputSrcConnectStatus(inputSrc);
}

int TvServerHidlClient::getCurrentInputSrc() {
    return mTvServer->getCurrentInputSrc();
}

int TvServerHidlClient::getHdmiAvHotplugStatus() {
    return mTvServer->getHdmiAvHotplugStatus();
}

std::string TvServerHidlClient::getSupportInputDevices() {
    int ret;
    std::string tvDevices;
    mTvServer->getSupportInputDevices([&](int32_t result, const ::android::hardware::hidl_string& devices) {
        ret = result;
        tvDevices = devices;
    });
    return tvDevices;
}

int TvServerHidlClient::getHdmiPorts(int32_t inputSrc) {
    return mTvServer->getHdmiPorts(inputSrc);
}

SignalInfo TvServerHidlClient::getCurSignalInfo() {
    SignalInfo signalInfo;
    mTvServer->getCurSignalInfo([&](const SignalInfo& info) {
        signalInfo.fmt = info.fmt;
        signalInfo.transFmt = info.transFmt;
        signalInfo.status = info.status;
        signalInfo.frameRate = info.frameRate;
    });
    return signalInfo;
}

int TvServerHidlClient::setMiscCfg(const std::string& key, const std::string& val) {
    return mTvServer->setMiscCfg(key, val);
}

std::string TvServerHidlClient::getMiscCfg(const std::string& key, const std::string& def) {
    std::string miscCfg;
    mTvServer->getMiscCfg(key, def, [&](const std::string& cfg) {
        miscCfg = cfg;
    });

    return miscCfg;
}

int TvServerHidlClient::setHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServer->setHdmiEdidVersion(port_id, ver);
}

int TvServerHidlClient::getHdmiEdidVersion(int32_t port_id) {
    return mTvServer->getHdmiEdidVersion(port_id);
}

int TvServerHidlClient::saveHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServer->saveHdmiEdidVersion(port_id, ver);
}

int TvServerHidlClient::setHdmiColorRangeMode(int32_t range_mode) {
    return mTvServer->setHdmiColorRangeMode(range_mode);
}

int TvServerHidlClient::getHdmiColorRangeMode() {
    return mTvServer->getHdmiColorRangeMode();
}

int TvServerHidlClient::handleGPIO(const std::string& key, int32_t is_out, int32_t edge) {
    return mTvServer->handleGPIO(key, is_out, edge);
}

int TvServerHidlClient::vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag) {
    return mTvServer->vdinUpdateForPQ(gameStatus, pcStatus, autoSwitchFlag);
}

int TvServerHidlClient::setWssStatus(int status) {
    return mTvServer->setWssStatus(status);
}

int TvServerHidlClient::setDeviceIdForCec(int DeviceId) {
    return mTvServer->setDeviceIdForCec(DeviceId);
}

int TvServerHidlClient::getTvRunStatus(void) {
    return mTvServer->getTvRunStatus();
}

int TvServerHidlClient::setLcdEnable(int32_t enable) {
    return mTvServer->setLcdEnable(enable);
}

int TvServerHidlClient::readMacAddress(char *value) {
    hidl_array<int32_t, 6> result;
    mTvServer->readMacAddress([&result](const Result &ret, const hidl_array<int32_t, 6> v) {
        if (Result::OK == ret) {
            for (int i=0;i<6;i++) {
                result[i] = v[i];
            }
        }
    });

    for (int j=0;j<6;j++) {
        value[j] = result[j];
    }

    return 0;
}

int TvServerHidlClient::saveMacAddress(const char *data_buf) {
    int value[6] = {0};
    int i = 0;
    for (i=0;i<6;i++) {
        value[i] = data_buf[i];
    }

    return mTvServer->saveMacAddress(value);
}

int TvServerHidlClient::setSameSourceEnable(int isEnable) {
    return mTvServer->setSameSourceEnable(isEnable);
}

std::string TvServerHidlClient::getTvSupportCountries() {
    std::string tvCountries;
    mTvServer->getTvSupportCountries([&](const ::android::hardware::hidl_string& Countries) {
        tvCountries = Countries;
    });
    return tvCountries;
}

std::string TvServerHidlClient::getTvDefaultCountry() {
    std::string Country;
    mTvServer->getTvDefaultCountry([&](const ::android::hardware::hidl_string& CountryName){
        Country = CountryName;
    });
    return Country;
}

std::string TvServerHidlClient::getTvCountryName(const std::string& country_code) {
    std::string Country;
    mTvServer->getTvCountryName(country_code, [&](const ::android::hardware::hidl_string& CountryName){
        Country = CountryName;
    });
    return Country;
}

std::string TvServerHidlClient::getTvSearchMode(const std::string& country_code) {
    std::string mode;
    mTvServer->getTvSearchMode(country_code, [&](const ::android::hardware::hidl_string& strMode){
        mode = strMode;
    });
    return mode;
}

bool TvServerHidlClient::getTvDtvSupport(const std::string& country_code) {
    return mTvServer->getTvDtvSupport(country_code);
}

std::string TvServerHidlClient::getTvDtvSystem(const std::string& country_code) {
    std::string strSystem;
    mTvServer->getTvDtvSystem(country_code, [&](const ::android::hardware::hidl_string& str){
        strSystem = str;
    });
    return strSystem;
}

bool TvServerHidlClient::getTvAtvSupport(const std::string& country_code) {
    return mTvServer->getTvAtvSupport(country_code);
}

std::string TvServerHidlClient::getTvAtvColorSystem(const std::string& country_code) {
    std::string ColorSystem;
    mTvServer->getTvAtvColorSystem(country_code, [&](const ::android::hardware::hidl_string& TmpColorSystem){
        ColorSystem = TmpColorSystem;
    });
    return ColorSystem;
}
std::string TvServerHidlClient::getTvAtvSoundSystem(const std::string& country_code) {
    std::string SoundSystem;
    mTvServer->getTvAtvSoundSystem(country_code, [&](const ::android::hardware::hidl_string& TmpSoundSystem){
        SoundSystem = TmpSoundSystem;
    });
    return SoundSystem;
}

std::string TvServerHidlClient::getTvAtvMinMaxFreq(const std::string& country_code) {
    std::string MinMaxFreq;
    mTvServer->getTvAtvMinMaxFreq(country_code, [&](const ::android::hardware::hidl_string& TmpMinMaxFreq){
        MinMaxFreq = TmpMinMaxFreq;
    });
    return MinMaxFreq;
}

bool TvServerHidlClient::getTvAtvStepScan(const std::string& country_code) {
    return mTvServer->getTvAtvStepScan(country_code);
}

void TvServerHidlClient::setTvCountry(const std::string& country) {
    mTvServer->setTvCountry(country);
}

void TvServerHidlClient::setCurrentLanguage(const std::string& lang) {
    mTvServer->setCurrentLanguage(lang);
}

int TvServerHidlClient::getTvAction(void) {
    return mTvServer->getTvAction();
}

int TvServerHidlClient::getCurrentSourceInput() {
    return mTvServer->getCurrentSourceInput();
}

int TvServerHidlClient::getCurrentVirtualSourceInput() {
    return mTvServer->getCurrentVirtualSourceInput();
}

int TvServerHidlClient::setSourceInput(int inputSrc) {
    return mTvServer->setSourceInput(inputSrc);
}

int TvServerHidlClient::setSourceInputExt(int inputSrc, int vInputSrc) {
    return mTvServer->setSourceInputExt(inputSrc, vInputSrc);
}

int TvServerHidlClient::isDviSIgnal() {
     return mTvServer->isDviSIgnal();
}

int TvServerHidlClient::isVgaTimingInHdmi() {
    return mTvServer->isVgaTimingInHdmi();
}

int TvServerHidlClient::setAudioOutmode(int mode) {
    return mTvServer->setAudioOutmode(mode);
}

int TvServerHidlClient::getAudioOutmode() {
    return mTvServer->getAudioOutmode();
}

int TvServerHidlClient::getAudioStreamOutmode() {
    return mTvServer->getAudioStreamOutmode();
}

int TvServerHidlClient::getAtvAutoScanMode() {
    return mTvServer->getAtvAutoScanMode();
}

int TvServerHidlClient::setAmAudioPreMute(int mute) {
    return mTvServer->setAmAudioPreMute(mute);
}

int TvServerHidlClient::SSMInitDevice() {
    return mTvServer->SSMInitDevice();
}

int TvServerHidlClient::dtvScan(int mode, int scan_mode, int beginFreq, int endFreq, int para1, int para2) {
    return mTvServer->dtvScan(mode, scan_mode, beginFreq, endFreq, para1, para2);
}

int TvServerHidlClient::atvAutoScan(int videoStd, int audioStd, int searchType, int procMode) {
    return mTvServer->atvAutoScan(videoStd, audioStd, searchType, procMode);
}

int TvServerHidlClient::atvManualScan(int startFreq, int endFreq, int videoStd, int audioStd) {
    return mTvServer->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
}

int TvServerHidlClient::pauseScan() {
    return mTvServer->pauseScan();
}

int TvServerHidlClient::resumeScan() {
    return mTvServer->resumeScan();
}

int TvServerHidlClient::operateDeviceForScan(int type) {
    return mTvServer->operateDeviceForScan(type);
}

int TvServerHidlClient::atvdtvGetScanStatus() {
    return mTvServer->atvdtvGetScanStatus();
}

int TvServerHidlClient::setDvbTextCoding(const std::string& coding) {
    return mTvServer->setDvbTextCoding(coding);
}

int TvServerHidlClient::setBlackoutEnable(int status, int is_save) {
    return mTvServer->setBlackoutEnable(status, is_save);
}

int TvServerHidlClient::getBlackoutEnable() {
    return mTvServer->getBlackoutEnable();
}

int TvServerHidlClient::getATVMinMaxFreq(int scanMinFreq, int scanMaxFreq) {
    int ret =-1;
    mTvServer->getATVMinMaxFreq([&](int retT, int scanMinFreqT, int scanMaxFreqT){
        scanMinFreq = scanMinFreqT;
        scanMaxFreq = scanMaxFreqT;
        ret = retT;
    });
    return ret;
}

std::vector<FreqList> TvServerHidlClient::dtvGetScanFreqListMode(int mode) {
    std::vector<FreqList> freqlist;
    mTvServer->dtvGetScanFreqListMode(mode, [&](const hidl_vec<FreqList>& freqlistT){
        freqlist = freqlistT;
    });
    return freqlist;
}

int TvServerHidlClient::updateRRT(int freq, int moudle, int mode) {
    return mTvServer->updateRRT(freq, moudle, mode);
}

RRTSearchInfo TvServerHidlClient::searchRrtInfo(int rating_region_id, int dimension_id, int value_id, int program_id) {
    RRTSearchInfo info;
    mTvServer->searchRrtInfo(rating_region_id, dimension_id, value_id, program_id, [&](const RRTSearchInfo searchInfo){
        info.RatingRegionName = searchInfo.RatingRegionName;
        info.DimensionsName   = searchInfo.DimensionsName;
        info.RatingValueText  = searchInfo.RatingValueText;
        info.status           = searchInfo.status;
    });
    return info;
}

int TvServerHidlClient::dtvStopScan() {
    return mTvServer->dtvStopScan();
}

int TvServerHidlClient::dtvGetSignalStrength() {
    return mTvServer->dtvGetSignalStrength();
}

int TvServerHidlClient::dtvSetAudioChannleMod(int audioChannelIdx) {
    return mTvServer->dtvSetAudioChannleMod(audioChannelIdx);
}

int TvServerHidlClient::DtvSwitchAudioTrack3(int audio_pid, int audio_format, int audio_param) {
    return mTvServer->DtvSwitchAudioTrack3(audio_pid, audio_format,audio_param);
}

int TvServerHidlClient::DtvSwitchAudioTrack(int prog_id, int audio_track_id) {
    return mTvServer->DtvSwitchAudioTrack(prog_id, audio_track_id);
}

int TvServerHidlClient::DtvSetAudioAD(int enable, int audio_pid, int audio_format) {

    return mTvServer->DtvSetAudioAD(enable, audio_pid, audio_format);
}

FormatInfo TvServerHidlClient::dtvGetVideoFormatInfo() {
    FormatInfo info;
    mTvServer->dtvGetVideoFormatInfo([&](const FormatInfo formatInfo){
        info.width     = formatInfo.width;
        info.height    = formatInfo.height;
        info.fps       = formatInfo.fps;
        info.interlace = formatInfo.interlace;
    });
    return info;
}

int TvServerHidlClient::Scan(const std::string& feparas, const std::string& scanparas) {
    return mTvServer->Scan(feparas, scanparas);
}

int TvServerHidlClient::tvSetFrontEnd(const std::string& feparas, int force) {
    return mTvServer->tvSetFrontEnd(feparas, force);
}

int TvServerHidlClient::tvSetFrontendParms(int feType, int freq, int vStd, int aStd, int vfmt, int soundsys, int p1, int p2) {
    return mTvServer->tvSetFrontendParms(feType, freq, vStd, aStd, vfmt, soundsys, p1, p2);
}

int TvServerHidlClient::sendRecordingCmd(int cmd, const std::string& id, const std::string& param) {
    return mTvServer->sendRecordingCmd(cmd, id, param);
}

int TvServerHidlClient::sendPlayCmd(int32_t cmd, const std::string& id, const std::string& param) {
    return mTvServer->sendPlayCmd(cmd, id, param);
}

int TvServerHidlClient::getIwattRegs() {
    return mTvServer->getIwattRegs();
}

int TvServerHidlClient::FactoryCleanAllTableForProgram() {
    return mTvServer->FactoryCleanAllTableForProgram();
}

int TvServerHidlClient::setPreviewWindow(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    return mTvServer->setPreviewWindow(x1, y1, x2, y2);
}

int TvServerHidlClient::setPreviewWindowMode(int32_t enable) {
    return mTvServer->setPreviewWindowMode(enable);
}

// callback from tv service
Return<void> TvServerHidlClient::TvServerHidlCallback::notifyCallback(const TvHidlParcel& hidlParcel)
{
    ALOGI("notifyCallback event type:%d", hidlParcel.msgType);

#if 0
    Parcel p;

    sp<IMemory> memory = android::hardware::mapMemory(parcelMem);
    void* data = memory->getPointer();
    memory->update();
    // update memory however you wish after calling update and before calling commit
    p.setDataPosition(0);
    p.write(data, size);
    memory->commit();

#endif
    sp<TvListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = tvserverClient->mListener;
    }

    tv_parcel_t parcel;
    parcel.msgType = hidlParcel.msgType;
    for (int i = 0; i < hidlParcel.bodyInt.size(); i++) {
        parcel.bodyInt.push_back(hidlParcel.bodyInt[i]);
    }

    for (int j = 0; j < hidlParcel.bodyString.size(); j++) {
        parcel.bodyString.push_back(hidlParcel.bodyString[j]);
    }

    if (listener != NULL) {
        listener->notify(parcel);
    }
    return Void();
}

void TvServerHidlClient::TvServerDaemonDeathRecipient::serviceDied(uint64_t cookie __unused,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who __unused)
{
    ALOGE("tvserver daemon died.");
    Mutex::Autolock _l(mLock);

    usleep(200*1000);//sleep 200ms
    tvserverClient->reconnect();
}
}
