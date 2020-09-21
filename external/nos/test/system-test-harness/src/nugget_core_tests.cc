
#include <app_nugget.h>
#include <nos/NuggetClientInterface.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "avb_tools.h"
#include "nugget_tools.h"
#include "util.h"


using std::string;
using std::vector;
using std::unique_ptr;

namespace {

class NuggetCoreTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  static unique_ptr<nos::NuggetClientInterface> client;
  static unique_ptr<test_harness::TestHarness> uart_printer;
  static vector<uint8_t> input_buffer;
  static vector<uint8_t> output_buffer;
};

unique_ptr<nos::NuggetClientInterface> NuggetCoreTest::client;
unique_ptr<test_harness::TestHarness> NuggetCoreTest::uart_printer;

vector<uint8_t> NuggetCoreTest::input_buffer;
vector<uint8_t> NuggetCoreTest::output_buffer;

void NuggetCoreTest::SetUpTestCase() {
  uart_printer = test_harness::TestHarness::MakeUnique();

  client = nugget_tools::MakeNuggetClient();
  client->Open();
  input_buffer.reserve(0x4000);
  output_buffer.reserve(0x4000);
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void NuggetCoreTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();

  uart_printer = nullptr;
}

TEST_F(NuggetCoreTest, GetVersionStringTest) {
  input_buffer.resize(0);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_VERSION, input_buffer, &output_buffer), "");
  ASSERT_GT(output_buffer.size(), 0u);
}

TEST_F(NuggetCoreTest, GetDeviceIdTest) {
  input_buffer.resize(0);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_DEVICE_ID, input_buffer, &output_buffer), "");
  ASSERT_EQ(output_buffer.size(), 18u);
  for (size_t i = 0; i < output_buffer.size(); i++) {
    if (i == 8) {
      ASSERT_EQ(output_buffer[i], ':');
    } else if (i == 17) {
      ASSERT_EQ(output_buffer[i], '\0');
    } else {
      ASSERT_TRUE(std::isxdigit(output_buffer[i]));
    }
  }
}

TEST_F(NuggetCoreTest, EnterDeepSleep) {
  ASSERT_TRUE(nugget_tools::WaitForSleep(client.get(), nullptr));
}

TEST_F(NuggetCoreTest, HardRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get()));
}

TEST_F(NuggetCoreTest, WipeUserData) {
  ASSERT_TRUE(nugget_tools::WipeUserData(client.get()));
}

TEST_F(NuggetCoreTest, GetLowPowerStats) {
  struct nugget_app_low_power_stats stats;
  vector<uint8_t> buffer;

  buffer.reserve(1000);                         // Much more than needed
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
      buffer, &buffer), "");
  ASSERT_GE(buffer.size(), sizeof(stats));

  memcpy(&stats, buffer.data(), sizeof(stats));

  /* We must have booted once and been awake long enough to reply, but that's
   * about all we can be certain of. */
  ASSERT_GT(stats.hard_reset_count, 0UL);
  ASSERT_GT(stats.time_since_hard_reset, 0UL);
  ASSERT_GT(stats.time_spent_awake, 0UL);
}

TEST_F(NuggetCoreTest, GetUartPassthruInBootloader) {
  std::vector<uint8_t> send;
  std::vector<uint8_t> get;

  get.reserve(1);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_AP_UART_PASSTHRU,
      send, &get), "Get UART passthru value");
  /* Should be either on or off. If USB is active, disconnect SuzyQable */
  ASSERT_TRUE(get[0] == NUGGET_AP_UART_OFF ||
              get[0] == NUGGET_AP_UART_ENABLED);
}

TEST_F(NuggetCoreTest, GetUartPassthruInHLOS) {
  std::vector<uint8_t> send;
  std::vector<uint8_t> get;

  avb_tools::BootloaderDone(client.get());

  get.reserve(1);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_AP_UART_PASSTHRU,
      send, &get), "Get UART passthru value");
  /* Should be either on or off. If USB is active, disconnect SuzyQable */
  ASSERT_TRUE(get[0] == NUGGET_AP_UART_OFF ||
              get[0] == NUGGET_AP_UART_ENABLED);
}


TEST_F(NuggetCoreTest, EnableUartPassthruInBootloader) {
  std::vector<uint8_t> send;
  std::vector<uint8_t> get;

  avb_tools::SetBootloader(client.get());

  send.push_back(NUGGET_AP_UART_ENABLED);
  get.reserve(1);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_AP_UART_PASSTHRU,
      send, &get), "Enable UART passthru");

  ASSERT_EQ(get[0], NUGGET_AP_UART_ENABLED);
}

TEST_F(NuggetCoreTest, DisableUartPassthruInBootloader) {
  std::vector<uint8_t> send;
  std::vector<uint8_t> get;

  avb_tools::SetBootloader(client.get());

  send.push_back(NUGGET_AP_UART_OFF);
  get.reserve(1);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_AP_UART_PASSTHRU,
      send, &get), "Disable UART passthru");

  ASSERT_EQ(get[0], NUGGET_AP_UART_OFF);
}

TEST_F(NuggetCoreTest, EnableUartPassthruInHLOSFails) {
  std::vector<uint8_t> send;
  std::vector<uint8_t> get;

  avb_tools::BootloaderDone(client.get());

  send.push_back(NUGGET_AP_UART_ENABLED);
  get.reserve(1);
  /* This should fail */
  ASSERT_NE(APP_SUCCESS, NuggetCoreTest::client->CallApp(
              APP_ID_NUGGET, NUGGET_PARAM_AP_UART_PASSTHRU,
              send, &get));
}

TEST_F(NuggetCoreTest, DisableUartPassthruInHLOSFails) {
  std::vector<uint8_t> send;
  std::vector<uint8_t> get;

  avb_tools::BootloaderDone(client.get());

  send.push_back(NUGGET_AP_UART_OFF);
  get.reserve(1);
  /* This should fail */
  ASSERT_NE(APP_SUCCESS, NuggetCoreTest::client->CallApp(
              APP_ID_NUGGET, NUGGET_PARAM_AP_UART_PASSTHRU,
              send, &get));
}

}  // namespace
