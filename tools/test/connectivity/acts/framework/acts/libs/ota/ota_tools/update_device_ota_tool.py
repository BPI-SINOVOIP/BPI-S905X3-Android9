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
import os
import shutil
import tempfile

from acts.libs.ota.ota_tools import ota_tool
from acts.libs.proc import job
from acts import utils

# OTA Packages can be upwards of 1 GB. This may take some time to transfer over
# USB 2.0. A/B devices must also complete the update in the background.
UPDATE_TIMEOUT = 20 * 60
UPDATE_LOCATION = '/data/ota_package/update.zip'


class UpdateDeviceOtaTool(ota_tool.OtaTool):
    """Runs an OTA Update with system/update_engine/scripts/update_device.py."""

    def __init__(self, command):
        super(UpdateDeviceOtaTool, self).__init__(command)

        self.unzip_path = tempfile.mkdtemp()
        utils.unzip_maintain_permissions(self.command, self.unzip_path)

        self.command = os.path.join(self.unzip_path, 'update_device.py')

    def update(self, ota_runner):
        logging.info('Forcing adb to be in root mode.')
        ota_runner.android_device.root_adb()
        update_command = 'python2.7 %s -s %s %s' % (
            self.command, ota_runner.serial, ota_runner.get_ota_package())
        logging.info('Running %s' % update_command)
        result = job.run(update_command, timeout=UPDATE_TIMEOUT)
        logging.info('Output: %s' % result.stdout)

        logging.info('Rebooting device for update to go live.')
        ota_runner.android_device.reboot(stop_at_lock_screen=True)
        logging.info('Reboot sent.')

    def __del__(self):
        """Delete the unzipped update_device folder before ACTS exits."""
        shutil.rmtree(self.unzip_path)
