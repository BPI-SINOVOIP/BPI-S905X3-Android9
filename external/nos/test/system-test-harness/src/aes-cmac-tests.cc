#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include "gtest/gtest.h"
#include "nugget/app/protoapi/control.pb.h"
#include "nugget/app/protoapi/header.pb.h"
#include "nugget/app/protoapi/testing_api.pb.h"
#include "src/macros.h"
#include "src/util.h"

using nugget::app::protoapi::AesCmacTest;
using nugget::app::protoapi::AesCmacTestResult;
using nugget::app::protoapi::APImessageID;
using nugget::app::protoapi::DcryptError;
using nugget::app::protoapi::Notice;
using nugget::app::protoapi::NoticeCode;
using nugget::app::protoapi::OneofTestParametersCase;
using nugget::app::protoapi::OneofTestResultsCase;
using std::cout;
using std::stringstream;
using std::unique_ptr;

#define ASSERT_MSG_TYPE(msg, type_) \
do{ \
  if (type_ != APImessageID::NOTICE && msg.type == APImessageID::NOTICE) { \
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
do { \
  EXPECT_GT(msg.data_len, 2); \
  uint16_t subtype = (msg.data[0] << 8) | msg.data[1]; \
  EXPECT_EQ(subtype, type_); \
} while(0)

namespace {

using test_harness::BYTE_TIME;

class DcryptoTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  static unique_ptr<test_harness::TestHarness> harness;
  static std::random_device random_number_generator;
};

unique_ptr<test_harness::TestHarness> DcryptoTest::harness;
std::random_device DcryptoTest::random_number_generator;

void DcryptoTest::SetUpTestCase() {
  harness = unique_ptr<test_harness::TestHarness>(
      new test_harness::TestHarness());
}

void DcryptoTest::TearDownTestCase() {
  harness = unique_ptr<test_harness::TestHarness>();
}

#include "src/test-data/dcrypto/aes-cmac-rfc4493.h"

TEST_F(DcryptoTest, AesCmacRfc4493Test) {
  const int verbosity = harness->getVerbosity();
  harness->setVerbosity(verbosity - 1);

  for (size_t i = 0; i < ARRAYSIZE(RFC4493_AES_CMAC_DATA); i++) {
    const cmac_data *test_case = &RFC4493_AES_CMAC_DATA[i];
    AesCmacTest request;
    request.set_key(test_case->K, sizeof(test_case->K));

    if (test_case->M_len > 0) {
      request.set_plain_text(test_case->M, test_case->M_len);
    }

    ASSERT_NO_ERROR(harness->SendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAesCmacTest,
        request), "");

    test_harness::raw_message msg;
    ASSERT_NO_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME), "");
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kAesCmacTestResult);

    AesCmacTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR)
      << result.result_code() << " is "
      << DcryptError_Name(result.result_code());

    for (size_t j = 0; j < sizeof(test_case->CMAC); j++) {
      ASSERT_EQ(result.cmac()[j] & 0x00ff, test_case->CMAC[j] & 0x00ff);
    }
  }

  harness->setVerbosity(verbosity);
}

}  // namespace
