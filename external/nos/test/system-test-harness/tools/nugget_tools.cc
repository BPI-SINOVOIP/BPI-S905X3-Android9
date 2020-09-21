#include "nugget_tools.h"

#include <app_nugget.h>
#include <nos/NuggetClient.h>

#include <chrono>
#include <cinttypes>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#ifdef ANDROID
#include <android-base/endian.h>
#include "nos/CitadeldProxyClient.h"
#else
#include "gflags/gflags.h"

DEFINE_string(nos_core_serial, "", "USB device serial number to open");
#endif  // ANDROID

#ifndef LOG
#define LOG(x) std::cerr << __FILE__ << ":" << __LINE__ << " " << #x << ": "
#endif  // LOG

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::string;

namespace nugget_tools {

std::string GetCitadelUSBSerialNo() {
#ifdef ANDROID
  return "";
#else
  if (FLAGS_nos_core_serial.empty()) {
    const char *env_default = secure_getenv("CITADEL_DEVICE");
    if (env_default && *env_default) {
      FLAGS_nos_core_serial.assign(env_default);
      std::cerr << "Using CITADEL_DEVICE=" << FLAGS_nos_core_serial << "\n";
    }
  }
  return FLAGS_nos_core_serial;
#endif
}

std::unique_ptr<nos::NuggetClientInterface> MakeNuggetClient() {
#ifdef ANDROID
  std::unique_ptr<nos::NuggetClientInterface> client =
      std::unique_ptr<nos::NuggetClientInterface>(new nos::NuggetClient());
  client->Open();
  if (!client->IsOpen()) {
    client = std::unique_ptr<nos::NuggetClientInterface>(
        new nos::CitadeldProxyClient());
  }
  return client;
#else
  return std::unique_ptr<nos::NuggetClientInterface>(
      new nos::NuggetClient(GetCitadelUSBSerialNo()));
#endif
}

bool CyclesSinceBoot(nos::NuggetClientInterface *client, uint32_t *cycles) {
  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(uint32_t));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_CYCLES_SINCE_BOOT,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    perror("test");
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_CYCLES_SINCE_BOOT, ...) failed!\n";
    return false;
  };
  if (buffer.size() != sizeof(uint32_t)) {
    LOG(ERROR) << "Unexpected size of cycle count!\n";
    return false;
  }
  *cycles = le32toh(*reinterpret_cast<uint32_t *>(buffer.data()));
  return true;
}

static void ShowStats(const char *msg,
                      const struct nugget_app_low_power_stats& stats) {
  printf("%s\n", msg);
  printf("  hard_reset_count         %" PRIu64 "\n", stats.hard_reset_count);
  printf("  time_since_hard_reset    %" PRIu64 "\n",
         stats.time_since_hard_reset);
  printf("  wake_count               %" PRIu64 "\n", stats.wake_count);
  printf("  time_at_last_wake        %" PRIu64 "\n", stats.time_at_last_wake);
  printf("  time_spent_awake         %" PRIu64 "\n", stats.time_spent_awake);
  printf("  deep_sleep_count         %" PRIu64 "\n", stats.deep_sleep_count);
  printf("  time_at_last_deep_sleep  %" PRIu64 "\n",
         stats.time_at_last_deep_sleep);
  printf("  time_spent_in_deep_sleep %" PRIu64 "\n",
         stats.time_spent_in_deep_sleep);
}

bool RebootNugget(nos::NuggetClientInterface *client) {
  struct nugget_app_low_power_stats stats0;
  struct nugget_app_low_power_stats stats1;
  std::vector<uint8_t> buffer;

  // Grab stats before sleeping
  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats0, buffer.data(), sizeof(stats0));

  // Capture the time here to allow for some tolerance on the reported time.
  auto start = high_resolution_clock::now();

  // Tell Nugget OS to reboot
  std::vector<uint8_t> ignored;
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, ignored,
                      nullptr) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_REBOOT, ...) failed!\n";
    return false;
  }

  // Grab stats after sleeping
  buffer.empty();
  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats1, buffer.data(), sizeof(stats1));

  // Figure a max elapsed time that Nugget OS should see (our time + 5%).
  auto max_usecs =
      duration_cast<microseconds>(high_resolution_clock::now() - start) *
          105 / 100;

  // Verify that Citadel rebooted
  if (stats1.hard_reset_count == stats0.hard_reset_count + 1 &&
      stats1.time_at_last_wake == 0 &&
      stats1.deep_sleep_count == 0 &&
      std::chrono::microseconds(stats1.time_since_hard_reset) < max_usecs) {
    return true;
  }

  LOG(ERROR) << "Citadel didn't reboot within "
             << max_usecs.count() << " microseconds\n";
  ShowStats("stats before waiting", stats0);
  ShowStats("stats after waiting", stats1);

  return false;
}

bool WaitForSleep(nos::NuggetClientInterface *client, uint32_t *seconds_waited) {
  struct nugget_app_low_power_stats stats0;
  struct nugget_app_low_power_stats stats1;
  std::vector<uint8_t> buffer;

  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  // Grab stats before sleeping
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats0, buffer.data(), sizeof(stats0));

  // Wait for Citadel to fall asleep
  constexpr uint32_t wait_seconds = 4;
  std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));

  // Grab stats after sleeping
  buffer.empty();
  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats1, buffer.data(), sizeof(stats1));

  // Verify that Citadel went to sleep but didn't reboot
  if (stats1.hard_reset_count == stats0.hard_reset_count &&
      stats1.deep_sleep_count == stats0.deep_sleep_count + 1 &&
      stats1.wake_count == stats0.wake_count + 1 &&
      stats1.time_spent_in_deep_sleep > stats0.time_spent_in_deep_sleep) {
    // Yep, looks good
    if (seconds_waited) {
      *seconds_waited = wait_seconds;
    }
    return true;
  }

  LOG(ERROR) << "Citadel didn't sleep\n";
  ShowStats("stats before waiting", stats0);
  ShowStats("stats after waiting", stats1);

  return false;
}

bool WipeUserData(nos::NuggetClientInterface *client) {
  struct nugget_app_low_power_stats stats0;
  struct nugget_app_low_power_stats stats1;
  std::vector<uint8_t> buffer;

  // Grab stats before sleeping
  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats0, buffer.data(), sizeof(stats0));

  // Request wipe of user data which should hard reboot
  buffer.resize(4);
  *reinterpret_cast<uint32_t *>(buffer.data()) = htole32(ERASE_CONFIRMATION);
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_NUKE_FROM_ORBIT,
                         buffer, nullptr) != app_status::APP_SUCCESS) {
    return false;
  }

  // Grab stats after sleeping
  buffer.empty();
  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats1, buffer.data(), sizeof(stats1));

  // Verify that Citadel didn't reset
  const bool ret = stats1.hard_reset_count == stats0.hard_reset_count;
  if (!ret) {
    LOG(ERROR) << "Citadel reset while wiping user data\n";
  }
  return ret;
}

}  // namespace nugget_tools
