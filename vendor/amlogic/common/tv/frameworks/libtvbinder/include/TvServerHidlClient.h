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

#ifndef _ANDROID_TV_SERVER_HIDL_CLIENT_H_
#define _ANDROID_TV_SERVER_HIDL_CLIENT_H_

#include <utils/Timers.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <vendor/amlogic/hardware/tvserver/1.0/ITvServer.h>

namespace android {

using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServer;
using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServerCallback;
using ::vendor::amlogic::hardware::tvserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::tvserver::V1_0::SignalInfo;
using ::vendor::amlogic::hardware::tvserver::V1_0::TvHidlParcel;
using ::vendor::amlogic::hardware::tvserver::V1_0::Result;
using ::vendor::amlogic::hardware::tvserver::V1_0::FreqList;
using ::vendor::amlogic::hardware::tvserver::V1_0::FormatInfo;
using ::vendor::amlogic::hardware::tvserver::V1_0::RRTSearchInfo;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

typedef enum {
    CONNECT_TYPE_HAL            = 0,
    CONNECT_TYPE_EXTEND         = 1
} tv_connect_type_t;

typedef struct tv_parcel_s {
    int msgType;
    std::vector<int> bodyInt;
    std::vector<std::string> bodyString;
} tv_parcel_t;

class TvListener : virtual public RefBase {
public:
    virtual void notify(const tv_parcel_t &parcel) = 0;
};

class TvServerHidlClient : virtual public RefBase {
public:
    static sp<TvServerHidlClient> connect(tv_connect_type_t type);
    TvServerHidlClient(tv_connect_type_t type);
    ~TvServerHidlClient();

    void reconnect();
    void disconnect();
    //status_t processCmd(const Parcel &p, Parcel *r);
    void setListener(const sp<TvListener> &listener);

    int startTv();
    int stopTv();
    int switchInputSrc(int32_t inputSrc);
    int getInputSrcConnectStatus(int32_t inputSrc);
    int getCurrentInputSrc();
    int getHdmiAvHotplugStatus();
    std::string getSupportInputDevices();
    int getHdmiPorts(int32_t inputSrc);

    SignalInfo getCurSignalInfo();
    int setMiscCfg(const std::string& key, const std::string& val);
    std::string getMiscCfg(const std::string& key, const std::string& def);
    int setHdmiEdidVersion(int32_t port_id, int32_t ver);
    int getHdmiEdidVersion(int32_t port_id);
    int saveHdmiEdidVersion(int32_t port_id, int32_t ver);
    int setHdmiColorRangeMode(int32_t range_mode);
    int getHdmiColorRangeMode();
    int handleGPIO(const std::string& key, int32_t is_out, int32_t edge);
    int vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag);
    int setWssStatus(int status);
    int setDeviceIdForCec(int DeviceId);
    int getTvRunStatus(void);
    int setLcdEnable(int32_t enable);
    int readMacAddress(char *value);
    int saveMacAddress(const char *value);
    int setSameSourceEnable(int isEnable);
    std::string getTvSupportCountries();
    std::string getTvDefaultCountry();
    std::string getTvCountryName(const std::string& country_code);
    std::string getTvSearchMode(const std::string& country_code);
    bool getTvDtvSupport(const std::string& country_code);
    std::string getTvDtvSystem(const std::string& country_code);
    bool getTvAtvSupport(const std::string& country_code);
    std::string getTvAtvColorSystem(const std::string& country_code);
    std::string getTvAtvSoundSystem(const std::string& country_code);
    std::string getTvAtvMinMaxFreq(const std::string& country_code);
    bool getTvAtvStepScan(const std::string& country_code);
    void setTvCountry(const std::string& country);
    void setCurrentLanguage(const std::string& lang);
    int getTvAction(void);
    int getCurrentSourceInput();
    int getCurrentVirtualSourceInput();
    int setSourceInput(int inputSrc);
    int setSourceInputExt(int inputSrc, int vInputSrc);
    int isDviSIgnal();
    int isVgaTimingInHdmi();
    int setAudioOutmode(int mode);
    int getAudioOutmode();
    int getAudioStreamOutmode();
    int getAtvAutoScanMode();
    int setAmAudioPreMute(int mute);
    int SSMInitDevice();
    int dtvScan(int mode, int scan_mode, int beginFreq, int endFreq, int para1, int para2);
    int atvAutoScan(int videoStd, int audioStd, int searchType, int procMode);
    int atvManualScan(int startFreq, int endFreq, int videoStd, int audioStd);
    int pauseScan();
    int resumeScan();
    int operateDeviceForScan(int type);
    int atvdtvGetScanStatus();
    int setDvbTextCoding(const std::string& coding);
    int setBlackoutEnable(int status, int is_save);
    int getBlackoutEnable();
    int getATVMinMaxFreq(int scanMinFreq, int scanMaxFreq);
    std::vector<FreqList> dtvGetScanFreqListMode(int mode);
    int updateRRT(int freq, int moudle, int mode);
    RRTSearchInfo searchRrtInfo(int rating_region_id, int dimension_id, int value_id, int program_id);
    int dtvStopScan();
    int dtvGetSignalStrength();
    int dtvSetAudioChannleMod(int audioChannelIdx);
    int DtvSwitchAudioTrack3(int audio_pid, int audio_format, int audio_param);
    int DtvSwitchAudioTrack(int prog_id, int audio_track_id);
    int DtvSetAudioAD(int enable, int audio_pid, int audio_format);
    FormatInfo dtvGetVideoFormatInfo();
    int Scan(const std::string& feparas, const std::string& scanparas);
    int tvSetFrontEnd(const std::string& feparas, int force);
    int tvSetFrontendParms(int feType, int freq, int vStd, int aStd, int vfmt, int soundsys, int p1, int p2);
    int sendRecordingCmd(int cmd, const std::string& id, const std::string& param);
    int sendPlayCmd(int32_t cmd, const std::string& id, const std::string& param);
    int getIwattRegs();
    int FactoryCleanAllTableForProgram();
    int setPreviewWindow(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    int setPreviewWindowMode(int32_t enable);

private:
    class TvServerHidlCallback : public ITvServerCallback {
    public:
        TvServerHidlCallback(TvServerHidlClient *client): tvserverClient(client) {};
        Return<void> notifyCallback(const TvHidlParcel& parcel) override;

    private:
        TvServerHidlClient *tvserverClient;
    };

    struct TvServerDaemonDeathRecipient : public android::hardware::hidl_death_recipient  {
        TvServerDaemonDeathRecipient(TvServerHidlClient *client): tvserverClient(client) {};

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    private:
        TvServerHidlClient *tvserverClient;
    };
    sp<TvServerDaemonDeathRecipient> mDeathRecipient = nullptr;

    static Mutex mLock;
    tv_connect_type_t mType;
    // helper function to obtain tv service handle
    sp<ITvServer> getTvService();

    sp<TvListener> mListener;
    sp<ITvServer> mTvServer;
    sp<TvServerHidlCallback> mTvServerHidlCallback = nullptr;
};

}//namespace android

#endif/*_ANDROID_TV_SERVER_HIDL_CLIENT_H_*/
