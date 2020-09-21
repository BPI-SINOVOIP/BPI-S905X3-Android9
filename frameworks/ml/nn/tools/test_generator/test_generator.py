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

"""NN model compiler

Compile models and examples into NDK-based CTS unit tests
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
import pprint

@contextlib.contextmanager
def smart_open(filename=None):
  if filename and filename != '-':
    fh = open(filename, 'w')
  else:
    fh = sys.stdout

  try:
    yield fh
  finally:
    if fh is not sys.stdout:
      fh.close()

class Phase(object):
  def __init__(self):
    self.__objects = []
    self.__contents = []
    self.__dict_of_objects = {}

  def append(self, obj, x):
    self.__objects.append(obj)
    self.__contents.append(x)
    self.__dict_of_objects[obj.ID()] = obj

  def dump(self, filename):
    for x in self.__contents:
      print ("  " + x + ";", file=filename)

  def objects(self):
    return self.__objects

  def search(self, i):
    return self.__dict_of_objects[i]

# Tracking objects inside a model with a not necessarily unique name and
# an unique number
class NamedObject(object):
  __serial = 0

  def __init__(self, name = "NamedObject"):
    self.__name = name
    self.__id = NamedObject.serial()
    NamedObject.__serial += 1

  def ID(self):
    return self.__id

  def serial():
    return NamedObject.__serial

  def get_name(self):
    return self.__name

  def __str__(self):
    return self.get_name()

  def __hash__(self):
    return self.__id

# Object that can be traversed during topological sorting phase
class Traversable(object):
  def traversable(self):
    return True

class Nontraversable(object):
  def traversable(self):
    return False

# Object that can take input from other objects
class Uses(object):
  all_uses = set()
  def __init__(self, ins = []):
    self.ins = ins.copy()
    Uses.all_uses.add(self)
    for i in ins:
      i.outs.append(self)

# Object that other objects takes its definition from
class Definitions(object):
  def __init__(self, outs = []):
    self.outs = outs.copy()
    for o in outs:
      o.ins.append(self)

class TypeLookup:
  __type_lookup = {
      "INT32": "int32_t",
      "UINT32": "uint32_t",
      "FLOAT32": "float",
      "TENSOR_INT32": "int32_t",
      "TENSOR_FLOAT32": "float",
      "TENSOR_QUANT8_ASYMM": "uint8_t",
#     "OEM_SCALAR": this is service-defined.
      "TENSOR_OEM_BYTE": "uint8_t",
    }

  def get_cpptype(nnapi_type):
    return TypeLookup.__type_lookup[nnapi_type]

  def is_float(nnapi_type):
    return TypeLookup.get_cpptype(nnapi_type) == "float"

  def get_size(nnapi_type):
    return 1 if TypeLookup.get_cpptype(nnapi_type) == "uint8_t" else 4


class Type(object):
  __types =  {}
  __type_serial = 0 # types have their own numbering
  def __init__(self, vt = None, shape = None):
    self.__vt = vt
    self.__shape = shape
    if vt is None or shape is None:
      self.__name = None
      return

    key = str(self)
    if key not in Type.__types:
      self.__id = Type.__type_serial
      Type.__types[str(self)] = self
      Type.__type_serial += 1
    else:
      self.__id = Type.__types[key].__id
    self.__name = "type" + str(self.__id)

  def get_shape(self):
    return self.__shape

  def get_element_type(self):
    return self.__vt

  def get_name(self):
    return self.__name

  def __str__(self):
    return (", ".join([self.__vt, self.__shape]))

  def __hash__(self):
    return self.__id

  def dump(filename):
    for key, value in sorted(Type.__types.items()):
      print ("  OperandType " + str(value.__name) + "(Type::" + str(key) + ");", file=filename)

  def get_raw_shape(self):
    return self.__shape

  def get_parsed_shape(self):
    # Parse shape
    if (self.__shape != "" and self.__shape != "{}"):
      left, sep, right = self.__shape.partition('{')
      real_shape, sep, right = right.partition('}')
      shape = [int(x) for x in real_shape.split(",")]
      # left now looks like "0.0f, 127.5f, "
      scale, sep, zero_point = right.rpartition(',')
      if scale == "":
        if zero_point == "":
          return real_shape, "0", "0"
        return real_shape, zero_point, "0"
      left, sep, scale = scale.partition(',')
      return real_shape, scale.replace("f", ""), zero_point
    else:
      return "", "0", "0"

  def get_nr_elements(self):
    # Parse shape
    nr_elements = 1
    real_shape, scale, zero_point = self.get_parsed_shape()

    if (real_shape != "" and real_shape != "{}"):
      shape = [int(x) for x in real_shape.split(",")]
      nr_elements = reduce((lambda x, y: x*y), shape)
    return nr_elements

  def get_size(self):
    element_size = TypeLookup.get_size(self.__vt)
    return self.get_nr_elements() * element_size

# A value is a typed, named object
class Value(NamedObject):
  def __init__(self, name, vt):
    NamedObject.__init__(self, name)
    self.type = vt

# An operand that can be fed into operations. Also, an operand is always
# declared before operations.
class Operand(Value):
  # All operand declarations in string
  operands = Phase()

  def __init__(self, name, vt):
    Value.__init__(self, name, vt)
    def_string = (
        "auto " + self.get_name() + " = "\
            "model->addOperand(&" + vt.get_name() + ")")
    Operand.operands.append(self, def_string)

  # By default, produce nothing (when asked by the Topological Sort phase)
  def Definition(self):
    pass

  def Reference(self):
    return NamedObject.__str__(self)

  # Print a set of operands in curly braces
  def print_operands(operands):
    return [ x.Reference() for x in operands ]

  # Defined with the model or not
  def is_weight(self):
    return False

# A user-declared input operand
class Input(Operand, Definitions, Traversable):
  # for enumerating inputs
  __next_number = 0
  # Holds reference to all Inputs; used by Topoligcal sort as starting nodes.
  __inputs = set()

  def __init__(self, name, vt, shape, increase_next_number=True):
    Operand.__init__(self, name, Type(vt, shape))
    Definitions.__init__(self)
    Input.__inputs.add(self)
    self.number = Input.__next_number
    if increase_next_number is True:
      Input.__next_number += 1

  def lifetime(self):
    return "MODEL_INPUT"

  def is_internal(self):
    return False

  def get_inputs(exclude_internal = None):
    if exclude_internal is not None:
      external = { x for x in Input.__inputs if not x.is_internal() }
      return external
    else:
      return Input.__inputs

# A user-declared output operand
class Output(Operand, Uses, Nontraversable):
  # for enumerating outputs
  __next_number = 0
  __outputs = []

  def __init__(self, name, vt, shape):
    Operand.__init__(self, name, Type(vt, shape))
    Uses.__init__(self)
    Output.__outputs.append(self)
    self.number = Output.__next_number
    Output.__next_number += 1

  def lifetime(self):
    return "MODEL_OUTPUT"

  # return all unique outputs in the original order
  def get_outputs():
    saw = set()
    unique = [x for x in Output.__outputs if x not in saw and (saw.add(x) or True)]
    return unique

# An output that we don't want to compare the results
class IgnoredOutput(Output):
  __ignored = set()
  def __init__(self, name, vt, shape):
    Output.__init__(self, name, vt, shape)
    IgnoredOutput.__ignored.add(self)
  def gen_ignored():
    ignored_func = """
bool is_ignored(int i) {
  static std::set<int> ignore = {%s};
  return ignore.find(i) != ignore.end();
}""" % ", ".join([str(x.number) for x in IgnoredOutput.__ignored])
    return ignored_func

class ModelArgument:
  __arguments = []

  def __init__(self, arg_type, arg_name):
    self.__arg_type = arg_type
    self.__arg_name = arg_name
    ModelArgument.__arguments.append(" ".join([arg_type, arg_name]))

  def get_arg_type(self):
    return self.__arg_type

  def get_arg_name(self):
    return self.__arg_name

  def get_arguments():
    return ModelArgument.__arguments

  def lifetime(self):
    return "CONSTANT_COPY"

# Print in C float literal format
def pretty_print_as_float(x):
  s = str(float(x))
  if s.find(".") >= 0 or s.find("e") >= 0:
    return s + "f"
  else:
    return s + ".0f"

class Parameter(Input):
  # TODO seems wrong that's an Input.
  def __init__(self, name, vt, shape, initializer):
    Input.__init__(self, name, vt, shape, False)
    self.initializer = initializer
    self.cpptype = TypeLookup.get_cpptype(vt)
  def is_internal(self):
    return True
  def Definition(self):
    init_name = self.get_name() + "_init"
    initializer = [str(x) for x in self.initializer]
    if self.cpptype == "float":
      initializer = [ pretty_print_as_float(x) for x in initializer]
    init = self.cpptype + " " + init_name + "[]"
    init = "static " + init + " = {" + ", ".join(initializer) + "};"
    args = [ self.get_name(), init_name,
            "sizeof(" + self.cpptype + ") * " + str(len(self.initializer)) ]
    stmt = "\n  ".join([init,
                      "model->setOperandValue(" + ", ".join(args)+");"])
    return stmt
  def is_weight(self):
    return True
  def lifetime(self):
    if Configuration.useSHM():
      return "CONSTANT_REFERENCE"
    else:
      return "CONSTANT_COPY"

class Int32Scalar(Parameter):
  def __init__(self, name, value):
    Parameter.__init__(self, name, "INT32", "{}", [value])

class Float32Scalar(Parameter):
  def __init__(self, name, value):
    Parameter.__init__(self, name, "FLOAT32", "{}", [value])

# A compiler-generated intermediate result from an operation
class IntermediateResult(Operand, Definitions, Uses, Traversable):
  def __init__(self, src: Value):
    tmp_name = "tmp" + str(NamedObject.serial())
    Operand.__init__(self, tmp_name, src.type)
    Definitions.__init__(self)
    Uses.__init__(self, [src])

  def lifetime(self):
    return "TEMPORARY_VARIABLE"

# An explicitly declared intermediate result
class Internal(Operand, Definitions, Uses, Traversable):
  def __init__(self, name, vt, shape):
    Operand.__init__(self, name, Type(vt, shape))
    Definitions.__init__(self)
    Uses.__init__(self)

  def lifetime(self):
    return "TEMPORARY_VARIABLE"

# An operation in a model
class Operation(Definitions, Uses, Traversable):
  def __init__(self, optype, ins, outs):
    self.type = ins[0].type
    Definitions.__init__(self, outs)
    Uses.__init__(self, ins)
    self.optype = optype

  def __str__(self):
    inputs = [ str(x) for x in self.ins ]
    return "Operation:" + self.optype + " " + ", ".join(inputs)

  def Reference(self):
    return "operation" + str(self.ID());

  def Definition(self):
    inputs = Operand.print_operands(self.ins);
    outputs = Operand.print_operands(self.outs);
    return "model->addOperation(ANEURALNETWORKS_"+self.optype+", " + \
        "{"+", ".join(inputs)+"}, {" + ", ".join(outputs) + "});"

  # Get Python-ish dump for the op
  def PyDefinition(self):
    py_op_string = """Operation("{optype}", {inputs}).To({outputs})"""
    inputs = [str(x) for x in Operand.print_operands(self.ins)]
    inputs = ", ".join(inputs)
    assert len(self.outs) <= 1
    outputs = str(Operand.print_operands(self.outs)[0])
    ops = {"optype": self.optype, "inputs": inputs, "outputs": outputs}
    return py_op_string.format(**ops)

# Main interface
class Model(object):
  __isRelaxed = False

  def __init__(self):
    self.__currentOp = None

  # TODO turn this into generic binary operations
  def Add(self, i1: Value, i2 = None) -> Operation:
    ins = [i1]
    if i2 is not None:
      ins.append(i2)
    if self.__currentOp is not None:
      ir = IntermediateResult(self.__currentOp)
      self.__currentOp = ir
      ins.append(self.__currentOp)

    op = Operation("ADD", ins, [])

    self.__currentOp = op
    return self

  def Operation(self, op_name, *args):
    ins = [i for i in args]
    outs = []
    op = Operation(op_name, ins, outs)
    self.__currentOp = op
    return self

  def RawAdd(self, i1: Value, i2: Value, o = None) -> Operation:
    ins = [i1, i2]
    outs = []
    if o is not None:
      outs = [o]
    op = Operation("ADD", ins, outs)

    self.__currentOp = op
    return self

  # See CpuExecutor::executeOperation() for the arguments of each op
  def AveragePool(self, input, padding, stride_width, stride_height, filter_width, filter_height, activation):
    ins = [input, padding, stride_width,
           stride_height, filter_width, filter_height, activation]
    outs = []
    op = Operation("AVERAGE_POOL_2D", ins, outs)
    self.__currentOp = op
    return self

  def Concatenation(self, *args):
    ins = [i for i in args]
    outs = []
    op = Operation("CONCATENATION", ins, outs)
    self.__currentOp = op
    return self

  def Conv(self, filter, bias, input, padding, stride_width, stride_height, activation):
    ins = [filter, bias, input, padding, stride_width,
           stride_height, activation]
    outs = []
    op = Operation("CONV_2D", ins, outs)
    self.__currentOp = op
    return self

  def DepthWiseConv(self, filter, bias, input, padding, stride_width, stride_height, depth_multiplier, activation):
    ins = [filter, bias, input, padding, stride_width,
           stride_height, depth_multiplier, activation]
    outs = []
    op = Operation("DEPTHWISE_CONV_2D", ins, outs)
    self.__currentOp = op
    return self

  def FullyConnected(self, input, weights, bias, activation):
    ins = [input, weights, bias, activation]
    outs = []
    op = Operation("FULLY_CONNECTED", ins, outs)
    self.__currentOp = op
    return self

  def Logistic(self, input):
    ins = [input]
    outs = []
    op = Operation("LOGISTIC", ins, outs)
    self.__currentOp = op
    return self

  def L2Pool(self, input, padding, stride_width, stride_height, filter_width, filter_height, activation):
    ins = [input, padding, stride_width,
           stride_height, filter_width, filter_height, activation]
    outs = []
    op = Operation("L2_POOL_2D", ins, outs)
    self.__currentOp = op
    return self

  def MaxPool(self, input, padding, stride_width, stride_height, filter_width, filter_height, activation):
    ins = [input, padding, stride_width,
           stride_height, filter_width, filter_height, activation]
    outs = []
    op = Operation("MAX_POOL_2D", ins, outs)
    self.__currentOp = op
    return self

  def SoftMax(self, input, beta):
    ins = [input, beta]
    outs = []
    op = Operation("SOFTMAX", ins, outs)
    self.__currentOp = op
    return self

  def Reshape(self, input, shape):
    ins = [input, shape]
    outs = []
    op = Operation("RESHAPE", ins, outs)
    self.__currentOp = op
    return self

  def Out(self, o):
    if (type(o) is list or type(o) is tuple):
      for i in o:
        self.__currentOp.outs.append(i)
        i.ins.append(self.__currentOp)
    else:
      self.__currentOp.outs.append(o)
      o.ins.append(self.__currentOp)
    return self

  def To(self, o:Value):
    ret = Model.Out(self, o)
    self.__currentOp = None
    return self

  def RelaxedExecution(self, isRelaxed):
    Model.__isRelaxed = isRelaxed
    return self

  def isRelaxed():
    return Model.__isRelaxed


class FileNames:
  SpecFile = ""

class Example():
  __examples = []
  def __init__(self, list_of_examples):
    Example.__examples.append(list_of_examples)

  def dump_dict(d):
    ret = []
    for k, v in d.items():
      key = str(k)
      suffix = "f"
      if type(k) is not int:
        key = str(k.number)
        if not TypeLookup.is_float(k.type.get_element_type()):
          suffix = ""
      init = ", ".join(
          [str(i) + (suffix if str(i).find(".") != -1 else "") for i in v])
      ret.append("{%s, {%s}}" % (key, init))
    return ", ".join(ret)

  def dump_mixed_types(d):
    ret = []

    float32_dict = {}
    int32_dict = {}
    uint8_dict = {}

    for k, v in d.items():
      key_id = k.ID() if type(k) is not int else k
      ty = Operand.operands.search(key_id).type.get_element_type()
      # find out type of the operand addressed by the key
      if (ty == "TENSOR_FLOAT32"):
        float32_dict[k] = v
      elif (ty == "TENSOR_INT32"):
        int32_dict[k] = v
      elif (ty == "TENSOR_OEM_BYTE"):
        uint8_dict[k] = v
      elif (ty == "TENSOR_QUANT8_ASYMM"):
        uint8_dict[k] = v
      else:
        print ("Unhandled type %s"%ty,  file = sys.stderr)
        assert 0 and "unsupported example type"

    tuple_init = """\
{{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> FLOAT32 map
  {{{float32_dict}}},
  // int -> INT32 map
  {{{int32_dict}}},
  // int -> QUANT8_ASYMM map
  {{{uint8_dict}}}
}}"""
    tuple_contents = {
        'float32_dict': Example.dump_dict(float32_dict),
        'int32_dict': Example.dump_dict(int32_dict),
        'uint8_dict': Example.dump_dict(uint8_dict)
    }
    return tuple_init.format(**tuple_contents)


  def dump(example_file):
    if len(Example.__examples) > 0:
      spec_file = " (from: %s)" % (FileNames.SpecFile)
      print ('// Generated file%s. Do not edit' % (spec_file),
             file = example_file)
    for i, o in Example.__examples:
      print ('// Begin of an example', file = example_file)
      print ('{', file = example_file)
      inputs = Example.dump_mixed_types(i)
      outputs = Example.dump_mixed_types(o)
      print ('//Input(s)\n%s,' % inputs , file = example_file)
      print ('//Output(s)\n%s' % outputs, file = example_file)
      print ('}, // End of an example', file = example_file)

  # Similar to dump_dict, but in python. Used by the slicing tool
  # if referenced is not None, only print operands that are present there
  def py_dump_dict(d, referenced):
    ret = []
    for k, v in d.items():
      if referenced != None and k not in referenced:
        continue
      key = str(k)
      init = pprint.pformat(v)
      ret.append("%s: %s" % (key, init))
    return ", ".join(ret)

  # similar to dump, but in python. Used by the slicing tool
  # if referenced is not None, only print operands that are present there
  def py_dump(example_file, override, referenced):
    if len(Example.__examples) > 0:
      example_no = 0
      example_template = """\
input{no} = {{{inputs}}}
# Only executed during data collection phase
if collecting_data is True:
  Example((input{no}, {{{outputs}}}))
"""
      for i, o in Example.__examples:
        print ('# Begin of an example', file = example_file)
        inputs = Example.py_dump_dict(i, referenced)
        output_list = []
        for k, v in override.items():
          output_list.append("%s: [0] * %d" % (k, v))
        outputs = ",".join(output_list)

        # TODO: handle >1 outputs
        for k, v in o.items():
          assert k.number == 0
        example_contents = {
            'no': example_no,
            'inputs': inputs,
            'outputs': outputs
        }
        print (example_template.format(**example_contents), file = example_file)


def TopologicalSort(format_op):
  start = Input.get_inputs().copy()
  deps = { x: set(x.ins) for x in Uses.all_uses }

  while len(start) > 0:
    cur = start.pop()
    if format_op(cur) is False:
      return
    distinct_outs = set(cur.outs)
    for o in distinct_outs:
      deps[o].remove(cur)
      if len(deps[o]) == 0 and o.traversable():
        start.add(o)

class Configuration:
  use_shm_for_weights = False
  def useSHM():
    return Configuration.use_shm_for_weights

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
    FileNames.SpecFile = os.path.basename(args.spec)
    exec (open(args.spec).read())

  return (args.model, args.example)


def print_cts_op(model_file, op):
  fmt = op.Definition()
  if fmt is not None:
    print ("  %s" % fmt, file = model_file)
  return True

if __name__ == '__main__':
  (model, example) = import_source()
  # Boilerplate
  args = ""
  if len(ModelArgument.get_arguments()) > 0:
    args = ", " + ", ".join(ModelArgument.get_arguments())

  print("Output CTS model: %s" % model, file=sys.stderr)
  print("Output example:" + example, file=sys.stderr)

  with smart_open(model) as model_file:
    spec_file = " (from: %s)" % (FileNames.SpecFile)

    print ('// Generated file%s. Do not edit'%(spec_file), file = model_file)
    print ("void CreateModel(Model *model" + args + ") {", file=model_file)

    # Phase 0: types
    Type.dump(model_file)
    # Phase 1: add operands
    print ("  // Phase 1, operands", file=model_file)
    Operand.operands.dump(model_file)

    # Phase 2: operations
    print ("  // Phase 2, operations", file=model_file)
    TopologicalSort(lambda x: print_cts_op(model_file, x))

    # Phase 3: add inputs and outputs
    print ("  // Phase 3, inputs and outputs", file=model_file)
    inputs = Operand.print_operands(Input.get_inputs(True));
    outputs = Operand.print_operands(Output.get_outputs());
    print ("  model->identifyInputsAndOutputs(\n" +
           "    {"+", ".join(inputs)+"},\n    {" + ", ".join(outputs) + "});",
           file=model_file)

    # Phase 4: set relaxed execution if needed
    if (Model.isRelaxed()):
      print ("  // Phase 4: set relaxed execution", file=model_file)
      print ("  model->relaxComputationFloat32toFloat16(true);", file=model_file)

    # Boilerplate
    print ("  assert(model->isValid());", file=model_file);
    print ("}", file=model_file)
    print (IgnoredOutput.gen_ignored(), file=model_file)

  with smart_open(example) as example_file:
    Example.dump(example_file)
