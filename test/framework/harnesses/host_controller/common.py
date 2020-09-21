#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# The default Partner Android Build (PAB) public account.
# To obtain access permission, please reach out to Android partner engineering
# department of Google LLC.
_DEFAULT_ACCOUNT_ID = '543365459'

# The default Partner Android Build (PAB) internal account.
_DEFAULT_ACCOUNT_ID_INTERNAL = '541462473'

# The key value used for getting a fetched .zip android img file.
FULL_ZIPFILE = "full-zipfile"

# The default value for "flash --current".
_DEFAULT_FLASH_IMAGES = [
    FULL_ZIPFILE,
    "bootloader.img",
    "boot.img",
    "cache.img",
    "radio.img",
    "system.img",
    "userdata.img",
    "vbmeta.img",
    "vendor.img",
]

# The environment variable for default serial numbers.
_ANDROID_SERIAL = "ANDROID_SERIAL"

_DEVICE_STATUS_DICT = {
    "unknown": 0,
    "fastboot": 1,
    "online": 2,
    "ready": 3,
    "use": 4,
    "error": 5}

# Default SPL date, used for gsispl command
_SPL_DEFAULT_DAY = 5

# Maximum number of leased jobs per host.
_MAX_LEASED_JOBS = 14
