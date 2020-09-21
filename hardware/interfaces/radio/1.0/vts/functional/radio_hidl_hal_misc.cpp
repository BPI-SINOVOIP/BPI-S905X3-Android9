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

#include <radio_hidl_hal_utils_v1_0.h>

/*
 * Test IRadio.getSignalStrength() for the response returned.
 */
TEST_F(RadioHidlTest, getSignalStrength) {
    serial = GetRandomSerialNumber();

    radio->getSignalStrength(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getVoiceRegistrationState() for the response returned.
 */
TEST_F(RadioHidlTest, getVoiceRegistrationState) {
    serial = GetRandomSerialNumber();

    radio->getVoiceRegistrationState(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getOperator() for the response returned.
 */
TEST_F(RadioHidlTest, getOperator) {
    serial = GetRandomSerialNumber();

    radio->getOperator(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setRadioPower() for the response returned.
 */
TEST_F(RadioHidlTest, setRadioPower) {
    serial = GetRandomSerialNumber();

    radio->setRadioPower(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getNetworkSelectionMode() for the response returned.
 */
TEST_F(RadioHidlTest, getNetworkSelectionMode) {
    serial = GetRandomSerialNumber();

    radio->getNetworkSelectionMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setNetworkSelectionModeAutomatic() for the response returned.
 */
TEST_F(RadioHidlTest, setNetworkSelectionModeAutomatic) {
    serial = GetRandomSerialNumber();

    radio->setNetworkSelectionModeAutomatic(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME, RadioError::OPERATION_NOT_ALLOWED},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setNetworkSelectionModeManual() for the response returned.
 */
TEST_F(RadioHidlTest, setNetworkSelectionModeManual) {
    serial = GetRandomSerialNumber();

    radio->setNetworkSelectionModeManual(serial, "123456");
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME,
                                      RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getAvailableNetworks() for the response returned.
 */
TEST_F(RadioHidlTest, getAvailableNetworks) {
    serial = GetRandomSerialNumber();

    radio->getAvailableNetworks(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait(300));
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    ASSERT_TRUE(radioRsp->rspInfo.type == RadioResponseType::SOLICITED ||
                radioRsp->rspInfo.type == RadioResponseType::SOLICITED_ACK_EXP);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::CANCELLED, RadioError::DEVICE_IN_USE,
                              RadioError::MODEM_ERR, RadioError::OPERATION_NOT_ALLOWED},
                             CHECK_GENERAL_ERROR));
  }
}

/*
 * Test IRadio.getBasebandVersion() for the response returned.
 */
TEST_F(RadioHidlTest, getBasebandVersion) {
    serial = GetRandomSerialNumber();

    radio->getBasebandVersion(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setBandMode() for the response returned.
 */
TEST_F(RadioHidlTest, setBandMode) {
    serial = GetRandomSerialNumber();

    radio->setBandMode(serial, RadioBandMode::BAND_MODE_USA);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getAvailableBandModes() for the response returned.
 */
TEST_F(RadioHidlTest, getAvailableBandModes) {
    serial = GetRandomSerialNumber();

    radio->getAvailableBandModes(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setPreferredNetworkType() for the response returned.
 */
TEST_F(RadioHidlTest, setPreferredNetworkType) {
    serial = GetRandomSerialNumber();

    radio->setPreferredNetworkType(serial, PreferredNetworkType::GSM_ONLY);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getPreferredNetworkType() for the response returned.
 */
TEST_F(RadioHidlTest, getPreferredNetworkType) {
    serial = GetRandomSerialNumber();

    radio->getPreferredNetworkType(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getNeighboringCids() for the response returned.
 */
TEST_F(RadioHidlTest, getNeighboringCids) {
    serial = GetRandomSerialNumber();

    radio->getNeighboringCids(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setLocationUpdates() for the response returned.
 */
TEST_F(RadioHidlTest, setLocationUpdates) {
    serial = GetRandomSerialNumber();

    radio->setLocationUpdates(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.setCdmaRoamingPreference() for the response returned.
 */
TEST_F(RadioHidlTest, setCdmaRoamingPreference) {
    serial = GetRandomSerialNumber();

    radio->setCdmaRoamingPreference(serial, CdmaRoamingType::HOME_NETWORK);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.getCdmaRoamingPreference() for the response returned.
 */
TEST_F(RadioHidlTest, getCdmaRoamingPreference) {
    serial = GetRandomSerialNumber();

    radio->getCdmaRoamingPreference(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::MODEM_ERR},
                             CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getTTYMode() for the response returned.
 */
TEST_F(RadioHidlTest, getTTYMode) {
    serial = GetRandomSerialNumber();

    radio->getTTYMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setTTYMode() for the response returned.
 */
TEST_F(RadioHidlTest, setTTYMode) {
    serial = GetRandomSerialNumber();

    radio->setTTYMode(serial, TtyMode::OFF);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setPreferredVoicePrivacy() for the response returned.
 */
TEST_F(RadioHidlTest, setPreferredVoicePrivacy) {
    serial = GetRandomSerialNumber();

    radio->setPreferredVoicePrivacy(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.getPreferredVoicePrivacy() for the response returned.
 */
TEST_F(RadioHidlTest, getPreferredVoicePrivacy) {
    serial = GetRandomSerialNumber();

    radio->getPreferredVoicePrivacy(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.getCDMASubscription() for the response returned.
 */
TEST_F(RadioHidlTest, getCDMASubscription) {
    serial = GetRandomSerialNumber();

    radio->getCDMASubscription(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.getDeviceIdentity() for the response returned.
 */
TEST_F(RadioHidlTest, getDeviceIdentity) {
    serial = GetRandomSerialNumber();

    radio->getDeviceIdentity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::EMPTY_RECORD}));
    }
}

/*
 * Test IRadio.exitEmergencyCallbackMode() for the response returned.
 */
TEST_F(RadioHidlTest, exitEmergencyCallbackMode) {
    serial = GetRandomSerialNumber();

    radio->exitEmergencyCallbackMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.getCdmaSubscriptionSource() for the response returned.
 */
TEST_F(RadioHidlTest, getCdmaSubscriptionSource) {
    serial = GetRandomSerialNumber();

    radio->getCdmaSubscriptionSource(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.setCdmaSubscriptionSource() for the response returned.
 */
TEST_F(RadioHidlTest, setCdmaSubscriptionSource) {
    serial = GetRandomSerialNumber();

    radio->setCdmaSubscriptionSource(serial, CdmaSubscriptionSource::RUIM_SIM);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::SUBSCRIPTION_NOT_AVAILABLE},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getVoiceRadioTechnology() for the response returned.
 */
TEST_F(RadioHidlTest, getVoiceRadioTechnology) {
    serial = GetRandomSerialNumber();

    radio->getVoiceRadioTechnology(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getCellInfoList() for the response returned.
 */
TEST_F(RadioHidlTest, getCellInfoList) {
    serial = GetRandomSerialNumber();

    radio->getCellInfoList(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::NO_NETWORK_FOUND},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setCellInfoListRate() for the response returned.
 */
TEST_F(RadioHidlTest, setCellInfoListRate) {
    serial = GetRandomSerialNumber();

    // TODO(sanketpadawe): RIL crashes with value of rate = 10
    radio->setCellInfoListRate(serial, 10);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.nvReadItem() for the response returned.
 */
TEST_F(RadioHidlTest, nvReadItem) {
    serial = GetRandomSerialNumber();

    radio->nvReadItem(serial, NvItem::LTE_BAND_ENABLE_25);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.nvWriteItem() for the response returned.
 */
TEST_F(RadioHidlTest, nvWriteItem) {
    serial = GetRandomSerialNumber();
    NvWriteItem item;
    memset(&item, 0, sizeof(item));
    item.value = hidl_string();

    radio->nvWriteItem(serial, item);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.nvWriteCdmaPrl() for the response returned.
 */
TEST_F(RadioHidlTest, nvWriteCdmaPrl) {
    serial = GetRandomSerialNumber();
    std::vector<uint8_t> prl = {1, 2, 3, 4, 5};

    radio->nvWriteCdmaPrl(serial, hidl_vec<uint8_t>(prl));
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.nvResetConfig() for the response returned.
 */
TEST_F(RadioHidlTest, nvResetConfig) {
    serial = GetRandomSerialNumber();

    radio->nvResetConfig(serial, ResetNvType::ERASE);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setUiccSubscription() for the response returned.
 */
TEST_F(RadioHidlTest, setUiccSubscription) {
    serial = GetRandomSerialNumber();
    SelectUiccSub item;
    memset(&item, 0, sizeof(item));

    radio->setUiccSubscription(serial, item);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                              RadioError::MODEM_ERR, RadioError::SUBSCRIPTION_NOT_SUPPORTED},
                             CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getHardwareConfig() for the response returned.
 */
TEST_F(RadioHidlTest, getHardwareConfig) {
    serial = GetRandomSerialNumber();

    radio->getHardwareConfig(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.requestShutdown() for the response returned.
 */
TEST_F(RadioHidlTest, requestShutdown) {
    serial = GetRandomSerialNumber();

    radio->requestShutdown(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getRadioCapability() for the response returned.
 */
TEST_F(RadioHidlTest, getRadioCapability) {
    serial = GetRandomSerialNumber();

    radio->getRadioCapability(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setRadioCapability() for the response returned.
 */
TEST_F(RadioHidlTest, setRadioCapability) {
    serial = GetRandomSerialNumber();
    RadioCapability rc;
    memset(&rc, 0, sizeof(rc));
    rc.logicalModemUuid = hidl_string();

    radio->setRadioCapability(serial, rc);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.startLceService() for the response returned.
 */
TEST_F(RadioHidlTest, startLceService) {
    serial = GetRandomSerialNumber();

    radio->startLceService(serial, 5, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INTERNAL_ERR, RadioError::LCE_NOT_SUPPORTED,
                                      RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.stopLceService() for the response returned.
 */
TEST_F(RadioHidlTest, stopLceService) {
    serial = GetRandomSerialNumber();

    radio->stopLceService(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::LCE_NOT_SUPPORTED,
                                      RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.pullLceData() for the response returned.
 */
TEST_F(RadioHidlTest, pullLceData) {
    serial = GetRandomSerialNumber();

    radio->pullLceData(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::RADIO_NOT_AVAILABLE},
            CHECK_OEM_ERROR));
    }
}

/*
 * Test IRadio.getModemActivityInfo() for the response returned.
 */
TEST_F(RadioHidlTest, getModemActivityInfo) {
    serial = GetRandomSerialNumber();

    radio->getModemActivityInfo(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setAllowedCarriers() for the response returned.
 */
TEST_F(RadioHidlTest, setAllowedCarriers) {
    serial = GetRandomSerialNumber();
    CarrierRestrictions carriers;
    memset(&carriers, 0, sizeof(carriers));
    carriers.allowedCarriers.resize(1);
    carriers.excludedCarriers.resize(0);
    carriers.allowedCarriers[0].mcc = hidl_string();
    carriers.allowedCarriers[0].mnc = hidl_string();
    carriers.allowedCarriers[0].matchType = CarrierMatchType::ALL;
    carriers.allowedCarriers[0].matchData = hidl_string();

    radio->setAllowedCarriers(serial, false, carriers);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }

    /* Setting to carrier restriction needs some time */
    updateSimCardStatus();
    auto startTime = std::chrono::system_clock::now();
    while (cardStatus.cardState != CardState::RESTRICTED &&
           std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() - startTime)
                   .count() < 10) {
        /* Set 2 seconds as interval to check card status */
        sleep(2);
        updateSimCardStatus();
    }
    EXPECT_EQ(CardState::RESTRICTED, cardStatus.cardState);
    sleep(10);

    /* Reset back to no carrier restriction */
    memset(&carriers, 0, sizeof(carriers));
    carriers.allowedCarriers.resize(0);
    carriers.excludedCarriers.resize(0);

    serial = GetRandomSerialNumber();
    radio->setAllowedCarriers(serial, true, carriers);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }

    /* Resetting back to no carrier restriction needs some time */
    updateSimCardStatus();
    startTime = std::chrono::system_clock::now();
    while (cardStatus.cardState == CardState::RESTRICTED &&
           std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() - startTime)
                   .count() < 10) {
        /* Set 2 seconds as interval to check card status */
        sleep(2);
        updateSimCardStatus();
    }
    EXPECT_NE(CardState::RESTRICTED, cardStatus.cardState);
    sleep(10);
}

/*
 * Test IRadio.getAllowedCarriers() for the response returned.
 */
TEST_F(RadioHidlTest, getAllowedCarriers) {
    serial = GetRandomSerialNumber();

    radio->getAllowedCarriers(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.sendDeviceState() for the response returned.
 */
TEST_F(RadioHidlTest, sendDeviceState) {
    serial = GetRandomSerialNumber();

    radio->sendDeviceState(serial, DeviceStateType::POWER_SAVE_MODE, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    std::cout << static_cast<int>(radioRsp->rspInfo.error) << std::endl;

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setIndicationFilter() for the response returned.
 */
TEST_F(RadioHidlTest, setIndicationFilter) {
    serial = GetRandomSerialNumber();

    radio->setIndicationFilter(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    std::cout << static_cast<int>(radioRsp->rspInfo.error) << std::endl;

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setSimCardPower() for the response returned.
 */
TEST_F(RadioHidlTest, setSimCardPower) {
    serial = GetRandomSerialNumber();

    radio->setSimCardPower(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}
