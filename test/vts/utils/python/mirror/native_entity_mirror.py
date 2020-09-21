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
from vts.utils.python.mirror import mirror_object
from vts.utils.python.mirror import py2pb

_DEFAULT_TARGET_BASE_PATHS = ["/system/lib64/hw"]
_DEFAULT_HWBINDER_SERVICE = "default"

INTERFACE = "interface"
API = "api"


class MirrorObjectError(Exception):
    """Raised when there is a general error in manipulating a mirror object."""
    pass


class VtsEnum(object):
    """Enum's host-side mirror instance."""

    def __init__(self, attribute):
        logging.info(attribute)
        for enumerator, scalar_value in zip(attribute.enum_value.enumerator,
                                            attribute.enum_value.scalar_value):
            setattr(self, enumerator,
                    getattr(scalar_value, attribute.enum_value.scalar_type))


class NativeEntityMirror(mirror_object.MirrorObject):
    """The class that acts as the mirror to an Android device's HAL layer.

    This is the base class used to communicate to a particular HAL or Lib in
    the VTS agent on the target side.

    Attributes:
        _client: the TCP client instance.
        _caller_uid: string, the caller's UID if not None.
        _if_spec_msg: the interface specification message of a host object to
                      mirror.
        _last_raw_code_coverage_data: NativeCodeCoverageRawDataMessage,
                                      last seen raw code coverage data.
    """

    def __init__(self,
                 client,
                 driver_id=None,
                 if_spec_message=None,
                 caller_uid=None):
        super(NativeEntityMirror, self).__init__(client, caller_uid)
        self._driver_id = driver_id
        self._if_spec_msg = if_spec_message
        self._last_raw_code_coverage_data = None

    def GetApi(self, api_name):
        """Gets the ProtoBuf message for given api.

        Args:
            api_name: string, the name of the target function API.

        Returns:
            FunctionSpecificationMessage if found, None otherwise
        """
        logging.debug("GetAPI %s for %s", api_name, self._if_spec_msg)
        # handle reserved methods first.
        if api_name == "notifySyspropsChanged":
            func_msg = CompSpecMsg.FunctionSpecificationMessage()
            func_msg.name = api_name
            return func_msg
        if isinstance(self._if_spec_msg,
                      CompSpecMsg.ComponentSpecificationMessage):
            if len(self._if_spec_msg.interface.api) > 0:
                for api in self._if_spec_msg.interface.api:
                    if api.name == api_name:
                        return copy.copy(api)
        else:
            logging.error("unknown spec type %s", type(self._if_spec_msg))
            sys.exit(1)
        return None

    def GetAttribute(self, attribute_name):
        """Gets the ProtoBuf message for given attribute.

        Args:
            attribute_name: string, the name of a target attribute
                            (optionally excluding the namespace).

        Returns:
            VariableSpecificationMessage if found, None otherwise
        """
        if self._if_spec_msg.attribute:
            for attribute in self._if_spec_msg.attribute:
                if (not attribute.is_const and
                    (attribute.name == attribute_name or
                     attribute.name.endswith("::" + attribute_name))):
                    return attribute
        if (self._if_spec_msg.interface and
                self._if_spec_msg.interface.attribute):
            for attribute in self._if_spec_msg.interface.attribute:
                if (not attribute.is_const and
                    (attribute.name == attribute_name or
                     attribute.name.endswith("::" + attribute_name))):
                    return attribute
        return None

    def GetConstType(self, type_name):
        """Returns the ProtoBuf message for given const type.

        Args:
            type_name: string, the name of the target const data variable.

        Returns:
            VariableSpecificationMessage if found, None otherwise
        """
        try:
            if self._if_spec_msg.attribute:
                for attribute in self._if_spec_msg.attribute:
                    if attribute.is_const and attribute.name == type_name:
                        return attribute
                    elif (attribute.type == CompSpecMsg.TYPE_ENUM and
                          attribute.name.endswith(type_name)):
                        return attribute
            if self._if_spec_msg.interface and self._if_spec_msg.interface.attribute:
                for attribute in self._if_spec_msg.interface.attribute:
                    if attribute.is_const and attribute.name == type_name:
                        return attribute
                    elif (attribute.type == CompSpecMsg.TYPE_ENUM and
                          attribute.name.endswith(type_name)):
                        return attribute
            return None
        except AttributeError as e:
            # TODO: check in advance whether self._if_spec_msg Interface
            # SpecificationMessage.
            return None

    def Py2Pb(self, attribute_name, py_values):
        """Returns the ProtoBuf of a give Python values.

        Args:
            attribute_name: string, the name of a target attribute.
            py_values: Python values.

        Returns:
            Converted VariableSpecificationMessage if found, None otherwise
        """
        attribute_spec = self.GetAttribute(attribute_name)
        if attribute_spec:
            return py2pb.Convert(attribute_spec, py_values)
        return None

    # TODO: Guard against calls to this function after self.CleanUp is called.
    def __getattr__(self, api_name, *args, **kwargs):
        """Calls a target component's API.

        Args:
            api_name: string, the name of an API function to call.
            *args: a list of arguments
            **kwargs: a dict for the arg name and value pairs
        """

        def RemoteCall(*args, **kwargs):
            """Dynamically calls a remote API and returns the result value."""
            func_msg = self.GetApi(api_name)
            if not func_msg:
                raise MirrorObjectError("api %s unknown", func_msg)

            logging.info("remote call %s%s", api_name, args)
            if args:
                for arg_msg, value_msg in zip(func_msg.arg, args):
                    logging.debug("arg msg %s", arg_msg)
                    logging.debug("value %s", value_msg)
                    if value_msg is not None:
                        converted_msg = py2pb.Convert(arg_msg, value_msg)
                        logging.debug("converted_message: %s", converted_msg)
                        arg_msg.CopyFrom(converted_msg)
            else:
                # TODO: use kwargs
                for arg in func_msg.arg:
                    # TODO: handle other
                    if (arg.type == CompSpecMsg.TYPE_SCALAR and
                            arg.scalar_type == "pointer"):
                        arg.scalar_value.pointer = 0
                logging.debug(func_msg)

            call_msg = CompSpecMsg.FunctionCallMessage()
            if self._if_spec_msg.component_class:
                call_msg.component_class = self._if_spec_msg.component_class
            call_msg.hal_driver_id = self._driver_id
            call_msg.api.CopyFrom(func_msg)
            logging.info("final msg %s", call_msg)
            result = self._client.CallApi(
                text_format.MessageToString(call_msg), self._caller_uid)
            logging.debug(result)
            if (isinstance(result, tuple) and len(result) == 2 and
                    isinstance(result[1], dict) and "coverage" in result[1]):
                self._last_raw_code_coverage_data = result[1]["coverage"]
                result = result[0]

            if (result and isinstance(
                    result, CompSpecMsg.VariableSpecificationMessage) and
                    result.type == CompSpecMsg.TYPE_HIDL_INTERFACE):
                if result.hidl_interface_id <= -1:
                    return None
                driver_id = result.hidl_interface_id
                nested_interface_name = result.predefined_type.split("::")[-1]
                logging.debug("Nested interface name: %s",
                              nested_interface_name)
                nested_interface = self.GetHalMirrorForInterface(
                    nested_interface_name, driver_id)
                return nested_interface
            return result

        def MessageGenerator(*args, **kwargs):
            """Dynamically generates a custom message instance."""
            if args:
                return self.Py2Pb(api_name, args)
            else:
                #TODO: handle kwargs
                return None

        #TODO: deprecate this or move it to a separate class.
        def MessageFuzzer(arg_msg):
            """Fuzz a custom message instance."""
            if not self.GetAttribute(api_name):
                raise MirrorObjectError("fuzz arg %s unknown" % arg_msg)

            if arg_msg.type == CompSpecMsg.TYPE_STRUCT:
                index = random.randint(0, len(arg_msg.struct_value))
                count = 0
                for struct_value in arg_msg.struct_value:
                    if count == index:
                        if struct_value.scalar_type == "uint32_t":
                            struct_value.scalar_value.uint32_t ^= FuzzerUtils.mask_uint32_t(
                            )
                        elif struct_value.scalar_type == "int32_t":
                            mask = FuzzerUtils.mask_int32_t()
                            if mask == (1 << 31):
                                struct_value.scalar_value.int32_t *= -1
                                struct_value.scalar_value.int32_t += 1
                            else:
                                struct_value.scalar_value.int32_t ^= mask
                        else:
                            raise MirrorObjectError("support %s" %
                                                    struct_value.scalar_type)
                        break
                    count += 1
                logging.debug("fuzzed %s", arg_msg)
            else:
                raise MirrorObjectError("unsupported fuzz message type %s." %
                                        arg_msg.type)
            return arg_msg

        def ConstGenerator():
            """Dynamically generates a const variable's value."""
            arg_msg = self.GetConstType(api_name)
            if not arg_msg:
                raise MirrorObjectError("const %s unknown" % arg_msg)
            logging.debug("check %s", api_name)
            if arg_msg.type == CompSpecMsg.TYPE_SCALAR:
                ret_v = getattr(arg_msg.scalar_value, arg_msg.scalar_type,
                                None)
                if ret_v is None:
                    raise MirrorObjectError("No value found for type %s in %s."
                                            % (arg_msg.scalar_type, api_name))
                return ret_v
            elif arg_msg.type == CompSpecMsg.TYPE_STRING:
                return arg_msg.string_value.message
            elif arg_msg.type == CompSpecMsg.TYPE_ENUM:
                return VtsEnum(arg_msg)
            raise MirrorObjectError("const %s not found" % api_name)

        # handle APIs.
        func_msg = self.GetApi(api_name)
        if func_msg:
            logging.debug("api %s", func_msg)
            return RemoteCall

        # handle attributes.
        arg_msg = self.GetConstType(api_name)
        if arg_msg:
            logging.debug("const %s *\n%s", api_name, arg_msg)
            return ConstGenerator()

        fuzz = False
        if api_name.endswith("_fuzz"):
            fuzz = True
            api_name = api_name[:-5]
        arg_msg = self.GetAttribute(api_name)
        if arg_msg:
            logging.debug("arg %s", arg_msg)
            if not fuzz:
                return MessageGenerator
            else:
                return MessageFuzzer

        raise MirrorObjectError("unknown api name %s" % api_name)

    def GetRawCodeCoverage(self):
        """Returns any measured raw code coverage data."""
        return self._last_raw_code_coverage_data

    def __str__(self):
        """Prints all the attributes and methods."""
        result = ""
        if self._if_spec_msg:
            if self._if_spec_msg.attribute:
                for attribute in self._if_spec_msg.attribute:
                    result += "attribute %s\n" % attribute.name
            if self._if_spec_msg.interface:
                for attribute in self._if_spec_msg.interface.attribute:
                    result += "interface attribute %s\n" % attribute.name
                for api in self._if_spec_msg.interface.api:
                    result += "api %s\n" % api.name
        return result
