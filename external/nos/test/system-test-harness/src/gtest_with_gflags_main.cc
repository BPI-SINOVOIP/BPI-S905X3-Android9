
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#ifdef ANDROID
#define FLAGS_list_slow_tests false
#define FLAGS_disable_slow_tests false
// TODO: how does FLAGS_release_tests feature here?
#define FLAGS_release_tests true
#else
#include <gflags/gflags.h>
DEFINE_bool(list_slow_tests, false, "List tests included in the set of slow tests.");
DEFINE_bool(disable_slow_tests, false, "Enables a filter to disable a set of slow tests.");
DEFINE_bool(release_tests, false, "Disables tests that would fail for firmware images built with TEST_IMAGE=0");
#endif  // ANDROID

static void generate_disabled_test_list(
    const std::vector<std::string>& tests,
    std::stringstream *ss) {
    for (const auto& test : tests) {
      if (ss->tellp() == 0) {
        *ss << "-";
      } else {
        *ss << ":";
      }
      *ss << test;
    }
}

int main(int argc, char** argv) {
  const std::vector<std::string> slow_tests{
      "AvbTest.*",
      "ImportKeyTest.RSASuccess",
      "NuggetCoreTest.EnterDeepSleep",
      "NuggetCoreTest.HardRebootTest",
      "WeaverTest.ReadAttemptCounterPersistsDeepSleep",
      "WeaverTest.ReadAttemptCounterPersistsHardReboot",
      "WeaverTest.ReadThrottleAfterDeepSleep",
      "WeaverTest.ReadThrottleAfterHardReboot",
      "WeaverTest.ReadThrottleAfterSleep",
      "WeaverTest.WriteDeepSleepRead",
      "WeaverTest.WriteHardRebootRead",
  };

  const std::vector<std::string> disabled_for_release_tests{
      "DcryptoTest.AesCmacRfc4493Test",
      "KeymasterProvisionTest.ProvisionDeviceIdsSuccess",
      "KeymasterProvisionTest.ReProvisionDeviceIdsSuccess",
      "KeymasterProvisionTest.ProductionModeProvisionFails",
      "KeymasterProvisionTest.InvalidDeviceIdFails",
      "KeymasterProvisionTest.MaxDeviceIdSuccess",
      "KeymasterProvisionTest.NoMeidSuccess",
      "NuggetCoreTest.GetUartPassthruInBootloader",
      "NuggetCoreTest.EnableUartPassthruInBootloader",
      "NuggetCoreTest.DisableUartPassthruInBootloader",
      "NuggetOsTest.NoticePing",
      "NuggetOsTest.InvalidMessageType",
      "NuggetOsTest.Sequence",
      "NuggetOsTest.Echo",
      "NuggetOsTest.AesCbc",
      "NuggetOsTest.Trng",
      "WeaverTest.ProductionResetWipesUserData",
      "AvbTest.*",
      "ImportKeyTest.*",
      "ImportWrappedKeyTest.ImportSuccess",
  };

  testing::InitGoogleMock(&argc, argv);
#ifndef ANDROID
  google::ParseCommandLineFlags(&argc, &argv, true);
#endif  // ANDROID

  if (FLAGS_list_slow_tests) {
    std::cout << "Slow tests:\n";
    for (const auto& test : slow_tests) {
      std::cout << "  " << test << "\n";
    }
    std::cout.flush();
    exit(0);
  }

  std::stringstream ss;
  if (FLAGS_disable_slow_tests) {
    generate_disabled_test_list(slow_tests, &ss);
  }
  if (FLAGS_release_tests) {
    generate_disabled_test_list(disabled_for_release_tests, &ss);
  }

  if (FLAGS_disable_slow_tests || FLAGS_release_tests) {
    ::testing::GTEST_FLAG(filter) = ss.str();
  }

  return RUN_ALL_TESTS();
}
