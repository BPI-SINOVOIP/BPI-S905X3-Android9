/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "VtsTestabilityChecker.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vintf/CompatibilityMatrix.h>
#include <vintf/HalManifest.h>
#include <vintf/parse_xml.h>

using namespace testing;

using android::hidl::base::V1_0::IBase;
using android::hidl::manager::V1_0::IServiceManager;
using android::hidl::manager::V1_0::IServiceNotification;
using android::hardware::hidl_array;
using android::hardware::hidl_death_recipient;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::vintf::Arch;
using android::vintf::CompatibilityMatrix;
using android::vintf::HalManifest;
using android::vintf::ManifestHal;
using android::vintf::MatrixHal;
using android::vintf::Version;
using android::vintf::XmlConverter;
using android::vintf::gCompatibilityMatrixConverter;
using android::vintf::gHalManifestConverter;
using std::set;
using std::string;

namespace android {
namespace vts {

class MockServiceManager : public IServiceManager {
 public:
  template <typename T>
  using R = ::android::hardware::Return<T>;
  using String = const hidl_string &;
  ~MockServiceManager() = default;

#define MOCK_METHOD_CB(name) \
  MOCK_METHOD1(name, R<void>(IServiceManager::name##_cb))

  MOCK_METHOD2(get, R<sp<IBase>>(String, String));
  MOCK_METHOD2(add, R<bool>(String, const sp<IBase> &));
  MOCK_METHOD2(getTransport, R<IServiceManager::Transport>(String, String));
  MOCK_METHOD_CB(list);
  MOCK_METHOD2(listByInterface, R<void>(String, listByInterface_cb));
  MOCK_METHOD3(registerForNotifications,
               R<bool>(String, String, const sp<IServiceNotification> &));
  MOCK_METHOD_CB(debugDump);
  MOCK_METHOD2(registerPassthroughClient, R<void>(String, String));
  MOCK_METHOD_CB(interfaceChain);
  MOCK_METHOD2(debug,
               R<void>(const hidl_handle &, const hidl_vec<hidl_string> &));
  MOCK_METHOD_CB(interfaceDescriptor);
  MOCK_METHOD_CB(getHashChain);
  MOCK_METHOD0(setHalInstrumentation, R<void>());
  MOCK_METHOD2(linkToDeath,
               R<bool>(const sp<hidl_death_recipient> &, uint64_t));
  MOCK_METHOD0(ping, R<void>());
  MOCK_METHOD_CB(getDebugInfo);
  MOCK_METHOD0(notifySyspropsChanged, R<void>());
  MOCK_METHOD1(unlinkToDeath, R<bool>(const sp<hidl_death_recipient> &));
};

class VtsTestabilityCheckerTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    test_cm_ = testFrameworkCompMatrix();
    test_fm_ = testFrameworkManfiest();
    test_vm_ = testDeviceManifest();
    sm_ = new NiceMock<MockServiceManager>();
    checker_.reset(
        new VtsTestabilityChecker(&test_cm_, &test_fm_, &test_vm_, sm_));
  }
  virtual void TearDown() override {}

  HalManifest testDeviceManifest() {
    HalManifest vm;
    string xml =
        "<manifest version=\"1.0\" type=\"framework\">\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.audio</name>\n"
        "        <transport arch=\"32\">passthrough</transport>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IAudio</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.camera</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.2</version>\n"
        "        <version>2.5</version>\n"
        "        <interface>\n"
        "            <name>ICamera</name>\n"
        "            <instance>legacy/0</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>IBetterCamera</name>\n"
        "            <instance>camera</instance>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.drm</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IDrm</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.nfc</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>INfc</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.renderscript</name>\n"
        "        <transport arch=\"32+64\">passthrough</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IRenderscript</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.vibrator</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IVibrator</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</manifest>\n";
    gHalManifestConverter(&vm, xml);
    return vm;
  }

  HalManifest testFrameworkManfiest() {
    HalManifest fm;
    string xml =
        "<manifest version=\"1.0\" type=\"framework\">\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.nfc</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>INfc</name>\n"
        "            <instance>default</instance>\n"
        "            <instance>fnfc</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</manifest>\n";
    gHalManifestConverter(&fm, xml);
    return fm;
  }

  CompatibilityMatrix testFrameworkCompMatrix() {
    CompatibilityMatrix cm;
    string xml =
        "<compatibility-matrix version=\"1.0\" type=\"framework\">\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.audio</name>\n"
        "        <version>2.0-1</version>\n"
        "        <interface>\n"
        "            <name>IAudio</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.camera</name>\n"
        "        <version>2.2-3</version>\n"
        "        <version>4.5-6</version>\n"
        "        <interface>\n"
        "            <name>ICamera</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>IBetterCamera</name>\n"
        "            <instance>camera</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"false\">\n"
        "        <name>android.hardware.drm</name>\n"
        "        <version>1.0-1</version>\n"
        "        <interface>\n"
        "            <name>IDrm</name>\n"
        "            <instance>default</instance>\n"
        "            <instance>drm</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>IDrmTest</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"false\">\n"
        "        <name>android.hardware.light</name>\n"
        "        <version>2.0-1</version>\n"
        "        <interface>\n"
        "            <name>ILight</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.nfc</name>\n"
        "        <version>1.0-2</version>\n"
        "        <interface>\n"
        "            <name>INfc</name>\n"
        "            <instance>default</instance>\n"
        "            <instance>nfc</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>INfcTest</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.radio</name>\n"
        "        <version>1.0-1</version>\n"
        "        <interface>\n"
        "            <name>IRadio</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"false\">\n"
        "        <name>android.hardware.vibrator</name>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IVibrator</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</compatibility-matrix>\n";
    gCompatibilityMatrixConverter(&cm, xml);
    return cm;
  }

 protected:
  CompatibilityMatrix test_cm_;
  HalManifest test_fm_;
  HalManifest test_vm_;
  sp<MockServiceManager> sm_;
  std::unique_ptr<VtsTestabilityChecker> checker_;
};

TEST_F(VtsTestabilityCheckerTest, CheckComplianceTest) {
  set<string> instances;
  // Check non-existent hal.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "nonexistent", {1, 0}, "None", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported version by vendor.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.nfc", {2, 0}, "INfc", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported interface by vendor.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.nfc", {1, 0}, "INfcTest", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported arch by vendor.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.audio", {1, 0}, "IAudio", Arch::ARCH_64, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal claimed by framework but not supported by vendor (error case).
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.light", {2, 0}, "ILight", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal interface claimed by framework but not supported by vendor (error
  // case).
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.drm", {1, 0}, "IDrmTest", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal claimed by framework and supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest("android.hardware.vibrator",
                                                  {1, 0}, "IVibrator",
                                                  Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal not claimed by framework but supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.renderscript", {1, 0}, "IRenderscript", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal with version not claimed by framework by supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest("android.hardware.vibrator",
                                                  {1, 0}, "IVibrator",
                                                  Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal with instance not claimed by framework but supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {2, 2}, "ICamera", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"legacy/0"})));

  // Check hal with additional vendor instance not claimed by framework.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest("android.hardware.camera",
                                                  {1, 2}, "IBetterCamera",
                                                  Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default", "camera"})));

  // Check hal supported by both framework and vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.nfc", {1, 0}, "INfc", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default", "fnfc"})));

  // Check hal instance claimed by framework but not supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.drm", {2, 0}, "IDrm", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.vibrator", {1, 0}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());
}

TEST_F(VtsTestabilityCheckerTest, CheckNonComplianceTest) {
  set<string> instances;
  ON_CALL(*sm_, listByInterface(_, _))
      .WillByDefault(
          Invoke([](hidl_string str, IServiceManager::listByInterface_cb cb) {
            if (str == "android.hardware.foo@1.0::IFoo") {
              cb({"default", "footest"});
            } else if (str == "android.hardware.nfc@3.0::INfc") {
              cb({"default"});
            } else if (str == "android.hardware.drm@2.0::IDrm") {
              cb({"drmtest"});
            }
            return hardware::Void();
          }));

  ON_CALL(*sm_, list(_)).WillByDefault(Invoke([](IServiceManager::list_cb cb) {
    cb({"android.hardware.foo@1.0::IFoo/default",
        "android.hardware.foo@1.0::IFoo/footest",
        "android.hardware.nfc@3.0::INfc/default",
        "android.hardware.drm@2.0::IDrm/drmtest"});
    return hardware::Void();
  }));

  // Check non-existent hal.
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "non-existent", {1, 0}, "None", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported version by vendor.
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.nfc", {2, 0}, "INfc", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported interface by vendor.
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.nfc", {1, 0}, "INfcTest", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported arch by vendor.
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.audio", {1, 0}, "IAudio", Arch::ARCH_64, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal claimed by framework but not supported by vendor (error case).
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.light", {2, 0}, "ILight", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal interface claimed by framework but not supported by vendor (error
  // case).
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.drm", {1, 0}, "IDrmTest", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal claimed by framework and supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.vibrator", {1, 0}, "IVibrator", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal not claimed by framework but supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.renderscript", {1, 0}, "IRenderscript", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal with version not claimed by framework by supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.vibrator", {1, 0}, "IVibrator", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal with instance not claimed by framework but supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.camera", {2, 2}, "ICamera", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"legacy/0"})));

  // Check hal with additional vendor instance not claimed by framework.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.camera", {1, 2}, "IBetterCamera", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default", "camera"})));

  // Check hal supported by both framework and vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.nfc", {1, 0}, "INfc", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.vibrator", {1, 0}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal only registered with hwmanger.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.foo", {1, 0}, "IFoo", Arch::ARCH_EMPTY, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default", "footest"})));

  // Check hal with version only registered with hwmanger.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.nfc", {3, 0}, "INfc", Arch::ARCH_EMPTY, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check hal with additional instance registered with hwmanger.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.drm", {2, 0}, "IDrm", Arch::ARCH_EMPTY, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default", "drmtest"})));

  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.foo", {1, 0}, "", Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());
}

TEST_F(VtsTestabilityCheckerTest, CheckFrameworkCompatibleHalOptional) {
  set<string> instances;
  // Check non-existent hal.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "nonexistent", {1, 0}, "None", Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal not required by framework
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.renderscript", {1, 0}, "IRenderscript",
      Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal with unsupported version.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.camera", {1, 0}, "ICamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal with non-existent interface.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.camera", {1, 2}, "None", Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal not supported by vendor.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.radio", {1, 0}, "IRadio", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal with version not supported by vendor.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.camera", {4, 5}, "ICamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal with interface not supported by vendor.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.nfc", {4, 5}, "INfcTest", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an option passthrough hal with unsupported arch.
  EXPECT_FALSE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.audio", {2, 0}, "IAudio", Arch::ARCH_64, &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal supported by vendor but with no compatible
  // instance.
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.camera", {2, 2}, "ICamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.camera", {2, 2}, "IBetterCamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"camera"})));

  // Check an optional passthrough hal supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.audio", {2, 0}, "IAudio", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.camera", {2, 2}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());
}

TEST_F(VtsTestabilityCheckerTest, CheckFrameworkCompatibleHalRequired) {
  set<string> instances;
  // Check a required hal not supported by vendor.
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.light", {2, 0}, "ILight", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check a required hal with version not supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal("android.hardware.vibrator",
                                                    {2, 0}, "IVibrator",
                                                    Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check a required hal with interface not supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.drm", {1, 0}, "IDrmTest", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default"})));

  // Check a required hal supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.drm", {1, 0}, "IDrm", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ContainerEq(set<string>({"default", "drm"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckFrameworkCompatibleHal(
      "android.hardware.vibrator", {2, 0}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());
}

}  // namespace vts
}  // namespace android

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
