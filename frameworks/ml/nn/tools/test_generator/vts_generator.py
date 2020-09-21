#!/usr/bin/python3

# Copyright 2017, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""VTS testcase generator

Implements VTS test backend. Shares most logic with the CTS test
generator. Invoked by ml/nn/runtime/test/specs/generate_vts_tests.sh;
See that script for details on how this script is used.

"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
import argparse
from functools import reduce
import math
import os
import struct
import sys
import contextlib
import test_generator
import pprint
# Stuff from test generator
from test_generator import Configuration
from test_generator import Example
from test_generator import Float32Scalar
from test_generator import IgnoredOutput
from test_generator import Input
from test_generator import Int32Scalar
from test_generator import Internal
from test_generator import Model
from test_generator import Operand
from test_generator import Output
from test_generator import Parameter
from test_generator import smart_open

# Take a model from command line
def import_source():
  parser = argparse.ArgumentParser()
  parser.add_argument("spec", help="the spec file")
  parser.add_argument(
      "-m", "--model", help="the output model file", default="-")
  parser.add_argument(
      "-e", "--example", help="the output example file", default="-")
  args = parser.parse_args()

  if os.path.exists(args.spec):
    test_generator.FileNames.SpecFile = os.path.basename(args.spec)
    exec (open(args.spec).read())

  return (args.model, args.example)

# Generate operands in VTS format
def generate_vts_operands():
  # Dump operand definitions
  op_def = """\
        {{
            .type = OperandType::{operand_type},
            .dimensions = {shape},
            .numberOfConsumers = {no_consumers},
            .scale = {scale},
            .zeroPoint = {zero_point},
            .lifetime = OperandLifeTime::{lifetime},
            .location = {{.poolIndex = 0, .offset = {offset}, .length = {length}}},
        }}"""
  offset = 0
  op_definitions = []
  for o in Operand.operands.objects():
    ty = o.type
    no_consumers = len(o.outs) if o.traversable() else 0
    lifetime = o.lifetime()
    length = ty.get_size() if o.is_weight() else 0
    real_shape, scale, zero_point = ty.get_parsed_shape()
    scale = float(scale)
    zero_point = int(zero_point)
    op = {
        "operand_type": ty.get_element_type(),
        "shape": "{%s}" % real_shape,
        "no_consumers": no_consumers,
        "scale": test_generator.pretty_print_as_float(scale),
        "zero_point": str(int(zero_point)),
        "lifetime": lifetime,
        "offset": offset if o.is_weight() else 0,
        "length": length
    }
    offset += length
    op_definitions.append(op_def.format(**op))

  op_vec = """\
    const std::vector<Operand> operands = {{
{0}
    }};""".format(",\n".join(op_definitions))
  return op_vec

# Generate VTS operand values
def generate_vts_operand_values():
  weights = [o for o in Operand.operands.objects() if o.is_weight()]
  binit = []
  for w in weights:
    ty = w.type.get_element_type()
    if ty == "TENSOR_QUANT8_ASYMM":
      binit += w.initializer
    elif ty in {"TENSOR_FLOAT32", "FLOAT32", "TENSOR_INT32", "INT32"}:
      fmt = "f" if (ty == "TENSOR_FLOAT32" or ty == "FLOAT32") else "i"
      for f in w.initializer:
        binit += [int(x) for x in struct.pack(fmt, f)]
    else:
      assert 0 and "Unsupported VTS operand type"

  init_defs = ", ".join([str(x) for x in binit])
  if (init_defs != ""):
    init_defs = "\n      %s\n    " % init_defs
  byte_vec_fmt = """{%s}""" % init_defs
  return byte_vec_fmt

# Generate VTS operations
class VTSOps(object):
  vts_ops = []
  def generate_vts_operation(op):
    try:
      opcode =op.optype
    except AttributeError: # not an op, but things like weights
      return
    op_fmt = """\
        {{
            .type = OperationType::{op_code},
            .inputs = {{{ins}}},
            .outputs = {{{outs}}},
        }}"""
    op_content = {
        'op_code': op.optype,
        'op_type': op.type.get_element_type(),
        'ins': ", ".join([str(x.ID()) for x in op.ins]),
        'outs': ", ".join([str(x.ID()) for x in op.outs]),
    }
    VTSOps.vts_ops.append(op_fmt.format(**op_content))
    return True

def generate_vts_operations(model_file):
  test_generator.TopologicalSort(lambda x: VTSOps.generate_vts_operation(x))
  return ",\n".join(VTSOps.vts_ops)


def generate_vts_model(model_file):
  operand_values_fmt = ""
  if Configuration.useSHM():
    # Boilerplate code for passing weights in shared memory
    operand_values_fmt = """\
    std::vector<uint8_t> operandValues = {{}};
    const uint8_t data[] = {operand_values};

    // Allocate segment of android shared memory, wrapped in hidl_memory.
    // This object will be automatically freed when sharedMemory is destroyed.
    hidl_memory sharedMemory = allocateSharedMemory(sizeof(data));

    // Mmap ashmem into usable address and hold it within the mappedMemory object.
    // MappedMemory will automatically munmap the memory when it is destroyed.
    sp<IMemory> mappedMemory = mapMemory(sharedMemory);

    if (mappedMemory != nullptr) {{
        // Retrieve the mmapped pointer.
        uint8_t* mappedPointer =
                static_cast<uint8_t*>(static_cast<void*>(mappedMemory->getPointer()));

        if (mappedPointer != nullptr) {{
            // Acquire the write lock for the shared memory segment, upload the data,
            // and release the lock.
            mappedMemory->update();
            std::copy(data, data + sizeof(data), mappedPointer);
            mappedMemory->commit();
        }}
    }}

    const std::vector<hidl_memory> pools = {{sharedMemory}};
"""
  else:
    # Passing weights via operandValues
    operand_values_fmt = """\
    std::vector<uint8_t> operandValues = {operand_values};
    const std::vector<hidl_memory> pools = {{}};
"""

  operand_values_val = {
      'operand_values': generate_vts_operand_values()
  }
  operand_values = operand_values_fmt.format(**operand_values_val)
  #  operand_values = operand_values_fmt
  model_fmt = """\
// Generated code. Do not edit
// Create the model
Model createTestModel() {{
{operand_decls}

    const std::vector<Operation> operations = {{
{operations}
    }};

    const std::vector<uint32_t> inputIndexes = {{{input_indices}}};
    const std::vector<uint32_t> outputIndexes = {{{output_indices}}};
{operand_values}
    return {{
        .operands = operands,
        .operations = operations,
        .inputIndexes = inputIndexes,
        .outputIndexes = outputIndexes,
        .operandValues = operandValues,
        .pools = pools,{relaxed_field}
    }};
}}
"""
  model = {
      "operations": generate_vts_operations(sys.stdout),
      "operand_decls": generate_vts_operands(),
      "operand_values": operand_values,
      "output_indices": ", ".join([str(i.ID()) for i in Output.get_outputs()]),
      "input_indices": ", ".join([str(i.ID()) for i in Input.get_inputs(True)]),
      "relaxed_field":
        "\n        .relaxComputationFloat32toFloat16 = true," if (Model.isRelaxed()) else ""
  }
  print(model_fmt.format(**model), file = model_file)

def generate_vts(model_file):
  generate_vts_model(model_file)
  print (IgnoredOutput.gen_ignored(), file=model_file)

if __name__ == "__main__":
  (model, example) = import_source()
  print("Output VTS model: %s" % model, file=sys.stderr)
  print("Output example:" + example, file=sys.stderr)

  with smart_open(model) as model_file:
    generate_vts(model_file)
  with smart_open(example) as example_file:
    Example.dump(example_file)
