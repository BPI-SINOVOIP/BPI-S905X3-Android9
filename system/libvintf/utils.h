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

#ifndef ANDROID_VINTF_UTILS_H
#define ANDROID_VINTF_UTILS_H

#include <dirent.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include <utils/Errors.h>
#include <vintf/RuntimeInfo.h>
#include <vintf/parse_xml.h>

namespace android {
namespace vintf {
namespace details {

// Return the file from the given location as a string.
//
// This class can be used to create a mock for overriding.
class FileFetcher {
   public:
    virtual ~FileFetcher() {}
    status_t fetchInternal(const std::string& path, std::string& fetched, std::string* error) {
        std::ifstream in;

        in.open(path);
        if (!in.is_open()) {
            if (error) {
                *error = "Cannot open " + path;
            }
            return NAME_NOT_FOUND;
        }

        std::stringstream ss;
        ss << in.rdbuf();
        fetched = ss.str();

        return OK;
    }
    virtual status_t fetch(const std::string& path, std::string& fetched, std::string* error) {
        return fetchInternal(path, fetched, error);
    }
    virtual status_t fetch(const std::string& path, std::string& fetched) {
        return fetchInternal(path, fetched, nullptr);
    }
    virtual status_t listFiles(const std::string& path, std::vector<std::string>* out,
                               std::string* error) {
        std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(path.c_str()), closedir);
        if (!dir) {
            if (error) {
                *error = "Cannot open " + path;
            }
            return NAME_NOT_FOUND;
        }

        dirent* dp;
        while ((dp = readdir(dir.get())) != nullptr) {
            if (dp->d_type != DT_DIR) {
                out->push_back(dp->d_name);
            }
        }
        return OK;
    }
};

extern FileFetcher* gFetcher;

class PartitionMounter {
   public:
    virtual ~PartitionMounter() {}
    virtual status_t mountSystem() const { return OK; }
    virtual status_t mountVendor() const { return OK; }
    virtual status_t umountSystem() const { return OK; }
    virtual status_t umountVendor() const { return OK; }
};

extern PartitionMounter* gPartitionMounter;

template <typename T>
status_t fetchAllInformation(const std::string& path, const XmlConverter<T>& converter,
                             T* outObject, std::string* error = nullptr) {
    std::string info;

    if (gFetcher == nullptr) {
        // Should never happen.
        return NO_INIT;
    }

    status_t result = gFetcher->fetch(path, info, error);

    if (result != OK) {
        return result;
    }

    bool success = converter(outObject, info, error);
    if (!success) {
        if (error) {
            *error = "Illformed file: " + path + ": " + *error;
        }
        return BAD_VALUE;
    }
    return OK;
}

template <typename T>
class ObjectFactory {
   public:
    virtual ~ObjectFactory() = default;
    virtual std::shared_ptr<T> make_shared() const { return std::make_shared<T>(); }
};
extern ObjectFactory<RuntimeInfo>* gRuntimeInfoFactory;

// TODO(b/70628538): Do not infer from Shipping API level.
inline Level convertFromApiLevel(size_t apiLevel) {
    if (apiLevel < 26) {
        return Level::LEGACY;
    } else if (apiLevel == 26) {
        return Level::O;
    } else if (apiLevel == 27) {
        return Level::O_MR1;
    } else {
        return Level::UNSPECIFIED;
    }
}

class PropertyFetcher {
   public:
    virtual ~PropertyFetcher() = default;
    virtual std::string getProperty(const std::string& key,
                                    const std::string& defaultValue = "") const;
    virtual uint64_t getUintProperty(const std::string& key, uint64_t defaultValue,
                                     uint64_t max = UINT64_MAX) const;
    virtual bool getBoolProperty(const std::string& key, bool defaultValue) const;
};

const PropertyFetcher& getPropertyFetcher();

}  // namespace details
}  // namespace vintf
}  // namespace android



#endif
