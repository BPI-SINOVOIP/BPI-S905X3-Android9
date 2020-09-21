#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging

from acts.libs.ota.ota_tools.ota_tool import OtaTool

# OTA Packages can be upwards of 1 GB. This may take some time to transfer over
# USB 2.0.
PUSH_TIMEOUT = 10 * 60


class AdbSideloadOtaTool(OtaTool):
    """Updates an AndroidDevice using adb sideload."""

    def __init__(self, ignored_command):
        # "command" is ignored. The ACTS adb version is used to prevent
        # differing adb versions from constantly killing adbd.
        super(AdbSideloadOtaTool, self).__init__(ignored_command)

    def update(self, ota_runner):
        logging.info('Rooting adb')
        ota_runner.android_device.root_adb()
        logging.info('Rebooting to sideload')
        ota_runner.android_device.adb.reboot('sideload')
        ota_runner.android_device.adb.wait_for_sideload()
        logging.info('Sideloading ota package')
        package_path = ota_runner.get_ota_package()
        logging.info('Running adb sideload with package "%s"' % package_path)
        ota_runner.android_device.adb.sideload(
            package_path, timeout=PUSH_TIMEOUT)
        logging.info('Sideload complete. Waiting for device to come back up.')
        ota_runner.android_device.adb.wait_for_recovery()
        ota_runner.android_device.reboot(stop_at_lock_screen=True)
        logging.info('Device is up. Update complete.')
