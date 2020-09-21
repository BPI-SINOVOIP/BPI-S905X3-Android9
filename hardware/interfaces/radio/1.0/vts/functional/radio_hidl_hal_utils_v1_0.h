/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <android-base/logging.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/1.0/IRadio.h>
#include <android/hardware/radio/1.0/IRadioIndication.h>
#include <android/hardware/radio/1.0/IRadioResponse.h>
#include <android/hardware/radio/1.0/types.h>

#include "vts_test_util.h"

using namespace ::android::hardware::radio::V1_0;

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define TIMEOUT_PERIOD 75
#define RADIO_SERVICE_NAME "slot1"

class RadioHidlTest;
extern CardStatus cardStatus;

/* Callback class for radio response */
class RadioResponse : public IRadioResponse {
   protected:
    RadioHidlTest& parent;

   public:
    RadioResponseInfo rspInfo;
    hidl_string imsi;
    IccIoResult iccIoResult;
    int channelId;

    // Sms
    SendSmsResult sendSmsResult;
    hidl_string smscAddress;
    uint32_t writeSmsToSimIndex;
    uint32_t writeSmsToRuimIndex;

    RadioResponse(RadioHidlTest& parent);

    virtual ~RadioResponse() = default;

    Return<void> getIccCardStatusResponse(const RadioResponseInfo& info,
                                          const CardStatus& cardStatus);

    Return<void> supplyIccPinForAppResponse(const RadioResponseInfo& info,
                                            int32_t remainingRetries);

    Return<void> supplyIccPukForAppResponse(const RadioResponseInfo& info,
                                            int32_t remainingRetries);

    Return<void> supplyIccPin2ForAppResponse(const RadioResponseInfo& info,
                                             int32_t remainingRetries);

    Return<void> supplyIccPuk2ForAppResponse(const RadioResponseInfo& info,
                                             int32_t remainingRetries);

    Return<void> changeIccPinForAppResponse(const RadioResponseInfo& info,
                                            int32_t remainingRetries);

    Return<void> changeIccPin2ForAppResponse(const RadioResponseInfo& info,
                                             int32_t remainingRetries);

    Return<void> supplyNetworkDepersonalizationResponse(const RadioResponseInfo& info,
                                                        int32_t remainingRetries);

    Return<void> getCurrentCallsResponse(const RadioResponseInfo& info,
                                         const ::android::hardware::hidl_vec<Call>& calls);

    Return<void> dialResponse(const RadioResponseInfo& info);

    Return<void> getIMSIForAppResponse(const RadioResponseInfo& info,
                                       const ::android::hardware::hidl_string& imsi);

    Return<void> hangupConnectionResponse(const RadioResponseInfo& info);

    Return<void> hangupWaitingOrBackgroundResponse(const RadioResponseInfo& info);

    Return<void> hangupForegroundResumeBackgroundResponse(const RadioResponseInfo& info);

    Return<void> switchWaitingOrHoldingAndActiveResponse(const RadioResponseInfo& info);

    Return<void> conferenceResponse(const RadioResponseInfo& info);

    Return<void> rejectCallResponse(const RadioResponseInfo& info);

    Return<void> getLastCallFailCauseResponse(const RadioResponseInfo& info,
                                              const LastCallFailCauseInfo& failCauseInfo);

    Return<void> getSignalStrengthResponse(const RadioResponseInfo& info,
                                           const SignalStrength& sigStrength);

    Return<void> getVoiceRegistrationStateResponse(const RadioResponseInfo& info,
                                                   const VoiceRegStateResult& voiceRegResponse);

    Return<void> getDataRegistrationStateResponse(const RadioResponseInfo& info,
                                                  const DataRegStateResult& dataRegResponse);

    Return<void> getOperatorResponse(const RadioResponseInfo& info,
                                     const ::android::hardware::hidl_string& longName,
                                     const ::android::hardware::hidl_string& shortName,
                                     const ::android::hardware::hidl_string& numeric);

    Return<void> setRadioPowerResponse(const RadioResponseInfo& info);

    Return<void> sendDtmfResponse(const RadioResponseInfo& info);

    Return<void> sendSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> sendSMSExpectMoreResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> setupDataCallResponse(const RadioResponseInfo& info,
                                       const SetupDataCallResult& dcResponse);

    Return<void> iccIOForAppResponse(const RadioResponseInfo& info, const IccIoResult& iccIo);

    Return<void> sendUssdResponse(const RadioResponseInfo& info);

    Return<void> cancelPendingUssdResponse(const RadioResponseInfo& info);

    Return<void> getClirResponse(const RadioResponseInfo& info, int32_t n, int32_t m);

    Return<void> setClirResponse(const RadioResponseInfo& info);

    Return<void> getCallForwardStatusResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<CallForwardInfo>& call_forwardInfos);

    Return<void> setCallForwardResponse(const RadioResponseInfo& info);

    Return<void> getCallWaitingResponse(const RadioResponseInfo& info, bool enable,
                                        int32_t serviceClass);

    Return<void> setCallWaitingResponse(const RadioResponseInfo& info);

    Return<void> acknowledgeLastIncomingGsmSmsResponse(const RadioResponseInfo& info);

    Return<void> acceptCallResponse(const RadioResponseInfo& info);

    Return<void> deactivateDataCallResponse(const RadioResponseInfo& info);

    Return<void> getFacilityLockForAppResponse(const RadioResponseInfo& info, int32_t response);

    Return<void> setFacilityLockForAppResponse(const RadioResponseInfo& info, int32_t retry);

    Return<void> setBarringPasswordResponse(const RadioResponseInfo& info);

    Return<void> getNetworkSelectionModeResponse(const RadioResponseInfo& info, bool manual);

    Return<void> setNetworkSelectionModeAutomaticResponse(const RadioResponseInfo& info);

    Return<void> setNetworkSelectionModeManualResponse(const RadioResponseInfo& info);

    Return<void> getAvailableNetworksResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<OperatorInfo>& networkInfos);

    Return<void> startDtmfResponse(const RadioResponseInfo& info);

    Return<void> stopDtmfResponse(const RadioResponseInfo& info);

    Return<void> getBasebandVersionResponse(const RadioResponseInfo& info,
                                            const ::android::hardware::hidl_string& version);

    Return<void> separateConnectionResponse(const RadioResponseInfo& info);

    Return<void> setMuteResponse(const RadioResponseInfo& info);

    Return<void> getMuteResponse(const RadioResponseInfo& info, bool enable);

    Return<void> getClipResponse(const RadioResponseInfo& info, ClipStatus status);

    Return<void> getDataCallListResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<SetupDataCallResult>& dcResponse);

    Return<void> sendOemRilRequestRawResponse(const RadioResponseInfo& info,
                                              const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendOemRilRequestStringsResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);

    Return<void> setSuppServiceNotificationsResponse(const RadioResponseInfo& info);

    Return<void> writeSmsToSimResponse(const RadioResponseInfo& info, int32_t index);

    Return<void> deleteSmsOnSimResponse(const RadioResponseInfo& info);

    Return<void> setBandModeResponse(const RadioResponseInfo& info);

    Return<void> getAvailableBandModesResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<RadioBandMode>& bandModes);

    Return<void> sendEnvelopeResponse(const RadioResponseInfo& info,
                                      const ::android::hardware::hidl_string& commandResponse);

    Return<void> sendTerminalResponseToSimResponse(const RadioResponseInfo& info);

    Return<void> handleStkCallSetupRequestFromSimResponse(const RadioResponseInfo& info);

    Return<void> explicitCallTransferResponse(const RadioResponseInfo& info);

    Return<void> setPreferredNetworkTypeResponse(const RadioResponseInfo& info);

    Return<void> getPreferredNetworkTypeResponse(const RadioResponseInfo& info,
                                                 PreferredNetworkType nwType);

    Return<void> getNeighboringCidsResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_vec<NeighboringCell>& cells);

    Return<void> setLocationUpdatesResponse(const RadioResponseInfo& info);

    Return<void> setCdmaSubscriptionSourceResponse(const RadioResponseInfo& info);

    Return<void> setCdmaRoamingPreferenceResponse(const RadioResponseInfo& info);

    Return<void> getCdmaRoamingPreferenceResponse(const RadioResponseInfo& info,
                                                  CdmaRoamingType type);

    Return<void> setTTYModeResponse(const RadioResponseInfo& info);

    Return<void> getTTYModeResponse(const RadioResponseInfo& info, TtyMode mode);

    Return<void> setPreferredVoicePrivacyResponse(const RadioResponseInfo& info);

    Return<void> getPreferredVoicePrivacyResponse(const RadioResponseInfo& info, bool enable);

    Return<void> sendCDMAFeatureCodeResponse(const RadioResponseInfo& info);

    Return<void> sendBurstDtmfResponse(const RadioResponseInfo& info);

    Return<void> sendCdmaSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> acknowledgeLastIncomingCdmaSmsResponse(const RadioResponseInfo& info);

    Return<void> getGsmBroadcastConfigResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& configs);

    Return<void> setGsmBroadcastConfigResponse(const RadioResponseInfo& info);

    Return<void> setGsmBroadcastActivationResponse(const RadioResponseInfo& info);

    Return<void> getCdmaBroadcastConfigResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& configs);

    Return<void> setCdmaBroadcastConfigResponse(const RadioResponseInfo& info);

    Return<void> setCdmaBroadcastActivationResponse(const RadioResponseInfo& info);

    Return<void> getCDMASubscriptionResponse(const RadioResponseInfo& info,
                                             const ::android::hardware::hidl_string& mdn,
                                             const ::android::hardware::hidl_string& hSid,
                                             const ::android::hardware::hidl_string& hNid,
                                             const ::android::hardware::hidl_string& min,
                                             const ::android::hardware::hidl_string& prl);

    Return<void> writeSmsToRuimResponse(const RadioResponseInfo& info, uint32_t index);

    Return<void> deleteSmsOnRuimResponse(const RadioResponseInfo& info);

    Return<void> getDeviceIdentityResponse(const RadioResponseInfo& info,
                                           const ::android::hardware::hidl_string& imei,
                                           const ::android::hardware::hidl_string& imeisv,
                                           const ::android::hardware::hidl_string& esn,
                                           const ::android::hardware::hidl_string& meid);

    Return<void> exitEmergencyCallbackModeResponse(const RadioResponseInfo& info);

    Return<void> getSmscAddressResponse(const RadioResponseInfo& info,
                                        const ::android::hardware::hidl_string& smsc);

    Return<void> setSmscAddressResponse(const RadioResponseInfo& info);

    Return<void> reportSmsMemoryStatusResponse(const RadioResponseInfo& info);

    Return<void> reportStkServiceIsRunningResponse(const RadioResponseInfo& info);

    Return<void> getCdmaSubscriptionSourceResponse(const RadioResponseInfo& info,
                                                   CdmaSubscriptionSource source);

    Return<void> requestIsimAuthenticationResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_string& response);

    Return<void> acknowledgeIncomingGsmSmsWithPduResponse(const RadioResponseInfo& info);

    Return<void> sendEnvelopeWithStatusResponse(const RadioResponseInfo& info,
                                                const IccIoResult& iccIo);

    Return<void> getVoiceRadioTechnologyResponse(const RadioResponseInfo& info,
                                                 RadioTechnology rat);

    Return<void> getCellInfoListResponse(const RadioResponseInfo& info,
                                         const ::android::hardware::hidl_vec<CellInfo>& cellInfo);

    Return<void> setCellInfoListRateResponse(const RadioResponseInfo& info);

    Return<void> setInitialAttachApnResponse(const RadioResponseInfo& info);

    Return<void> getImsRegistrationStateResponse(const RadioResponseInfo& info, bool isRegistered,
                                                 RadioTechnologyFamily ratFamily);

    Return<void> sendImsSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> iccTransmitApduBasicChannelResponse(const RadioResponseInfo& info,
                                                     const IccIoResult& result);

    Return<void> iccOpenLogicalChannelResponse(
        const RadioResponseInfo& info, int32_t channelId,
        const ::android::hardware::hidl_vec<int8_t>& selectResponse);

    Return<void> iccCloseLogicalChannelResponse(const RadioResponseInfo& info);

    Return<void> iccTransmitApduLogicalChannelResponse(const RadioResponseInfo& info,
                                                       const IccIoResult& result);

    Return<void> nvReadItemResponse(const RadioResponseInfo& info,
                                    const ::android::hardware::hidl_string& result);

    Return<void> nvWriteItemResponse(const RadioResponseInfo& info);

    Return<void> nvWriteCdmaPrlResponse(const RadioResponseInfo& info);

    Return<void> nvResetConfigResponse(const RadioResponseInfo& info);

    Return<void> setUiccSubscriptionResponse(const RadioResponseInfo& info);

    Return<void> setDataAllowedResponse(const RadioResponseInfo& info);

    Return<void> getHardwareConfigResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_vec<HardwareConfig>& config);

    Return<void> requestIccSimAuthenticationResponse(const RadioResponseInfo& info,
                                                     const IccIoResult& result);

    Return<void> setDataProfileResponse(const RadioResponseInfo& info);

    Return<void> requestShutdownResponse(const RadioResponseInfo& info);

    Return<void> getRadioCapabilityResponse(const RadioResponseInfo& info,
                                            const RadioCapability& rc);

    Return<void> setRadioCapabilityResponse(const RadioResponseInfo& info,
                                            const RadioCapability& rc);

    Return<void> startLceServiceResponse(const RadioResponseInfo& info,
                                         const LceStatusInfo& statusInfo);

    Return<void> stopLceServiceResponse(const RadioResponseInfo& info,
                                        const LceStatusInfo& statusInfo);

    Return<void> pullLceDataResponse(const RadioResponseInfo& info, const LceDataInfo& lceInfo);

    Return<void> getModemActivityInfoResponse(const RadioResponseInfo& info,
                                              const ActivityStatsInfo& activityInfo);

    Return<void> setAllowedCarriersResponse(const RadioResponseInfo& info, int32_t numAllowed);

    Return<void> getAllowedCarriersResponse(const RadioResponseInfo& info, bool allAllowed,
                                            const CarrierRestrictions& carriers);

    Return<void> sendDeviceStateResponse(const RadioResponseInfo& info);

    Return<void> setIndicationFilterResponse(const RadioResponseInfo& info);

    Return<void> setSimCardPowerResponse(const RadioResponseInfo& info);

    Return<void> acknowledgeRequest(int32_t serial);
};

/* Callback class for radio indication */
class RadioIndication : public IRadioIndication {
   protected:
    RadioHidlTest& parent;

   public:
    RadioIndication(RadioHidlTest& parent);
    virtual ~RadioIndication() = default;

    Return<void> radioStateChanged(RadioIndicationType type, RadioState radioState);

    Return<void> callStateChanged(RadioIndicationType type);

    Return<void> networkStateChanged(RadioIndicationType type);

    Return<void> newSms(RadioIndicationType type,
                        const ::android::hardware::hidl_vec<uint8_t>& pdu);

    Return<void> newSmsStatusReport(RadioIndicationType type,
                                    const ::android::hardware::hidl_vec<uint8_t>& pdu);

    Return<void> newSmsOnSim(RadioIndicationType type, int32_t recordNumber);

    Return<void> onUssd(RadioIndicationType type, UssdModeType modeType,
                        const ::android::hardware::hidl_string& msg);

    Return<void> nitzTimeReceived(RadioIndicationType type,
                                  const ::android::hardware::hidl_string& nitzTime,
                                  uint64_t receivedTime);

    Return<void> currentSignalStrength(RadioIndicationType type,
                                       const SignalStrength& signalStrength);

    Return<void> dataCallListChanged(
        RadioIndicationType type, const ::android::hardware::hidl_vec<SetupDataCallResult>& dcList);

    Return<void> suppSvcNotify(RadioIndicationType type, const SuppSvcNotification& suppSvc);

    Return<void> stkSessionEnd(RadioIndicationType type);

    Return<void> stkProactiveCommand(RadioIndicationType type,
                                     const ::android::hardware::hidl_string& cmd);

    Return<void> stkEventNotify(RadioIndicationType type,
                                const ::android::hardware::hidl_string& cmd);

    Return<void> stkCallSetup(RadioIndicationType type, int64_t timeout);

    Return<void> simSmsStorageFull(RadioIndicationType type);

    Return<void> simRefresh(RadioIndicationType type, const SimRefreshResult& refreshResult);

    Return<void> callRing(RadioIndicationType type, bool isGsm, const CdmaSignalInfoRecord& record);

    Return<void> simStatusChanged(RadioIndicationType type);

    Return<void> cdmaNewSms(RadioIndicationType type, const CdmaSmsMessage& msg);

    Return<void> newBroadcastSms(RadioIndicationType type,
                                 const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> cdmaRuimSmsStorageFull(RadioIndicationType type);

    Return<void> restrictedStateChanged(RadioIndicationType type, PhoneRestrictedState state);

    Return<void> enterEmergencyCallbackMode(RadioIndicationType type);

    Return<void> cdmaCallWaiting(RadioIndicationType type,
                                 const CdmaCallWaiting& callWaitingRecord);

    Return<void> cdmaOtaProvisionStatus(RadioIndicationType type, CdmaOtaProvisionStatus status);

    Return<void> cdmaInfoRec(RadioIndicationType type, const CdmaInformationRecords& records);

    Return<void> indicateRingbackTone(RadioIndicationType type, bool start);

    Return<void> resendIncallMute(RadioIndicationType type);

    Return<void> cdmaSubscriptionSourceChanged(RadioIndicationType type,
                                               CdmaSubscriptionSource cdmaSource);

    Return<void> cdmaPrlChanged(RadioIndicationType type, int32_t version);

    Return<void> exitEmergencyCallbackMode(RadioIndicationType type);

    Return<void> rilConnected(RadioIndicationType type);

    Return<void> voiceRadioTechChanged(RadioIndicationType type, RadioTechnology rat);

    Return<void> cellInfoList(RadioIndicationType type,
                              const ::android::hardware::hidl_vec<CellInfo>& records);

    Return<void> imsNetworkStateChanged(RadioIndicationType type);

    Return<void> subscriptionStatusChanged(RadioIndicationType type, bool activate);

    Return<void> srvccStateNotify(RadioIndicationType type, SrvccState state);

    Return<void> hardwareConfigChanged(
        RadioIndicationType type, const ::android::hardware::hidl_vec<HardwareConfig>& configs);

    Return<void> radioCapabilityIndication(RadioIndicationType type, const RadioCapability& rc);

    Return<void> onSupplementaryServiceIndication(RadioIndicationType type,
                                                  const StkCcUnsolSsResult& ss);

    Return<void> stkCallControlAlphaNotify(RadioIndicationType type,
                                           const ::android::hardware::hidl_string& alpha);

    Return<void> lceData(RadioIndicationType type, const LceDataInfo& lce);

    Return<void> pcoData(RadioIndicationType type, const PcoDataInfo& pco);

    Return<void> modemReset(RadioIndicationType type,
                            const ::android::hardware::hidl_string& reason);
};

// Test environment for Radio HIDL HAL.
class RadioHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static RadioHidlEnvironment* Instance() {
        static RadioHidlEnvironment* instance = new RadioHidlEnvironment;
        return instance;
    }
    virtual void registerTestServices() override { registerTestService<IRadio>(); }

   private:
    RadioHidlEnvironment() {}
};

// The main test class for Radio HIDL.
class RadioHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   protected:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

    /* Serial number for radio request */
    int serial;

    /* Update Sim Card Status */
    void updateSimCardStatus();

   public:
    virtual void SetUp() override;

    /* Used as a mechanism to inform the test about data/event callback */
    void notify(int receivedSerial);

    /* Test code calls this function to wait for response */
    std::cv_status wait(int sec = TIMEOUT_PERIOD);

    sp<IRadio> radio;
    sp<RadioResponse> radioRsp;
    sp<RadioIndication> radioInd;
};
