#ifndef AVB_TOOLS_H
#define AVB_TOOLS_H

#include <app_nugget.h>
#include <application.h>
#include <avb.h>
#include <nos/debug.h>
#include <nos/NuggetClientInterface.h>

#include "nugget/app/avb/avb.pb.h"

#include <memory>
#include <string>

#define ASSERT_NO_ERROR(code, msg) \
  do { \
    int value = code; \
    ASSERT_EQ(value, app_status::APP_SUCCESS) \
        << value << " is " << nos::StatusCodeString(value) << msg; \
  } while(0)

using namespace nugget::app::avb;

namespace avb_tools {

// Utility AVB calls.
struct ResetMessage {
  uint64_t nonce;
  uint8_t data[AVB_DEVICE_DATA_SIZE];
};

void SetBootloader(nos::NuggetClientInterface *client);
void BootloaderDone(nos::NuggetClientInterface *client);
void GetState(nos::NuggetClientInterface *client, bool *bootloader,
              bool *production, uint8_t *locks);
int Reset(nos::NuggetClientInterface *client, ResetRequest_ResetKind kind,
          const uint8_t *sig, size_t size);
int GetResetChallenge(nos::NuggetClientInterface *client,
                      uint32_t *selector, uint64_t *nonce,
                      uint8_t *device_data, size_t *len);
int SetProduction(nos::NuggetClientInterface *client, bool production,
                  const uint8_t *data, size_t size);
void ResetProduction(nos::NuggetClientInterface *client);

}  // namespace avb_tools

#endif  // AVB_TOOLS_H
