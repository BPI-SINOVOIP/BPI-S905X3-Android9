
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

#include "utils.h"

// Default implementations for classes defined in utils.h

namespace android {
namespace vintf {
namespace details {

std::string PropertyFetcher::getProperty(const std::string&,
                                         const std::string& defaultValue) const {
    return defaultValue;
}

uint64_t PropertyFetcher::getUintProperty(const std::string&, uint64_t,
                                          uint64_t defaultValue) const {
    return defaultValue;
}

bool PropertyFetcher::getBoolProperty(const std::string&, bool defaultValue) const {
    return defaultValue;
}

}  // namespace details
}  // namespace vintf
}  // namespace android
