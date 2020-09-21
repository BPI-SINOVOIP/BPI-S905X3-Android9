/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "vibrator_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/vibrator/1.0/types.h>
#include <android/hardware/vibrator/1.2/IVibrator.h>
#include <android/hardware/vibrator/1.2/types.h>
#include <unistd.h>

using ::android::hardware::vibrator::V1_0::Status;
using ::android::hardware::vibrator::V1_0::EffectStrength;
using ::android::hardware::vibrator::V1_2::Effect;
using ::android::hardware::vibrator::V1_2::IVibrator;
using ::android::hardware::hidl_enum_iterator;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define EXPECT_OK(ret) ASSERT_TRUE((ret).isOk())

// Test environment for Vibrator HIDL HAL.
class VibratorHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static VibratorHidlEnvironment* Instance() {
        static VibratorHidlEnvironment* instance = new VibratorHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IVibrator>(); }

   private:
    VibratorHidlEnvironment() {}
};

// The main test class for VIBRATOR HIDL HAL 1.2.
class VibratorHidlTest_1_2 : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        vibrator = ::testing::VtsHalHidlTargetTestBase::getService<IVibrator>(
            VibratorHidlEnvironment::Instance()->getServiceName<IVibrator>());
        ASSERT_NE(vibrator, nullptr);
    }

    virtual void TearDown() override {}

    sp<IVibrator> vibrator;
};

static void validatePerformEffect(Status status, uint32_t lengthMs) {
    ASSERT_TRUE(status == Status::OK || status == Status::UNSUPPORTED_OPERATION);
    if (status == Status::OK) {
        ASSERT_LT(static_cast<uint32_t>(0), lengthMs)
            << "Effects that return OK must return a positive duration";
    } else {
        ASSERT_EQ(static_cast<uint32_t>(0), lengthMs)
            << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
    }
}

static void validatePerformEffectBadInput(Status status, uint32_t lengthMs) {
    ASSERT_EQ(Status::UNSUPPORTED_OPERATION, status);
    ASSERT_EQ(static_cast<uint32_t>(0), lengthMs)
        << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
}

/*
 * Test to make sure effects within the valid range return are either supported and return OK with
 * a valid duration, or are unsupported and return UNSUPPORTED_OPERATION with a duration of 0.
 */
TEST_F(VibratorHidlTest_1_2, PerformEffect_1_2) {
    for (const auto& effect : hidl_enum_iterator<Effect>()) {
        for (const auto& strength : hidl_enum_iterator<EffectStrength>()) {
            EXPECT_OK(vibrator->perform_1_2(effect, strength, validatePerformEffect));
        }
    }
}

/*
 * Test to make sure effect values above the valid range are rejected.
 */
TEST_F(VibratorHidlTest_1_2, PerformEffect_1_2_BadEffects_AboveValidRange) {
    Effect effect = *std::prev(hidl_enum_iterator<Effect>().end());
    Effect badEffect = static_cast<Effect>(static_cast<int32_t>(effect) + 1);
    EXPECT_OK(
        vibrator->perform_1_2(badEffect, EffectStrength::LIGHT, validatePerformEffectBadInput));
}

/*
 * Test to make sure effect values below the valid range are rejected.
 */
TEST_F(VibratorHidlTest_1_2, PerformEffect_1_2_BadEffects_BelowValidRange) {
    Effect effect = *hidl_enum_iterator<Effect>().begin();
    Effect badEffect = static_cast<Effect>(static_cast<int32_t>(effect) - 1);
    EXPECT_OK(
        vibrator->perform_1_2(badEffect, EffectStrength::LIGHT, validatePerformEffectBadInput));
}

/*
 * Test to make sure strength values above the valid range are rejected.
 */
TEST_F(VibratorHidlTest_1_2, PerformEffect_1_2_BadStrength_AboveValidRange) {
    EffectStrength strength = *std::prev(hidl_enum_iterator<EffectStrength>().end());
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) + 1);
    EXPECT_OK(vibrator->perform_1_2(Effect::THUD, badStrength, validatePerformEffectBadInput));
}

/*
 * Test to make sure strength values below the valid range are rejected.
 */
TEST_F(VibratorHidlTest_1_2, PerformEffect_1_2_BadStrength_BelowValidRange) {
    EffectStrength strength = *hidl_enum_iterator<EffectStrength>().begin();
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) - 1);
    EXPECT_OK(vibrator->perform_1_2(Effect::THUD, badStrength, validatePerformEffectBadInput));
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(VibratorHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    VibratorHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
