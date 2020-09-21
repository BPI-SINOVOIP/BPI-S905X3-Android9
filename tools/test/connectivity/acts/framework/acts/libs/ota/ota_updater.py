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

from acts import utils
from acts.libs.ota.ota_runners import ota_runner_factory

# Maps AndroidDevices to OtaRunners
ota_runners = {}


def initialize(user_params, android_devices):
    """Initialize OtaRunners for each device.

    Args:
        user_params: The user_params from the ACTS config.
        android_devices: The android_devices in the test.
    """
    for ad in android_devices:
        ota_runners[ad] = ota_runner_factory.create_from_configs(
            user_params, ad)


def _check_initialization(android_device):
    """Check if a given device was initialized."""
    if android_device not in ota_runners:
        raise KeyError('Android Device with serial "%s" has not been '
                       'initialized for OTA Updates. Did you forget to call'
                       'ota_updater.initialize()?' % android_device.serial)


def update(android_device, ignore_update_errors=False):
    """Update a given AndroidDevice.

    Args:
        android_device: The device to update
        ignore_update_errors: Whether or not to ignore update errors such as
           no more updates available for a given device. Default is false.
    Throws:
        OtaError if ignore_update_errors is false and the OtaRunner has run out
        of packages to update the phone with.
    """
    _check_initialization(android_device)
    try:
        ota_runners[android_device].update()
    except Exception as e:
        if ignore_update_errors:
            return
        android_device.log.error(e)
        android_device.take_bug_report('ota_update',
                                       utils.get_current_epoch_time())
        raise e


def can_update(android_device):
    """Whether or not a device can be updated."""
    _check_initialization(android_device)
    return ota_runners[android_device].can_update()
