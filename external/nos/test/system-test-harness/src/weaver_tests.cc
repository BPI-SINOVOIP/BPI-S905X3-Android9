
#include <memory>
#include <random>

#include "gtest/gtest.h"
#include "avb_tools.h"
#include "nugget_tools.h"
#include "nugget/app/weaver/weaver.pb.h"
#include "util.h"
#include "Weaver.client.h"

#define __STAMP_STR1__(a) #a
#define __STAMP_STR__(a) __STAMP_STR1__(a)
#define __STAMP__ __STAMP_STR__(__FILE__) ":" __STAMP_STR__(__LINE__)

using std::cout;
using std::string;
using std::unique_ptr;

using namespace nugget::app::weaver;

namespace {

class WeaverTest: public testing::Test {
 protected:
  static const uint32_t SLOT_MASK = 0x3f;
  static std::random_device random_number_generator;
  static uint32_t slot;

  static unique_ptr<nos::NuggetClientInterface> client;
  static unique_ptr<test_harness::TestHarness> uart_printer;

  static void SetUpTestCase();
  static void TearDownTestCase();

  void testWrite(const string& msg, uint32_t slot, const uint8_t *key,
                 const uint8_t *value);
  void testRead(const string& msg, const uint32_t slot, const uint8_t *key,
                const uint8_t *value);
  void testEraseValue(const string& msg, uint32_t slot);
  void testReadWrongKey(const string& msg, uint32_t slot, const uint8_t *key,
                        uint32_t throttle_sec);
  void testReadThrottle(const string& msg, uint32_t slot, const uint8_t *key,
                        uint32_t throttle_sec);

  void activateThrottle(uint32_t slot, const uint8_t *key,
                        const uint8_t *wrong_key, uint32_t throttle_sec);
 public:
  static constexpr size_t KEY_SIZE = 16;
  static constexpr size_t VALUE_SIZE = 16;
  const uint8_t TEST_KEY[KEY_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8,
                                      9, 10, 11, 12, 13, 14, 15, 16};
  const uint8_t WRONG_KEY[KEY_SIZE] = {100, 2, 3, 4, 5, 6, 7, 8,
                                       9, 10, 11, 12, 13, 14, 15, 16};
  const uint8_t TEST_VALUE[VALUE_SIZE] = {1, 0, 3, 0, 5, 0, 7, 0,
                                          0, 10, 0, 12, 0, 14, 0, 16};
  const uint8_t ZERO_VALUE[VALUE_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0};
};

std::random_device WeaverTest::random_number_generator;
// Randomly select a slot for the test rather than testing all slots to reduce
// the wear on the flash. All slots behave the same, independently of each
// other.
uint32_t WeaverTest::slot = WeaverTest::random_number_generator() & SLOT_MASK;

unique_ptr<nos::NuggetClientInterface> WeaverTest::client;
unique_ptr<test_harness::TestHarness> WeaverTest::uart_printer;

void WeaverTest::SetUpTestCase() {
  uart_printer = test_harness::TestHarness::MakeUnique();

  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void WeaverTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();

  uart_printer = nullptr;
}

void WeaverTest::testWrite(const string& msg, uint32_t slot, const uint8_t *key,
                           const uint8_t *value) {
  WriteRequest request;
  WriteResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);
  request.set_value(value, VALUE_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Write(request, &response), msg);
}

void WeaverTest::testRead(const string& msg, uint32_t slot, const uint8_t *key,
                          const uint8_t *value) {
  ReadRequest request;
  ReadResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Read(request, &response), msg);
  ASSERT_EQ(response.error(), ReadResponse::NONE) << msg;
  ASSERT_EQ(response.throttle_msec(), 0u) << msg;
  auto response_value = response.value();
  for (size_t x = 0; x < KEY_SIZE; ++x) {
    ASSERT_EQ(response_value[x], value[x]) << "Inconsistency at index " << x
                                           <<" " << msg;
  }
}

void WeaverTest::testEraseValue(const string& msg, uint32_t slot) {
  EraseValueRequest request;
  EraseValueResponse response;
  request.set_slot(slot);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.EraseValue(request, &response), msg);
}

void WeaverTest::testReadWrongKey(const string& msg, uint32_t slot,
                                  const uint8_t *key, uint32_t throttle_sec) {
  ReadRequest request;
  ReadResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Read(request, &response), msg);
  ASSERT_EQ(response.error(), ReadResponse::WRONG_KEY) << msg;
  ASSERT_EQ(response.throttle_msec(), throttle_sec * 1000) << msg;
  auto response_value = response.value();
  for (size_t x = 0; x < response_value.size(); ++x) {
    ASSERT_EQ(response_value[x], 0) << "Inconsistency at index " << x
                                    <<" " << msg;
  }
}

void WeaverTest::testReadThrottle(const string& msg, uint32_t slot,
                                  const uint8_t *key, uint32_t throttle_sec) {
  ReadRequest request;
  ReadResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Read(request, &response), msg);
  ASSERT_EQ(response.error(), ReadResponse::THROTTLE) << msg;
  ASSERT_NE(response.throttle_msec(), 0u) << msg;
  ASSERT_LE(response.throttle_msec(), throttle_sec * 1000) << msg;
  auto response_value = response.value();
  for (size_t x = 0; x < response_value.size(); ++x) {
    ASSERT_EQ(response_value[x], 0) << "Inconsistency at index " << x
                                    <<" " << msg;
  }
}

void WeaverTest::activateThrottle(uint32_t slot, const uint8_t *key,
                                  const uint8_t *wrong_key,
                                  uint32_t throttle_sec) {
  // Reset the slot
  testWrite(__STAMP__, slot, key, TEST_VALUE);

  // First throttle comes after 5 attempts
  testReadWrongKey(__STAMP__, slot, wrong_key, 0);
  testReadWrongKey(__STAMP__, slot, wrong_key, 0);
  testReadWrongKey(__STAMP__, slot, wrong_key, 0);
  testReadWrongKey(__STAMP__, slot, wrong_key, 0);
  testReadWrongKey(__STAMP__, slot, wrong_key, throttle_sec);
}

TEST_F(WeaverTest, GetConfig) {
  GetConfigRequest request;
  GetConfigResponse response;

  Weaver service(*client);
  ASSERT_NO_ERROR(service.GetConfig(request, &response), "");
  EXPECT_EQ(response.number_of_slots(), 64u);
  EXPECT_EQ(response.key_size(), 16u);
  EXPECT_EQ(response.value_size(), 16u);
}

TEST_F(WeaverTest, WriteReadErase) {
  const uint8_t ZERO_VALUE[16] = {0};

  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
  testRead(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
  testEraseValue(__STAMP__, WeaverTest::slot);

  testRead(__STAMP__, WeaverTest::slot, TEST_KEY, ZERO_VALUE);
}

// 5 slots per record
TEST_F(WeaverTest, WriteToMultipleSlotsInSameRecordIncreasingOrder) {
  testWrite(__STAMP__, 0, TEST_KEY, TEST_VALUE);
  testWrite(__STAMP__, 1, TEST_KEY, TEST_VALUE);

  testRead(__STAMP__, 0, TEST_KEY, TEST_VALUE);
  testRead(__STAMP__, 1, TEST_KEY, TEST_VALUE);
}

TEST_F(WeaverTest, WriteToMultipleSlotsInSameRecordDecreasingOrder) {
  testWrite(__STAMP__, 8, TEST_KEY, TEST_VALUE);
  testWrite(__STAMP__, 7, TEST_KEY, TEST_VALUE);

  testRead(__STAMP__, 8, TEST_KEY, TEST_VALUE);
  testRead(__STAMP__, 7, TEST_KEY, TEST_VALUE);
}

TEST_F(WeaverTest, WriteToMultipleSlotsInDifferentRecordsIncreasingOrder) {
  testWrite(__STAMP__, 9, TEST_KEY, TEST_VALUE);
  testWrite(__STAMP__, 10, TEST_KEY, TEST_VALUE);

  testRead(__STAMP__, 9, TEST_KEY, TEST_VALUE);
  testRead(__STAMP__, 10, TEST_KEY, TEST_VALUE);
}

TEST_F(WeaverTest, WriteToMultipleSlotsInDifferentRecordsDecreasingOrder) {
  testWrite(__STAMP__, 5, TEST_KEY, TEST_VALUE);
  testWrite(__STAMP__, 4, TEST_KEY, TEST_VALUE);

  testRead(__STAMP__, 4, TEST_KEY, TEST_VALUE);
  testRead(__STAMP__, 5, TEST_KEY, TEST_VALUE);
}

TEST_F(WeaverTest, WriteDeepSleepRead) {
  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
  ASSERT_TRUE(nugget_tools::WaitForSleep(client.get(), 0));
  testRead(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
}

TEST_F(WeaverTest, WriteHardRebootRead) {
  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get()));
  testRead(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
}

TEST_F(WeaverTest, ReadThrottle) {
  activateThrottle(WeaverTest::slot, TEST_KEY, WRONG_KEY, 30);
  testReadThrottle(__STAMP__, WeaverTest::slot, WRONG_KEY, 30);
}

TEST_F(WeaverTest, ReadThrottleAfterDeepSleep) {
  activateThrottle(WeaverTest::slot, TEST_KEY, WRONG_KEY, 30);
  ASSERT_TRUE(nugget_tools::WaitForSleep(client.get(), 0));
  testReadThrottle(__STAMP__, WeaverTest::slot, WRONG_KEY, 30);
}

TEST_F(WeaverTest, ReadThrottleAfterHardReboot) {
  activateThrottle(WeaverTest::slot, TEST_KEY, WRONG_KEY, 30);
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get()));
  testReadThrottle(__STAMP__, WeaverTest::slot, WRONG_KEY, 30);
}

TEST_F(WeaverTest, ReadThrottleAfterSleep) {
  uint32_t waited = 0;
  activateThrottle(WeaverTest::slot, TEST_KEY, WRONG_KEY, 30);
  ASSERT_TRUE(nugget_tools::WaitForSleep(client.get(), &waited));
  testReadThrottle(__STAMP__, WeaverTest::slot, WRONG_KEY, 30 - waited);
}

TEST_F(WeaverTest, ReadAttemptCounterPersistsDeepSleep) {
  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);

  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);

  ASSERT_TRUE(nugget_tools::WaitForSleep(client.get(), 0));

  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 30);
}

TEST_F(WeaverTest, ReadAttemptCounterPersistsHardReboot) {
  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);

  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);

  ASSERT_TRUE(nugget_tools::RebootNugget(client.get()));

  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(__STAMP__, WeaverTest::slot, WRONG_KEY, 30);
}

TEST_F(WeaverTest, ReadInvalidSlot) {
  ReadRequest request;
  request.set_slot(std::numeric_limits<uint32_t>::max() - 3);
  request.set_key(TEST_KEY, sizeof(TEST_KEY));

  Weaver service(*client);
  ASSERT_EQ(service.Read(request, nullptr), APP_ERROR_BOGUS_ARGS);
}

TEST_F(WeaverTest, WriteInvalidSlot) {
  WriteRequest request;
  request.set_slot(std::numeric_limits<uint32_t>::max() - 5);
  request.set_key(TEST_KEY, sizeof(TEST_KEY));
  request.set_value(TEST_VALUE, sizeof(TEST_VALUE));

  Weaver service(*client);
  ASSERT_EQ(service.Write(request, nullptr), APP_ERROR_BOGUS_ARGS);
}

TEST_F(WeaverTest, EraseValueInvalidSlot) {
  EraseValueRequest request;
  request.set_slot(std::numeric_limits<uint32_t>::max() - 8);

  Weaver service(*client);
  ASSERT_EQ(service.EraseValue(request, nullptr), APP_ERROR_BOGUS_ARGS);
}

TEST_F(WeaverTest, WipeUserDataOnlyClearsValues) {
  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
  ASSERT_TRUE(nugget_tools::WipeUserData(client.get()));
  testRead(__STAMP__, WeaverTest::slot, TEST_KEY, ZERO_VALUE);
}

TEST_F(WeaverTest, ProductionResetWipesUserData) {
  avb_tools::SetProduction(client.get(), true, NULL, 0);
  testWrite(__STAMP__, WeaverTest::slot, TEST_KEY, TEST_VALUE);
  avb_tools::ResetProduction(client.get());
  testRead(__STAMP__, WeaverTest::slot, TEST_KEY, ZERO_VALUE);
}

// Regression tests
TEST_F(WeaverTest, WipeUserDataWriteSlot0ReadSlot1) {
  testWrite(__STAMP__, 0, TEST_KEY, TEST_VALUE);
  testWrite(__STAMP__, 1, TEST_KEY, TEST_VALUE);
  ASSERT_TRUE(nugget_tools::WipeUserData(client.get()));
  testWrite(__STAMP__, 0, TEST_KEY, TEST_VALUE);
  testRead(__STAMP__, 1, TEST_KEY, ZERO_VALUE);
}

}  // namespace
