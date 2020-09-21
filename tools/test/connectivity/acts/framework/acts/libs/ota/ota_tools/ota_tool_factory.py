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

from acts.libs.ota.ota_tools.adb_sideload_ota_tool import AdbSideloadOtaTool
from acts.libs.ota.ota_tools.update_device_ota_tool import UpdateDeviceOtaTool

_CONSTRUCTORS = {
    AdbSideloadOtaTool.__name__: lambda command: AdbSideloadOtaTool(command),
    UpdateDeviceOtaTool.__name__: lambda command: UpdateDeviceOtaTool(command),
}
_constructed_tools = {}


def create(ota_tool_class, command):
    """Returns an OtaTool with the given class name.

    If the tool has already been created, the existing instance will be
    returned.

    Args:
        ota_tool_class: the class/type of the tool you wish to use.
        command: the command line tool being used.

    Returns:
        An OtaTool.
    """
    if ota_tool_class in _constructed_tools:
        return _constructed_tools[ota_tool_class]

    if ota_tool_class not in _CONSTRUCTORS:
        raise KeyError('Given Ota Tool class name does not match a known '
                       'name. Found "%s". Expected any of %s. If this tool '
                       'does exist, add it to the _CONSTRUCTORS dict in this '
                       'module.' % (ota_tool_class, _CONSTRUCTORS.keys()))

    new_update_tool = _CONSTRUCTORS[ota_tool_class](command)
    _constructed_tools[ota_tool_class] = new_update_tool

    return new_update_tool
