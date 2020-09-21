#include "keymaster_tools.h"
#include "avb_tools.h"

#include "gtest/gtest.h"
#include "nugget/app/keymaster/keymaster.pb.h"

#include <app_nugget.h>
#include "Keymaster.client.h"
#include <keymaster.h>
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

using std::string;
using namespace nugget::app::keymaster;
using namespace avb_tools;

namespace keymaster_tools {

void SetRootOfTrust(nos::NuggetClientInterface *client)
{

  // Do keymaster setup that is normally executed by the bootloader.
  avb_tools::SetBootloader(client);

  SetRootOfTrustRequest request;
  SetRootOfTrustResponse response;
  Keymaster service(*client);
  request.set_digest(string(32, '\0'));
  ASSERT_NO_ERROR(service.SetRootOfTrust(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);

  avb_tools::BootloaderDone(client);
}

}  // namespace keymaster_tools
