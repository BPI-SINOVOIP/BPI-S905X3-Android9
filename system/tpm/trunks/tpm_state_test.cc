//
// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "trunks/mock_tpm.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_state_impl.h"
#include "trunks/trunks_factory_for_test.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;
using testing::WithArgs;

namespace trunks {

// A test fixture for TpmState tests.
class TpmStateTest : public testing::Test {
 public:
  TpmStateTest() = default;
  ~TpmStateTest() override = default;

  void SetUp() override {
    factory_.set_tpm(&mock_tpm_);
    // All auth set (i.e. IsOwned() -> true) and in lockout.
    fake_tpm_properties_[TPM_PT_PERMANENT] = 0x207;
    // Orderly shutdown, storage and endorsement enabled, platform disabled
    // (i.e. IsEnabled() -> true).
    fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = 0x80000006;
    fake_tpm_properties_[TPM_PT_LOCKOUT_COUNTER] = 2;
    fake_tpm_properties_[TPM_PT_MAX_AUTH_FAIL] = 5;
    fake_tpm_properties_[TPM_PT_LOCKOUT_INTERVAL] = 100;
    fake_tpm_properties_[TPM_PT_LOCKOUT_RECOVERY] = 200;
    fake_tpm_properties_[TPM_PT_NV_INDEX_MAX] = 2048;
    fake_tpm_properties_[TPM_PT_NV_BUFFER_MAX] = 2048;
    fake_algorithm_properties_[TPM_ALG_RSA] = 0x9;
    fake_algorithm_properties_[TPM_ALG_ECC] = 0x9;
    EXPECT_CALL(mock_tpm_, GetCapabilitySync(_, _, _, _, _, _))
        .WillRepeatedly(Invoke(this, &TpmStateTest::FakeGetCapability));
  }

 protected:
  TPM_RC FakeGetCapability(const TPM_CAP& capability,
                           const UINT32& property,
                           const UINT32& property_count,
                           TPMI_YES_NO* more_data,
                           TPMS_CAPABILITY_DATA* capability_data,
                           AuthorizationDelegate* /* not_used */) {
    // Return only two properties at a time, this will exercise the more_data
    // logic.
    constexpr uint32_t kMaxProperties = 2;
    *more_data = NO;
    memset(capability_data, 0, sizeof(TPMS_CAPABILITY_DATA));
    capability_data->capability = capability;
    TPMU_CAPABILITIES& data = capability_data->data;
    if (capability == TPM_CAP_TPM_PROPERTIES) {
      // TPM properties get returned one group at a time, mimic this.
      uint32_t group = (property >> 8);
      uint32_t stop = PT_GROUP * (group + 1);
      for (uint32_t i = property; i < stop; ++i) {
        if (fake_tpm_properties_.count(i) > 0) {
          if (data.tpm_properties.count == kMaxProperties ||
              data.tpm_properties.count == property_count) {
            // There are more properties than we can fit.
            *more_data = YES;
            break;
          }
          data.tpm_properties.tpm_property[data.tpm_properties.count].property =
              i;
          data.tpm_properties.tpm_property[data.tpm_properties.count].value =
              fake_tpm_properties_[i];
          data.tpm_properties.count++;
        }
      }
    } else if (capability == TPM_CAP_ALGS) {
      // Algorithm properties.
      uint32_t stop = TPM_ALG_LAST + 1;
      for (uint32_t i = property; i < stop; ++i) {
        if (fake_algorithm_properties_.count(i) > 0) {
          if (data.algorithms.count == kMaxProperties ||
              data.algorithms.count == property_count) {
            // There are more properties than we can fit.
            *more_data = YES;
            break;
          }
          data.algorithms.alg_properties[data.algorithms.count].alg = i;
          data.algorithms.alg_properties[data.algorithms.count].alg_properties =
              fake_algorithm_properties_[i];
          data.algorithms.count++;
        }
      }
    }
    return TPM_RC_SUCCESS;
  }

  TrunksFactoryForTest factory_;
  NiceMock<MockTpm> mock_tpm_;
  std::map<TPM_PT, uint32_t> fake_tpm_properties_;
  std::map<TPM_ALG_ID, TPMA_ALGORITHM> fake_algorithm_properties_;
};

TEST(TpmState_DeathTest, NotInitialized) {
  TrunksFactoryForTest factory;
  TpmStateImpl tpm_state(factory);
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsOwnerPasswordSet(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsEndorsementPasswordSet(),
                            "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsLockoutPasswordSet(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsOwned(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsInLockout(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsPlatformHierarchyEnabled(),
                            "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsStorageHierarchyEnabled(),
                            "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsEndorsementHierarchyEnabled(),
                            "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsEnabled(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.WasShutdownOrderly(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsRSASupported(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.IsECCSupported(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetLockoutCounter(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetLockoutThreshold(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetLockoutInterval(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetLockoutRecovery(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetMaxNVSize(), "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetTpmProperty(0, nullptr),
                            "Check failed");
  EXPECT_DEATH_IF_SUPPORTED(tpm_state.GetAlgorithmProperties(0, nullptr),
                            "Check failed");
}

TEST_F(TpmStateTest, FlagsClear) {
  fake_tpm_properties_[TPM_PT_PERMANENT] = 0;
  fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = 0;
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsOwnerPasswordSet());
  EXPECT_FALSE(tpm_state.IsEndorsementPasswordSet());
  EXPECT_FALSE(tpm_state.IsLockoutPasswordSet());
  EXPECT_FALSE(tpm_state.IsInLockout());
  EXPECT_FALSE(tpm_state.IsOwned());
  EXPECT_FALSE(tpm_state.IsPlatformHierarchyEnabled());
  EXPECT_FALSE(tpm_state.IsStorageHierarchyEnabled());
  EXPECT_FALSE(tpm_state.IsEndorsementHierarchyEnabled());
  EXPECT_FALSE(tpm_state.WasShutdownOrderly());
}

TEST_F(TpmStateTest, FlagsSet) {
  fake_tpm_properties_[TPM_PT_PERMANENT] = ~0;
  fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = ~0;

  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_TRUE(tpm_state.IsOwnerPasswordSet());
  EXPECT_TRUE(tpm_state.IsEndorsementPasswordSet());
  EXPECT_TRUE(tpm_state.IsLockoutPasswordSet());
  EXPECT_TRUE(tpm_state.IsOwned());
  EXPECT_TRUE(tpm_state.IsInLockout());
  EXPECT_TRUE(tpm_state.IsPlatformHierarchyEnabled());
  EXPECT_TRUE(tpm_state.IsStorageHierarchyEnabled());
  EXPECT_TRUE(tpm_state.IsEndorsementHierarchyEnabled());
  EXPECT_TRUE(tpm_state.WasShutdownOrderly());
}

TEST_F(TpmStateTest, EnabledTpm) {
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_TRUE(tpm_state.IsEnabled());
  // All hierarchies enabled.
  fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = 0x7;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsEnabled());
  // All hierarchies disabled.
  fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = 0x0;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsEnabled());
  // Storage disabled.
  fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = 0x5;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsEnabled());
  // Endorsement disabled.
  fake_tpm_properties_[TPM_PT_STARTUP_CLEAR] = 0x3;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsEnabled());
}

TEST_F(TpmStateTest, OwnedTpm) {
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_TRUE(tpm_state.IsOwned());
  // All auth missing.
  fake_tpm_properties_[TPM_PT_PERMANENT] = 0x0;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsOwned());
  // Owner auth missing.
  fake_tpm_properties_[TPM_PT_PERMANENT] = 0x6;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsOwned());
  // Endorsement auth missing.
  fake_tpm_properties_[TPM_PT_PERMANENT] = 0x5;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsOwned());
  // Lockout auth missing.
  fake_tpm_properties_[TPM_PT_PERMANENT] = 0x3;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.IsOwned());
}

TEST_F(TpmStateTest, AlgorithmSupport) {
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_TRUE(tpm_state.IsRSASupported());
  EXPECT_TRUE(tpm_state.IsECCSupported());

  fake_algorithm_properties_.clear();
  // Use a new instance because algorithm properties will not be queried again.
  TpmStateImpl tpm_state2(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state2.Initialize());
  EXPECT_FALSE(tpm_state2.IsRSASupported());
  EXPECT_FALSE(tpm_state2.IsECCSupported());
}

TEST_F(TpmStateTest, LockoutValuePassthrough) {
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_EQ(tpm_state.GetLockoutCounter(),
            fake_tpm_properties_[TPM_PT_LOCKOUT_COUNTER]);
  EXPECT_EQ(tpm_state.GetLockoutThreshold(),
            fake_tpm_properties_[TPM_PT_MAX_AUTH_FAIL]);
  EXPECT_EQ(tpm_state.GetLockoutInterval(),
            fake_tpm_properties_[TPM_PT_LOCKOUT_INTERVAL]);
  EXPECT_EQ(tpm_state.GetLockoutRecovery(),
            fake_tpm_properties_[TPM_PT_LOCKOUT_RECOVERY]);

  fake_tpm_properties_[TPM_PT_LOCKOUT_COUNTER]++;
  fake_tpm_properties_[TPM_PT_MAX_AUTH_FAIL]++;
  fake_tpm_properties_[TPM_PT_LOCKOUT_INTERVAL]++;
  fake_tpm_properties_[TPM_PT_LOCKOUT_RECOVERY]++;
  // Refresh and check for the new values.
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_EQ(tpm_state.GetLockoutCounter(),
            fake_tpm_properties_[TPM_PT_LOCKOUT_COUNTER]);
  EXPECT_EQ(tpm_state.GetLockoutThreshold(),
            fake_tpm_properties_[TPM_PT_MAX_AUTH_FAIL]);
  EXPECT_EQ(tpm_state.GetLockoutInterval(),
            fake_tpm_properties_[TPM_PT_LOCKOUT_INTERVAL]);
  EXPECT_EQ(tpm_state.GetLockoutRecovery(),
            fake_tpm_properties_[TPM_PT_LOCKOUT_RECOVERY]);
}

TEST_F(TpmStateTest, MaxNVSize) {
  auto CheckMaxNVSize = [this]() {
    TpmStateImpl tpm_state(factory_);
    ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
    bool has_index = fake_tpm_properties_.count(TPM_PT_NV_INDEX_MAX) > 0;
    bool has_buffer = fake_tpm_properties_.count(TPM_PT_NV_BUFFER_MAX) > 0;
    if (has_index && has_buffer) {
      EXPECT_EQ(tpm_state.GetMaxNVSize(),
                std::min(fake_tpm_properties_[TPM_PT_NV_INDEX_MAX],
                         fake_tpm_properties_[TPM_PT_NV_BUFFER_MAX]));
    } else if (has_index) {
      EXPECT_EQ(tpm_state.GetMaxNVSize(),
                fake_tpm_properties_[TPM_PT_NV_INDEX_MAX]);
    } else if (has_buffer) {
      EXPECT_EQ(tpm_state.GetMaxNVSize(),
                fake_tpm_properties_[TPM_PT_NV_BUFFER_MAX]);
    } else {
      // Check for a reasonable default value. Brillo specs a minimum of 2048 so
      // it shouldn't be less than that.
      EXPECT_GE(tpm_state.GetMaxNVSize(), 2048u);
    }
  };
  // Check with the defaults (same index and buffer max).
  CheckMaxNVSize();
  // Check with lower buffer max.
  fake_tpm_properties_[TPM_PT_NV_INDEX_MAX] = 20;
  fake_tpm_properties_[TPM_PT_NV_BUFFER_MAX] = 10;
  CheckMaxNVSize();
  // Check with lower index max.
  fake_tpm_properties_[TPM_PT_NV_INDEX_MAX] = 10;
  fake_tpm_properties_[TPM_PT_NV_BUFFER_MAX] = 20;
  CheckMaxNVSize();
  // Check without index property.
  fake_tpm_properties_.erase(TPM_PT_NV_INDEX_MAX);
  fake_tpm_properties_[TPM_PT_NV_BUFFER_MAX] = 5;
  CheckMaxNVSize();
  // Check without buffer property.
  fake_tpm_properties_[TPM_PT_NV_INDEX_MAX] = 5;
  fake_tpm_properties_.erase(TPM_PT_NV_BUFFER_MAX);
  CheckMaxNVSize();
  // Check without any properties.
  fake_tpm_properties_.erase(TPM_PT_NV_INDEX_MAX);
  fake_tpm_properties_.erase(TPM_PT_NV_BUFFER_MAX);
  CheckMaxNVSize();
}

TEST_F(TpmStateTest, RawTpmProperty) {
  constexpr TPM_PT kProperty = 0x2FF;
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.GetTpmProperty(kProperty, nullptr));
  uint32_t value;
  EXPECT_FALSE(tpm_state.GetTpmProperty(kProperty, &value));

  fake_tpm_properties_[kProperty] = 1234;
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_TRUE(tpm_state.GetTpmProperty(kProperty, nullptr));
  EXPECT_TRUE(tpm_state.GetTpmProperty(kProperty, &value));
  EXPECT_EQ(value, fake_tpm_properties_[kProperty]);
}

TEST_F(TpmStateTest, RawAlgorithmProperties) {
  constexpr TPM_ALG_ID kAlgorithm = 0x39;
  TpmStateImpl tpm_state(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state.Initialize());
  EXPECT_FALSE(tpm_state.GetAlgorithmProperties(kAlgorithm, nullptr));
  uint32_t value;
  EXPECT_FALSE(tpm_state.GetAlgorithmProperties(kAlgorithm, &value));

  fake_algorithm_properties_[kAlgorithm] = 1234;
  TpmStateImpl tpm_state2(factory_);
  ASSERT_EQ(TPM_RC_SUCCESS, tpm_state2.Initialize());
  EXPECT_TRUE(tpm_state2.GetAlgorithmProperties(kAlgorithm, nullptr));
  EXPECT_TRUE(tpm_state2.GetAlgorithmProperties(kAlgorithm, &value));
  EXPECT_EQ(value, fake_algorithm_properties_[kAlgorithm]);
}

TEST_F(TpmStateTest, InitFailOnMissingPermanentFlags) {
  fake_tpm_properties_.erase(TPM_PT_PERMANENT);
  TpmStateImpl tpm_state(factory_);
  EXPECT_NE(TPM_RC_SUCCESS, tpm_state.Initialize());
}

TEST_F(TpmStateTest, InitFailOnMissingStartupClearFlags) {
  fake_tpm_properties_.erase(TPM_PT_STARTUP_CLEAR);
  TpmStateImpl tpm_state(factory_);
  EXPECT_NE(TPM_RC_SUCCESS, tpm_state.Initialize());
}

TEST_F(TpmStateTest, InitFailOnFailedTPMCommand) {
  EXPECT_CALL(mock_tpm_, GetCapabilitySync(_, _, _, _, _, _))
      .WillRepeatedly(Return(TPM_RC_FAILURE));
  TpmStateImpl tpm_state(factory_);
  EXPECT_EQ(TPM_RC_FAILURE, tpm_state.Initialize());
}

}  // namespace trunks
