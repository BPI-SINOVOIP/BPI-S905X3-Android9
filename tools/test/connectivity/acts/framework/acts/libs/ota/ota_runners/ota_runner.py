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

import time

SL4A_SERVICE_SETUP_TIME = 5


class OtaError(Exception):
    """Raised when an error in the OTA Update process occurs."""


class OtaRunner(object):
    """The base class for all OTA Update Runners."""

    def __init__(self, ota_tool, android_device):
        self.ota_tool = ota_tool
        self.android_device = android_device
        self.serial = self.android_device.serial

    def _update(self):
        log = self.android_device.log
        old_info = self.android_device.adb.getprop('ro.build.fingerprint')
        log.info('Starting Update. Beginning build info: %s', old_info)
        log.info('Stopping services.')
        self.android_device.stop_services()
        log.info('Beginning tool.')
        self.ota_tool.update(self)
        log.info('Tool finished. Waiting for boot completion.')
        self.android_device.wait_for_boot_completion()
        new_info = self.android_device.adb.getprop('ro.build.fingerprint')
        if not old_info or old_info == new_info:
            raise OtaError('The device was not updated to a new build. '
                           'Previous build: %s. New build: %s' % (old_info,
                                                                  new_info))
        log.info('Boot completed. Rooting adb.')
        self.android_device.root_adb()
        log.info('Root complete.')
        if self.android_device.skip_sl4a:
            self.android_device.log.info('Skipping SL4A install.')
        else:
            for _ in range(3):
                self.android_device.log.info('Re-installing SL4A from "%s".',
                                             self.get_sl4a_apk())
                self.android_device.adb.install(
                    '-r -g %s' % self.get_sl4a_apk(), ignore_status=True)
                time.sleep(SL4A_SERVICE_SETUP_TIME)
                if self.android_device.is_sl4a_installed():
                    break
        log.info('Starting services.')
        self.android_device.start_services()
        log.info('Services started. Running ota tool cleanup.')
        self.ota_tool.cleanup(self)
        log.info('Cleanup complete.')

    def can_update(self):
        """Whether or not an update package is available for the device."""
        return NotImplementedError()

    def get_ota_package(self):
        raise NotImplementedError()

    def get_sl4a_apk(self):
        raise NotImplementedError()


class SingleUseOtaRunner(OtaRunner):
    """A single use OtaRunner.

    SingleUseOtaRunners can only be ran once. If a user attempts to run it more
    than once, an error will be thrown. Users can avoid the error by checking
    can_update() before calling update().
    """

    def __init__(self, ota_tool, android_device, ota_package, sl4a_apk):
        super(SingleUseOtaRunner, self).__init__(ota_tool, android_device)
        self._ota_package = ota_package
        self._sl4a_apk = sl4a_apk
        self._called = False

    def can_update(self):
        return not self._called

    def update(self):
        """Starts the update process."""
        if not self.can_update():
            raise OtaError('A SingleUseOtaTool instance cannot update a phone '
                           'multiple times.')
        self._called = True
        self._update()

    def get_ota_package(self):
        return self._ota_package

    def get_sl4a_apk(self):
        return self._sl4a_apk


class MultiUseOtaRunner(OtaRunner):
    """A multiple use OtaRunner.

    MultiUseOtaRunner can only be ran for as many times as there have been
    packages provided to them. If a user attempts to run it more than the number
    of provided packages, an error will be thrown. Users can avoid the error by
    checking can_update() before calling update().
    """

    def __init__(self, ota_tool, android_device, ota_packages, sl4a_apks):
        super(MultiUseOtaRunner, self).__init__(ota_tool, android_device)
        self._ota_packages = ota_packages
        self._sl4a_apks = sl4a_apks
        self.current_update_number = 0

    def can_update(self):
        return not self.current_update_number == len(self._ota_packages)

    def update(self):
        """Starts the update process."""
        if not self.can_update():
            raise OtaError('This MultiUseOtaRunner has already updated all '
                           'given packages onto the phone.')
        self._update()
        self.current_update_number += 1

    def get_ota_package(self):
        return self._ota_packages[self.current_update_number]

    def get_sl4a_apk(self):
        return self._sl4a_apks[self.current_update_number]
