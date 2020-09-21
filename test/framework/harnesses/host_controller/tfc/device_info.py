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

from host_controller.tfc import api_message
from vts.utils.python.controllers import android_device


class DeviceInfo(api_message.ApiMessage):
    """The device information defined by TFC API.

    Attributes:
        _DEVICE_SNAPSHOT: The parameters of host_events.
        _LEASE_HOST_TASKS: The parameters of leasehosttasks.
        _OTHER: The data retrieved from TradeFed host but not used by the API.
        _ALL_KEYS: Union of above.
    """
    _DEVICE_SNAPSHOT = {
            "battery_level",
            "build_id",
            "device_serial",
            "group_name",
            "mac_address",
            "product",
            "product_variant",
            "run_target",
            "sim_operator",
            "sim_state",
            "sdk_version",
            "state"}
    _LEASE_HOST_TASKS = {
            "device_serial",
            "group_name",
            "run_target",
            "state"}
    _OTHER = {"stub"}
    _ALL_KEYS = (_DEVICE_SNAPSHOT | _LEASE_HOST_TASKS | _OTHER)

    def __init__(self, device_serial, **kwargs):
        """Initializes the attributes.

        Args:
            device_serial: The serial number of the device.
            **kwargs: The optional attributes.
        """
        super(DeviceInfo, self).__init__(self._ALL_KEYS,
                                         device_serial=device_serial, **kwargs)

    def IsAvailable(self):
        """Returns whether the device is available for running commands.

        Returns:
            A boolean.
        """
        return getattr(self, "state", None) == "Available"

    def IsStub(self):
        """Returns whether the device is a stub.

        Returns:
            A boolean.
        """
        return getattr(self, "stub", False)

    def ToDeviceSnapshotJson(self):
        """Converts to the parameter of host_events.

        Returns:
            A JSON object.
        """
        return self.ToJson(self._DEVICE_SNAPSHOT)

    def ToLeaseHostTasksJson(self):
        """Converts to the parameter of leasehosttasks.

        Returns:
            A JSON object.
        """
        return self.ToJson(self._LEASE_HOST_TASKS)

    def Extend(self, properties):
        """Adds properties to a DeviceInfo object, using AndroidDevice

        Args:
            properties: list of strings, list of properties to update
        """
        serial = getattr(self, "device_serial", None)
        ad = android_device.AndroidDevice(serial)
        for prop in properties:
            val = getattr(ad, prop, None)
            if val is None:
                continue
            setattr(self, prop, val)
