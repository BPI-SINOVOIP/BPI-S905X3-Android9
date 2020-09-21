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

from acts.config_parser import ActsConfigError
from acts.libs.ota.ota_runners import ota_runner
from acts.libs.ota.ota_tools import ota_tool_factory
from acts.libs.ota.ota_tools import adb_sideload_ota_tool

_bound_devices = {}

DEFAULT_OTA_TOOL = adb_sideload_ota_tool.AdbSideloadOtaTool.__name__
DEFAULT_OTA_COMMAND = 'adb'


def create_all_from_configs(config, android_devices):
    """Creates a new OtaTool for each given AndroidDevice.

    After an OtaTool is assigned to a device, another OtaTool cannot be created
    for that device. This will prevent OTA Update tests that accidentally flash
    the same build onto a device more than once.

    Args:
        config: the ACTS config user_params.
        android_devices: The devices to run an OTA Update on.

    Returns:
        A list of OtaRunners responsible for updating the given devices. The
        indexes match the indexes of the corresponding AndroidDevice in
        android_devices.
    """
    return [create_from_configs(config, ad) for ad in android_devices]


def create_from_configs(config, android_device):
    """Creates a new OtaTool for the given AndroidDevice.

    After an OtaTool is assigned to a device, another OtaTool cannot be created
    for that device. This will prevent OTA Update tests that accidentally flash
    the same build onto a device more than once.

    Args:
        config: the ACTS config user_params.
        android_device: The device to run the OTA Update on.

    Returns:
        An OtaRunner responsible for updating the given device.
    """
    # Default to adb sideload
    try:
        ota_tool_class_name = get_ota_value_from_config(
            config, 'ota_tool', android_device)
    except ActsConfigError:
        ota_tool_class_name = DEFAULT_OTA_TOOL

    if ota_tool_class_name not in config:
        if ota_tool_class_name is not DEFAULT_OTA_TOOL:
            raise ActsConfigError(
                'If the ota_tool is overloaded, the path to the tool must be '
                'added to the ACTS config file under {"OtaToolName": '
                '"path/to/tool"} (in this case, {"%s": "path/to/tool"}.' %
                ota_tool_class_name)
        else:
            command = DEFAULT_OTA_COMMAND
    else:
        command = config[ota_tool_class_name]
        if type(command) is list:
            # If file came as a list in the config.
            if len(command) == 1:
                command = command[0]
            else:
                raise ActsConfigError(
                    'Config value for "%s" must be either a string or a list '
                    'of exactly one element' % ota_tool_class_name)

    ota_package = get_ota_value_from_config(config, 'ota_package',
                                            android_device)
    ota_sl4a = get_ota_value_from_config(config, 'ota_sl4a', android_device)
    if type(ota_sl4a) != type(ota_package):
        raise ActsConfigError(
            'The ota_package and ota_sl4a must either both be strings, or '
            'both be lists. Device with serial "%s" has mismatched types.' %
            android_device.serial)
    return create(ota_package, ota_sl4a, android_device, ota_tool_class_name,
                  command)


def create(ota_package,
           ota_sl4a,
           android_device,
           ota_tool_class_name=DEFAULT_OTA_TOOL,
           command=DEFAULT_OTA_COMMAND,
           use_cached_runners=True):
    """
    Args:
        ota_package: A string or list of strings corresponding to the
            update.zip package location(s) for running an OTA update.
        ota_sl4a: A string or list of strings corresponding to the
            sl4a.apk package location(s) for running an OTA update.
        ota_tool_class_name: The class name for the desired ota_tool
        command: The command line tool name for the updater
        android_device: The AndroidDevice to run the OTA Update on.
        use_cached_runners: Whether or not to use runners cached by previous
            create calls.

    Returns:
        An OtaRunner with the given properties from the arguments.
    """
    ota_tool = ota_tool_factory.create(ota_tool_class_name, command)
    return create_from_package(ota_package, ota_sl4a, android_device, ota_tool,
                               use_cached_runners)


def create_from_package(ota_package,
                        ota_sl4a,
                        android_device,
                        ota_tool,
                        use_cached_runners=True):
    """
    Args:
        ota_package: A string or list of strings corresponding to the
            update.zip package location(s) for running an OTA update.
        ota_sl4a: A string or list of strings corresponding to the
            sl4a.apk package location(s) for running an OTA update.
        ota_tool: The OtaTool to be paired with the returned OtaRunner
        android_device: The AndroidDevice to run the OTA Update on.
        use_cached_runners: Whether or not to use runners cached by previous
            create calls.

    Returns:
        An OtaRunner with the given properties from the arguments.
    """
    if android_device in _bound_devices and use_cached_runners:
        logging.warning('Android device %s has already been assigned an '
                        'OtaRunner. Returning previously created runner.')
        return _bound_devices[android_device]

    if type(ota_package) != type(ota_sl4a):
        raise TypeError(
            'The ota_package and ota_sl4a must either both be strings, or '
            'both be lists. Device with serial "%s" has requested mismatched '
            'types.' % android_device.serial)

    if type(ota_package) is str:
        runner = ota_runner.SingleUseOtaRunner(ota_tool, android_device,
                                               ota_package, ota_sl4a)
    elif type(ota_package) is list:
        runner = ota_runner.MultiUseOtaRunner(ota_tool, android_device,
                                              ota_package, ota_sl4a)
    else:
        raise TypeError('The "ota_package" value in the acts config must be '
                        'either a list or a string.')

    _bound_devices[android_device] = runner
    return runner


def get_ota_value_from_config(config, key, android_device):
    """Returns a key for the given AndroidDevice.

    Args:
        config: The ACTS config
        key: The base key desired (ota_tool, ota_sl4a, or ota_package)
        android_device: An AndroidDevice

    Returns: The value at the specified key.
    Throws: ActsConfigError if the value cannot be determined from the config.
    """
    suffix = ''
    if 'ota_map' in config:
        if android_device.serial in config['ota_map']:
            suffix = '_%s' % config['ota_map'][android_device.serial]

    ota_package_key = '%s%s' % (key, suffix)
    if ota_package_key not in config:
        if suffix is not '':
            raise ActsConfigError(
                'Asked for an OTA Update without specifying a required value. '
                '"ota_map" has entry {"%s": "%s"}, but there is no '
                'corresponding entry {"%s":"/path/to/file"} found within the '
                'ACTS config.' % (android_device.serial, suffix[1:],
                                  ota_package_key))
        else:
            raise ActsConfigError(
                'Asked for an OTA Update without specifying a required value. '
                '"ota_map" does not exist or have a key for serial "%s", and '
                'the default value entry "%s" cannot be found within the ACTS '
                'config.' % (android_device.serial, ota_package_key))

    return config[ota_package_key]
