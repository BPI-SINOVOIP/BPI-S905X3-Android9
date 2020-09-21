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

import copy
import logging
import random
import sys

from google.protobuf import text_format

from vts.proto import AndroidSystemControlMessage_pb2 as ASysCtrlMsg
from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg
from vts.utils.python.fuzzer import FuzzerUtils
from vts.utils.python.mirror import native_entity_mirror
from vts.utils.python.mirror import py2pb

_DEFAULT_TARGET_BASE_PATHS = ["/system/lib64/hw"]
_DEFAULT_HWBINDER_SERVICE = "default"

INTERFACE = "interface"
API = "api"


class MirrorObjectError(Exception):
    """Raised when there is a general error in manipulating a mirror object."""
    pass


class HalMirror(native_entity_mirror.NativeEntityMirror):
    """The class that acts as the mirror to an Android device's HAL layer.

    This class exists on the host and can be used to communicate to a
    particular HIDL HAL on the target side.

    Attributes:
        _callback_server: the instance of a callback server.
    """

    def __init__(self,
                 client,
                 callback_server,
                 hal_driver_id=None,
                 if_spec_message=None,
                 caller_uid=None):
        super(HalMirror, self).__init__(client, hal_driver_id, if_spec_message,
                                        caller_uid)
        self._callback_server = callback_server

    def InitHalDriver(self, target_type, target_version, target_package,
                      target_component_name, hw_binder_service_name,
                      handler_name, bits):
        """Initiates the driver for a HIDL HAL on the target device and loads
        the interface specification message.

        Args:
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_package: . separated string (e.g., a.b.c) to denote the
                            package name of target component.
            target_component_name: string, the target componet name (e.g., INfc).
            hw_binder_service_name: string, name of the HAL service instance
                                    (e.g. default)
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.

        Raises:
            errors.ComponentLoadingError is raised when error occurs trying to
            create a MirrorObject.
        """
        driver_id = self.LaunchMirrorDriver(
            ASysCtrlMsg.VTS_DRIVER_TYPE_HAL_HIDL,
            "hal_hidl",
            target_type,
            target_version,
            target_package=target_package,
            target_component_name=target_component_name,
            handler_name=handler_name,
            hw_binder_service_name=hw_binder_service_name,
            bits=bits)
        self._driver_id = driver_id

        #TODO: ListApis assumes only one HAL is loaded at a time, need to
        #      figure out a way to get the api_spec when we want to test
        #      multiple HALs together.
        found_api_spec = self._client.ListApis()
        if not found_api_spec:
            raise errors.ComponentLoadingError("No API found for %s" %
                                               target_type)
        if_spec_msg = CompSpecMsg.ComponentSpecificationMessage()
        text_format.Merge(found_api_spec, if_spec_msg)

        self._if_spec_msg = if_spec_msg

    def GetCallbackFunctionID(self, function_pointer):
        """Gets registsred callback function id for the given function_pointer.

        Args:
            function_pointer: the callback function pointer.

        Returns:
            Id for the call back function registered with callback server.
        """
        if self._callback_server:
            id = self._callback_server.GetCallbackId(function_pointer)
            if id is None:
                id = self._callback_server.RegisterCallback(function_pointer)
            return str(id)
        else:
            raise MirrorObjectError("callback server is not started.")

    def GetHidlCallbackInterface(self, interface_name, **kwargs):
        """Gets the ProtoBuf message for a callback interface based on args.

        Args:
            interface_name: string, the callback interface name.
            **kwargs: a dict for the arg name and value pairs

        Returns:
            VariableSpecificationMessage that contains the callback interface
            description.
        """
        var_msg = CompSpecMsg.VariableSpecificationMessage()
        var_msg.name = interface_name
        var_msg.type = CompSpecMsg.TYPE_FUNCTION_POINTER
        var_msg.is_callback = True

        msg = self._if_spec_msg
        specification = self._client.ReadSpecification(
            interface_name, msg.component_class, msg.component_type,
            msg.component_type_version, msg.package)
        logging.info("specification: %s", specification)
        interface = getattr(specification, INTERFACE, None)
        apis = getattr(interface, API, [])
        for api in apis:
            function_pointer = None
            if api.name in kwargs:
                function_pointer = kwargs[api.name]
            else:

                def dummy(*args):
                    """Dummy implementation for any callback function."""
                    logging.info("Entering dummy implementation"
                                 " for callback function: %s", api.name)
                    for arg_index in range(len(args)):
                        logging.info("arg%s: %s", arg_index, args[arg_index])

                function_pointer = dummy
            func_pt_msg = var_msg.function_pointer.add()
            func_pt_msg.function_name = api.name
            func_pt_msg.id = self.GetCallbackFunctionID(function_pointer)

        return var_msg

    def GetHidlTypeInterface(self, interface_name):
        """Gets a HalMirror for HIDL HAL types specification. """
        return self.GetHalMirrorForInterface(interface_name)

    def GetHalMirrorForInterface(self, interface_name, driver_id=None):
        """Gets a HalMirror for a HIDL HAL interface.

        Args:
            interface_name: string, the name of a target interface to read.
            driver_id: int, driver is of the corresponding HIDL HAL interface.

        Returns:
            a host-side mirror of a HIDL HAL interface.
        """
        if not self._if_spec_msg:
            raise MirrorObjectError("spcification is not loaded")
        msg = self._if_spec_msg
        found_api_spec = self._client.ReadSpecification(
            interface_name,
            msg.component_class,
            msg.component_type,
            msg.component_type_version,
            msg.package,
            recursive=True)

        logging.info("found_api_spec %s", found_api_spec)
        if not driver_id:
            driver_id = self._driver_id
        # Instantiate a MirrorObject and return it.
        return HalMirror(self._client, self._callback_server, driver_id,
                         found_api_spec)
