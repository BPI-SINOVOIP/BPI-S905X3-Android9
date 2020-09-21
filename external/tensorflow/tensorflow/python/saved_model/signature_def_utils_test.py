# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Tests for SignatureDef utils."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.core.framework import types_pb2
from tensorflow.core.protobuf import meta_graph_pb2
from tensorflow.python.framework import constant_op
from tensorflow.python.framework import dtypes
from tensorflow.python.ops import array_ops
from tensorflow.python.platform import test
from tensorflow.python.saved_model import signature_constants
from tensorflow.python.saved_model import signature_def_utils_impl
from tensorflow.python.saved_model import utils


# We'll reuse the same tensor_infos in multiple contexts just for the tests.
# The validator doesn't check shapes so we just omit them.
_STRING = meta_graph_pb2.TensorInfo(
    name="foobar",
    dtype=dtypes.string.as_datatype_enum
)


_FLOAT = meta_graph_pb2.TensorInfo(
    name="foobar",
    dtype=dtypes.float32.as_datatype_enum
)


def _make_signature(inputs, outputs, name=None):
  input_info = {
      input_name: utils.build_tensor_info(tensor)
      for input_name, tensor in inputs.items()
  }
  output_info = {
      output_name: utils.build_tensor_info(tensor)
      for output_name, tensor in outputs.items()
  }
  return signature_def_utils_impl.build_signature_def(input_info, output_info,
                                                      name)


class SignatureDefUtilsTest(test.TestCase):

  def testBuildSignatureDef(self):
    x = array_ops.placeholder(dtypes.float32, 1, name="x")
    x_tensor_info = utils.build_tensor_info(x)
    inputs = dict()
    inputs["foo-input"] = x_tensor_info

    y = array_ops.placeholder(dtypes.float32, name="y")
    y_tensor_info = utils.build_tensor_info(y)
    outputs = dict()
    outputs["foo-output"] = y_tensor_info

    signature_def = signature_def_utils_impl.build_signature_def(
        inputs, outputs, "foo-method-name")
    self.assertEqual("foo-method-name", signature_def.method_name)

    # Check inputs in signature def.
    self.assertEqual(1, len(signature_def.inputs))
    x_tensor_info_actual = signature_def.inputs["foo-input"]
    self.assertEqual("x:0", x_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_FLOAT, x_tensor_info_actual.dtype)
    self.assertEqual(1, len(x_tensor_info_actual.tensor_shape.dim))
    self.assertEqual(1, x_tensor_info_actual.tensor_shape.dim[0].size)

    # Check outputs in signature def.
    self.assertEqual(1, len(signature_def.outputs))
    y_tensor_info_actual = signature_def.outputs["foo-output"]
    self.assertEqual("y:0", y_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_FLOAT, y_tensor_info_actual.dtype)
    self.assertEqual(0, len(y_tensor_info_actual.tensor_shape.dim))

  def testRegressionSignatureDef(self):
    input1 = constant_op.constant("a", name="input-1")
    output1 = constant_op.constant(2.2, name="output-1")
    signature_def = signature_def_utils_impl.regression_signature_def(
        input1, output1)

    self.assertEqual(signature_constants.REGRESS_METHOD_NAME,
                     signature_def.method_name)

    # Check inputs in signature def.
    self.assertEqual(1, len(signature_def.inputs))
    x_tensor_info_actual = (
        signature_def.inputs[signature_constants.REGRESS_INPUTS])
    self.assertEqual("input-1:0", x_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, x_tensor_info_actual.dtype)
    self.assertEqual(0, len(x_tensor_info_actual.tensor_shape.dim))

    # Check outputs in signature def.
    self.assertEqual(1, len(signature_def.outputs))
    y_tensor_info_actual = (
        signature_def.outputs[signature_constants.REGRESS_OUTPUTS])
    self.assertEqual("output-1:0", y_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_FLOAT, y_tensor_info_actual.dtype)
    self.assertEqual(0, len(y_tensor_info_actual.tensor_shape.dim))

  def testClassificationSignatureDef(self):
    input1 = constant_op.constant("a", name="input-1")
    output1 = constant_op.constant("b", name="output-1")
    output2 = constant_op.constant(3.3, name="output-2")
    signature_def = signature_def_utils_impl.classification_signature_def(
        input1, output1, output2)

    self.assertEqual(signature_constants.CLASSIFY_METHOD_NAME,
                     signature_def.method_name)

    # Check inputs in signature def.
    self.assertEqual(1, len(signature_def.inputs))
    x_tensor_info_actual = (
        signature_def.inputs[signature_constants.CLASSIFY_INPUTS])
    self.assertEqual("input-1:0", x_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, x_tensor_info_actual.dtype)
    self.assertEqual(0, len(x_tensor_info_actual.tensor_shape.dim))

    # Check outputs in signature def.
    self.assertEqual(2, len(signature_def.outputs))
    classes_tensor_info_actual = (
        signature_def.outputs[signature_constants.CLASSIFY_OUTPUT_CLASSES])
    self.assertEqual("output-1:0", classes_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, classes_tensor_info_actual.dtype)
    self.assertEqual(0, len(classes_tensor_info_actual.tensor_shape.dim))
    scores_tensor_info_actual = (
        signature_def.outputs[signature_constants.CLASSIFY_OUTPUT_SCORES])
    self.assertEqual("output-2:0", scores_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_FLOAT, scores_tensor_info_actual.dtype)
    self.assertEqual(0, len(scores_tensor_info_actual.tensor_shape.dim))

  def testPredictionSignatureDef(self):
    input1 = constant_op.constant("a", name="input-1")
    input2 = constant_op.constant("b", name="input-2")
    output1 = constant_op.constant("c", name="output-1")
    output2 = constant_op.constant("d", name="output-2")
    signature_def = signature_def_utils_impl.predict_signature_def({
        "input-1": input1,
        "input-2": input2
    }, {"output-1": output1,
        "output-2": output2})

    self.assertEqual(signature_constants.PREDICT_METHOD_NAME,
                     signature_def.method_name)

    # Check inputs in signature def.
    self.assertEqual(2, len(signature_def.inputs))
    input1_tensor_info_actual = (signature_def.inputs["input-1"])
    self.assertEqual("input-1:0", input1_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, input1_tensor_info_actual.dtype)
    self.assertEqual(0, len(input1_tensor_info_actual.tensor_shape.dim))
    input2_tensor_info_actual = (signature_def.inputs["input-2"])
    self.assertEqual("input-2:0", input2_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, input2_tensor_info_actual.dtype)
    self.assertEqual(0, len(input2_tensor_info_actual.tensor_shape.dim))

    # Check outputs in signature def.
    self.assertEqual(2, len(signature_def.outputs))
    output1_tensor_info_actual = (signature_def.outputs["output-1"])
    self.assertEqual("output-1:0", output1_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, output1_tensor_info_actual.dtype)
    self.assertEqual(0, len(output1_tensor_info_actual.tensor_shape.dim))
    output2_tensor_info_actual = (signature_def.outputs["output-2"])
    self.assertEqual("output-2:0", output2_tensor_info_actual.name)
    self.assertEqual(types_pb2.DT_STRING, output2_tensor_info_actual.dtype)
    self.assertEqual(0, len(output2_tensor_info_actual.tensor_shape.dim))

  def testGetShapeAndTypes(self):
    inputs = {
        "input-1": constant_op.constant(["a", "b"]),
        "input-2": array_ops.placeholder(dtypes.float32, [10, 11]),
    }
    outputs = {
        "output-1": array_ops.placeholder(dtypes.float32, [10, 32]),
        "output-2": constant_op.constant([["b"]]),
    }
    signature_def = _make_signature(inputs, outputs)
    self.assertEqual(
        signature_def_utils_impl.get_signature_def_input_shapes(signature_def),
        {"input-1": [2], "input-2": [10, 11]})
    self.assertEqual(
        signature_def_utils_impl.get_signature_def_output_shapes(signature_def),
        {"output-1": [10, 32], "output-2": [1, 1]})
    self.assertEqual(
        signature_def_utils_impl.get_signature_def_input_types(signature_def),
        {"input-1": dtypes.string, "input-2": dtypes.float32})
    self.assertEqual(
        signature_def_utils_impl.get_signature_def_output_types(signature_def),
        {"output-1": dtypes.float32, "output-2": dtypes.string})

  def testGetNonFullySpecifiedShapes(self):
    outputs = {
        "output-1": array_ops.placeholder(dtypes.float32, [None, 10, None]),
        "output-2": array_ops.sparse_placeholder(dtypes.float32),
    }
    signature_def = _make_signature({}, outputs)
    shapes = signature_def_utils_impl.get_signature_def_output_shapes(
        signature_def)
    self.assertEqual(len(shapes), 2)
    # Must compare shapes with as_list() since 2 equivalent non-fully defined
    # shapes are not equal to each other.
    self.assertEqual(shapes["output-1"].as_list(), [None, 10, None])
    # Must compare `dims` since its an unknown shape.
    self.assertEqual(shapes["output-2"].dims, None)

  def _assertValidSignature(self, inputs, outputs, method_name):
    signature_def = signature_def_utils_impl.build_signature_def(
        inputs, outputs, method_name)
    self.assertTrue(
        signature_def_utils_impl.is_valid_signature(signature_def))

  def _assertInvalidSignature(self, inputs, outputs, method_name):
    signature_def = signature_def_utils_impl.build_signature_def(
        inputs, outputs, method_name)
    self.assertFalse(
        signature_def_utils_impl.is_valid_signature(signature_def))

  def testValidSignaturesAreAccepted(self):
    self._assertValidSignature(
        {"inputs": _STRING},
        {"classes": _STRING, "scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertValidSignature(
        {"inputs": _STRING},
        {"classes": _STRING},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertValidSignature(
        {"inputs": _STRING},
        {"scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertValidSignature(
        {"inputs": _STRING},
        {"outputs": _FLOAT},
        signature_constants.REGRESS_METHOD_NAME)

    self._assertValidSignature(
        {"foo": _STRING, "bar": _FLOAT},
        {"baz": _STRING, "qux": _FLOAT},
        signature_constants.PREDICT_METHOD_NAME)

  def testInvalidMethodNameSignatureIsRejected(self):
    # WRONG METHOD
    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"classes": _STRING, "scores": _FLOAT},
        "WRONG method name")

  def testInvalidClassificationSignaturesAreRejected(self):
    # CLASSIFY: wrong types
    self._assertInvalidSignature(
        {"inputs": _FLOAT},
        {"classes": _STRING, "scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"classes": _FLOAT, "scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"classes": _STRING, "scores": _STRING},
        signature_constants.CLASSIFY_METHOD_NAME)

    # CLASSIFY: wrong keys
    self._assertInvalidSignature(
        {},
        {"classes": _STRING, "scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs_WRONG": _STRING},
        {"classes": _STRING, "scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"classes_WRONG": _STRING, "scores": _FLOAT},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {},
        signature_constants.CLASSIFY_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"classes": _STRING, "scores": _FLOAT, "extra_WRONG": _STRING},
        signature_constants.CLASSIFY_METHOD_NAME)

  def testInvalidRegressionSignaturesAreRejected(self):
    # REGRESS: wrong types
    self._assertInvalidSignature(
        {"inputs": _FLOAT},
        {"outputs": _FLOAT},
        signature_constants.REGRESS_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"outputs": _STRING},
        signature_constants.REGRESS_METHOD_NAME)

    # REGRESS: wrong keys
    self._assertInvalidSignature(
        {},
        {"outputs": _FLOAT},
        signature_constants.REGRESS_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs_WRONG": _STRING},
        {"outputs": _FLOAT},
        signature_constants.REGRESS_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"outputs_WRONG": _FLOAT},
        signature_constants.REGRESS_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {},
        signature_constants.REGRESS_METHOD_NAME)

    self._assertInvalidSignature(
        {"inputs": _STRING},
        {"outputs": _FLOAT, "extra_WRONG": _STRING},
        signature_constants.REGRESS_METHOD_NAME)

  def testInvalidPredictSignaturesAreRejected(self):
    # PREDICT: wrong keys
    self._assertInvalidSignature(
        {},
        {"baz": _STRING, "qux": _FLOAT},
        signature_constants.PREDICT_METHOD_NAME)

    self._assertInvalidSignature(
        {"foo": _STRING, "bar": _FLOAT},
        {},
        signature_constants.PREDICT_METHOD_NAME)

if __name__ == "__main__":
  test.main()
