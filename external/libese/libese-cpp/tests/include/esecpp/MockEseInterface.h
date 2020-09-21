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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <esecpp/EseInterface.h>

namespace android {

using ::testing::_;
using ::testing::ContainerEq;
using ::testing::Invoke;

struct MockEseInterface : public EseInterface {
    MOCK_METHOD0(init, void());
    MOCK_METHOD0(open, int());
    MOCK_METHOD0(close, void());
    MOCK_METHOD0(name, const char*());
    MOCK_METHOD2(transceive, int(const std::vector<uint8_t>&, std::vector<uint8_t>&));
    MOCK_METHOD0(error, bool());
    MOCK_METHOD0(error_message, const char*());
    MOCK_METHOD0(error_code, int());
};

inline auto EseReceive(const std::vector<uint8_t>& response) {
    return Invoke([response](const std::vector<uint8_t>& /* tx */, std::vector<uint8_t>& rx) {
        const auto end = (rx.size() >= response.size())
                ? response.end() : (response.begin() + rx.size());
        std::copy(response.begin(), end, rx.begin());
        return (int) response.size();
    });
}

// Returns a mocked device that expects a command, will pass the response to the
// callback and finally returns the given value.
// Needs to allocate on heap as it can't copy/move the mock.
inline std::unique_ptr<MockEseInterface> singleCommandEseMock(
        const std::vector<uint8_t>& command, const std::vector<uint8_t>& response) {
    auto mockEse = std::make_unique<MockEseInterface>();
    EXPECT_CALL(*mockEse, transceive(ContainerEq(command), _)).WillOnce(EseReceive(response));
    return mockEse;
}

} // namespace android
