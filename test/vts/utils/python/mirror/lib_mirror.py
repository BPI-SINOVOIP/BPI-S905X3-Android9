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

from google.protobuf import text_format

from vts.runners.host import errors
from vts.proto import AndroidSystemControlMessage_pb2 as ASysCtrlMsg
from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg
from vts.utils.python.mirror import native_entity_mirror


class LibMirror(native_entity_mirror.NativeEntityMirror):
    """The class that acts as the mirror to an Android device's Lib layer.

    This class holds and manages the life cycle of multiple mirror objects that
    map to different lib components.

    One can use this class to create and destroy a lib mirror object.
    """

    def InitLibDriver(self, target_type, target_version, target_package,
                      target_filename, target_basepaths, handler_name, bits):
        """Initiates the driver for a lib on the target device and loads
        the interface specification message.

        Args:
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_package: . separated string (e.g., a.b.c) to denote the
                            package name of target component.
            target_filename: string, the target file name (e.g., libm.so).
            target_basepaths: list of strings, the paths to look for target
                             files in.
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.
        """
        # Get all the libs available on the target.
        lib_list = self._client.ListHals(target_basepaths)
        if not lib_list:
            raise errors.ComponentLoadingError(
                "Could not find any lib under path %s" % target_basepaths)
        logging.debug(lib_list)

        # Find the corresponding filename for Lib target type.
        if target_filename is not None:
            for name in lib_list:
                if name.endswith(target_filename):
                    target_filename = name
                    break
        else:
            for name in lib_list:
                if target_type in name:
                    # TODO: check more exactly (e.g., multiple hits).
                    target_filename = name

        if not target_filename:
            raise errors.ComponentLoadingError(
                "No file found for target type %s." % target_type)

        driver_id = self.LaunchMirrorDriver(
            ASysCtrlMsg.VTS_DRIVER_TYPE_HAL_CONVENTIONAL,
            "lib_shared",
            target_type,
            target_version,
            target_package=target_package,
            target_filename=target_filename,
            handler_name=handler_name,
            bits=bits)

        self._driver_id = driver_id

        #TODO: ListApis assumes only one lib is loaded at a time, need to
        #      figure out a way to get the api_spec when we want to test
        #      multiple libs together.
        found_api_spec = self._client.ListApis()
        if not found_api_spec:
            raise errors.ComponentLoadingError("No API found for %s" %
                                               target_type)
        if_spec_msg = CompSpecMsg.ComponentSpecificationMessage()
        text_format.Merge(found_api_spec, if_spec_msg)

        self._if_spec_msg = if_spec_msg
