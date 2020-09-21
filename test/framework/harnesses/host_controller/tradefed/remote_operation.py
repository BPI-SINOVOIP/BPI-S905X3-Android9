#
# Copyright (C) 2017 The Android Open Source Project
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

import json

from host_controller.tfc import device_info


class RemoteOperationException(Exception):
    """Raised when remote operation fails."""
    pass


class RemoteOperation(object):
    """The operation sent to TradeFed remote manager.

    Args:
        _obj: A JSON object with at least 2 entries, "type" and "version".
    """
    CURRENT_PROTOCOL_VERSION = 8

    def __init__(self, type, **kwargs):
        """Initializes a remote operation.

        Args:
            type: A string, the type of the operation.
            **kwargs: The arguments which are specific to the operation type.
        """
        self._obj = kwargs
        self._obj["type"] = type
        if "version" not in self._obj:
            self._obj["version"] = self.CURRENT_PROTOCOL_VERSION

    def ParseResponse(self, response_str):
        """Parses the response to the operation.

        Args:
            response_str: A JSON string.

        Returns:
            A JSON object.

        Raises:
            RemoteOperationException if the response is an error.
        """
        response = json.loads(response_str)
        if "error" in response:
            raise RemoteOperationException(response["error"])
        return response

    @property
    def type(self):
        """Returns the type of this operation."""
        return self._obj["type"]

    def __str__(self):
        """Converts the JSON object to string."""
        return json.dumps(self._obj)


def ListDevices():
    """Creates an operation of listing devices."""
    return RemoteOperation("LIST_DEVICES")


def ParseListDevicesResponse(json_obj):
    """Parses ListDevices response to a list of DeviceInfo.

    Sample response:
    {"serials": [
        {"product": "unknown", "battery": "0", "variant": "unknown",
         "stub": True, "state": "Available", "build": "unknown",
         "serial": "emulator-5554", "sdk": "unknown"},
    ]}

    Args:
        json_obj: A JSON object, the response to ListDevices.

    Returns:
        A list of DeviceInfo object.
    """
    dev_infos = []
    for dev_obj in json_obj["serials"]:
        if dev_obj["product"] == dev_obj["variant"]:
            run_target = dev_obj["product"]
        else:
            run_target = dev_obj["product"] + ":" + dev_obj["variant"]
        dev_info = device_info.DeviceInfo(
                battery_level=dev_obj["battery"],
                build_id=dev_obj["build"],
                device_serial=dev_obj["serial"],
                product=dev_obj["product"],
                product_variant=dev_obj["variant"],
                run_target=run_target,
                sdk_version=dev_obj["sdk"],
                state=dev_obj["state"],
                stub=dev_obj["stub"])
        dev_infos.append(dev_info)
    return dev_infos


def AllocateDevice(serial):
    """Creates an operation of allocating a device.

    Args:
        serial: The serial number of the device.
    """
    return RemoteOperation("ALLOCATE_DEVICE", serial=serial)


def FreeDevice(serial):
    """Creates an operation of freeing a device.

    Args:
        serial: The serial number of the device.
    """
    return RemoteOperation("FREE_DEVICE", serial=serial)


def Close():
    """Creates an operation of stopping the remote manager."""
    return RemoteOperation("CLOSE")


def AddCommand(time, *command_args):
    """Creates an operation of adding a command to the queue.

    Args:
        time: The time in ms that the command has been executing for. The value
              is non-zero in handover situation.
        command_args: The command to execute.
    """
    return RemoteOperation("ADD_COMMAND", time=time, commandArgs=command_args)


def ExecuteCommand(serial, *command_args):
    """Creates an operation of executing a command on a device.

    Args:
        serial: The serial number of the device.
        command_args: The command to execute.
    """
    return RemoteOperation(
            "EXEC_COMMAND", serial=serial, commandArgs=command_args)


def GetLastCommandResult(serial):
    """Creates an operation of getting last EXEC_COMMAND result on a device.

    Sample response:
    {"status": "INVOCATION_ERROR",
     "invocation_error": "java.lang.NullPointerException",
     "free_device_state": "AVAILABLE"
    }

    Args:
        serial: The serial number of the device.
    """
    return RemoteOperation("GET_LAST_COMMAND_RESULT", serial=serial)
