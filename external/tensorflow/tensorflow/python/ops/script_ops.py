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

"""Script Language Operators. See the @{$python/script_ops} guide.

@@py_func
"""

# pylint: disable=g-bad-name
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import threading

import numpy as np
import six

from tensorflow.python import pywrap_tensorflow
from tensorflow.python.eager import context
from tensorflow.python.framework import function
from tensorflow.python.framework import ops
from tensorflow.python.ops import gen_script_ops
from tensorflow.python.util import nest
from tensorflow.python.util.tf_export import tf_export


class EagerFunc(object):
  """A wrapper for a function owned by an EagerPyFunc."""

  def __init__(self, func, Tout):
    """Constructs an EagerFunc.

    Args:
      func: The function to wrap.
      Tout: A list of datatypes for the output; an empty list if the output is
            None.
    """
    self._func = func
    self._out_dtypes = Tout

  def __call__(self, on_gpu, args):
    """Passes `args` to `self._func`, which is executed eagerly."""
    with context.eager_mode():
      ret = self._func(*args)
      maybe_copy_to_gpu = lambda x: x if not on_gpu else x.gpu()
      if isinstance(ret, (tuple, list)):
        return [
            maybe_copy_to_gpu(ops.convert_to_tensor(x, dtype=dtype))
            for (x, dtype) in zip(ret, self._out_dtypes)
        ]
      elif ret is None:
        return ret
      else:
        return maybe_copy_to_gpu(
            ops.convert_to_tensor(ret, dtype=self._out_dtypes[0]))


class FuncRegistry(object):
  """A helper class to keep track of registered py functions.

  FuncRegistry keeps a map from unique tokens (string) to python
  functions, which takes numpy arrays and outputs numpy arrays.
  """

  def __init__(self):
    self._lock = threading.Lock()
    self._unique_id = 0  # GUARDED_BY(self._lock)
    self._funcs = {}

  def insert(self, func):
    """Registers `func` and returns a unique token for this entry."""
    token = self._next_unique_token()
    self._funcs[token] = func
    return token

  def remove(self, token):
    """Removes the registered function corresponding to `token`."""
    self._funcs.pop(token, None)

  @staticmethod
  def _convert(value, dtype=None):
    """Converts an arg to numpy, avoiding dangerous string and unicode dtypes.

    Numpy pads with zeros when using string and unicode dtypes if different
    components of a tensor have different lengths.  This is bad: ignoring the
    padding is wrong for text data, and removing the padding is wrong for binary
    data.  To avoid this bug, we redo the conversion using an object dtype.
    Additionally, we convert unicode strings to (byte-)strings for
    compatibility.

    Args:
      value: Value to convert to a numpy array.
      dtype: (Optional.) Desired NumPy type for the returned value.

    Returns:
      A numpy array.
    """
    result = np.asarray(value, dtype=dtype, order="C")
    if result.dtype.char == "S" and result is not value:
      return np.asarray(value, order="C", dtype=object)
    elif result.dtype.char == "U" and result is not value:
      value = np.vectorize(lambda x: x.encode("utf8"))(value)
      return np.asarray(value, order="C", dtype=object)
    elif result.dtype.char == "U":
      return result.astype(np.bytes_)
    else:
      return result

  def __call__(self, token, on_gpu, args):
    """Calls the registered function for `token` with args.

    Args:
      token: A key into this `FuncRegistry` identifying which function to call.
      on_gpu: A boolean indicating whether or not `token`'s corresponding
        operation was placed on GPU; only used if the function registered for
        `token` is an `EagerPyFunc`.
      args: The arguments to pass to the function registered for `token`.

    Returns:
      The output of the function registered for `token`.

    Raises:
      ValueError: if no function is registered for `token`.
    """
    func = self._funcs[token]
    if func is None:
      raise ValueError("callback %s is not found" % token)
    if isinstance(func, EagerFunc):
      return func(on_gpu, args)
    else:
      ret = func(*args)
      # Strings seem to lead to a memory leak here if they're not wrapped in a
      # list.
      if isinstance(ret, six.binary_type):
        ret = [ret]
      # Ensures that we return either a single numpy array or a list of numpy
      # arrays.
      if isinstance(ret, (tuple, list)):
        return [self._convert(x) for x in ret]
      else:
        return self._convert(ret)

  def size(self):
    """Returns how many functions are currently registered."""
    return len(self._funcs)

  def _next_unique_token(self):
    """Returns a unique token."""
    with self._lock:
      uid = self._unique_id
      self._unique_id += 1
    return "pyfunc_%d" % uid

# Global registry for py functions.
_py_funcs = FuncRegistry()

pywrap_tensorflow.InitializePyTrampoline(_py_funcs)


class CleanupFunc(object):
  """A helper class to remove a registered function from _py_funcs."""

  def __init__(self, token):
    self._token = token

  def __del__(self):
    if _py_funcs is not None:
      # If _py_funcs is None, the program is most likely in shutdown, and the
      # _py_funcs object has been destroyed already.
      _py_funcs.remove(self._token)


def _internal_py_func(func, inp, Tout, stateful=None, eager=False, name=None):
  """See documentation for py_func and eager_py_func."""

  is_list_or_tuple = False
  if isinstance(Tout, (list, tuple)):
    is_list_or_tuple = True
  else:
    Tout = [Tout]

  if eager:
    func = EagerFunc(func, Tout)

  token = _py_funcs.insert(func)
  # We tie the registered function's lifetime with the current default graph,
  # i.e., when the current graph is destroyed, we remove its py funcs.
  graph = ops.get_default_graph()

  # pylint: disable=protected-access
  while isinstance(graph, function._FuncGraph):
    # If the py_func was declared inside a _FuncGraph, its lifetime should be
    # bound to that of the outer graph instead.
    graph = graph._outer_graph

  cleanup = CleanupFunc(token)

  # TODO(zhifengc): Consider adding a Graph method to collect
  # `cleanup` objects in one of its member.
  if not hasattr(graph, "_cleanup_py_funcs_used_in_graph"):
    graph._cleanup_py_funcs_used_in_graph = []

  # When `graph` is destroyed, elements in _cleanup_py_funcs_used_in_graph
  # will be destroyed and their __del__ will remove the 'token' from
  # the funcs registry.
  graph._cleanup_py_funcs_used_in_graph.append(cleanup)
  # pylint: enable=protected-access

  # pylint: disable=protected-access
  if eager:
    result = gen_script_ops._eager_py_func(
        input=inp, token=token, Tout=Tout, name=name)
  else:
    if stateful:
      result = gen_script_ops._py_func(
          input=inp, token=token, Tout=Tout, name=name)
    else:
      result = gen_script_ops._py_func_stateless(
          input=inp, token=token, Tout=Tout, name=name)
  # pylint: enable=protected-access
  return result if is_list_or_tuple else result[0]


def eager_py_func(func, inp, Tout, name=None):
  """Wraps a python function into a TensorFlow op.

  When the returned op is executed, `func` is invoked with eager execution
  enabled. Inputs are Tensor objects and func must return None or objects
  that may be converted to Tensor objects.

  This function has the same limitations as `py_func` with respect to
  serialization and distribution.

  Args:
    func: A Python function which accepts a list of `Tensor` objects
      having element types that match the corresponding `tf.Tensor` objects
      in `inp` and returns a list of `Tensor` objects (or a single
      `Tensor`, or `None`) having element types that match the
      corresponding values in `Tout`.
    inp: A list of `Tensor` objects.
    Tout: A list or tuple of tensorflow data types or a single tensorflow data
      type if there is only one, indicating what `func` returns; an empty list
      if no value is returned (i.e., if the return value is `None`).
    name: A name for the operation (optional).

  Returns:
    A list of `Tensor` or a single `Tensor` which `func` computes; an empty list
    if `func` returns None.
  """
  return _internal_py_func(func=func, inp=inp, Tout=Tout, eager=True, name=name)


@tf_export("py_func")
def py_func(func, inp, Tout, stateful=True, name=None):
  """Wraps a python function and uses it as a TensorFlow op.

  Given a python function `func`, which takes numpy arrays as its
  arguments and returns numpy arrays as its outputs, wrap this function as an
  operation in a TensorFlow graph. The following snippet constructs a simple
  TensorFlow graph that invokes the `np.sinh()` NumPy function as a operation
  in the graph:

  ```python
  def my_func(x):
    # x will be a numpy array with the contents of the placeholder below
    return np.sinh(x)
  input = tf.placeholder(tf.float32)
  y = tf.py_func(my_func, [input], tf.float32)
  ```

  **N.B.** The `tf.py_func()` operation has the following known limitations:

  * The body of the function (i.e. `func`) will not be serialized in a
    `GraphDef`. Therefore, you should not use this function if you need to
    serialize your model and restore it in a different environment.

  * The operation must run in the same address space as the Python program
    that calls `tf.py_func()`. If you are using distributed TensorFlow, you
    must run a `tf.train.Server` in the same process as the program that calls
    `tf.py_func()` and you must pin the created operation to a device in that
    server (e.g. using `with tf.device():`).

  Args:
    func: A Python function, which accepts `ndarray` objects as arguments and
      returns a list of `ndarray` objects (or a single `ndarray`). This function
      must accept as many arguments as there are tensors in `inp`, and these
      argument types will match the corresponding `tf.Tensor` objects
      in `inp`. The returns `ndarray`s must match the number and types defined
      `Tout`.
      Important Note: Input and output numpy `ndarray`s of `func` are not
      guaranteed to be copies. In some cases their underlying memory will be
      shared with the corresponding TensorFlow tensors.
      In-place modification or storing `func` input or return values in
      python datastructures without explicit (np.)copy
      can have non-deterministic consequences.
    inp: A list of `Tensor` objects.
    Tout: A list or tuple of tensorflow data types or a single tensorflow data
      type if there is only one, indicating what `func` returns.
    stateful: (Boolean.) If True, the function should be considered stateful.
      If a function is stateless, when given the same input it will return the
      same output and have no observable side effects. Optimizations such as
      common subexpression elimination are only performed on stateless
      operations.
    name: A name for the operation (optional).

  Returns:
    A list of `Tensor` or a single `Tensor` which `func` computes.
  """
  if context.in_eager_mode():
    result = func(*[x.numpy() for x in inp])
    result = nest.flatten(result)

    return [x if x is None else ops.convert_to_tensor(x) for x in result]

  return _internal_py_func(
      func=func, inp=inp, Tout=Tout, stateful=stateful, eager=False, name=name)


ops.NotDifferentiable("PyFunc")
ops.NotDifferentiable("PyFuncStateless")
