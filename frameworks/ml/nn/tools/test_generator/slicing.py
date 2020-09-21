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
"""Slicing the input Model file

Invoked by ml/nn/runtime/test/specs/slicing.sh; this Python code is
not intended to be invoked directly by the users. See that script for
details on how to use the slicing tool is used.

This script does the following work:

Perform a topological sort similar to the test generator, except that:
* It would stop at the N-th operation it encounters, and
* Rename the output of the N-th operation to a model output, and
* Name that as the output of the model.
* Also only inputs and weights used by the submodel would be emitted.

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
from test_generator import Input
from test_generator import Int32Scalar
from test_generator import Internal
from test_generator import Model
from test_generator import Output
from test_generator import Parameter
from test_generator import smart_open


# Take a model from command line
def import_source():
  parser = argparse.ArgumentParser()
  parser.add_argument("spec", help="the spec file")
  parser.add_argument(
      "-n", "--number",
      help="number of operations in the sliced model. Default = 1",
      default=1)
  parser.add_argument(
      "-m", "--model", help="the output model file", default="-")
  parser.add_argument(
      "-e", "--example", help="the output example file", default="-")
  args = parser.parse_args()

  if os.path.exists(args.spec):
    test_generator.FileNames.SpecFile = os.path.basename(args.spec)
    exec (open(args.spec).read())
  else:
    print("cannot find file %s" % args.spec)
    sys.exit(1)

  return (args.model, args.example, args.number)


# Slice till the Nth op the topological sort finds
# the output of that op becomes the output of the model
class slicing:

  def __init__(self, threshold):
    self.__nr_op_seen = 0
    self.__threshold = threshold
    self.__last_outs = []
    self.__all_formatted_ops = []
    self.__referenced_operands = set()

  def format_as_py_op(self, op):
    try:
      fmt = op.PyDefinition()
    except AttributeError:  # not an op, but things like weights
      return True
    if fmt is not None:
      self.__nr_op_seen += 1
      if self.__nr_op_seen > self.__threshold:
        return False
      self.__last_outs = op.outs
      for o in op.ins:
        self.__referenced_operands.add(o)
      for o in op.outs:
        self.__referenced_operands.add(o)
      self.__all_formatted_ops.append("model = model.%s" % fmt)
      return True

  def dump(self, model_file):
    for x in self.__all_formatted_ops:
      print(x, file=model_file)

  def dump_example(self, example_file):
    override = {}
    # Make alias for the output variable
    for lo in self.__last_outs:
      override[lo.get_name()] = lo.type.get_nr_elements()
      alias_def = """\
# Alias for the output variable {operand_name}
aliased_output{number} = {operand_name}
"""
      op = {
          'operand_name': lo.get_name(),
          'number': 0 # only support one output as of now
      }
      print (alias_def.format(**op), file=example_file)
    Example.py_dump(example_file, override, self.__referenced_operands)

  def format_operands(self):
    # Dump operand definitions
    op_definitions = []
    for o in test_generator.Operand.operands.objects():
      if o not in self.__referenced_operands:
        continue
      ty = o.type
      raw_shape = ty.get_raw_shape()
      op_def = """{op_name} = {operand}("{op_name}", "{element_type}", "{shape}" """
      if isinstance(o, test_generator.Parameter):
        op_def += """, {initializer})"""
        init = o.initializer
        py_operand_name = "Parameter"
      else:
        op_def += ")"
        init = []
        py_operand_name = "IgnoredOutput" if o in set(
            self.__last_outs) else o.__class__.__name__

      op = {
          "element_type": ty.get_element_type(),
          "shape": ty.get_raw_shape(),
          "op_name": o.get_name(),
          "operand": py_operand_name,
          "initializer": init
      }
      op_definitions.append(op_def.format(**op))
    return "\n".join(op_definitions)


if __name__ == "__main__":
  (model, example, number) = import_source()
  s = slicing(int(number))

  with smart_open(model) as model_file:
    spec_file = " (from: %s)" % (test_generator.FileNames.SpecFile)
    print("# Generated file%s. Do not edit" % (spec_file), file=model_file)
    print("model = Model()", file=model_file)
    test_generator.TopologicalSort(lambda x: s.format_as_py_op(x))
    print(s.format_operands(), file=model_file)
    s.dump(model_file)
  with smart_open(example) as example_file:
    s.dump_example(example_file)
