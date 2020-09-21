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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "Models.h"
#include "VtsHalNeuralnetworks.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_1 {
namespace vts {
namespace functional {

// forward declarations
std::vector<Request> createRequests(const std::vector<MixedTypedExample>& examples);

// generate validation tests
#define VTS_CURRENT_TEST_CASE(TestName)                                           \
    TEST_F(ValidationTest, TestName) {                                            \
        const Model model = TestName::createTestModel();                          \
        const std::vector<Request> requests = createRequests(TestName::examples); \
        validateModel(model);                                                     \
        validateRequests(model, requests);                                        \
    }

FOR_EACH_TEST_MODEL(VTS_CURRENT_TEST_CASE)

#undef VTS_CURRENT_TEST_CASE

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
