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

#include <radio_hidl_hal_utils_v1_1.h>
#include <vector>

/*
 * Test IRadio.setSimCardPower() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, setSimCardPower_1_1) {
    /* Record the sim card state for the testing environment */
    CardState cardStateForTest = cardStatus.cardState;

    /* Test setSimCardPower power down */
    serial = GetRandomSerialNumber();
    radio_v1_1->setSimCardPower_1_1(serial, CardPowerState::POWER_DOWN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_1->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::INVALID_ARGUMENTS, RadioError::RADIO_NOT_AVAILABLE}));
    /* Wait some time for setting sim power down and then verify it */
    updateSimCardStatus();
    auto startTime = std::chrono::system_clock::now();
    while (cardStatus.cardState != CardState::ABSENT &&
           std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() - startTime)
                   .count() < 80) {
        /* Set 2 seconds as interval to check card status */
        sleep(2);
        updateSimCardStatus();
    }
    EXPECT_EQ(CardState::ABSENT, cardStatus.cardState);

    /* Test setSimCardPower power up */
    serial = GetRandomSerialNumber();
    radio_v1_1->setSimCardPower_1_1(serial, CardPowerState::POWER_UP);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_1->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::INVALID_ARGUMENTS, RadioError::RADIO_NOT_AVAILABLE}));

    /**
     * If the sim card status for the testing environment is PRESENT,
     * verify if sim status is reset back.
     */
    if (cardStateForTest == CardState::PRESENT) {
        /* Wait some time for resetting back to sim power on and then verify it */
        updateSimCardStatus();
        startTime = std::chrono::system_clock::now();
        while (cardStatus.cardState != CardState::PRESENT &&
               std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() -
                                                           startTime)
                       .count() < 80) {
            /* Set 2 seconds as interval to check card status */
            sleep(2);
            updateSimCardStatus();
        }
        EXPECT_EQ(CardState::PRESENT, cardStatus.cardState);
    }
}

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, startNetworkScan) {
    serial = GetRandomSerialNumber();

    NetworkScanRequest request;
    request.type = ScanType::ONE_SHOT;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;

    radio_v1_1->startNetworkScan(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ALOGI("startNetworkScan, rspInfo.error = %d\n", (int32_t)radioRsp_v1_1->rspInfo.error);
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_1->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::INVALID_ARGUMENTS,
             RadioError::SIM_ABSENT, RadioError::OPERATION_NOT_ALLOWED}));
    }
}

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, startNetworkScan_InvalidArgument) {
    serial = GetRandomSerialNumber();

    NetworkScanRequest request;
    request.type = ScanType::ONE_SHOT;
    request.interval = 60;

    radio_v1_1->startNetworkScan(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %d\n",
              (int32_t)radioRsp_v1_1->rspInfo.error);
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_1->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::SIM_ABSENT,
                                      RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.stopNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, stopNetworkScan) {
    serial = GetRandomSerialNumber();

    radio_v1_1->stopNetworkScan(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ALOGI("stopNetworkScan rspInfo.error = %d\n", (int32_t)radioRsp_v1_1->rspInfo.error);
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_1->rspInfo.error,
            {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setCarrierInfoForImsiEncryption() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, setCarrierInfoForImsiEncryption) {
    serial = GetRandomSerialNumber();
    ImsiEncryptionInfo imsiInfo;
    imsiInfo.mcc = "310";
    imsiInfo.mnc = "004";
    imsiInfo.carrierKey = (std::vector<uint8_t>){1, 2, 3, 4, 5, 6};
    imsiInfo.keyIdentifier = "Test";
    imsiInfo.expirationTime = 20180101;

    radio_v1_1->setCarrierInfoForImsiEncryption(serial, imsiInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_1->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startKeepalive() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, startKeepalive) {
    std::vector<KeepaliveRequest> requests = {
        {
            // Invalid IPv4 source address
            KeepaliveType::NATT_IPV4,
            {192, 168, 0 /*, 100*/},
            1234,
            {8, 8, 4, 4},
            4500,
            20000,
            0xBAD,
        },
        {
            // Invalid IPv4 destination address
            KeepaliveType::NATT_IPV4,
            {192, 168, 0, 100},
            1234,
            {8, 8, 4, 4, 1, 2, 3, 4},
            4500,
            20000,
            0xBAD,
        },
        {
            // Invalid Keepalive Type
            static_cast<KeepaliveType>(-1),
            {192, 168, 0, 100},
            1234,
            {8, 8, 4, 4},
            4500,
            20000,
            0xBAD,
        },
        {
            // Invalid IPv6 source address
            KeepaliveType::NATT_IPV6,
            {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xED,
             0xBE, 0xEF, 0xBD},
            1234,
            {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x88, 0x44},
            4500,
            20000,
            0xBAD,
        },
        {
            // Invalid IPv6 destination address
            KeepaliveType::NATT_IPV6,
            {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xED,
             0xBE, 0xEF},
            1234,
            {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x88,
             /*0x44*/},
            4500,
            20000,
            0xBAD,
        },
        {
            // Invalid Context ID (cid), this should survive the initial
            // range checking and fail in the modem data layer
            KeepaliveType::NATT_IPV4,
            {192, 168, 0, 100},
            1234,
            {8, 8, 4, 4},
            4500,
            20000,
            0xBAD,
        },
        {
            // Invalid Context ID (cid), this should survive the initial
            // range checking and fail in the modem data layer
            KeepaliveType::NATT_IPV6,
            {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xED,
             0xBE, 0xEF},
            1234,
            {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x88, 0x44},
            4500,
            20000,
            0xBAD,
        }};

    for (auto req = requests.begin(); req != requests.end(); req++) {
        serial = GetRandomSerialNumber();
        radio_v1_1->startKeepalive(serial, *req);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_1->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.stopKeepalive() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, stopKeepalive) {
    serial = GetRandomSerialNumber();

    radio_v1_1->stopKeepalive(serial, 0xBAD);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    ASSERT_TRUE(
        CheckAnyOfErrors(radioRsp_v1_1->rspInfo.error,
                         {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}
