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

using namespace ::android::hardware::radio::V1_0;

/*
 * Test IRadio.sendSms() for the response returned.
 */
TEST_F(RadioHidlTest, sendSms) {
    serial = GetRandomSerialNumber();
    GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio->sendSms(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait(300));
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
        EXPECT_EQ(0, radioRsp->sendSmsResult.errorCode);
    }
}

/*
 * Test IRadio.sendSMSExpectMore() for the response returned.
 */
TEST_F(RadioHidlTest, sendSMSExpectMore) {
    serial = GetRandomSerialNumber();
    GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio->sendSMSExpectMore(serial, msg);

    // TODO(shuoq): add more test for this API when inserted sim card is
    // considered

    EXPECT_EQ(std::cv_status::no_timeout, wait(300));
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.acknowledgeLastIncomingGsmSms() for the response returned.
 */
TEST_F(RadioHidlTest, acknowledgeLastIncomingGsmSms) {
    serial = GetRandomSerialNumber();
    bool success = true;

    radio->acknowledgeLastIncomingGsmSms(serial, success,
                                         SmsAcknowledgeFailCause::MEMORY_CAPACITY_EXCEEDED);

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
 * Test IRadio.acknowledgeIncomingGsmSmsWithPdu() for the response returned.
 */
TEST_F(RadioHidlTest, acknowledgeIncomingGsmSmsWithPdu) {
    serial = GetRandomSerialNumber();
    bool success = true;
    std::string ackPdu = "";

    radio->acknowledgeIncomingGsmSmsWithPdu(serial, success, ackPdu);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        // TODO(shuoq): Will add error check when we know the expected error from QC
    }
}

/*
 * Test IRadio.sendCdmaSms() for the response returned.
 */
TEST_F(RadioHidlTest, sendCdmaSms) {
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    android::hardware::radio::V1_0::CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
        (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    radio->sendCdmaSms(serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.acknowledgeLastIncomingCdmaSms() for the response returned.
 */
TEST_F(RadioHidlTest, acknowledgeLastIncomingCdmaSms) {
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAck
    CdmaSmsAck cdmaSmsAck;
    cdmaSmsAck.errorClass = CdmaSmsErrorClass::NO_ERROR;
    cdmaSmsAck.smsCauseCode = 1;

    radio->acknowledgeLastIncomingCdmaSms(serial, cdmaSmsAck);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::NO_SMS_TO_ACK},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.sendImsSms() for the response returned.
 */
TEST_F(RadioHidlTest, sendImsSms) {
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
        (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    // Creata an ImsSmsMessage
    ImsSmsMessage msg;
    msg.tech = RadioTechnologyFamily::THREE_GPP2;
    msg.retry = false;
    msg.messageRef = 0;
    msg.cdmaMessage = (std::vector<CdmaSmsMessage>){cdmaSmsMessage};
    msg.gsmMessage = (std::vector<GsmSmsMessage>){};

    radio->sendImsSms(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::INVALID_ARGUMENTS},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getSmscAddress() for the response returned.
 */
TEST_F(RadioHidlTest, getSmscAddress) {
    serial = GetRandomSerialNumber();

    radio->getSmscAddress(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setSmscAddress() for the response returned.
 */
TEST_F(RadioHidlTest, setSmscAddress) {
    serial = GetRandomSerialNumber();
    hidl_string address = hidl_string("smscAddress");

    radio->setSmscAddress(serial, address);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_SMS_FORMAT, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.writeSmsToSim() for the response returned.
 */
TEST_F(RadioHidlTest, writeSmsToSim) {
    serial = GetRandomSerialNumber();
    SmsWriteArgs smsWriteArgs;
    smsWriteArgs.status = SmsWriteArgsStatus::REC_UNREAD;
    smsWriteArgs.smsc = "";
    smsWriteArgs.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio->writeSmsToSim(serial, smsWriteArgs);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::ENCODING_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_SMSC_ADDRESS, RadioError::MODEM_ERR, RadioError::NETWORK_NOT_READY,
             RadioError::NO_RESOURCES, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.deleteSmsOnSim() for the response returned.
 */
TEST_F(RadioHidlTest, deleteSmsOnSim) {
    serial = GetRandomSerialNumber();
    int index = 1;

    radio->deleteSmsOnSim(serial, index);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::ENCODING_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::NO_SUCH_ENTRY, RadioError::MODEM_ERR,
             RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.writeSmsToRuim() for the response returned.
 */
TEST_F(RadioHidlTest, writeSmsToRuim) {
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
        (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    // Create a CdmaSmsWriteArgs
    CdmaSmsWriteArgs cdmaSmsWriteArgs;
    cdmaSmsWriteArgs.status = CdmaSmsWriteArgsStatus::REC_UNREAD;
    cdmaSmsWriteArgs.message = cdmaSmsMessage;

    radio->writeSmsToRuim(serial, cdmaSmsWriteArgs);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_SMS_FORMAT,
             RadioError::INVALID_SMSC_ADDRESS, RadioError::INVALID_STATE, RadioError::MODEM_ERR,
             RadioError::NO_SUCH_ENTRY, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.deleteSmsOnRuim() for the response returned.
 */
TEST_F(RadioHidlTest, deleteSmsOnRuim) {
    serial = GetRandomSerialNumber();
    int index = 1;

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
        (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    // Create a CdmaSmsWriteArgs
    CdmaSmsWriteArgs cdmaSmsWriteArgs;
    cdmaSmsWriteArgs.status = CdmaSmsWriteArgsStatus::REC_UNREAD;
    cdmaSmsWriteArgs.message = cdmaSmsMessage;

    radio->deleteSmsOnRuim(serial, index);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_MODEM_STATE,
             RadioError::MODEM_ERR, RadioError::NO_SUCH_ENTRY, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.reportSmsMemoryStatus() for the response returned.
 */
TEST_F(RadioHidlTest, reportSmsMemoryStatus) {
    serial = GetRandomSerialNumber();
    bool available = true;

    radio->reportSmsMemoryStatus(serial, available);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE,
                                      RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
}
