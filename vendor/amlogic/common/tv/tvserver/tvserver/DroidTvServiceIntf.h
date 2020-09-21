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
 *  @date     2018/1/16
 *  @par function description:
 *  - 1 droidlogic tvservice interface
 */

#ifndef ANDROID_DROID_TV_TVSERVICE_INTF_H
#define ANDROID_DROID_TV_TVSERVICE_INTF_H

//#include <include/Tv.h>
#include <utils/threads.h>
#include <utils/Vector.h>
#include <stdint.h>
#include <tv/CTv.h>

#include <vendor/amlogic/hardware/tvserver/1.0/ITvServer.h>

using namespace android;

using ::vendor::amlogic::hardware::tvserver::V1_0::TvHidlParcel;
using ::vendor::amlogic::hardware::tvserver::V1_0::FreqList;
using ::vendor::amlogic::hardware::tvserver::V1_0::RRTSearchInfo;

class TvServiceNotify : virtual public RefBase {
public:
    TvServiceNotify() {}
    virtual ~TvServiceNotify(){}
    virtual void onEvent(const TvHidlParcel &hidlParcel) = 0;
};

class DroidTvServiceIntf: public CTv::TvIObserver {
public:
    DroidTvServiceIntf();
    virtual ~DroidTvServiceIntf();
        int processCmd(const Parcel &p);
    void setListener(const sp<TvServiceNotify>& listener);
    virtual void onTvEvent(const CTvEv &ev);

    void startListener();
    int startTv();
    int stopTv();
    int switchInputSrc(int32_t inputSrc);
    int getInputSrcConnectStatus(int32_t inputSrc);
    int getCurrentInputSrc();
    int getHdmiAvHotplugStatus();
    std::string getSupportInputDevices();
    int getHdmiPorts(int32_t inputSrc);
    void getCurSignalInfo(int &fmt, int &transFmt, int &status, int &frameRate);
    int setMiscCfg(const std::string& key, const std::string& val);
    std::string getMiscCfg(const std::string& key, const std::string& def);

    int isDviSIgnal();
    int isVgaTimingInHdmi();
    int setHdmiEdidVersion(int32_t port_id, int32_t ver);
    int getHdmiEdidVersion(int32_t port_id);
    int saveHdmiEdidVersion(int32_t port_id, int32_t ver);
    int setHdmiColorRangeMode(int32_t range_mode);
    int getHdmiColorRangeMode();
    int handleGPIO(const std::string& key, int is_out, int edge);
    int setSourceInput(int32_t inputSrc);
    int setSourceInput(int32_t inputSrc, int32_t vInputSrc);
    int setBlackoutEnable(int32_t status, int32_t is_save);
    int getBlackoutEnable();
    int getATVMinMaxFreq(int32_t &scanMinFreq, int32_t &scanMaxFreq);
    int setAmAudioPreMute(int32_t mute);
    int setDvbTextCoding(const std::string& coding);
    int operateDeviceForScan(int32_t type);
    int atvAutoScan(int32_t videoStd, int32_t audioStd, int32_t searchType, int32_t procMode);
    int atvMunualScan(int32_t startFreq, int32_t endFreq, int32_t videoStd, int32_t audioStd);
    int Scan(const std::string& feparas, const std::string& scanparas);
    int dtvScan(int32_t mode, int32_t scan_mode, int32_t beginFreq, int32_t endFreq, int32_t para1, int32_t para2);
    int pauseScan();
    int resumeScan();
    int dtvStopScan();
    int dtvGetSignalStrength();
    int tvSetFrontEnd(const std::string& feparas, int32_t force);
    int tvSetFrontendParms(int32_t feType, int32_t freq, int32_t vStd, int32_t aStd, int32_t vfmt, int32_t soundsys, int32_t p1, int32_t p2);
    int sendPlayCmd(int32_t cmd, const std::string& id, const std::string& param);
    int getCurrentSourceInput();
    int getCurrentVirtualSourceInput();
    int dtvSetAudioChannleMod(int32_t audioChannelIdx);
    int dtvGetVideoFormatInfo(int &srcWidth, int &srcHeight, int &srcFps, int &srcInterlace);
    void dtvGetScanFreqListMode(int mode, std::vector<FreqList> &freqlist);
    int atvdtvGetScanStatus();
    int SSMInitDevice();
    int FactoryCleanAllTableForProgram();
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
    int setAudioOutmode(int32_t mode);
    int getAudioOutmode();
    int getAudioStreamOutmode();
    int getAtvAutoScanMode();
    int vdinUpdateForPQ(int32_t gameStatus, int32_t pcStatus, int32_t autoSwitchFlag);
    int DtvSetAudioAD(int32_t enable, int32_t audio_pid, int32_t audio_format);
    int DtvSwitchAudioTrack(int32_t prog_id, int32_t audio_track_id);
    int DtvSwitchAudioTrack(int32_t audio_pid, int32_t audio_format, int32_t audio_param);
    int setWssStatus(int status);
    int sendRecordingCmd(int32_t cmd, const std::string& id, const std::string& param);
    rrt_select_info_t searchRrtInfo(int rating_region_id, int dimension_id, int value_id, int program_id);
    int updateRRT(int freq, int moudle, int mode);
    int updateEAS(int freq, int moudle, int mode);
    int setDeviceIdForCec(int DeviceId);
    int getTvRunStatus(void);
    int getTvAction(void);
    int setLcdEnable(int enable);
    int readMacAddress(unsigned char *dataBuf);
    int saveMacAddress(unsigned char *dataBuf);
    int getIwattRegs();
    int setSameSourceEnable(bool isEnable);
    int setPreviewWindow(int x1, int y1, int x2, int y2);
    int setPreviewWindowMode(bool enable);

    virtual status_t dump(int fd, const Vector<String16>& args);

    //wp<Client> mpScannerClient;
    //wp<Client> mpSubClient;
    //Vector< wp<Client> > mClients;

private:
    bool mIsStartTv;
    CTv *mpTv;
    sp<TvServiceNotify> mNotifyListener;
};

#endif/*ANDROID_DROID_TV_TVSERVICE_INTF_H*/
