#
# Copyright (C) 2016 The Android Open Source Project
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

import logging

from vts.runners.host import errors

_DEFAULT_HWBINDER_SERVICE = "default"

COMPONENT_CLASS_DICT = {
    "hal_conventional": 1,
    "hal_conventional_submodule": 2,
    "hal_legacy": 3,
    "hal_hidl": 4,
    "hal_hidl_wrapped_conventional": 5,
    "lib_shared": 11
}

COMPONENT_TYPE_DICT = {
    "audio": 1,
    "camera": 2,
    "gps": 3,
    "gnss": 3,
    "light": 4,
    "wifi": 5,
    "mobile": 6,
    "bluetooth": 7,
    "nfc": 8,
    "vibrator": 12,
    "thermal": 13,
    "tv_input": 14,
    "tv_cec": 15,
    "sensors": 16,
    "vehicle": 17,
    "vr": 18,
    "graphics_allocator": 19,
    "graphics_mapper": 20,
    "radio": 21,
    "contexthub": 22,
    "graphics_composer": 23,
    "media_omx": 24,
    "bionic_libm": 1001,
    "bionic_libc": 1002,
    "vndk_libcutils": 1101
}


class MirrorObject(object):
    """The class that mirrors objects on the native side.

    Attributes:
        _client: VtsTcpClient, the client instance that can be used to send
         commands to the target-side's agent.
        _caller_uid: string, the caller's UID if not None.
    """

    def __init__(self, client, caller_uid=None):
        self._client = client
        self._caller_uid = caller_uid

    def CleanUp(self):
        if self._client:
            self._client.Disconnect()

    def SetCallerUid(self, uid):
        """Sets target-side caller's UID.

        Args:
            uid: string, the caller's UID.
        """
        self._caller_uid = uid

    def LaunchMirrorDriver(self,
                           driver_type,
                           target_class,
                           target_type,
                           target_version,
                           target_package="",
                           target_filename=None,
                           target_component_name=None,
                           handler_name=None,
                           service_name=None,
                           hw_binder_service_name=_DEFAULT_HWBINDER_SERVICE,
                           bits=64):
        """Initiates the driver for a lib on the target device and creates a top
        level MirroObject for it.

        Args:
            driver_type: type of
            target_class: string, the target class name (e.g., lib).
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_basepaths: list of strings, the paths to look for target
                             files in. Default is _DEFAULT_TARGET_BASE_PATHS.
            target_package: . separated string (e.g., a.b.c) to denote the
                            package name of target component.
            target_filename: string, the target file name (e.g., libm.so).
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.

        Raises:
            errors.ComponentLoadingError is raised when error occurs trying to
            create a MirrorObject.
        """
        if bits not in [32, 64]:
            raise error.ComponentLoadingError("Invalid value for bits: %s" %
                                              bits)
        if not handler_name:
            handler_name = target_type
        if not service_name:
            service_name = "vts_driver_%s" % handler_name

        # Launch the corresponding driver of the requested HAL on the target.
        logging.info("Init the driver service for %s", target_type)
        target_class_id = COMPONENT_CLASS_DICT[target_class.lower()]
        target_type_id = COMPONENT_TYPE_DICT[target_type.lower()]

        driver_id = self._client.LaunchDriverService(
            driver_type=driver_type,
            service_name=service_name,
            bits=bits,
            file_path=target_filename,
            target_class=target_class_id,
            target_type=target_type_id,
            target_version=target_version,
            target_package=target_package,
            target_component_name=target_component_name,
            hw_binder_service_name=hw_binder_service_name)

        if driver_id == -1:
            raise errors.ComponentLoadingError(
                "Failed to launch driver service %s from file path %s" %
                (target_type, target_filename))

        return driver_id

    def Ping(self):
        """Returns true iff pinging the agent is successful, False otherwise."""
        return self._client.Ping()