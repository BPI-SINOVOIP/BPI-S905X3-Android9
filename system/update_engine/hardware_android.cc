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

#include "update_engine/hardware_android.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <memory>

#include <bootloader.h>

#include <android-base/properties.h>
#include <base/files/file_util.h>
#include <base/strings/stringprintf.h>

#include "update_engine/common/hardware.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/utils.h"
#include "update_engine/utils_android.h"

using android::base::GetBoolProperty;
using android::base::GetIntProperty;
using android::base::GetProperty;
using std::string;

namespace chromeos_update_engine {

namespace {

// The powerwash arguments passed to recovery. Arguments are separated by \n.
const char kAndroidRecoveryPowerwashCommand[] =
    "recovery\n"
    "--wipe_data\n"
    "--reason=wipe_data_from_ota\n";

// Android properties that identify the hardware and potentially non-updatable
// parts of the bootloader (such as the bootloader version and the baseband
// version).
const char kPropBootBootloader[] = "ro.boot.bootloader";
const char kPropBootBaseband[] = "ro.boot.baseband";
const char kPropProductManufacturer[] = "ro.product.manufacturer";
const char kPropBootHardwareSKU[] = "ro.boot.hardware.sku";
const char kPropBootRevision[] = "ro.boot.revision";
const char kPropBuildDateUTC[] = "ro.build.date.utc";

// Write a recovery command line |message| to the BCB. The arguments to recovery
// must be separated by '\n'. An empty string will erase the BCB.
bool WriteBootloaderRecoveryMessage(const string& message) {
  base::FilePath misc_device;
  if (!utils::DeviceForMountPoint("/misc", &misc_device))
    return false;

  // Setup a bootloader_message with just the command and recovery fields set.
  bootloader_message boot = {};
  if (!message.empty()) {
    strncpy(boot.command, "boot-recovery", sizeof(boot.command) - 1);
    memcpy(boot.recovery,
           message.data(),
           std::min(message.size(), sizeof(boot.recovery) - 1));
  }

  int fd = HANDLE_EINTR(open(misc_device.value().c_str(), O_WRONLY | O_SYNC));
  if (fd < 0) {
    PLOG(ERROR) << "Opening misc";
    return false;
  }
  ScopedFdCloser fd_closer(&fd);
  // We only re-write the first part of the bootloader_message, up to and
  // including the recovery message.
  size_t boot_size =
      offsetof(bootloader_message, recovery) + sizeof(boot.recovery);
  if (!utils::WriteAll(fd, &boot, boot_size)) {
    PLOG(ERROR) << "Writing recovery command to misc";
    return false;
  }
  return true;
}

}  // namespace

namespace hardware {

// Factory defined in hardware.h.
std::unique_ptr<HardwareInterface> CreateHardware() {
  return std::make_unique<HardwareAndroid>();
}

}  // namespace hardware

// In Android there are normally three kinds of builds: eng, userdebug and user.
// These builds target respectively a developer build, a debuggable version of
// the final product and the pristine final product the end user will run.
// Apart from the ro.build.type property name, they differ in the following
// properties that characterize the builds:
// * eng builds: ro.secure=0 and ro.debuggable=1
// * userdebug builds: ro.secure=1 and ro.debuggable=1
// * user builds: ro.secure=1 and ro.debuggable=0
//
// See IsOfficialBuild() and IsNormalMode() for the meaning of these options in
// Android.

bool HardwareAndroid::IsOfficialBuild() const {
  // We run an official build iff ro.secure == 1, because we expect the build to
  // behave like the end user product and check for updates. Note that while
  // developers are able to build "official builds" by just running "make user",
  // that will only result in a more restrictive environment. The important part
  // is that we don't produce and push "non-official" builds to the end user.
  //
  // In case of a non-bool value, we take the most restrictive option and
  // assume we are in an official-build.
  return GetBoolProperty("ro.secure", true);
}

bool HardwareAndroid::IsNormalBootMode() const {
  // We are running in "dev-mode" iff ro.debuggable == 1. In dev-mode the
  // update_engine will allow extra developers options, such as providing a
  // different update URL. In case of error, we assume the build is in
  // normal-mode.
  return !GetBoolProperty("ro.debuggable", false);
}

bool HardwareAndroid::AreDevFeaturesEnabled() const {
  return !IsNormalBootMode();
}

bool HardwareAndroid::IsOOBEEnabled() const {
  // No OOBE flow blocking updates for Android-based boards.
  return false;
}

bool HardwareAndroid::IsOOBEComplete(base::Time* out_time_of_oobe) const {
  LOG(WARNING) << "OOBE is not enabled but IsOOBEComplete() called.";
  if (out_time_of_oobe)
    *out_time_of_oobe = base::Time();
  return true;
}

string HardwareAndroid::GetHardwareClass() const {
  auto manufacturer = GetProperty(kPropProductManufacturer, "");
  auto sku = GetProperty(kPropBootHardwareSKU, "");
  auto revision = GetProperty(kPropBootRevision, "");

  return manufacturer + ":" + sku + ":" + revision;
}

string HardwareAndroid::GetFirmwareVersion() const {
  return GetProperty(kPropBootBootloader, "");
}

string HardwareAndroid::GetECVersion() const {
  return GetProperty(kPropBootBaseband, "");
}

int HardwareAndroid::GetPowerwashCount() const {
  LOG(WARNING) << "STUB: Assuming no factory reset was performed.";
  return 0;
}

bool HardwareAndroid::SchedulePowerwash() {
  LOG(INFO) << "Scheduling a powerwash to BCB.";
  return WriteBootloaderRecoveryMessage(kAndroidRecoveryPowerwashCommand);
}

bool HardwareAndroid::CancelPowerwash() {
  return WriteBootloaderRecoveryMessage("");
}

bool HardwareAndroid::GetNonVolatileDirectory(base::FilePath* path) const {
  base::FilePath local_path(constants::kNonVolatileDirectory);
  if (!base::PathExists(local_path)) {
    LOG(ERROR) << "Non-volatile directory not found: " << local_path.value();
    return false;
  }
  *path = local_path;
  return true;
}

bool HardwareAndroid::GetPowerwashSafeDirectory(base::FilePath* path) const {
  // On Android, we don't have a directory persisted across powerwash.
  return false;
}

int64_t HardwareAndroid::GetBuildTimestamp() const {
  return GetIntProperty<int64_t>(kPropBuildDateUTC, 0);
}

bool HardwareAndroid::GetFirstActiveOmahaPingSent() const {
  LOG(WARNING) << "STUB: Assuming first active omaha was never set.";
  return false;
}

void HardwareAndroid::SetFirstActiveOmahaPingSent() {
  LOG(WARNING) << "STUB: Assuming first active omaha is never set.";
  return;
}

}  // namespace chromeos_update_engine
