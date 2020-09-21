#include "avb_tools.h"

#include "gtest/gtest.h"
#include "nugget/app/avb/avb.pb.h"

#include <app_nugget.h>
#include "Avb.client.h"
#include <avb.h>
#include <nos/NuggetClient.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#ifdef ANDROID
#include <android-base/endian.h>
#include "nos/CitadeldProxyClient.h"
#else
#include "gflags/gflags.h"
#endif  // ANDROID

using namespace nugget::app::avb;

namespace avb_tools {

void SetBootloader(nos::NuggetClientInterface *client)
{
  // Force AVB to believe that the AP is in the BIOS.
  ::nos::AppClient app(*client, APP_ID_AVB_TEST);

  /* We have to have a buffer, because it's called by reference. */
  std::vector<uint8_t> buffer;

  // No params, no args needed. This is all that the fake "AVB_TEST" app does.
  uint32_t retval = app.Call(0, buffer, &buffer);

  EXPECT_EQ(retval, APP_SUCCESS);
}

void BootloaderDone(nos::NuggetClientInterface *client)
{
  BootloaderDoneRequest request;

  Avb service(*client);
  ASSERT_NO_ERROR(service.BootloaderDone(request, nullptr), "");
}

void GetState(nos::NuggetClientInterface *client, bool *bootloader,
                 bool *production, uint8_t *locks) {
  GetStateRequest request;
  GetStateResponse response;

  Avb service(*client);
  ASSERT_NO_ERROR(service.GetState(request, &response), "");
  EXPECT_EQ(response.number_of_locks(), 4U);

  if (bootloader != NULL)
    *bootloader = response.bootloader();
  if (production != NULL)
    *production = response.production();

  auto response_locks = response.locks();
  if (locks != NULL) {
    for (size_t i = 0; i < response_locks.size(); i++)
      locks[i] = response_locks[i];
  }
}

int Reset(nos::NuggetClientInterface *client, ResetRequest_ResetKind kind,
             const uint8_t *sig, size_t size) {
  ResetRequest request;

  request.set_kind(kind);
  request.mutable_token()->set_selector(ResetToken::CURRENT);
  if (sig && size) {
    request.mutable_token()->set_signature(sig, size);
  } else {
    uint8_t empty[AVB_SIGNATURE_SIZE];
    memset(empty, 0, sizeof(empty));
    request.mutable_token()->set_signature(empty, sizeof(empty));
  }

  Avb service(*client);
  return service.Reset(request, nullptr);
}

int GetResetChallenge(nos::NuggetClientInterface *client,
                               uint32_t *selector, uint64_t *nonce,
                               uint8_t *device_data, size_t *len) {
  GetResetChallengeRequest request;
  GetResetChallengeResponse response;

  Avb service(*client);
  uint32_t ret = service.GetResetChallenge(request, &response);
  if (ret != APP_SUCCESS) {
    return ret;
  }
  *selector = response.selector();
  *nonce = response.nonce();
  // Only copy what there is space for.
  *len = (*len < response.device_data().size() ? *len : response.device_data().size());
  memcpy(device_data, response.device_data().data(), *len);
  // Let the caller assert if the requested size was too large.
  *len = response.device_data().size();
  return ret;
}

int SetProduction(nos::NuggetClientInterface *client, bool production,
                     const uint8_t *data, size_t size) {
  SetProductionRequest request;

  request.set_production(production);
  if (size != 0 && data != NULL) {
    request.set_device_data(data, size);
  }
  // Substitute an empty hash
  uint8_t empty[AVB_DEVICE_DATA_SIZE];
  memset(empty, 0, sizeof(empty));
  if (production && data == NULL) {
    request.set_device_data(empty, sizeof(empty));
  }

  Avb service(*client);
  return service.SetProduction(request, nullptr);
}

void ResetProduction(nos::NuggetClientInterface *client)
{
  struct ResetMessage message;
  int code;
  uint32_t selector;
  size_t len = sizeof(message.data);
  uint8_t signature[AVB_SIGNATURE_SIZE];
  size_t siglen = sizeof(signature);
  memset(signature, 0, siglen);

  // We need the nonce to be set before we get fallthrough.
  memset(message.data, 0, sizeof(message.data));
  code = GetResetChallenge(client, &selector, &message.nonce, message.data, &len);
  ASSERT_NO_ERROR(code, "");
  // No signature is needed for TEST_IMAGE.
  //EXPECT_EQ(0, SignChallenge(&message, signature, &siglen));
  Reset(client, ResetRequest::PRODUCTION, signature, siglen);
  bool bootloader;
  bool production;
  uint8_t locks[4];
  GetState(client, &bootloader, &production, locks);
  ASSERT_EQ(production, false);
}

}  // namespace nugget_tools
