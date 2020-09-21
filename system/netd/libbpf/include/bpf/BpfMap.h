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

#ifndef BPF_BPFMAP_H
#define BPF_BPFMAP_H

#include <linux/bpf.h>

#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <utils/Log.h>
#include "bpf/BpfUtils.h"
#include "netdutils/Status.h"
#include "netdutils/StatusOr.h"

namespace android {
namespace bpf {

// This is a class wrapper for eBPF maps. The eBPF map is a special in-kernel
// data structure that stores data in <Key, Value> pairs. It can be read/write
// from userspace by passing syscalls with the map file descriptor. This class
// is used to generalize the procedure of interacting with eBPF maps and hide
// the implementation detail from other process. Besides the basic syscalls
// wrapper, it also provides some useful helper functions as well as an iterator
// nested class to iterate the map more easily.
//
// NOTE: A kernel eBPF map may be accessed by both kernel and userspace
// processes at the same time. Or if the map is pinned as a virtual file, it can
// be obtained by multiple eBPF map class object and and accessed concurrently.
// Though the map class object and the underlying kernel map are thread safe, it
// is not safe to iterate over a map while another thread or process is deleting
// from it. In this case the iteration can return duplicate entries.
template <class Key, class Value>
class BpfMap {
  public:
    class const_iterator {
      public:
        netdutils::Status start() {
            if (mMap == nullptr) {
                return netdutils::statusFromErrno(EINVAL, "Invalid map iterator");
            }
            auto firstKey = mMap->getFirstKey();
            if (isOk(firstKey)) {
                mCurKey = firstKey.value();
            } else if (firstKey.status().code() == ENOENT) {
                // The map is empty.
                mMap = nullptr;
                memset(&mCurKey, 0, sizeof(Key));
            } else {
                return firstKey.status();
            }
            return netdutils::status::ok;
        }

        netdutils::StatusOr<Key> next() {
            if (mMap == nullptr) {
                return netdutils::statusFromErrno(ENOENT, "Iterating past end of map");
            }
            auto nextKey = mMap->getNextKey(mCurKey);
            if (isOk(nextKey)) {
                mCurKey = nextKey.value();
            } else if (nextKey.status().code() == ENOENT) {
                // iterator reached the end of map
                mMap = nullptr;
                memset(&mCurKey, 0, sizeof(Key));
            } else {
                return nextKey.status();
            }
            return mCurKey;
        }

        const Key operator*() { return mCurKey; }

        bool operator==(const const_iterator& other) const {
            return (mMap == other.mMap) && (mCurKey == other.mCurKey);
        }

        bool operator!=(const const_iterator& other) const { return !(*this == other); }

        const_iterator(const BpfMap<Key, Value>* map) : mMap(map) {
            memset(&mCurKey, 0, sizeof(Key));
        }

      private:
        const BpfMap<Key, Value> * mMap;
        Key mCurKey;
    };

    BpfMap<Key, Value>() : mMapFd(-1){};
    BpfMap<Key, Value>(int fd) : mMapFd(fd){};
    BpfMap<Key, Value>(bpf_map_type map_type, uint32_t max_entries, uint32_t map_flags) {
        int map_fd = createMap(map_type, sizeof(Key), sizeof(Value), max_entries, map_flags);
        if (map_fd < 0) {
            mMapFd.reset(-1);
        } else {
            mMapFd.reset(map_fd);
        }
    }

    netdutils::Status pinToPath(const std::string path) {
        int ret = mapPin(mMapFd, path.c_str());
        if (ret) {
            return netdutils::statusFromErrno(errno,
                                              base::StringPrintf("pin to %s failed", path.c_str()));
        }
        mPinnedPath = path;
        return netdutils::status::ok;
    }

    netdutils::StatusOr<Key> getFirstKey() const {
        Key firstKey;
        if (getFirstMapKey(mMapFd, &firstKey)) {
            return netdutils::statusFromErrno(
                errno, base::StringPrintf("Get firstKey map %d failed", mMapFd.get()));
        }
        return firstKey;
    }

    netdutils::StatusOr<Key> getNextKey(const Key& key) const {
        Key nextKey;
        if (getNextMapKey(mMapFd, const_cast<Key*>(&key), &nextKey)) {
            return netdutils::statusFromErrno(
                errno, base::StringPrintf("Get next key of map %d failed", mMapFd.get()));
        }
        return nextKey;
    }

    netdutils::Status writeValue(const Key& key, const Value& value, uint64_t flags) {
        if (writeToMapEntry(mMapFd, const_cast<Key*>(&key), const_cast<Value*>(&value), flags)) {
            return netdutils::statusFromErrno(
                errno, base::StringPrintf("write to map %d failed", mMapFd.get()));
        }
        return netdutils::status::ok;
    }

    netdutils::StatusOr<Value> readValue(const Key key) const {
        Value value;
        if (findMapEntry(mMapFd, const_cast<Key*>(&key), &value)) {
            return netdutils::statusFromErrno(
                errno, base::StringPrintf("read value of map %d failed", mMapFd.get()));
        }
        return value;
    }

    netdutils::Status deleteValue(const Key& key) {
        if (deleteMapEntry(mMapFd, const_cast<Key*>(&key))) {
            return netdutils::statusFromErrno(
                errno, base::StringPrintf("delete entry from map %d failed", mMapFd.get()));
        }
        return netdutils::status::ok;
    }

    // Function that tries to get map from a pinned path, if the map doesn't
    // exist yet, create a new one and pinned to the path.
    netdutils::Status getOrCreate(const uint32_t maxEntries, const char* path,
                                  const bpf_map_type mapType);

    // Iterate through the map and handle each key retrieved based on the filter
    // without modification of map content.
    netdutils::Status iterate(
        const std::function<netdutils::Status(const Key& key, const BpfMap<Key, Value>& map)>&
            filter) const;

    // Iterate through the map and get each <key, value> pair, handle each <key,
    // value> pair based on the filter without modification of map content.
    netdutils::Status iterateWithValue(
        const std::function<netdutils::Status(const Key& key, const Value& value,
                                              const BpfMap<Key, Value>& map)>& filter) const;

    // Iterate through the map and handle each key retrieved based on the filter
    netdutils::Status iterate(
        const std::function<netdutils::Status(const Key& key, BpfMap<Key, Value>& map)>& filter);

    // Iterate through the map and get each <key, value> pair, handle each <key,
    // value> pair based on the filter.
    netdutils::Status iterateWithValue(
        const std::function<netdutils::Status(const Key& key, const Value& value,
                                              BpfMap<Key, Value>& map)>& filter);

    const base::unique_fd& getMap() const { return mMapFd; };

    const std::string getPinnedPath() const { return mPinnedPath; };

    // Move constructor
    void operator=(BpfMap<Key, Value>&& other) {
        mMapFd = std::move(other.mMapFd);
        if (!other.mPinnedPath.empty()) {
            mPinnedPath = other.mPinnedPath;
        } else {
            mPinnedPath.clear();
        }
        other.reset();
    }

    void reset(int fd = -1) {
        mMapFd.reset(fd);
        mPinnedPath.clear();
    }

    bool isValid() const { return mMapFd != -1; }

    const_iterator begin() const { return const_iterator(this); }

    const_iterator end() const { return const_iterator(nullptr); }

  private:
    base::unique_fd mMapFd;
    std::string mPinnedPath;
};

template <class Key, class Value>
netdutils::Status BpfMap<Key, Value>::getOrCreate(const uint32_t maxEntries, const char* path,
                                                  bpf_map_type mapType) {
    int ret = access(path, R_OK);
    /* Check the pinned location first to check if the map is already there.
     * otherwise create a new one.
     */
    if (ret == 0) {
        mMapFd = base::unique_fd(mapRetrieve(path, 0));
        if (mMapFd == -1) {
            reset();
            return netdutils::statusFromErrno(
                errno,
                base::StringPrintf("pinned map not accessible or does not exist: (%s)\n", path));
        }
        mPinnedPath = path;
    } else if (ret == -1 && errno == ENOENT) {
        mMapFd = base::unique_fd(
            createMap(mapType, sizeof(Key), sizeof(Value), maxEntries, BPF_F_NO_PREALLOC));
        if (mMapFd == -1) {
            reset();
            return netdutils::statusFromErrno(errno,
                                              base::StringPrintf("map create failed!: %s", path));
        }
        netdutils::Status pinStatus = pinToPath(path);
        if (!isOk(pinStatus)) {
            reset();
            return pinStatus;
        }
        mPinnedPath = path;
    } else {
        return netdutils::statusFromErrno(
            errno, base::StringPrintf("pinned map not accessible: %s", path));
    }
    return netdutils::status::ok;
}

template <class Key, class Value>
netdutils::Status BpfMap<Key, Value>::iterate(
    const std::function<netdutils::Status(const Key& key, const BpfMap<Key, Value>& map)>& filter)
    const {
    const_iterator itr = this->begin();
    RETURN_IF_NOT_OK(itr.start());
    while (itr != this->end()) {
        Key prevKey = *itr;
        netdutils::Status advanceStatus = itr.next();
        RETURN_IF_NOT_OK(filter(prevKey, *this));
        RETURN_IF_NOT_OK(advanceStatus);
    }
    return netdutils::status::ok;
}

template <class Key, class Value>
netdutils::Status BpfMap<Key, Value>::iterateWithValue(
    const std::function<netdutils::Status(const Key& key, const Value& value,
                                          const BpfMap<Key, Value>& map)>& filter) const {
    const_iterator itr = this->begin();
    RETURN_IF_NOT_OK(itr.start());
    while (itr != this->end()) {
        Key prevKey = *itr;
        Value prevValue;
        ASSIGN_OR_RETURN(prevValue, this->readValue(prevKey));
        netdutils::Status advanceStatus = itr.next();
        RETURN_IF_NOT_OK(filter(prevKey, prevValue, *this));
        RETURN_IF_NOT_OK(advanceStatus);
    }
    return netdutils::status::ok;
}

template <class Key, class Value>
netdutils::Status BpfMap<Key, Value>::iterate(
    const std::function<netdutils::Status(const Key& key, BpfMap<Key, Value>& map)>& filter) {
    const_iterator itr = this->begin();
    RETURN_IF_NOT_OK(itr.start());
    while (itr != this->end()) {
        Key prevKey = *itr;
        netdutils::Status advanceStatus = itr.next();
        RETURN_IF_NOT_OK(filter(prevKey, *this));
        RETURN_IF_NOT_OK(advanceStatus);
    }
    return netdutils::status::ok;
}

template <class Key, class Value>
netdutils::Status BpfMap<Key, Value>::iterateWithValue(
    const std::function<netdutils::Status(const Key& key, const Value& value,
                                          BpfMap<Key, Value>& map)>& filter) {
    const_iterator itr = this->begin();
    RETURN_IF_NOT_OK(itr.start());
    while (itr != this->end()) {
        Key prevKey = *itr;
        Value prevValue;
        ASSIGN_OR_RETURN(prevValue, this->readValue(prevKey));
        netdutils::Status advanceStatus = itr.next();
        RETURN_IF_NOT_OK(filter(prevKey, prevValue, *this));
        RETURN_IF_NOT_OK(advanceStatus);
    }
    return netdutils::status::ok;
}

}  // namespace bpf
}  // namespace android

#endif
