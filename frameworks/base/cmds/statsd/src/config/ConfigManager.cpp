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

#define DEBUG false  // STOPSHIP if true
#include "Log.h"

#include "config/ConfigManager.h"
#include "storage/StorageManager.h"

#include "guardrail/StatsdStats.h"
#include "stats_log_util.h"
#include "stats_util.h"
#include "stats_log_util.h"

#include <android-base/file.h>
#include <dirent.h>
#include <stdio.h>
#include <vector>
#include "android-base/stringprintf.h"

namespace android {
namespace os {
namespace statsd {

using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

#define STATS_SERVICE_DIR "/data/misc/stats-service"

using android::base::StringPrintf;
using std::unique_ptr;

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {
}

void ConfigManager::Startup() {
    map<ConfigKey, StatsdConfig> configsFromDisk;
    StorageManager::readConfigFromDisk(configsFromDisk);
    for (const auto& pair : configsFromDisk) {
        UpdateConfig(pair.first, pair.second);
    }
}

void ConfigManager::StartupForTest() {
    // Dummy function to avoid reading configs from disks for tests.
}

void ConfigManager::AddListener(const sp<ConfigListener>& listener) {
    lock_guard<mutex> lock(mMutex);
    mListeners.push_back(listener);
}

void ConfigManager::UpdateConfig(const ConfigKey& key, const StatsdConfig& config) {
    vector<sp<ConfigListener>> broadcastList;
    {
        lock_guard <mutex> lock(mMutex);

        const int numBytes = config.ByteSize();
        vector<uint8_t> buffer(numBytes);
        config.SerializeToArray(&buffer[0], numBytes);

        auto uidIt = mConfigs.find(key.GetUid());
        // GuardRail: Limit the number of configs per uid.
        if (uidIt != mConfigs.end()) {
            auto it = uidIt->second.find(key);
            if (it == uidIt->second.end() &&
                uidIt->second.size() >= StatsdStats::kMaxConfigCountPerUid) {
                ALOGE("ConfigManager: uid %d has exceeded the config count limit", key.GetUid());
                return;
            }
        }

        // Check if it's a duplicate config.
        if (uidIt != mConfigs.end() && uidIt->second.find(key) != uidIt->second.end() &&
            StorageManager::hasIdenticalConfig(key, buffer)) {
            // This is a duplicate config.
            ALOGI("ConfigManager This is a duplicate config %s", key.ToString().c_str());
            // Update saved file on disk. We still update timestamp of file when
            // there exists a duplicate configuration to avoid garbage collection.
            update_saved_configs_locked(key, buffer, numBytes);
            return;
        }

        // Update saved file on disk.
        update_saved_configs_locked(key, buffer, numBytes);

        // Add to set.
        mConfigs[key.GetUid()].insert(key);

        for (sp<ConfigListener> listener : mListeners) {
            broadcastList.push_back(listener);
        }
    }

    const int64_t timestampNs = getElapsedRealtimeNs();
    // Tell everyone
    for (sp<ConfigListener> listener : broadcastList) {
        listener->OnConfigUpdated(timestampNs, key, config);
    }
}

void ConfigManager::SetConfigReceiver(const ConfigKey& key, const sp<IBinder>& intentSender) {
    lock_guard<mutex> lock(mMutex);
    mConfigReceivers[key] = intentSender;
}

void ConfigManager::RemoveConfigReceiver(const ConfigKey& key) {
    lock_guard<mutex> lock(mMutex);
    mConfigReceivers.erase(key);
}

void ConfigManager::RemoveConfig(const ConfigKey& key) {
    vector<sp<ConfigListener>> broadcastList;
    {
        lock_guard <mutex> lock(mMutex);

        auto uidIt = mConfigs.find(key.GetUid());
        if (uidIt != mConfigs.end() && uidIt->second.find(key) != uidIt->second.end()) {
            // Remove from map
            uidIt->second.erase(key);
            for (sp<ConfigListener> listener : mListeners) {
                broadcastList.push_back(listener);
            }
        }

        auto itReceiver = mConfigReceivers.find(key);
        if (itReceiver != mConfigReceivers.end()) {
            // Remove from map
            mConfigReceivers.erase(itReceiver);
        }

        // Remove from disk. There can still be a lingering file on disk so we check
        // whether or not the config was on memory.
        remove_saved_configs(key);
    }

    for (sp<ConfigListener> listener:broadcastList) {
        listener->OnConfigRemoved(key);
    }
}

void ConfigManager::remove_saved_configs(const ConfigKey& key) {
    string suffix = StringPrintf("%d_%lld", key.GetUid(), (long long)key.GetId());
    StorageManager::deleteSuffixedFiles(STATS_SERVICE_DIR, suffix.c_str());
}

void ConfigManager::RemoveConfigs(int uid) {
    vector<ConfigKey> removed;
    vector<sp<ConfigListener>> broadcastList;
    {
        lock_guard <mutex> lock(mMutex);

        auto uidIt = mConfigs.find(uid);
        if (uidIt == mConfigs.end()) {
            return;
        }

        for (auto it = uidIt->second.begin(); it != uidIt->second.end(); ++it) {
            // Remove from map
                remove_saved_configs(*it);
                removed.push_back(*it);
                mConfigReceivers.erase(*it);
        }

        mConfigs.erase(uidIt);

        for (sp<ConfigListener> listener : mListeners) {
            broadcastList.push_back(listener);
        }
    }

    // Remove separately so if they do anything in the callback they can't mess up our iteration.
    for (auto& key : removed) {
        // Tell everyone
        for (sp<ConfigListener> listener:broadcastList) {
            listener->OnConfigRemoved(key);
        }
    }
}

void ConfigManager::RemoveAllConfigs() {
    vector<ConfigKey> removed;
    vector<sp<ConfigListener>> broadcastList;
    {
        lock_guard <mutex> lock(mMutex);

        for (auto uidIt = mConfigs.begin(); uidIt != mConfigs.end();) {
            for (auto it = uidIt->second.begin(); it != uidIt->second.end();) {
                // Remove from map
                removed.push_back(*it);
                it = uidIt->second.erase(it);
            }
            uidIt = mConfigs.erase(uidIt);
        }

        mConfigReceivers.clear();
        for (sp<ConfigListener> listener : mListeners) {
            broadcastList.push_back(listener);
        }
    }

    // Remove separately so if they do anything in the callback they can't mess up our iteration.
    for (auto& key : removed) {
        // Tell everyone
        for (sp<ConfigListener> listener:broadcastList) {
            listener->OnConfigRemoved(key);
        }
    }
}

vector<ConfigKey> ConfigManager::GetAllConfigKeys() const {
    lock_guard<mutex> lock(mMutex);

    vector<ConfigKey> ret;
    for (auto uidIt = mConfigs.cbegin(); uidIt != mConfigs.cend(); ++uidIt) {
        for (auto it = uidIt->second.cbegin(); it != uidIt->second.cend(); ++it) {
            ret.push_back(*it);
        }
    }
    return ret;
}

const sp<android::IBinder> ConfigManager::GetConfigReceiver(const ConfigKey& key) const {
    lock_guard<mutex> lock(mMutex);

    auto it = mConfigReceivers.find(key);
    if (it == mConfigReceivers.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

void ConfigManager::Dump(FILE* out) {
    lock_guard<mutex> lock(mMutex);

    fprintf(out, "CONFIGURATIONS\n");
    fprintf(out, "     uid name\n");
    for (auto uidIt = mConfigs.cbegin(); uidIt != mConfigs.cend(); ++uidIt) {
        for (auto it = uidIt->second.cbegin(); it != uidIt->second.cend(); ++it) {
            fprintf(out, "  %6d %lld\n", it->GetUid(), (long long)it->GetId());
            auto receiverIt = mConfigReceivers.find(*it);
            if (receiverIt != mConfigReceivers.end()) {
                fprintf(out, "    -> received by PendingIntent as binder\n");
            }
        }
    }
}

void ConfigManager::update_saved_configs_locked(const ConfigKey& key,
                                                const vector<uint8_t>& buffer,
                                                const int numBytes) {
    // If there is a pre-existing config with same key we should first delete it.
    remove_saved_configs(key);

    // Then we save the latest config.
    string file_name =
        StringPrintf("%s/%ld_%d_%lld", STATS_SERVICE_DIR, time(nullptr),
                     key.GetUid(), (long long)key.GetId());
    StorageManager::writeFile(file_name.c_str(), &buffer[0], numBytes);
}

}  // namespace statsd
}  // namespace os
}  // namespace android
