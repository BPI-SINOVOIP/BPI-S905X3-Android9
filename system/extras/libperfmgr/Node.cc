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
 * See the License for the specic language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "libperfmgr"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "perfmgr/Node.h"

namespace android {
namespace perfmgr {

Node::Node(std::string name, std::string node_path,
           std::vector<RequestGroup> req_sorted, std::size_t default_val_index,
           bool reset_on_init, bool hold_fd)
    : name_(name),
      node_path_(node_path),
      req_sorted_(std::move(req_sorted)),
      default_val_index_(default_val_index),
      reset_on_init_(reset_on_init),
      hold_fd_(hold_fd) {
    if (reset_on_init) {
        // Assigning an invalid value so the next Update() will update the
        // Node's value to default
        current_val_index_ = req_sorted_.size();
        Update();
    } else {
        current_val_index_ = default_val_index;
    }
}

bool Node::AddRequest(std::size_t value_index, const std::string& hint_type,
                      ReqTime end_time) {
    if (value_index >= req_sorted_.size()) {
        LOG(ERROR) << "Value index out of bound: " << value_index
                   << " ,size: " << req_sorted_.size();
        return false;
    }
    // Add/Update request to the new end_time for the specific hint_type
    req_sorted_[value_index].AddRequest(hint_type, end_time);
    return true;
}

bool Node::RemoveRequest(const std::string& hint_type) {
    bool ret = false;
    // Remove all requests for the specific hint_type
    for (auto& value : req_sorted_) {
        ret = value.RemoveRequest(hint_type) || ret;
    }
    return ret;
}

std::chrono::milliseconds Node::Update() {
    std::size_t value_index = default_val_index_;
    std::chrono::milliseconds expire_time = std::chrono::milliseconds::max();

    // Find the highest outstanding request's expire time
    for (std::size_t i = 0; i < req_sorted_.size(); i++) {
        if (req_sorted_[i].GetExpireTime(&expire_time)) {
            value_index = i;
            break;
        }
    }

    // Update node only if request index changes
    if (value_index != current_val_index_) {
        std::string req_value = req_sorted_[value_index].GetRequestValue();

        fd_.reset(TEMP_FAILURE_RETRY(
            open(node_path_.c_str(), O_WRONLY | O_CLOEXEC | O_TRUNC)));

        if (fd_ == -1 || !android::base::WriteStringToFd(req_value, fd_)) {
            LOG(ERROR) << "Failed to write to node: " << node_path_
                       << " with value: " << req_value << ", fd: " << fd_;
            // Retry in 500ms or sooner
            expire_time = std::min(expire_time, std::chrono::milliseconds(500));
        } else {
            // For regular file system, we need fsync
            fsync(fd_);
            // Some dev node requires file to remain open during the entire hint
            // duration e.g. /dev/cpu_dma_latency, so fd_ is intentionally kept
            // open during any requested value other than default one. If
            // request a default value, node will write the value and then
            // release the fd.
            if ((!hold_fd_) || value_index == default_val_index_) {
                fd_.reset();
            }
            // Update current index only when succeed
            current_val_index_ = value_index;
        }
    }
    return expire_time;
}

std::string Node::GetName() const {
    return name_;
}

std::string Node::GetPath() const {
    return node_path_;
}

bool Node::GetValueIndex(const std::string value, std::size_t* index) const {
    bool found = false;
    for (std::size_t i = 0; i < req_sorted_.size(); i++) {
        if (req_sorted_[i].GetRequestValue() == value) {
            *index = i;
            found = true;
            break;
        }
    }
    return found;
}

std::size_t Node::GetDefaultIndex() const {
    return default_val_index_;
}

bool Node::GetResetOnInit() const {
    return reset_on_init_;
}

bool Node::GetHoldFd() const {
    return hold_fd_;
}

std::vector<std::string> Node::GetValues() const {
    std::vector<std::string> values;
    for (const auto& value : req_sorted_) {
        values.emplace_back(value.GetRequestValue());
    }
    return values;
}

void Node::DumpToFd(int fd) {
    std::string node_value;
    if (!android::base::ReadFileToString(node_path_, &node_value)) {
        LOG(ERROR) << "Failed to read node path: " << node_path_;
    }
    node_value = android::base::Trim(node_value);
    std::string buf(android::base::StringPrintf(
        "%s\t%s\t%zu\t%s\n", name_.c_str(), node_path_.c_str(),
        current_val_index_, node_value.c_str()));
    if (!android::base::WriteStringToFd(buf, fd)) {
        LOG(ERROR) << "Failed to dump fd: " << fd;
    }
}

}  // namespace perfmgr
}  // namespace android
