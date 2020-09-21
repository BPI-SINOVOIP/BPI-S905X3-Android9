//
// Copyright (C) 2015 The Android Open Source Project
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

#include "update_engine/boot_control_recovery.h"

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <brillo/message_loops/message_loop.h>

#include "update_engine/common/utils.h"
#include "update_engine/utils_android.h"

using std::string;

#ifndef _UE_SIDELOAD
#error "BootControlRecovery should only be used for update_engine_sideload."
#endif

// When called from update_engine_sideload, we don't attempt to dynamically load
// the right boot_control HAL, instead we use the only HAL statically linked in
// via the PRODUCT_STATIC_BOOT_CONTROL_HAL make variable and access the module
// struct directly.
extern const hw_module_t HAL_MODULE_INFO_SYM;

namespace chromeos_update_engine {

namespace boot_control {

// Factory defined in boot_control.h.
std::unique_ptr<BootControlInterface> CreateBootControl() {
  std::unique_ptr<BootControlRecovery> boot_control(new BootControlRecovery());
  if (!boot_control->Init()) {
    return nullptr;
  }
  return std::move(boot_control);
}

}  // namespace boot_control

bool BootControlRecovery::Init() {
  const hw_module_t* hw_module;
  int ret;

  // For update_engine_sideload, we simulate the hw_get_module() by accessing it
  // from the current process directly.
  hw_module = &HAL_MODULE_INFO_SYM;
  ret = 0;
  if (!hw_module ||
      strcmp(BOOT_CONTROL_HARDWARE_MODULE_ID, hw_module->id) != 0) {
    ret = -EINVAL;
  }
  if (ret != 0) {
    LOG(ERROR) << "Error loading boot_control HAL implementation.";
    return false;
  }

  module_ = reinterpret_cast<boot_control_module_t*>(
      const_cast<hw_module_t*>(hw_module));
  module_->init(module_);

  LOG(INFO) << "Loaded boot_control HAL "
            << "'" << hw_module->name << "' "
            << "version " << (hw_module->module_api_version >> 8) << "."
            << (hw_module->module_api_version & 0xff) << " "
            << "authored by '" << hw_module->author << "'.";
  return true;
}

unsigned int BootControlRecovery::GetNumSlots() const {
  return module_->getNumberSlots(module_);
}

BootControlInterface::Slot BootControlRecovery::GetCurrentSlot() const {
  return module_->getCurrentSlot(module_);
}

bool BootControlRecovery::GetPartitionDevice(const string& partition_name,
                                             Slot slot,
                                             string* device) const {
  // We can't use fs_mgr to look up |partition_name| because fstab
  // doesn't list every slot partition (it uses the slotselect option
  // to mask the suffix).
  //
  // We can however assume that there's an entry for the /misc mount
  // point and use that to get the device file for the misc
  // partition. This helps us locate the disk that |partition_name|
  // resides on. From there we'll assume that a by-name scheme is used
  // so we can just replace the trailing "misc" by the given
  // |partition_name| and suffix corresponding to |slot|, e.g.
  //
  //   /dev/block/platform/soc.0/7824900.sdhci/by-name/misc ->
  //   /dev/block/platform/soc.0/7824900.sdhci/by-name/boot_a
  //
  // If needed, it's possible to relax the by-name assumption in the
  // future by trawling /sys/block looking for the appropriate sibling
  // of misc and then finding an entry in /dev matching the sysfs
  // entry.

  base::FilePath misc_device;
  if (!utils::DeviceForMountPoint("/misc", &misc_device))
    return false;

  if (!utils::IsSymlink(misc_device.value().c_str())) {
    LOG(ERROR) << "Device file " << misc_device.value() << " for /misc "
               << "is not a symlink.";
    return false;
  }

  const char* suffix = module_->getSuffix(module_, slot);
  if (suffix == nullptr) {
    LOG(ERROR) << "boot_control impl returned no suffix for slot "
               << SlotName(slot);
    return false;
  }

  base::FilePath path = misc_device.DirName().Append(partition_name + suffix);
  if (!base::PathExists(path)) {
    LOG(ERROR) << "Device file " << path.value() << " does not exist.";
    return false;
  }

  *device = path.value();
  return true;
}

bool BootControlRecovery::IsSlotBootable(Slot slot) const {
  int ret = module_->isSlotBootable(module_, slot);
  if (ret < 0) {
    LOG(ERROR) << "Unable to determine if slot " << SlotName(slot)
               << " is bootable: " << strerror(-ret);
    return false;
  }
  return ret == 1;
}

bool BootControlRecovery::MarkSlotUnbootable(Slot slot) {
  int ret = module_->setSlotAsUnbootable(module_, slot);
  if (ret < 0) {
    LOG(ERROR) << "Unable to mark slot " << SlotName(slot)
               << " as bootable: " << strerror(-ret);
    return false;
  }
  return ret == 0;
}

bool BootControlRecovery::SetActiveBootSlot(Slot slot) {
  int ret = module_->setActiveBootSlot(module_, slot);
  if (ret < 0) {
    LOG(ERROR) << "Unable to set the active slot to slot " << SlotName(slot)
               << ": " << strerror(-ret);
  }
  return ret == 0;
}

bool BootControlRecovery::MarkBootSuccessfulAsync(
    base::Callback<void(bool)> callback) {
  int ret = module_->markBootSuccessful(module_);
  if (ret < 0) {
    LOG(ERROR) << "Unable to mark boot successful: " << strerror(-ret);
  }
  return brillo::MessageLoop::current()->PostTask(
             FROM_HERE, base::Bind(callback, ret == 0)) !=
         brillo::MessageLoop::kTaskIdNull;
}

}  // namespace chromeos_update_engine
