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

# pylint: disable=invalid-name
"""Test utils for tensorflow."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import contextlib
import gc
import math
import random
import re
import tempfile
import threading

import numpy as np
import six

_portpicker_import_error = None
try:
  import portpicker  # pylint: disable=g-import-not-at-top
except ImportError as _error:
  _portpicker_import_error = _error
  portpicker = None

# pylint: disable=g-import-not-at-top
from google.protobuf import descriptor_pool
from google.protobuf import text_format

from tensorflow.core.framework import graph_pb2
from tensorflow.core.protobuf import config_pb2
from tensorflow.core.protobuf import rewriter_config_pb2
from tensorflow.python import pywrap_tensorflow
from tensorflow.python.client import device_lib
from tensorflow.python.client import session
from tensorflow.python.eager import backprop
from tensorflow.python.eager import context
from tensorflow.python.eager import tape  # pylint: disable=unused-import
from tensorflow.python.framework import device as pydev
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import errors
from tensorflow.python.framework import importer
from tensorflow.python.framework import ops
from tensorflow.python.framework import random_seed
from tensorflow.python.framework import versions
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import resource_variable_ops
from tensorflow.python.ops import variables
from tensorflow.python.platform import googletest
from tensorflow.python.platform import tf_logging as logging
from tensorflow.python.training import server_lib
from tensorflow.python.util import compat
from tensorflow.python.util import nest
from tensorflow.python.util.protobuf import compare
from tensorflow.python.util.tf_export import tf_export


@tf_export("test.gpu_device_name")
def gpu_device_name():
  """Returns the name of a GPU device if available or the empty string."""
  for x in device_lib.list_local_devices():
    if x.device_type == "GPU" or x.device_type == "SYCL":
      return compat.as_str(x.name)
  return ""


def assert_ops_in_graph(expected_ops, graph):
  """Assert all expected operations are found.

  Args:
    expected_ops: `dict<string, string>` of op name to op type.
    graph: Graph to check.
  Returns:
    `dict<string, node>` of node name to node.

  Raises:
    ValueError: If the expected ops are not present in the graph.
  """
  actual_ops = {}
  gd = graph.as_graph_def()
  for node in gd.node:
    if node.name in expected_ops:
      if expected_ops[node.name] != node.op:
        raise ValueError("Expected op for node %s is different. %s vs %s" %
                         (node.name, expected_ops[node.name], node.op))
      actual_ops[node.name] = node
  if set(expected_ops.keys()) != set(actual_ops.keys()):
    raise ValueError("Not all expected ops are present. Expected %s, found %s" %
                     (expected_ops.keys(), actual_ops.keys()))
  return actual_ops


@tf_export("test.assert_equal_graph_def")
def assert_equal_graph_def(actual, expected, checkpoint_v2=False):
  """Asserts that two `GraphDef`s are (mostly) the same.

  Compares two `GraphDef` protos for equality, ignoring versions and ordering of
  nodes, attrs, and control inputs.  Node names are used to match up nodes
  between the graphs, so the naming of nodes must be consistent.

  Args:
    actual: The `GraphDef` we have.
    expected: The `GraphDef` we expected.
    checkpoint_v2: boolean determining whether to ignore randomized attribute
        values that appear in V2 checkpoints.

  Raises:
    AssertionError: If the `GraphDef`s do not match.
    TypeError: If either argument is not a `GraphDef`.
  """
  if not isinstance(actual, graph_pb2.GraphDef):
    raise TypeError(
        "Expected tf.GraphDef for actual, got %s" % type(actual).__name__)
  if not isinstance(expected, graph_pb2.GraphDef):
    raise TypeError(
        "Expected tf.GraphDef for expected, got %s" % type(expected).__name__)

  if checkpoint_v2:
    _strip_checkpoint_v2_randomized(actual)
    _strip_checkpoint_v2_randomized(expected)

  diff = pywrap_tensorflow.EqualGraphDefWrapper(actual.SerializeToString(),
                                                expected.SerializeToString())
  if diff:
    raise AssertionError(compat.as_str(diff))


def assert_meta_graph_protos_equal(tester, a, b):
  """Compares MetaGraphDefs `a` and `b` in unit test class `tester`."""
  # Carefully check the collection_defs
  tester.assertEqual(set(a.collection_def), set(b.collection_def))
  collection_keys = a.collection_def.keys()
  for k in collection_keys:
    a_value = a.collection_def[k]
    b_value = b.collection_def[k]
    proto_type = ops.get_collection_proto_type(k)
    if proto_type:
      a_proto = proto_type()
      b_proto = proto_type()
      # Number of entries in the collections is the same
      tester.assertEqual(
          len(a_value.bytes_list.value), len(b_value.bytes_list.value))
      for (a_value_item, b_value_item) in zip(a_value.bytes_list.value,
                                              b_value.bytes_list.value):
        a_proto.ParseFromString(a_value_item)
        b_proto.ParseFromString(b_value_item)
        tester.assertProtoEquals(a_proto, b_proto)
    else:
      tester.assertEquals(a_value, b_value)
  # Compared the fields directly, remove their raw values from the
  # proto comparison below.
  a.ClearField("collection_def")
  b.ClearField("collection_def")

  # Check the graph_defs.
  assert_equal_graph_def(a.graph_def, b.graph_def, checkpoint_v2=True)
  # Check graph_def versions (ignored by assert_equal_graph_def).
  tester.assertProtoEquals(a.graph_def.versions, b.graph_def.versions)
  # Compared the fields directly, remove their raw values from the
  # proto comparison below.
  a.ClearField("graph_def")
  b.ClearField("graph_def")

  tester.assertProtoEquals(a, b)


# Matches attributes named via _SHARDED_SUFFIX in
# tensorflow/python/training/saver.py
_SHARDED_SAVE_OP_PATTERN = "_temp_[0-9a-z]{32}/part"


def _strip_checkpoint_v2_randomized(graph_def):
  for node in graph_def.node:
    delete_keys = []
    for attr_key in node.attr:
      attr_tensor_value = node.attr[attr_key].tensor
      if attr_tensor_value and len(attr_tensor_value.string_val) == 1:
        attr_tensor_string_value = attr_tensor_value.string_val[0]
        if (attr_tensor_string_value and
            re.match(_SHARDED_SAVE_OP_PATTERN, str(attr_tensor_string_value))):
          delete_keys.append(attr_key)
    for attr_key in delete_keys:
      del node.attr[attr_key]


def IsGoogleCudaEnabled():
  return pywrap_tensorflow.IsGoogleCudaEnabled()


def CudaSupportsHalfMatMulAndConv():
  return pywrap_tensorflow.CudaSupportsHalfMatMulAndConv()


def InstallStackTraceHandler():
  pywrap_tensorflow.InstallStacktraceHandler()


def NHWCToNCHW(input_tensor):
  """Converts the input from the NHWC format to NCHW.

  Args:
    input_tensor: a 4- or 5-D tensor, or an array representing shape

  Returns:
    converted tensor or shape array
  """
  # tensor dim -> new axis order
  new_axes = {4: [0, 3, 1, 2], 5: [0, 4, 1, 2, 3]}
  if isinstance(input_tensor, ops.Tensor):
    ndims = input_tensor.shape.ndims
    return array_ops.transpose(input_tensor, new_axes[ndims])
  else:
    ndims = len(input_tensor)
    return [input_tensor[a] for a in new_axes[ndims]]


def NHWCToNCHW_VECT_C(input_shape_or_tensor):
  """Transforms the input from the NHWC layout to NCHW_VECT_C layout.

  Note: Does not include quantization or type conversion steps, which should
  be applied afterwards.

  Args:
    input_shape_or_tensor: a 4- or 5-D tensor, or an array representing shape

  Returns:
    tensor or shape array transformed into NCHW_VECT_C

  Raises:
    ValueError: if last dimension of `input_shape_or_tensor` is not evenly
        divisible by 4.
  """
  permutations = {5: [0, 3, 1, 2, 4], 6: [0, 4, 1, 2, 3, 5]}
  is_tensor = isinstance(input_shape_or_tensor, ops.Tensor)
  temp_shape = (
      input_shape_or_tensor.shape.as_list()
      if is_tensor else input_shape_or_tensor)
  if temp_shape[-1] % 4 != 0:
    raise ValueError(
        "Last dimension of input must be evenly divisible by 4 to convert to "
        "NCHW_VECT_C.")
  temp_shape[-1] //= 4
  temp_shape.append(4)
  permutation = permutations[len(temp_shape)]
  if is_tensor:
    t = array_ops.reshape(input_shape_or_tensor, temp_shape)
    return array_ops.transpose(t, permutation)
  else:
    return [temp_shape[a] for a in permutation]


def NCHW_VECT_CToNHWC(input_shape_or_tensor):
  """Transforms the input from the NCHW_VECT_C layout to NHWC layout.

  Note: Does not include de-quantization or type conversion steps, which should
  be applied beforehand.

  Args:
    input_shape_or_tensor: a 5- or 6-D tensor, or an array representing shape

  Returns:
    tensor or shape array transformed into NHWC

  Raises:
    ValueError: if last dimension of `input_shape_or_tensor` is not 4.
  """
  permutations = {5: [0, 2, 3, 1, 4], 6: [0, 2, 3, 4, 1, 5]}
  is_tensor = isinstance(input_shape_or_tensor, ops.Tensor)
  input_shape = (
      input_shape_or_tensor.shape.as_list()
      if is_tensor else input_shape_or_tensor)
  if input_shape[-1] != 4:
    raise ValueError("Last dimension of NCHW_VECT_C must be 4.")
  permutation = permutations[len(input_shape)]
  nhwc_shape = [input_shape[a] for a in permutation[:-1]]
  nhwc_shape[-1] *= input_shape[-1]
  if is_tensor:
    t = array_ops.transpose(input_shape_or_tensor, permutation)
    return array_ops.reshape(t, nhwc_shape)
  else:
    return nhwc_shape


def NCHWToNHWC(input_tensor):
  """Converts the input from the NCHW format to NHWC.

  Args:
    input_tensor: a 4- or 5-D tensor, or an array representing shape

  Returns:
    converted tensor or shape array
  """
  # tensor dim -> new axis order
  new_axes = {4: [0, 2, 3, 1], 5: [0, 2, 3, 4, 1]}
  if isinstance(input_tensor, ops.Tensor):
    ndims = input_tensor.shape.ndims
    return array_ops.transpose(input_tensor, new_axes[ndims])
  else:
    ndims = len(input_tensor)
    return [input_tensor[a] for a in new_axes[ndims]]


# TODO(skyewm): remove this eventually
# pylint: disable=protected-access
def _use_c_api_wrapper(fn, use_c_api, *args, **kwargs):
  prev_value = ops._USE_C_API
  ops._USE_C_API = use_c_api
  try:
    # Reset the default graph so it has the C API enabled. We call
    # reset_default_graph() instead of creating a new default Graph context to
    # make this robust to tests that call reset_default_graph(), which requires
    # that the current default graph isn't nested.
    ops.reset_default_graph()
    fn(*args, **kwargs)
  finally:
    ops._USE_C_API = prev_value
    # Make sure default graph reflects prev_value in case next test doesn't call
    # reset_default_graph().
    ops.reset_default_graph()
# pylint: disable=protected-access


def c_api_and_cuda_enabled():
  return ops._USE_C_API and IsGoogleCudaEnabled()


def skip_if(condition):
  """Skips the decorated function if condition is or evaluates to True.

  Args:
    condition: Either an expression that can be used in "if not condition"
               statement, or a callable whose result should be a boolean.
  Returns:
    The wrapped function
  """

  def real_skip_if(fn):

    def wrapper(*args, **kwargs):
      if callable(condition):
        skip = condition()
      else:
        skip = condition
      if not skip:
        fn(*args, **kwargs)

    return wrapper

  return real_skip_if


# TODO(skyewm): remove this eventually
def disable_c_api(fn):
  """Decorator for disabling the C API on a test.

  Note this disables the C API after running the test class's setup/teardown
  methods.

  Args:
    fn: the function to be wrapped

  Returns:
    The wrapped function
  """

  def wrapper(*args, **kwargs):
    _use_c_api_wrapper(fn, False, *args, **kwargs)

  return wrapper


# TODO(skyewm): remove this eventually
def enable_c_api(fn):
  """Decorator for enabling the C API on a test.

  Note this enables the C API after running the test class's setup/teardown
  methods.

  Args:
    fn: the function to be wrapped

  Returns:
    The wrapped function
  """

  def wrapper(*args, **kwargs):
    _use_c_api_wrapper(fn, True, *args, **kwargs)

  return wrapper


# This decorator is a hacky way to run all the test methods in a decorated
# class with and without C API enabled.
# TODO(iga): Remove this and its uses once we switch to using C API by default.
def with_c_api(cls):
  """Adds methods that call original methods but with C API enabled.

  Note this enables the C API in new methods after running the test class's
  setup method. This can be a problem if some objects are created in it
  before the C API is enabled.

  Args:
    cls: class to decorate

  Returns:
    cls with new test methods added
  """
  for name, value in cls.__dict__.copy().items():
    if callable(value) and name.startswith("test"):
      setattr(cls, name + "WithCApi", enable_c_api(value))
  return cls


def assert_no_new_tensors(f):
  """Decorator for asserting that no new Tensors persist after a test.

  Mainly useful for checking that code using the Python C API has correctly
  manipulated reference counts.

  Clears the caches that it knows about, runs the garbage collector, then checks
  that there are no Tensor or Tensor-like objects still around. This includes
  Tensors to which something still has a reference (e.g. from missing
  Py_DECREFs) and uncollectable cycles (i.e. Python reference cycles where one
  of the objects has __del__ defined).

  Args:
    f: The test case to run.
  Returns:
    The decorated test case.
  """

  def decorator(self, **kwargs):
    """Finds existing Tensors, runs the test, checks for new Tensors."""

    def _is_tensor(obj):
      try:
        return (isinstance(obj, ops.Tensor) or
                isinstance(obj, variables.Variable))
      except ReferenceError:
        # If the object no longer exists, we don't care about it.
        return False

    tensors_before = set(id(obj) for obj in gc.get_objects() if _is_tensor(obj))
    outside_graph_key = ops.get_default_graph()._graph_key
    with ops.Graph().as_default():
      # Run the test in a new graph so that collections get cleared when it's
      # done, but inherit the graph key so optimizers behave.
      ops.get_default_graph()._graph_key = outside_graph_key
      f(self, **kwargs)
    # Make an effort to clear caches, which would otherwise look like leaked
    # Tensors.
    backprop._zeros_cache.flush()
    context.get_default_context().scalar_cache().clear()
    gc.collect()
    tensors_after = [
        obj for obj in gc.get_objects()
        if _is_tensor(obj) and id(obj) not in tensors_before
    ]
    if tensors_after:
      raise AssertionError(("%d Tensors not deallocated after test: %s" % (
          len(tensors_after),
          str(tensors_after),
      )))

  return decorator


def assert_no_garbage_created(f):
  """Test method decorator to assert that no garbage has been created.

  Note that this decorator sets DEBUG_SAVEALL, which in some Python interpreters
  cannot be un-set (i.e. will disable garbage collection for any other unit
  tests in the same file/shard).

  Args:
    f: The function to decorate.
  Returns:
    The decorated function.
  """

  def decorator(self, **kwargs):
    """Sets DEBUG_SAVEALL, runs the test, and checks for new garbage."""
    gc.disable()
    previous_debug_flags = gc.get_debug()
    gc.set_debug(gc.DEBUG_SAVEALL)
    gc.collect()
    previous_garbage = len(gc.garbage)
    f(self, **kwargs)
    gc.collect()
    # This will fail if any garbage has been created, typically because of a
    # reference cycle.
    self.assertEqual(previous_garbage, len(gc.garbage))
    # TODO(allenl): Figure out why this debug flag reset doesn't work. It would
    # be nice to be able to decorate arbitrary tests in a large test suite and
    # not hold on to every object in other tests.
    gc.set_debug(previous_debug_flags)
    gc.enable()

  return decorator


def run_in_graph_and_eager_modes(__unused__=None,
                                 graph=None,
                                 config=None,
                                 use_gpu=False,
                                 force_gpu=False,
                                 reset_test=True,
                                 assert_no_eager_garbage=False):
  """Runs the test in both graph and eager modes.

  Args:
    __unused__: Prevents sliently skipping tests.
    graph: Optional graph to use during the returned session.
    config: An optional config_pb2.ConfigProto to use to configure the
      session.
    use_gpu: If True, attempt to run as many ops as possible on GPU.
    force_gpu: If True, pin all ops to `/device:GPU:0`.
    reset_test: If True, tearDown and SetUp the test case again.
    assert_no_eager_garbage: If True, sets DEBUG_SAVEALL on the garbage
      collector and asserts that no extra garbage has been created when running
      the test in eager mode. This will fail if there are reference cycles
      (e.g. a = []; a.append(a)). Off by default because some tests may create
      garbage for legitimate reasons (e.g. they define a class which inherits
      from `object`), and because DEBUG_SAVEALL is sticky in some Python
      interpreters (meaning that tests which rely on objects being collected
      elsewhere in the unit test file will not work). Additionally, checks that
      nothing still has a reference to Tensors that the test allocated.
  Returns:
    Returns a decorator that will run the decorated test function
        using both a graph and using eager execution.
  """

  assert not __unused__, "Add () after run_in_graph_and_eager_modes."

  def decorator(f):
    """Test method decorator."""

    def decorated(self, **kwargs):
      """Decorated the test method."""
      with context.graph_mode():
        with self.test_session(graph, config, use_gpu, force_gpu):
          f(self, **kwargs)

      if reset_test:
        # This decorator runs the wrapped test twice.
        # Reset the test environment between runs.
        self.tearDown()
        self.setUp()

      def run_eager_mode(self, **kwargs):
        if force_gpu:
          gpu_name = gpu_device_name()
          if not gpu_name:
            gpu_name = "/device:GPU:0"
          with context.device(gpu_name):
            f(self)
        elif use_gpu:
          # TODO(xpan): Support softplacement and gpu by default when available.
          f(self, **kwargs)
        else:
          with context.device("/device:CPU:0"):
            f(self, **kwargs)

      if assert_no_eager_garbage:
        run_eager_mode = assert_no_new_tensors(
            assert_no_garbage_created(run_eager_mode))

      with context.eager_mode():
        with ops.Graph().as_default():
          run_eager_mode(self, **kwargs)

    return decorated

  return decorator


@tf_export("test.is_gpu_available")
def is_gpu_available(cuda_only=False, min_cuda_compute_capability=None):
  """Returns whether TensorFlow can access a GPU.

  Args:
    cuda_only: limit the search to CUDA gpus.
    min_cuda_compute_capability: a (major,minor) pair that indicates the minimum
      CUDA compute capability required, or None if no requirement.

  Returns:
    True iff a gpu device of the requested kind is available.
  """

  def compute_capability_from_device_desc(device_desc):
    # TODO(jingyue): The device description generator has to be in sync with
    # this file. Another option is to put compute capability in
    # DeviceAttributes, but I avoided that to keep DeviceAttributes
    # target-independent. Reconsider this option when we have more things like
    # this to keep in sync.
    # LINT.IfChange
    match = re.search(r"compute capability: (\d+)\.(\d+)", device_desc)
    # LINT.ThenChange(//tensorflow/core/\
    #                 common_runtime/gpu/gpu_device.cc)
    if not match:
      return 0, 0
    return int(match.group(1)), int(match.group(2))

  for local_device in device_lib.list_local_devices():
    if local_device.device_type == "GPU":
      if (min_cuda_compute_capability is None or
          compute_capability_from_device_desc(local_device.physical_device_desc)
          >= min_cuda_compute_capability):
        return True
    if local_device.device_type == "SYCL" and not cuda_only:
      return True
  return False


@contextlib.contextmanager
def device(use_gpu):
  """Uses gpu when requested and available."""
  if use_gpu and is_gpu_available():
    dev = "/device:GPU:0"
  else:
    dev = "/device:CPU:0"
  with ops.device(dev):
    yield


@tf_export("test.TestCase")
class TensorFlowTestCase(googletest.TestCase):
  """Base class for tests that need to test TensorFlow.
  """

  def __init__(self, methodName="runTest"):  # pylint: disable=invalid-name
    super(TensorFlowTestCase, self).__init__(methodName)
    self._threads = []
    self._tempdir = None
    self._cached_session = None

  def setUp(self):
    self._ClearCachedSession()
    random.seed(random_seed.DEFAULT_GRAPH_SEED)
    np.random.seed(random_seed.DEFAULT_GRAPH_SEED)
    # Note: The following line is necessary because some test methods may error
    # out from within nested graph contexts (e.g., via assertRaises and
    # assertRaisesRegexp), which may leave ops._default_graph_stack non-empty
    # under certain versions of Python. That would cause
    # ops.reset_default_graph() to throw an exception if the stack were not
    # cleared first.
    ops._default_graph_stack.reset()  # pylint: disable=protected-access
    ops.reset_default_graph()
    random_seed.set_random_seed(random_seed.DEFAULT_GRAPH_SEED)

  def tearDown(self):
    for thread in self._threads:
      thread.check_termination()

    self._ClearCachedSession()

  def _ClearCachedSession(self):
    if self._cached_session is not None:
      self._cached_session.close()
      self._cached_session = None

  def get_temp_dir(self):
    """Returns a unique temporary directory for the test to use.

    If you call this method multiple times during in a test, it will return the
    same folder. However, across different runs the directories will be
    different. This will ensure that across different runs tests will not be
    able to pollute each others environment.
    If you need multiple unique directories within a single test, you should
    use tempfile.mkdtemp as follows:
      tempfile.mkdtemp(dir=self.get_temp_dir()):

    Returns:
      string, the path to the unique temporary directory created for this test.
    """
    if not self._tempdir:
      self._tempdir = tempfile.mkdtemp(dir=googletest.GetTempDir())
    return self._tempdir

  def _AssertProtoEquals(self, a, b, msg=None):
    """Asserts that a and b are the same proto.

    Uses ProtoEq() first, as it returns correct results
    for floating point attributes, and then use assertProtoEqual()
    in case of failure as it provides good error messages.

    Args:
      a: a proto.
      b: another proto.
      msg: Optional message to report on failure.
    """
    if not compare.ProtoEq(a, b):
      compare.assertProtoEqual(self, a, b, normalize_numbers=True, msg=msg)

  def assertProtoEquals(self, expected_message_maybe_ascii, message, msg=None):
    """Asserts that message is same as parsed expected_message_ascii.

    Creates another prototype of message, reads the ascii message into it and
    then compares them using self._AssertProtoEqual().

    Args:
      expected_message_maybe_ascii: proto message in original or ascii form.
      message: the message to validate.
      msg: Optional message to report on failure.
    """
    msg = msg if msg else ""
    if isinstance(expected_message_maybe_ascii, type(message)):
      expected_message = expected_message_maybe_ascii
      self._AssertProtoEquals(expected_message, message)
    elif isinstance(expected_message_maybe_ascii, str):
      expected_message = type(message)()
      text_format.Merge(
          expected_message_maybe_ascii,
          expected_message,
          descriptor_pool=descriptor_pool.Default())
      self._AssertProtoEquals(expected_message, message, msg=msg)
    else:
      assert False, ("Can't compare protos of type %s and %s. %s" %
                     (type(expected_message_maybe_ascii), type(message), msg))

  def assertProtoEqualsVersion(
      self,
      expected,
      actual,
      producer=versions.GRAPH_DEF_VERSION,
      min_consumer=versions.GRAPH_DEF_VERSION_MIN_CONSUMER,
      msg=None):
    expected = "versions { producer: %d min_consumer: %d };\n%s" % (
        producer, min_consumer, expected)
    self.assertProtoEquals(expected, actual, msg=msg)

  def assertStartsWith(self, actual, expected_start, msg=None):
    """Assert that actual.startswith(expected_start) is True.

    Args:
      actual: str
      expected_start: str
      msg: Optional message to report on failure.
    """
    if not actual.startswith(expected_start):
      fail_msg = "%r does not start with %r" % (actual, expected_start)
      fail_msg += " : %r" % (msg) if msg else ""
      self.fail(fail_msg)

  def _eval_tensor(self, tensor):
    if tensor is None:
      return None
    elif isinstance(tensor, ops.EagerTensor):
      return tensor.numpy()
    elif isinstance(tensor, resource_variable_ops.ResourceVariable):
      return tensor.read_value().numpy()
    elif callable(tensor):
      return self._eval_helper(tensor())
    else:
      raise ValueError("Unsupported type %s." % type(tensor))

  def _eval_helper(self, tensors):
    if tensors is None:
      return None
    return nest.map_structure(self._eval_tensor, tensors)

  def evaluate(self, tensors):
    """Evaluates tensors and returns numpy values.

    Args:
      tensors: A Tensor or a nested list/tuple of Tensors.

    Returns:
      tensors numpy values.
    """
    if context.in_eager_mode():
      return self._eval_helper(tensors)
    else:
      sess = ops.get_default_session()
      if sess is None:
        with self.test_session() as sess:
          return sess.run(tensors)
      else:
        return sess.run(tensors)

  # pylint: disable=g-doc-return-or-yield
  @contextlib.contextmanager
  def test_session(self,
                   graph=None,
                   config=None,
                   use_gpu=False,
                   force_gpu=False):
    """Returns a TensorFlow Session for use in executing tests.

    This method should be used for all functional tests.

    This method behaves different than session.Session: for performance reasons
    `test_session` will by default (if `graph` is None) reuse the same session
    across tests. This means you may want to either call the function
    `reset_default_graph()` before tests, or if creating an explicit new graph,
    pass it here (simply setting it with `as_default()` won't do it), which will
    trigger the creation of a new session.

    Use the `use_gpu` and `force_gpu` options to control where ops are run. If
    `force_gpu` is True, all ops are pinned to `/device:GPU:0`. Otherwise, if
    `use_gpu`
    is True, TensorFlow tries to run as many ops on the GPU as possible. If both
    `force_gpu and `use_gpu` are False, all ops are pinned to the CPU.

    Example:
    ```python
    class MyOperatorTest(test_util.TensorFlowTestCase):
      def testMyOperator(self):
        with self.test_session(use_gpu=True):
          valid_input = [1.0, 2.0, 3.0, 4.0, 5.0]
          result = MyOperator(valid_input).eval()
          self.assertEqual(result, [1.0, 2.0, 3.0, 5.0, 8.0]
          invalid_input = [-1.0, 2.0, 7.0]
          with self.assertRaisesOpError("negative input not supported"):
            MyOperator(invalid_input).eval()
    ```

    Args:
      graph: Optional graph to use during the returned session.
      config: An optional config_pb2.ConfigProto to use to configure the
        session.
      use_gpu: If True, attempt to run as many ops as possible on GPU.
      force_gpu: If True, pin all ops to `/device:GPU:0`.

    Returns:
      A Session object that should be used as a context manager to surround
      the graph building and execution code in a test case.
    """
    if self.id().endswith(".test_session"):
      self.skipTest("Not a test.")

    def prepare_config(config):
      """Returns a config for sessions.

      Args:
        config: An optional config_pb2.ConfigProto to use to configure the
          session.
      Returns:
        A config_pb2.ConfigProto object.
      """
      if config is None:
        config = config_pb2.ConfigProto()
        config.allow_soft_placement = not force_gpu
        config.gpu_options.per_process_gpu_memory_fraction = 0.3
      elif force_gpu and config.allow_soft_placement:
        config = config_pb2.ConfigProto().CopyFrom(config)
        config.allow_soft_placement = False
      # Don't perform optimizations for tests so we don't inadvertently run
      # gpu ops on cpu
      config.graph_options.optimizer_options.opt_level = -1
      config.graph_options.rewrite_options.constant_folding = (
          rewriter_config_pb2.RewriterConfig.OFF)
      config.graph_options.rewrite_options.arithmetic_optimization = (
          rewriter_config_pb2.RewriterConfig.OFF)
      return config

    if graph is None:
      if self._cached_session is None:
        self._cached_session = session.Session(
            graph=None, config=prepare_config(config))
      sess = self._cached_session
      with sess.graph.as_default(), sess.as_default():
        if force_gpu:
          # Use the name of an actual device if one is detected, or '/device:GPU:0'
          # otherwise
          gpu_name = gpu_device_name()
          if not gpu_name:
            gpu_name = "/device:GPU:0"
          with sess.graph.device(gpu_name):
            yield sess
        elif use_gpu:
          yield sess
        else:
          with sess.graph.device("/cpu:0"):
            yield sess
    else:
      with session.Session(graph=graph, config=prepare_config(config)) as sess:
        if force_gpu:
          # Use the name of an actual device if one is detected, or '/device:GPU:0'
          # otherwise
          gpu_name = gpu_device_name()
          if not gpu_name:
            gpu_name = "/device:GPU:0"
          with sess.graph.device(gpu_name):
            yield sess
        elif use_gpu:
          yield sess
        else:
          with sess.graph.device("/cpu:0"):
            yield sess

  # pylint: enable=g-doc-return-or-yield

  class _CheckedThread(object):
    """A wrapper class for Thread that asserts successful completion.

    This class should be created using the TensorFlowTestCase.checkedThread()
    method.
    """

    def __init__(self, testcase, target, args=None, kwargs=None):
      """Constructs a new instance of _CheckedThread.

      Args:
        testcase: The TensorFlowTestCase for which this thread is being created.
        target: A callable object representing the code to be executed in the
          thread.
        args: A tuple of positional arguments that will be passed to target.
        kwargs: A dictionary of keyword arguments that will be passed to target.
      """
      self._testcase = testcase
      self._target = target
      self._args = () if args is None else args
      self._kwargs = {} if kwargs is None else kwargs
      self._thread = threading.Thread(target=self._protected_run)
      self._exception = None

      self._is_thread_joined = False

    def _protected_run(self):
      """Target for the wrapper thread. Sets self._exception on failure."""
      try:
        self._target(*self._args, **self._kwargs)
      except Exception as e:  # pylint: disable=broad-except
        self._exception = e

    def start(self):
      """Starts the thread's activity.

      This must be called at most once per _CheckedThread object. It arranges
      for the object's target to be invoked in a separate thread of control.
      """
      self._thread.start()

    def join(self):
      """Blocks until the thread terminates.

      Raises:
        self._testcase.failureException: If the thread terminates with due to
          an exception.
      """
      self._is_thread_joined = True
      self._thread.join()
      if self._exception is not None:
        self._testcase.fail("Error in checkedThread: %s" % str(self._exception))

    def is_alive(self):
      """Returns whether the thread is alive.

      This method returns True just before the run() method starts
      until just after the run() method terminates.

      Returns:
        True if the thread is alive, otherwise False.
      """
      return self._thread.is_alive()

    def check_termination(self):
      """Returns whether the checked thread was properly used and did terminate.

      Every checked thread should be "join"ed after starting, and before the
      test tears down. If it is not joined, it is possible the thread will hang
      and cause flaky failures in tests.

      Raises:
        self._testcase.failureException: If check_termination was called before
        thread was joined.

        RuntimeError: If the thread is not terminated. This means thread was not
        joined with the main thread.
      """
      if self._is_thread_joined:
        if self.is_alive():
          raise RuntimeError(
              "Thread was not joined with main thread, and is still running "
              "when the test finished.")
      else:
        self._testcase.fail("A checked thread was not joined.")

  def checkedThread(self, target, args=None, kwargs=None):
    """Returns a Thread wrapper that asserts 'target' completes successfully.

    This method should be used to create all threads in test cases, as
    otherwise there is a risk that a thread will silently fail, and/or
    assertions made in the thread will not be respected.

    Args:
      target: A callable object to be executed in the thread.
      args: The argument tuple for the target invocation. Defaults to ().
      kwargs: A dictionary of keyword arguments for the target invocation.
        Defaults to {}.

    Returns:
      A wrapper for threading.Thread that supports start() and join() methods.
    """
    ret = TensorFlowTestCase._CheckedThread(self, target, args, kwargs)
    self._threads.append(ret)
    return ret


# pylint: enable=invalid-name

  def assertNear(self, f1, f2, err, msg=None):
    """Asserts that two floats are near each other.

    Checks that |f1 - f2| < err and asserts a test failure
    if not.

    Args:
      f1: A float value.
      f2: A float value.
      err: A float value.
      msg: An optional string message to append to the failure message.
    """
    # f1 == f2 is needed here as we might have: f1, f2 = inf, inf
    self.assertTrue(f1 == f2 or math.fabs(f1 - f2) <= err,
                    "%f != %f +/- %f%s" % (f1, f2, err, " (%s)" % msg
                                           if msg is not None else ""))

  def assertArrayNear(self, farray1, farray2, err, msg=None):
    """Asserts that two float arrays are near each other.

    Checks that for all elements of farray1 and farray2
    |f1 - f2| < err.  Asserts a test failure if not.

    Args:
      farray1: a list of float values.
      farray2: a list of float values.
      err: a float value.
      msg: Optional message to report on failure.
    """
    self.assertEqual(len(farray1), len(farray2), msg=msg)
    for f1, f2 in zip(farray1, farray2):
      self.assertNear(float(f1), float(f2), err, msg=msg)

  def _NDArrayNear(self, ndarray1, ndarray2, err):
    return np.linalg.norm(ndarray1 - ndarray2) < err

  def assertNDArrayNear(self, ndarray1, ndarray2, err, msg=None):
    """Asserts that two numpy arrays have near values.

    Args:
      ndarray1: a numpy ndarray.
      ndarray2: a numpy ndarray.
      err: a float. The maximum absolute difference allowed.
      msg: Optional message to report on failure.
    """
    self.assertTrue(self._NDArrayNear(ndarray1, ndarray2, err), msg=msg)

  def _GetNdArray(self, a):
    if not isinstance(a, np.ndarray):
      a = np.array(a)
    return a

  def _assertArrayLikeAllClose(self, a, b, rtol=1e-6, atol=1e-6, msg=None):
    a = self._GetNdArray(a)
    b = self._GetNdArray(b)
    self.assertEqual(a.shape, b.shape, "Shape mismatch: expected %s, got %s." %
                     (a.shape, b.shape))
    if not np.allclose(a, b, rtol=rtol, atol=atol):
      # Prints more details than np.testing.assert_allclose.
      #
      # NOTE: numpy.allclose (and numpy.testing.assert_allclose)
      # checks whether two arrays are element-wise equal within a
      # tolerance. The relative difference (rtol * abs(b)) and the
      # absolute difference atol are added together to compare against
      # the absolute difference between a and b.  Here, we want to
      # print out which elements violate such conditions.
      cond = np.logical_or(
          np.abs(a - b) > atol + rtol * np.abs(b),
          np.isnan(a) != np.isnan(b))
      if a.ndim:
        x = a[np.where(cond)]
        y = b[np.where(cond)]
        print("not close where = ", np.where(cond))
      else:
        # np.where is broken for scalars
        x, y = a, b
      print("not close lhs = ", x)
      print("not close rhs = ", y)
      print("not close dif = ", np.abs(x - y))
      print("not close tol = ", atol + rtol * np.abs(y))
      print("dtype = %s, shape = %s" % (a.dtype, a.shape))
      # TODO(xpan): There seems to be a bug:
      # tensorflow/compiler/tests:binary_ops_test pass with float32
      # nan even though the equal_nan is False by default internally.
      np.testing.assert_allclose(
          a, b, rtol=rtol, atol=atol, err_msg=msg, equal_nan=True)

  def _assertAllCloseRecursive(self,
                               a,
                               b,
                               rtol=1e-6,
                               atol=1e-6,
                               path=None,
                               msg=None):
    path = path or []
    path_str = (("[" + "][".join([str(p) for p in path]) + "]") if path else "")
    msg = msg if msg else ""

    # Check if a and/or b are namedtuples.
    if hasattr(a, "_asdict"):
      a = a._asdict()
    if hasattr(b, "_asdict"):
      b = b._asdict()
    a_is_dict = isinstance(a, dict)
    if a_is_dict != isinstance(b, dict):
      raise ValueError("Can't compare dict to non-dict, a%s vs b%s. %s" %
                       (path_str, path_str, msg))
    if a_is_dict:
      self.assertItemsEqual(
          a.keys(),
          b.keys(),
          msg="mismatched keys: a%s has keys %s, but b%s has keys %s. %s" %
          (path_str, a.keys(), path_str, b.keys(), msg))
      for k in a:
        path.append(k)
        self._assertAllCloseRecursive(
            a[k], b[k], rtol=rtol, atol=atol, path=path, msg=msg)
        del path[-1]
    elif isinstance(a, (list, tuple)):
      # Try to directly compare a, b as ndarrays; if not work, then traverse
      # through the sequence, which is more expensive.
      try:
        a_as_ndarray = np.array(a)
        b_as_ndarray = np.array(b)
        self._assertArrayLikeAllClose(
            a_as_ndarray,
            b_as_ndarray,
            rtol=rtol,
            atol=atol,
            msg="Mismatched value: a%s is different from b%s. %s" %
            (path_str, path_str, msg))
      except (ValueError, TypeError) as e:
        if len(a) != len(b):
          raise ValueError(
              "Mismatched length: a%s has %d items, but b%s has %d items. %s" %
              (path_str, len(a), path_str, len(b), msg))
        for idx, (a_ele, b_ele) in enumerate(zip(a, b)):
          path.append(str(idx))
          self._assertAllCloseRecursive(
              a_ele, b_ele, rtol=rtol, atol=atol, path=path, msg=msg)
          del path[-1]
    # a and b are ndarray like objects
    else:
      try:
        self._assertArrayLikeAllClose(
            a,
            b,
            rtol=rtol,
            atol=atol,
            msg="Mismatched value: a%s is different from b%s." % (path_str,
                                                                  path_str))
      except TypeError as e:
        msg = "Error: a%s has %s, but b%s has %s" % (
            path_str, type(a), path_str, type(b))
        e.args = ((e.args[0] + ' : ' + msg,) + e.args[1:])
        raise

  def assertAllClose(self, a, b, rtol=1e-6, atol=1e-6, msg=None):
    """Asserts that two structures of numpy arrays, have near values.

    `a` and `b` can be arbitrarily nested structures. A layer of a nested
    structure can be a `dict`, `namedtuple`, `tuple` or `list`.

    Args:
      a: The expected numpy `ndarray`, or anything that can be converted into a
          numpy `ndarray`, or any arbitrarily nested of structure of these.
      b: The actual numpy `ndarray`, or anything that can be converted into a
          numpy `ndarray`, or any arbitrarily nested of structure of these.
      rtol: relative tolerance.
      atol: absolute tolerance.
      msg: Optional message to report on failure.

    Raises:
      ValueError: if only one of `a[p]` and `b[p]` is a dict or
          `a[p]` and `b[p]` have different length, where `[p]` denotes a path
          to the nested structure, e.g. given `a = [(1, 1), {'d': (6, 7)}]` and
          `[p] = [1]['d']`, then `a[p] = (6, 7)`.
    """
    self._assertAllCloseRecursive(a, b, rtol=rtol, atol=atol, msg=msg)

  def assertAllCloseAccordingToType(self,
                                    a,
                                    b,
                                    rtol=1e-6,
                                    atol=1e-6,
                                    float_rtol=1e-6,
                                    float_atol=1e-6,
                                    half_rtol=1e-3,
                                    half_atol=1e-3,
                                    bfloat16_rtol=1e-2,
                                    bfloat16_atol=1e-2,
                                    msg=None):
    """Like assertAllClose, but also suitable for comparing fp16 arrays.

    In particular, the tolerance is reduced to 1e-3 if at least
    one of the arguments is of type float16.

    Args:
      a: the expected numpy ndarray or anything can be converted to one.
      b: the actual numpy ndarray or anything can be converted to one.
      rtol: relative tolerance.
      atol: absolute tolerance.
      float_rtol: relative tolerance for float32.
      float_atol: absolute tolerance for float32.
      half_rtol: relative tolerance for float16.
      half_atol: absolute tolerance for float16.
      bfloat16_rtol: relative tolerance for bfloat16.
      bfloat16_atol: absolute tolerance for bfloat16.
      msg: Optional message to report on failure.
    """
    a = self._GetNdArray(a)
    b = self._GetNdArray(b)
    # types with lower tol are put later to overwrite previous ones.
    if (a.dtype == np.float32 or b.dtype == np.float32 or
        a.dtype == np.complex64 or b.dtype == np.complex64):
      rtol = max(rtol, float_rtol)
      atol = max(atol, float_atol)
    if a.dtype == np.float16 or b.dtype == np.float16:
      rtol = max(rtol, half_rtol)
      atol = max(atol, half_atol)
    if (a.dtype == dtypes.bfloat16.as_numpy_dtype or
        b.dtype == dtypes.bfloat16.as_numpy_dtype):
      rtol = max(rtol, bfloat16_rtol)
      atol = max(atol, bfloat16_atol)

    self.assertAllClose(a, b, rtol=rtol, atol=atol, msg=msg)

  def assertAllEqual(self, a, b, msg=None):
    """Asserts that two numpy arrays have the same values.

    Args:
      a: the expected numpy ndarray or anything can be converted to one.
      b: the actual numpy ndarray or anything can be converted to one.
      msg: Optional message to report on failure.
    """
    msg = msg if msg else ""
    a = self._GetNdArray(a)
    b = self._GetNdArray(b)
    self.assertEqual(a.shape, b.shape, "Shape mismatch: expected %s, got %s."
                     " %s" % (a.shape, b.shape, msg))
    same = (a == b)

    if a.dtype == np.float32 or a.dtype == np.float64:
      same = np.logical_or(same, np.logical_and(np.isnan(a), np.isnan(b)))
    if not np.all(same):
      # Prints more details than np.testing.assert_array_equal.
      diff = np.logical_not(same)
      if a.ndim:
        x = a[np.where(diff)]
        y = b[np.where(diff)]
        print("not equal where = ", np.where(diff))
      else:
        # np.where is broken for scalars
        x, y = a, b
      print("not equal lhs = ", x)
      print("not equal rhs = ", y)
      np.testing.assert_array_equal(a, b, err_msg=msg)

  # pylint: disable=g-doc-return-or-yield
  @contextlib.contextmanager
  def assertRaisesWithPredicateMatch(self, exception_type,
                                     expected_err_re_or_predicate):
    """Returns a context manager to enclose code expected to raise an exception.

    If the exception is an OpError, the op stack is also included in the message
    predicate search.

    Args:
      exception_type: The expected type of exception that should be raised.
      expected_err_re_or_predicate: If this is callable, it should be a function
        of one argument that inspects the passed-in exception and
        returns True (success) or False (please fail the test). Otherwise, the
        error message is expected to match this regular expression partially.

    Returns:
      A context manager to surround code that is expected to raise an
      exception.
    """
    if callable(expected_err_re_or_predicate):
      predicate = expected_err_re_or_predicate
    else:

      def predicate(e):
        err_str = e.message if isinstance(e, errors.OpError) else str(e)
        op = e.op if isinstance(e, errors.OpError) else None
        while op is not None:
          err_str += "\nCaused by: " + op.name
          op = op._original_op  # pylint: disable=protected-access
        logging.info("Searching within error strings: '%s' within '%s'",
                     expected_err_re_or_predicate, err_str)
        return re.search(expected_err_re_or_predicate, err_str)

    try:
      yield
      self.fail(exception_type.__name__ + " not raised")
    except Exception as e:  # pylint: disable=broad-except
      if not isinstance(e, exception_type) or not predicate(e):
        raise AssertionError("Exception of type %s: %s" % (str(type(e)),
                                                           str(e)))

  # pylint: enable=g-doc-return-or-yield

  def assertRaisesOpError(self, expected_err_re_or_predicate):
    return self.assertRaisesWithPredicateMatch(errors.OpError,
                                               expected_err_re_or_predicate)

  def assertShapeEqual(self, np_array, tf_tensor, msg=None):
    """Asserts that a Numpy ndarray and a TensorFlow tensor have the same shape.

    Args:
      np_array: A Numpy ndarray or Numpy scalar.
      tf_tensor: A Tensor.
      msg: Optional message to report on failure.

    Raises:
      TypeError: If the arguments have the wrong type.
    """
    if not isinstance(np_array, (np.ndarray, np.generic)):
      raise TypeError("np_array must be a Numpy ndarray or Numpy scalar")
    if not isinstance(tf_tensor, ops.Tensor):
      raise TypeError("tf_tensor must be a Tensor")
    self.assertAllEqual(
        np_array.shape, tf_tensor.get_shape().as_list(), msg=msg)

  def assertDeviceEqual(self, device1, device2, msg=None):
    """Asserts that the two given devices are the same.

    Args:
      device1: A string device name or TensorFlow `DeviceSpec` object.
      device2: A string device name or TensorFlow `DeviceSpec` object.
      msg: Optional message to report on failure.
    """
    device1 = pydev.canonical_name(device1)
    device2 = pydev.canonical_name(device2)
    self.assertEqual(device1, device2,
                     "Devices %s and %s are not equal. %s" % 
                     (device1, device2, msg))

  # Fix Python 3 compatibility issues
  if six.PY3:
    # pylint: disable=invalid-name

    # Silence a deprecation warning
    assertRaisesRegexp = googletest.TestCase.assertRaisesRegex

    # assertItemsEqual is assertCountEqual as of 3.2.
    assertItemsEqual = googletest.TestCase.assertCountEqual

    # pylint: enable=invalid-name


@tf_export("test.create_local_cluster")
def create_local_cluster(num_workers,
                         num_ps,
                         protocol="grpc",
                         worker_config=None,
                         ps_config=None):
  """Create and start local servers and return the associated `Server` objects.

  Example:
  ```python
  workers, _ = tf.test.create_local_cluster(num_workers=2, num_ps=2)

  worker_sessions = [tf.Session(w.target) for w in workers]

  with tf.device("/job:ps/task:0"):
    ...
  with tf.device("/job:ps/task:1"):
    ...
  with tf.device("/job:worker/task:0"):
    ...
  with tf.device("/job:worker/task:1"):
    ...

  worker_sessions[0].run(...)
  ```

  Args:
    num_workers: Number of worker servers to start.
    num_ps: Number of PS servers to start.
    protocol: Communication protocol.  Allowed values are documented in
      the documentation of `tf.train.Server`.
    worker_config: (optional) ConfigProto to initialize workers. Can be used
      to instantiate multiple devices etc.
    ps_config: (optional) ConfigProto to initialize PS servers.

  Returns:
    A tuple `(worker_servers, ps_servers)`.  `worker_servers` is a list
    of `num_workers` objects of type `tf.train.Server` (all running locally);
    and `ps_servers` is a list of `num_ps` objects of similar type.

  Raises:
    ImportError: if portpicker module was not found at load time
  """
  if _portpicker_import_error:
    raise _portpicker_import_error  # pylint: disable=raising-bad-type
  worker_ports = [portpicker.pick_unused_port() for _ in range(num_workers)]
  ps_ports = [portpicker.pick_unused_port() for _ in range(num_ps)]
  cluster_dict = {
      "worker": ["localhost:%s" % port for port in worker_ports],
      "ps": ["localhost:%s" % port for port in ps_ports]
  }
  cs = server_lib.ClusterSpec(cluster_dict)

  workers = [
      server_lib.Server(
          cs,
          job_name="worker",
          protocol=protocol,
          task_index=ix,
          config=worker_config,
          start=True) for ix in range(num_workers)
  ]
  ps_servers = [
      server_lib.Server(
          cs,
          job_name="ps",
          protocol=protocol,
          task_index=ix,
          config=ps_config,
          start=True) for ix in range(num_ps)
  ]

  return workers, ps_servers


def get_node_def_from_graph(node_name, graph_def):
  """Returns the `NodeDef` instance for given node name in the graph def.

  This method explores only the NodeDefs in `graph_def.node`.

  Args:
    node_name: Name of the NodeDef to search for.
    graph_def: An instance of `GraphDef` proto.

  Returns:
    the `NodeDef` instance whose name field matches the given node_name or None.
  """
  for node_def in graph_def.node:
    if node_def.name == node_name:
      return node_def
  return None


def set_producer_version(graph, producer_version):
  """Sets graph.graph_def_versions.producer to `producer_version`."""
  # The C API doesn't expose altering GraphDefVersions. We can indirectly set
  # it via import_graph_def though.
  graph_def = graph_pb2.GraphDef()
  graph_def.versions.producer = producer_version
  with graph.as_default():
    importer.import_graph_def(graph_def)
  assert graph.graph_def_versions.producer, producer_version
