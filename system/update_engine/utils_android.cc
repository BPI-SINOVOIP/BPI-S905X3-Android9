//
// Copyright (C) 2016 The Android Open Source Project
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

#include "update_engine/utils_android.h"

#include <fs_mgr.h>

using std::string;

namespace chromeos_update_engine {

namespace {

// Open the appropriate fstab file and fallback to /fstab.device if
// that's what's being used.
static struct fstab* OpenFSTab() {
  struct fstab* fstab = fs_mgr_read_fstab_default();
  if (fstab != nullptr)
    return fstab;

  fstab = fs_mgr_read_fstab("/fstab.device");
  return fstab;
}

}  // namespace

namespace utils {

bool DeviceForMountPoint(const string& mount_point, base::FilePath* device) {
  struct fstab* fstab;
  struct fstab_rec* record;

  fstab = OpenFSTab();
  if (fstab == nullptr) {
    LOG(ERROR) << "Error opening fstab file.";
    return false;
  }
  record = fs_mgr_get_entry_for_mount_point(fstab, mount_point.c_str());
  if (record == nullptr) {
    LOG(ERROR) << "Error finding " << mount_point << " entry in fstab file.";
    fs_mgr_free_fstab(fstab);
    return false;
  }

  *device = base::FilePath(record->blk_device);
  fs_mgr_free_fstab(fstab);
  return true;
}

}  // namespace utils

}  // namespace chromeos_update_engine
