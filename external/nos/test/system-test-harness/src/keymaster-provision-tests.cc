#include <memory>

#include "gtest/gtest.h"
#include "avb_tools.h"
#include "nugget_tools.h"
#include "nugget/app/avb/avb.pb.h"
#include "nugget/app/keymaster/keymaster.pb.h"
#include "Keymaster.client.h"
#include <application.h>
#include <keymaster.h>
#include <nos/AppClient.h>
#include <nos/NuggetClientInterface.h>
#include "util.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

using std::cout;
using std::string;
using std::unique_ptr;

using namespace nugget::app::avb;
using namespace nugget::app::keymaster;

namespace {

class KeymasterProvisionTest: public testing::Test {
 protected:
  static unique_ptr<nos::NuggetClientInterface> client;
  static unique_ptr<test_harness::TestHarness> uart_printer;

  static void SetUpTestCase();
  static void TearDownTestCase();

  virtual void SetUp(void);

  virtual void PopulateDefaultRequest(ProvisionDeviceIdsRequest *request);
};

unique_ptr<nos::NuggetClientInterface> KeymasterProvisionTest::client;
unique_ptr<test_harness::TestHarness> KeymasterProvisionTest::uart_printer;

void KeymasterProvisionTest::SetUpTestCase() {
  uart_printer = test_harness::TestHarness::MakeUnique();

  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void KeymasterProvisionTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();

  uart_printer = nullptr;
}

void KeymasterProvisionTest::SetUp(void) {
  avb_tools::ResetProduction(client.get());
}

void KeymasterProvisionTest::PopulateDefaultRequest(
    ProvisionDeviceIdsRequest *request) {
  request->set_product_brand("Pixel");
  request->set_product_device("3");
  request->set_product_name("Pixel");
  request->set_serialno("12345678");
  request->set_product_manufacturer("Google");
  request->set_product_model("3");
  request->set_imei("12345678");
  request->set_meid("12345678");
}

// Tests

TEST_F(KeymasterProvisionTest, ProvisionDeviceIdsSuccess) {
  ProvisionDeviceIdsRequest request;
  ProvisionDeviceIdsResponse response;

  PopulateDefaultRequest(&request);

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

TEST_F(KeymasterProvisionTest, ReProvisionDeviceIdsSuccess) {
  ProvisionDeviceIdsRequest request;
  ProvisionDeviceIdsResponse response;

  PopulateDefaultRequest(&request);

  Keymaster service(*client);

  // First instance.
  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);

  // Second ...
  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

TEST_F(KeymasterProvisionTest, ProductionModeProvisionFails) {
  ProvisionDeviceIdsRequest request;
  ProvisionDeviceIdsResponse response;

  PopulateDefaultRequest(&request);

  Keymaster service(*client);

  // Set production bit.
  avb_tools::SetProduction(client.get(), true, NULL, 0);

  // Provisioning is now disallowed.
  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(),
            ErrorCode::PRODUCTION_MODE_PROVISIONING);
}

TEST_F(KeymasterProvisionTest, InvalidDeviceIdFails) {

  ProvisionDeviceIdsRequest request;
  ProvisionDeviceIdsResponse response;

  PopulateDefaultRequest(&request);

  string bad_serialno(KM_MNF_MAX_ENTRY_SIZE + 1, '5');
  request.set_serialno(bad_serialno);

  Keymaster service(*client);

  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(),
            ErrorCode::INVALID_DEVICE_IDS);
}

TEST_F(KeymasterProvisionTest, MaxDeviceIdSuccess) {

  ProvisionDeviceIdsRequest request;
  ProvisionDeviceIdsResponse response;

  PopulateDefaultRequest(&request);

  string max_serialno(KM_MNF_MAX_ENTRY_SIZE, '5');  
  request.set_serialno(max_serialno);

  Keymaster service(*client);

  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

// Regression test for b/77830050#comment6
TEST_F(KeymasterProvisionTest, NoMeidSuccess) {

  ProvisionDeviceIdsRequest request;
  ProvisionDeviceIdsResponse response;

  PopulateDefaultRequest(&request);
  request.clear_meid();

  Keymaster service(*client);

  ASSERT_NO_ERROR(service.ProvisionDeviceIds(request, &response), "");
  ASSERT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

}  // namespace
