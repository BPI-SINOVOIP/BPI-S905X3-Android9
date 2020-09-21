#include <fstream>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include "nugget/app/protoapi/control.pb.h"
#include "nugget/app/protoapi/header.pb.h"
#include "nugget/app/protoapi/testing_api.pb.h"
#include "src/macros.h"
#include "src/util.h"

using nugget::app::protoapi::AesGcmEncryptTest;
using nugget::app::protoapi::AesGcmEncryptTestResult;
using nugget::app::protoapi::APImessageID;
using nugget::app::protoapi::DcryptError;
using nugget::app::protoapi::Notice;
using nugget::app::protoapi::NoticeCode;
using nugget::app::protoapi::OneofTestParametersCase;
using nugget::app::protoapi::OneofTestResultsCase;
using std::cout;
using std::stringstream;
using std::unique_ptr;

DEFINE_bool(nos_test_dump_protos, false, "Dump binary protobufs to a file.");
DEFINE_int32(test_input_number, -1, "Run a specific test input.");

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
  static unique_ptr<test_harness::TestHarness> harness;
};

unique_ptr<test_harness::TestHarness> NuggetOsTest::harness;

void NuggetOsTest::SetUpTestCase() {
  harness = TestHarness::MakeUnique();

  if (!harness->UsingSpi()) {
    EXPECT_TRUE(harness->SwitchFromConsoleToProtoApi());
    EXPECT_TRUE(harness->ttyState());
  }
}

void NuggetOsTest::TearDownTestCase() {
  harness->ReadUntil(test_harness::BYTE_TIME * 1024);
  if (!harness->UsingSpi()) {
    EXPECT_TRUE(harness->SwitchFromProtoApiToConsole(NULL));
  }
  harness = unique_ptr<test_harness::TestHarness>();
}

#include "src/test-data/NIST-CAVP/aes-gcm-cavp.h"

TEST_F(NuggetOsTest, AesGcm) {
  const int verbosity = harness->getVerbosity();
  harness->setVerbosity(verbosity - 1);
  harness->ReadUntil(test_harness::BYTE_TIME * 1024);

  size_t i = 0;
  size_t test_input_count = ARRAYSIZE(NIST_GCM_DATA);
  if (FLAGS_test_input_number != -1) {
    i = FLAGS_test_input_number;
    test_input_count = FLAGS_test_input_number + 1;
  }
  for (; i < test_input_count; i++) {
    const gcm_data *test_case = &NIST_GCM_DATA[i];

    AesGcmEncryptTest request;
    request.set_key(test_case->key, test_case->key_len / 8);
    request.set_iv(test_case->IV, test_case->IV_len / 8);
    request.set_plain_text(test_case->PT, test_case->PT_len / 8);
    request.set_aad(test_case->AAD, test_case->AAD_len / 8);
    request.set_tag_len(test_case->tag_len / 8);

    if (FLAGS_nos_test_dump_protos) {
      std::ofstream outfile;
      outfile.open("AesGcmEncryptTest_" + std::to_string(test_case->key_len) +
                   ".proto.bin", std::ios_base::binary);
      outfile << request.SerializeAsString();
      outfile.close();
    }

    ASSERT_NO_ERROR(harness->SendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAesGcmEncryptTest,
        request), "");

    test_harness::raw_message msg;
    ASSERT_NO_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME), "");
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kAesGcmEncryptTestResult);

    AesGcmEncryptTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR)
        << result.result_code() << " is "
        << DcryptError_Name(result.result_code());

    ASSERT_EQ(result.cipher_text().size(), test_case->PT_len / 8)
            << "\n" << result.DebugString();
    const uint8_t *CT = (const uint8_t *)test_case->CT;
    stringstream ct_ss;
    for (size_t j = 0; j < test_case->PT_len / 8; j++) {
      if (CT[j] < 16) {
        ct_ss << '0';
      }
      ct_ss << std::hex << (unsigned int)CT[j];
    }
    for (size_t j = 0; j < test_case->PT_len / 8; j++) {
      ASSERT_EQ(result.cipher_text()[j] & 0x00FF, CT[j] & 0x00FF)
              << "\n"
              << "test_case: " << i << "\n"
              << "result   : " << result.DebugString()
              << "CT       : " << ct_ss.str() << "\n"
              << "mis-match: " << j;
    }

    ASSERT_EQ(result.tag().size(), test_case->tag_len / 8)
            << "\n" << result.DebugString();
    const uint8_t *tag = (const uint8_t *)test_case->tag;
    stringstream tag_ss;
    for (size_t j = 0; j < test_case->tag_len / 8; j++) {
      if (tag[j] < 16) {
        tag_ss << '0';
      }
      tag_ss << std::hex << (unsigned int)tag[j];
    }
    for (size_t j = 0; j < test_case->tag_len / 8; j++) {
      ASSERT_EQ(result.tag()[j] & 0x00ff, tag[j] & 0x00ff)
              << "\n"
              << "test_case: " << i << "\n"
              << "result   : " << result.DebugString()
              << "TAG      : " << tag_ss.str() << "\n"
              << "mis-match: " << j;
    }
  }

  harness->ReadUntil(test_harness::BYTE_TIME * 1024);
  harness->setVerbosity(verbosity);
}

}  // namespace
