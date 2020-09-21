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
import sys

from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg


def PyValue2PbEnum(message, pb_spec, py_value):
    """Converts Python value to VTS VariableSecificationMessage (Enum).

    Args:
        message: VariableSpecificationMessage is the current and result
                 value message.
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        py_value: Python value provided by a test case.

    Returns:
        Converted VariableSpecificationMessage if found, None otherwise
    """
    if pb_spec.name:
        message.name = pb_spec.name
    message.type = CompSpecMsg.TYPE_ENUM
    # TODO(yim): derive the type by looking up its predefined_type.
    setattr(message.scalar_value, "int32_t", py_value)


def PyValue2PbScalar(message, pb_spec, py_value):
    """Converts Python value to VTS VariableSecificationMessage (Scalar).

    Args:
        message: VariableSpecificationMessage is the current and result
                 value message.
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        py_value: Python value provided by a test case.

    Returns:
        Converted VariableSpecificationMessage if found, None otherwise
    """
    if pb_spec.name:
        message.name = pb_spec.name
    message.type = CompSpecMsg.TYPE_SCALAR
    message.scalar_type = pb_spec.scalar_type
    setattr(message.scalar_value, pb_spec.scalar_type, py_value)


def PyString2PbString(message, pb_spec, py_value):
    """Converts Python string to VTS VariableSecificationMessage (String).

    Args:
        message: VariableSpecificationMessage is the current and result
                 value message.
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        py_value: Python value provided by a test case.

    Returns:
        Converted VariableSpecificationMessage if found, None otherwise
    """
    if pb_spec.name:
        message.name = pb_spec.name
    message.type = CompSpecMsg.TYPE_STRING
    message.string_value.message = py_value
    message.string_value.length = len(py_value)


def PyList2PbVector(message, pb_spec, py_value):
    """Converts Python list value to VTS VariableSecificationMessage (Vector).

    Args:
        message: VariableSpecificationMessage is the current and result
                 value message.
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        py_value: Python value provided by a test case.

    Returns:
        Converted VariableSpecificationMessage if found, None otherwise
    """
    if pb_spec.name:
        message.name = pb_spec.name
    message.type = CompSpecMsg.TYPE_VECTOR
    if len(py_value) == 0:
        return message

    vector_spec = pb_spec.vector_value[0]
    for curr_value in py_value:
        new_vector_message = message.vector_value.add()
        new_vector_message.CopyFrom(Convert(vector_spec, curr_value))
    message.vector_size = len(py_value)
    return message


def FindSubStructType(pb_spec, sub_struct_name):
    """Finds a specific sub_struct type.

    Args:
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        sub_struct_name: string, the name of a sub struct to look up.

    Returns:
        VariableSpecificationMessage if found or None otherwise.
    """
    for sub_struct in pb_spec.sub_struct:
        if sub_struct.name == sub_struct_name:
            return sub_struct
    return None


def PyDict2PbStruct(message, pb_spec, py_value):
    """Converts Python dict to VTS VariableSecificationMessage (struct).

    Args:
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        py_value: Python value provided by a test case.

    Returns:
        Converted VariableSpecificationMessage if found, None otherwise
    """
    if pb_spec.name:
        message.name = pb_spec.name
    message.type = CompSpecMsg.TYPE_STRUCT
    provided_attrs = set(py_value.keys())
    for attr in pb_spec.struct_value:
        if attr.name in py_value:
            provided_attrs.remove(attr.name)
            curr_value = py_value[attr.name]
            attr_msg = message.struct_value.add()
            if attr.type == CompSpecMsg.TYPE_ENUM:
                PyValue2PbEnum(attr_msg, attr, curr_value)
            elif attr.type == CompSpecMsg.TYPE_SCALAR:
                PyValue2PbScalar(attr_msg, attr, curr_value)
            elif attr.type == CompSpecMsg.TYPE_STRING:
                PyString2PbString(attr_msg, attr, curr_value)
            elif attr.type == CompSpecMsg.TYPE_VECTOR:
                PyList2PbVector(attr_msg, attr, curr_value)
            elif attr.type == CompSpecMsg.TYPE_STRUCT:
                sub_attr = FindSubStructType(pb_spec, attr.predefined_type)
                if sub_attr:
                    PyDict2PbStruct(attr_msg, sub_attr, curr_value)
                else:
                    logging.error("PyDict2PbStruct: substruct not found.")
                    sys.exit(-1)
            else:
                logging.error("PyDict2PbStruct: unsupported type %s",
                              attr.type)
                sys.exit(-1)
        else:
            # TODO: instead crash the test, consider to generate default value
            # in case not provided in the py_value.
            logging.error("PyDict2PbStruct: attr %s not provided", attr.name)
            sys.exit(-1)
    if len(provided_attrs) > 0:
        logging.error("PyDict2PbStruct: provided dictionary included elements" +
                      " not part of the type being converted to: %s",
                      provided_attrs)
        sys.exit(-1)
    return message


def Convert(pb_spec, py_value):
    """Converts Python native data structure to VTS VariableSecificationMessage.

    Args:
        pb_spec: VariableSpecificationMessage which captures the
                 specification of a target attribute.
        py_value: Python value provided by a test case.

    Returns:
        Converted VariableSpecificationMessage if found, None otherwise
    """
    if not pb_spec:
        logging.error("py2pb.Convert: ProtoBuf spec is None", pb_spec)
        return None

    message = CompSpecMsg.VariableSpecificationMessage()
    message.name = pb_spec.name

    if isinstance(py_value, CompSpecMsg.VariableSpecificationMessage):
        message.CopyFrom(py_value)
    elif pb_spec.type == CompSpecMsg.TYPE_STRUCT:
        PyDict2PbStruct(message, pb_spec, py_value)
    elif pb_spec.type == CompSpecMsg.TYPE_ENUM:
        PyValue2PbEnum(message, pb_spec, py_value)
    elif pb_spec.type == CompSpecMsg.TYPE_SCALAR:
        PyValue2PbScalar(message, pb_spec, py_value)
    elif pb_spec.type == CompSpecMsg.TYPE_STRING:
        PyString2PbString(message, pb_spec, py_value)
    elif pb_spec.type == CompSpecMsg.TYPE_VECTOR:
        PyList2PbVector(message, pb_spec, py_value)
    else:
        logging.error("py2pb.Convert: unsupported type %s",
                      pb_spec.type)
        sys.exit(-1)

    return message
