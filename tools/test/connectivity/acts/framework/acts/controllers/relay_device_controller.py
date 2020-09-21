#!/usr/bin/env python
#
#   Copyright 2016 - The Android Open Source Project
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

import json

from acts.controllers.relay_lib.relay_rig import RelayRig

ACTS_CONTROLLER_CONFIG_NAME = "RelayDevice"
ACTS_CONTROLLER_REFERENCE_NAME = "relay_devices"


def create(config):
    """Creates RelayDevice controller objects.

        Args:
            config: Either one of two types:

            A filename to a RelayController config (json file)
            A RelayController config/dict composed of:
                boards: A list of controller boards (see tests).
                devices: A list of RelayDevices attached to the boards.

        Returns:
                A list of RelayDevice objects.
    """
    if type(config) is str:
        return _create_from_external_config_file(config)
    elif type(config) is dict:
        return _create_from_dict(config)


def _create_from_external_config_file(config_filename):
    """Creates RelayDevice controller objects from an external config file.

    Args:
        config_filename: The filename of the RelayController config.

    Returns:
        A list of RelayDevice objects.
    """
    with open(config_filename) as json_file:
        return _create_from_dict(json.load(json_file))


def _create_from_dict(config):
    """Creates RelayDevice controller objects from a dictionary.

    Args:
        config: The dictionary containing the RelayController config.

    Returns:
        A list of RelayDevice objects.
    """
    devices = list()

    relay_rig = RelayRig(config)
    for device in relay_rig.devices.values():
        devices.append(device)

    return devices


def destroy(relay_devices):
    """Cleans up RelayDevice objects.

        Args:
            relay_devices: A list of AndroidDevice objects.
    """
    for device in relay_devices:
        device.clean_up()
    pass


def get_info(relay_devices):
    """Get information on a list of RelayDevice objects.

    Args:
        relay_devices: A list of RelayDevice objects.

    Returns:
        A list of dict, each representing info for an RelayDevice objects.
    """
    device_info = []
    for device in relay_devices:
        relay_ids = list()
        for relay in device.relays:
            relay_ids.append(relay)
        info = {"name": device.name, "relays": relay_ids}
        device_info.append(info)
    return device_info
