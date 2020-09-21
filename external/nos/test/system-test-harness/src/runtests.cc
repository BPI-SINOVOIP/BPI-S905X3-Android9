
#include <gtest/gtest.h>
#include <openssl/aes.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>

#include "nugget/app/protoapi/control.pb.h"
#include "nugget/app/protoapi/header.pb.h"
#include "nugget/app/protoapi/testing_api.pb.h"
#include "src/util.h"

#ifdef ANDROID
#define FLAGS_nos_test_dump_protos false
#else
#include <gflags/gflags.h>

DEFINE_bool(nos_test_dump_protos, false, "Dump binary protobufs to a file.");
#endif  // ANDROID

using nugget::app::protoapi::AesCbcEncryptTest;
using nugget::app::protoapi::AesCbcEncryptTestResult;
using nugget::app::protoapi::APImessageID;
using nugget::app::protoapi::DcryptError;
using nugget::app::protoapi::KeySize;
using nugget::app::protoapi::Notice;
using nugget::app::protoapi::NoticeCode;
using nugget::app::protoapi::OneofTestParametersCase;
using nugget::app::protoapi::OneofTestResultsCase;
using nugget::app::protoapi::TrngTest;
using nugget::app::protoapi::TrngTestResult;
using std::cout;
using std::vector;
using std::unique_ptr;
using test_harness::TestHarness;

#define ASSERT_NO_TH_ERROR(code) \
  ASSERT_EQ(code, test_harness::error_codes::NO_ERROR) \
      << code << " is " << test_harness::error_codes_name(code)

#define ASSERT_MSG_TYPE(msg, type_) \
do{if(type_ != APImessageID::NOTICE && msg.type == APImessageID::NOTICE){ \
  Notice received; \
  received.ParseFromArray(reinterpret_cast<char *>(msg.data), msg.data_len); \
  ASSERT_EQ(msg.type, type_) \
      << msg.type << " is " << APImessageID_Name((APImessageID) msg.type) \
      << "\n" << received.DebugString(); \
}else{ \
  ASSERT_EQ(msg.type, type_) \
      << msg.type << " is " << APImessageID_Name((APImessageID) msg.type); \
}}while(0)

#define ASSERT_SUBTYPE(msg, type_) \
  EXPECT_GT(msg.data_len, 2); \
  uint16_t subtype = (msg.data[0] << 8) | msg.data[1]; \
  EXPECT_EQ(subtype, type_)

namespace {

using test_harness::BYTE_TIME;

class NuggetOsTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  static unique_ptr<TestHarness> harness;
  static std::random_device random_number_generator;
};

unique_ptr<TestHarness> NuggetOsTest::harness;
std::random_device NuggetOsTest::random_number_generator;

void NuggetOsTest::SetUpTestCase() {
  harness = TestHarness::MakeUnique();

#ifndef CONFIG_NO_UART
  if (!harness->UsingSpi()) {
    EXPECT_TRUE(harness->SwitchFromConsoleToProtoApi());
    EXPECT_TRUE(harness->ttyState());
  }
#endif  // CONFIG_NO_UART
}

void NuggetOsTest::TearDownTestCase() {
#ifndef CONFIG_NO_UART
  if (!harness->UsingSpi()) {
    harness->ReadUntil(test_harness::BYTE_TIME * 1024);
    EXPECT_TRUE(harness->SwitchFromProtoApiToConsole(NULL));
  }
#endif  // CONFIG_NO_UART
  harness = unique_ptr<TestHarness>();
}

TEST_F(NuggetOsTest, NoticePing) {
  Notice ping_msg;
  ping_msg.set_notice_code(NoticeCode::PING);
  Notice pong_msg;

  ASSERT_NO_TH_ERROR(harness->SendProto(APImessageID::NOTICE, ping_msg));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << ping_msg.DebugString();
  }
  test_harness::raw_message receive_msg;
  ASSERT_NO_TH_ERROR(harness->GetData(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(receive_msg, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  ASSERT_TRUE(pong_msg.ParseFromArray(
      reinterpret_cast<char *>(receive_msg.data), receive_msg.data_len));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << pong_msg.DebugString() << std::endl;
  }
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);

  ASSERT_NO_TH_ERROR(harness->SendProto(APImessageID::NOTICE, ping_msg));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << ping_msg.DebugString();
  }
  ASSERT_NO_TH_ERROR(harness->GetData(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(receive_msg, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  ASSERT_TRUE(pong_msg.ParseFromArray(
      reinterpret_cast<char *>(receive_msg.data), receive_msg.data_len));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << pong_msg.DebugString() << std::endl;
  }
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);

  ASSERT_NO_TH_ERROR(harness->SendProto(APImessageID::NOTICE, ping_msg));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << ping_msg.DebugString();
  }
  ASSERT_NO_TH_ERROR(harness->GetData(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(receive_msg, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  ASSERT_TRUE(pong_msg.ParseFromArray(
      reinterpret_cast<char *>(receive_msg.data), receive_msg.data_len));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << pong_msg.DebugString() << std::endl;
  }
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);
}

TEST_F(NuggetOsTest, InvalidMessageType) {
  const char content[] = "This is a test message.";

  test_harness::raw_message msg;
  msg.type = 0;
  std::copy(content, content + sizeof(content), msg.data);
  msg.data_len = sizeof(content);

  ASSERT_NO_TH_ERROR(harness->SendData(msg));
  ASSERT_NO_TH_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(msg, APImessageID::NOTICE);

  Notice notice_msg;
  ASSERT_TRUE(notice_msg.ParseFromArray(reinterpret_cast<char *>(msg.data),
                                        msg.data_len));
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << notice_msg.DebugString() << std::endl;
  }
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::UNRECOGNIZED_MESSAGE);
}

TEST_F(NuggetOsTest, Sequence) {
  test_harness::raw_message msg;
  msg.type = APImessageID::SEND_SEQUENCE;
  msg.data_len = 256;
  for (size_t x = 0; x < msg.data_len; ++x) {
    msg.data[x] = x;
  }

  ASSERT_NO_TH_ERROR(harness->SendData(msg));
  ASSERT_NO_TH_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(msg, APImessageID::SEND_SEQUENCE);
  for (size_t x = 0; x < msg.data_len; ++x) {
    ASSERT_EQ(msg.data[x], x) << "Inconsistency at index " << x;
  }
}

TEST_F(NuggetOsTest, Echo) {
  test_harness::raw_message msg;
  msg.type = APImessageID::ECHO_THIS;
  // Leave some room for bytes which need escaping
  msg.data_len = sizeof(msg.data) - 64;
  for (size_t x = 0; x < msg.data_len; ++x) {
    msg.data[x] = random_number_generator();
  }

  ASSERT_NO_TH_ERROR(harness->SendData(msg));

  test_harness::raw_message receive_msg;
  ASSERT_NO_TH_ERROR(harness->GetData(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(msg, APImessageID::ECHO_THIS);
  ASSERT_EQ(receive_msg.data_len, msg.data_len);

  for (size_t x = 0; x < msg.data_len; ++x) {
    ASSERT_EQ(msg.data[x], receive_msg.data[x])
        << "Inconsistency at index " << x;
  }
}

TEST_F(NuggetOsTest, AesCbc) {
  const size_t number_of_blocks = 3;

  for (auto key_size : {KeySize::s128b, KeySize::s192b, KeySize::s256b}) {
    if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
      cout << "Testing with a key size of: " << std::dec << (key_size * 8)
           << std::endl;
    }
    AesCbcEncryptTest request;
    request.set_key_size(key_size);
    request.set_number_of_blocks(number_of_blocks);

    vector<int> key_data(key_size / sizeof(int));
    for (auto &part : key_data) {
      part = random_number_generator();
    }
    request.set_key(key_data.data(), key_data.size() * sizeof(int));


    if (FLAGS_nos_test_dump_protos) {
      std::ofstream outfile;
      outfile.open("AesCbcEncryptTest_" + std::to_string(key_size * 8) +
                   ".proto.bin", std::ios_base::binary);
      outfile << request.SerializeAsString();
      outfile.close();
    }

    ASSERT_NO_TH_ERROR(harness->SendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAesCbcEncryptTest,
        request));

    test_harness::raw_message msg;
    ASSERT_NO_TH_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME));
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kAesCbcEncryptTestResult);

    AesCbcEncryptTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR)
        << result.result_code() << " is "
        << DcryptError_Name(result.result_code());
    ASSERT_EQ(result.cipher_text().size(), number_of_blocks * AES_BLOCK_SIZE)
        << "\n" << result.DebugString();

    uint32_t in[4] = {0, 0, 0, 0};
    uint8_t sw_out[AES_BLOCK_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    memset(&iv, 0, sizeof(iv));
    AES_KEY aes_key;
    AES_set_encrypt_key(reinterpret_cast<uint8_t *>(key_data.data()),
                        key_size * 8, &aes_key);
    for (size_t x = 0; x < number_of_blocks; ++x) {
      AES_cbc_encrypt(reinterpret_cast<uint8_t *>(in),
                      reinterpret_cast<uint8_t *>(sw_out), AES_BLOCK_SIZE,
                      &aes_key, reinterpret_cast<uint8_t *>(iv), true);
      for (size_t y = 0; y < AES_BLOCK_SIZE; ++y) {
        size_t index = x * AES_BLOCK_SIZE + y;
        ASSERT_EQ(result.cipher_text()[index] & 0x00ff,
                  sw_out[y] & 0x00ff) << "Inconsistency at index " << index;
      }
    }

    ASSERT_EQ(result.initialization_vector().size(), (size_t) AES_BLOCK_SIZE)
        << "\n" << result.DebugString();
    for (size_t x = 0; x < AES_BLOCK_SIZE; ++x) {
      ASSERT_EQ(result.initialization_vector()[x] & 0x00ff, iv[x] & 0x00ff)
                << "Inconsistency at index " << x;
    }
  }
}

TEST_F(NuggetOsTest, Trng) {
  // Have a bin for every possible byte value.
  std::vector<size_t> counts(256, 0);

  // Use most of the available space while leaving room for the transport
  // header, escape sequences, etc.
  const size_t request_size = 475;
  const size_t repeats = 10;

  TrngTest request;
  request.set_number_of_bytes(request_size);

  int verbosity = harness->getVerbosity();
  for (size_t x = 0; x < repeats; ++x) {
    ASSERT_NO_TH_ERROR(harness->SendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kTrngTest,
        request));
    test_harness::raw_message msg;
    ASSERT_NO_TH_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME));
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kTrngTestResult);

    TrngTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    ASSERT_EQ(result.random_bytes().size(), request_size);
    for (const auto rand_byte : result.random_bytes()) {
      ++counts[0x00ff & rand_byte];
    }

    // Print the first exchange only for debugging.
    if (x == 0) {
      harness->setVerbosity(harness->getVerbosity() - 1);
    }
  }
  harness->setVerbosity(verbosity);

  double kl_divergence = 0;
  double ratio = (double) counts.size() / (repeats * request_size);
  for (const auto count : counts) {
    ASSERT_NE(count, 0u);
    kl_divergence += count * log2(count * ratio);
  }
  kl_divergence *= ratio;
  if (harness->getVerbosity() >= TestHarness::VerbosityLevels::INFO) {
    cout << "K.L. Divergence: " << kl_divergence << "\n";
    cout.flush();
  }
  ASSERT_LT(kl_divergence, 15.0);
}

}  // namespace
