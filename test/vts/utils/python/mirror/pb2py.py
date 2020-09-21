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

import logging
import sys

from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg


def PbEnum2PyValue(var):
    """Converts VariableSecificationMessage (Enum) to Python value.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        a converted value.
    """
    return getattr(var.scalar_value, var.scalar_type)


def PbMask2PyValue(var):
    """Converts VariableSecificationMessage (Mask) to Python value.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        a converted value.
    """
    return getattr(var.scalar_value, var.scalar_type)


def PbScalar2PyValue(var):
    """Converts VariableSecificationMessage (Scalar) to Python value.

    Args:
        message: VariableSpecificationMessage to convert.

    Returns:
        Converted scalar value.
    """
    return getattr(var.scalar_value, var.scalar_type)


def PbString2PyString(var):
    """Converts VTS VariableSecificationMessage (String) to Python string.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        Converted string.
    """
    return var.string_value.message


def PbVector2PyList(var):
    """Converts VariableSecificationMessage (Vector) to a Python list.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        A converted list.
    """
    result = []
    for curr_value in var.vector_value:
        if curr_value.type == CompSpecMsg.TYPE_SCALAR:
            result.append(PbScalar2PyValue(curr_value))
        elif curr_value.type == CompSpecMsg.TYPE_STRUCT:
            result.append(PbStruct2PyDict(curr_value))
        else:
            logging.error("unsupported type %s", curr_value.type)
            sys.exit(-1)
    return result


def PbArray2PyList(var):
    """Converts VariableSecificationMessage (Array) to a Python list.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        A converted list.
    """
    result = []
    for curr_value in var.vector_value:
        if curr_value.type == CompSpecMsg.TYPE_SCALAR:
            result.append(PbScalar2PyValue(curr_value))
        elif curr_value.type == CompSpecMsg.TYPE_STRUCT:
            result.append(PbStruct2PyDict(curr_value))
        else:
            logging.error("unsupported type %s", curr_value.type)
            sys.exit(-1)
    return result


def PbStruct2PyDict(var):
    """Converts VariableSecificationMessage (struct) to Python dict.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        a dict, containing the converted data.
    """
    result = {}
    for attr in var.struct_value:
        if attr.type == CompSpecMsg.TYPE_ENUM:
            result[attr.name] = PbEnum2PyValue(attr)
        elif attr.type == CompSpecMsg.TYPE_SCALAR:
            result[attr.name] = PbScalar2PyValue(attr)
        elif attr.type == CompSpecMsg.TYPE_STRING:
            result[attr.name] = PbString2PyString(attr)
        elif attr.type == CompSpecMsg.TYPE_VECTOR:
            result[attr.name] = PbVector2PyList(attr)
        elif attr.type == CompSpecMsg.TYPE_STRUCT:
            result[attr.name] = PbStruct2PyDict(attr)
        elif attr.type == CompSpecMsg.TYPE_Array:
            result[attr.name] = PbArray2PyList(attr)
        else:
            logging.error("PyDict2PbStruct: unsupported type %s",
                          attr.type)
            sys.exit(-1)
    return result


def PbPredefined2PyValue(var):
    """Converts VariableSecificationMessage (PREDEFINED_TYPE) to Python value.

    Args:
        var: VariableSpecificationMessage to convert.

    Returns:
        a converted value.
    """
    return var.predefined_type


def Convert(var):
    """Converts VariableSecificationMessage to Python native data structure.

    Args:
        var: VariableSpecificationMessage of a target variable to convert.

    Returns:
        A list containing the converted Python values.
    """
    if var.type == CompSpecMsg.TYPE_PREDEFINED:
        return PbPredefined2PyValue(var)
    elif var.type == CompSpecMsg.TYPE_SCALAR:
        return PbScalar2PyValue(var)
    elif var.type == CompSpecMsg.TYPE_VECTOR:
        return PbVector2PyList(var)
    elif var.type == CompSpecMsg.TYPE_STRUCT:
        return PbStruct2PyDict(var)
    elif var.type == CompSpecMsg.TYPE_ENUM:
        return PbEnum2PyValue(var)
    elif var.type == CompSpecMsg.TYPE_STRING:
        return PbString2PyString(var)
    elif var.type == CompSpecMsg.TYPE_MASK:
        return PbMask2PyValue(var)
    else:
        logging.error("Got unsupported callback arg type %s" % var.type)
        sys.exit(-1)

    return message
