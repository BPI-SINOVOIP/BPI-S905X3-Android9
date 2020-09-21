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

#include "utils.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::Return;

namespace android {
namespace vintf {
namespace details {

class MockFileFetcher : public FileFetcher {
   public:
    MockFileFetcher() {
        // By default call through to the original.
        ON_CALL(*this, fetch(_, _)).WillByDefault(Invoke([this](const auto& path, auto& fetched) {
            return real_.fetchInternal(path, fetched, nullptr);
        }));
        ON_CALL(*this, listFiles(_, _, _))
            .WillByDefault(
                Invoke([this](const std::string& path, std::vector<std::string>* out,
                              std::string* error) { return real_.listFiles(path, out, error); }));
    }

    MOCK_METHOD2(fetch, status_t(const std::string& path, std::string& fetched));
    MOCK_METHOD3(listFiles, status_t(const std::string&, std::vector<std::string>*, std::string*));

    status_t fetch(const std::string& path, std::string& fetched, std::string*) override final {
        return fetch(path, fetched);
    }

   private:
    FileFetcher real_;
};

class MockPartitionMounter : public PartitionMounter {
   public:
    MockPartitionMounter() {
        ON_CALL(*this, mountSystem()).WillByDefault(Invoke([&] {
            systemMounted_ = true;
            return OK;
        }));
        ON_CALL(*this, umountSystem()).WillByDefault(Invoke([&] {
            systemMounted_ = false;
            return OK;
        }));
        ON_CALL(*this, mountVendor()).WillByDefault(Invoke([&] {
            vendorMounted_ = true;
            return OK;
        }));
        ON_CALL(*this, umountVendor()).WillByDefault(Invoke([&] {
            vendorMounted_ = false;
            return OK;
        }));
    }
    MOCK_CONST_METHOD0(mountSystem, status_t());
    MOCK_CONST_METHOD0(umountSystem, status_t());
    MOCK_CONST_METHOD0(mountVendor, status_t());
    MOCK_CONST_METHOD0(umountVendor, status_t());

    bool systemMounted() const { return systemMounted_; }
    bool vendorMounted() const { return vendorMounted_; }

    void reset() {
        systemMounted_ = false;
        vendorMounted_ = false;
    }

   private:
    bool systemMounted_;
    bool vendorMounted_;
};

class MockRuntimeInfo : public RuntimeInfo {
   public:
    MockRuntimeInfo() {
        ON_CALL(*this, fetchAllInformation(_))
            .WillByDefault(Invoke(this, &MockRuntimeInfo::doFetch));
    }
    MOCK_METHOD1(fetchAllInformation, status_t(RuntimeInfo::FetchFlags));
    status_t doFetch(RuntimeInfo::FetchFlags flags);
    void failNextFetch() { failNextFetch_ = true; }

   private:
    bool failNextFetch_ = false;
};
class MockRuntimeInfoFactory : public ObjectFactory<RuntimeInfo> {
   public:
    MockRuntimeInfoFactory(const std::shared_ptr<MockRuntimeInfo>& info) { object_ = info; }
    std::shared_ptr<RuntimeInfo> make_shared() const override { return object_; }
    std::shared_ptr<MockRuntimeInfo> getInfo() const { return object_; }

   private:
    std::shared_ptr<MockRuntimeInfo> object_;
};

class MockPropertyFetcher : public PropertyFetcher {
   public:
    MockPropertyFetcher();
    MOCK_CONST_METHOD2(getProperty, std::string(const std::string&, const std::string&));

   private:
    PropertyFetcher real_;
};
extern MockPropertyFetcher* gPropertyFetcher;

}  // namespace details
}  // namespace vintf
}  // namespace android
