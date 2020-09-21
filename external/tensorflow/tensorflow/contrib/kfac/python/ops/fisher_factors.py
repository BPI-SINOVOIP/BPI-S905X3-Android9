# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
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
"""FisherFactor definitions."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import abc
import contextlib

import numpy as np
import six

from tensorflow.contrib.kfac.python.ops import utils
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import ops as tf_ops
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import init_ops
from tensorflow.python.ops import linalg_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import special_math_ops
from tensorflow.python.ops import variable_scope
from tensorflow.python.ops import variables
from tensorflow.python.training import moving_averages

# Whether to initialize covariance estimators at a zero matrix (or the identity
# matrix).
INIT_COVARIANCES_AT_ZERO = False

# Whether to zero-debias the moving averages.
ZERO_DEBIAS = False

# When the number of inverses requested from a FisherFactor exceeds this value,
# the inverses are computed using an eigenvalue decomposition.
EIGENVALUE_DECOMPOSITION_THRESHOLD = 2

# Numerical eigenvalues computed from covariance matrix estimates are clipped to
# be at least as large as this value before they are used to compute inverses or
# matrix powers. Must be nonnegative.
EIGENVALUE_CLIPPING_THRESHOLD = 0.0

# Colocate the covariance ops and variables with the input tensors for each
# factor.
COLOCATE_COV_OPS_WITH_INPUTS = True


@contextlib.contextmanager
def maybe_colocate_with(op):
  """Context to colocate with `op` if `COLOCATE_COV_OPS_WITH_INPUTS`."""
  if COLOCATE_COV_OPS_WITH_INPUTS:
    if isinstance(op, (list, tuple)):
      with tf_ops.colocate_with(op[0]):
        yield
    else:
      with tf_ops.colocate_with(op):
        yield
  else:
    yield


def set_global_constants(init_covariances_at_zero=None,
                         zero_debias=None,
                         eigenvalue_decomposition_threshold=None,
                         eigenvalue_clipping_threshold=None,
                         colocate_cov_ops_with_inputs=None):
  """Sets various global constants used by the classes in this module."""
  global INIT_COVARIANCES_AT_ZERO
  global ZERO_DEBIAS
  global EIGENVALUE_DECOMPOSITION_THRESHOLD
  global EIGENVALUE_CLIPPING_THRESHOLD
  global COLOCATE_COV_OPS_WITH_INPUTS

  if init_covariances_at_zero is not None:
    INIT_COVARIANCES_AT_ZERO = init_covariances_at_zero
  if zero_debias is not None:
    ZERO_DEBIAS = zero_debias
  if eigenvalue_decomposition_threshold is not None:
    EIGENVALUE_DECOMPOSITION_THRESHOLD = eigenvalue_decomposition_threshold
  if eigenvalue_clipping_threshold is not None:
    EIGENVALUE_CLIPPING_THRESHOLD = eigenvalue_clipping_threshold
  if colocate_cov_ops_with_inputs is not None:
    COLOCATE_COV_OPS_WITH_INPUTS = colocate_cov_ops_with_inputs


def inverse_initializer(shape, dtype, partition_info=None):  # pylint: disable=unused-argument
  return array_ops.diag(array_ops.ones(shape[0], dtype))


def covariance_initializer(shape, dtype, partition_info=None):  # pylint: disable=unused-argument
  if INIT_COVARIANCES_AT_ZERO:
    return array_ops.diag(array_ops.zeros(shape[0], dtype))
  return array_ops.diag(array_ops.ones(shape[0], dtype))


def diagonal_covariance_initializer(shape, dtype, partition_info):  # pylint: disable=unused-argument
  if INIT_COVARIANCES_AT_ZERO:
    return array_ops.zeros(shape, dtype)
  return array_ops.ones(shape, dtype)


def compute_cov(tensor, tensor_right=None, normalizer=None):
  """Compute the empirical second moment of the rows of a 2D Tensor.

  This function is meant to be applied to random matrices for which the true row
  mean is zero, so that the true second moment equals the true covariance.

  Args:
    tensor: A 2D Tensor.
    tensor_right: An optional 2D Tensor. If provided, this function computes
      the matrix product tensor^T * tensor_right instead of tensor^T * tensor.
    normalizer: optional scalar for the estimator (by default, the normalizer is
        the number of rows of tensor).

  Returns:
    A square 2D Tensor with as many rows/cols as the number of input columns.
  """
  if normalizer is None:
    normalizer = array_ops.shape(tensor)[0]
  if tensor_right is None:
    cov = (
        math_ops.matmul(tensor, tensor, transpose_a=True) / math_ops.cast(
            normalizer, tensor.dtype))
    return (cov + array_ops.transpose(cov)) / math_ops.cast(2.0, cov.dtype)
  else:
    return (math_ops.matmul(tensor, tensor_right, transpose_a=True) /
            math_ops.cast(normalizer, tensor.dtype))


def append_homog(tensor):
  """Appends a homogeneous coordinate to the last dimension of a Tensor.

  Args:
    tensor: A Tensor.

  Returns:
    A Tensor identical to the input but one larger in the last dimension.  The
    new entries are filled with ones.
  """
  rank = len(tensor.shape.as_list())
  shape = array_ops.concat([array_ops.shape(tensor)[:-1], [1]], axis=0)
  ones = array_ops.ones(shape, dtype=tensor.dtype)
  return array_ops.concat([tensor, ones], axis=rank - 1)


def scope_string_from_params(params):
  """Builds a variable scope string name from the given parameters.

  Supported parameters are:
    * tensors
    * booleans
    * ints
    * strings
    * depth-1 tuples/lists of ints
    * any depth tuples/lists of tensors
  Other parameter types will throw an error.

  Args:
    params: A parameter or list of parameters.

  Returns:
    A string to use for the variable scope.

  Raises:
    ValueError: if params includes an unsupported type.
  """
  params = params if isinstance(params, (tuple, list)) else (params,)

  name_parts = []
  for param in params:
    if isinstance(param, (tuple, list)):
      if all([isinstance(p, int) for p in param]):
        name_parts.append("-".join([str(p) for p in param]))
      else:
        name_parts.append(scope_string_from_name(param))
    elif isinstance(param, (str, int, bool)):
      name_parts.append(str(param))
    elif isinstance(param, (tf_ops.Tensor, variables.Variable)):
      name_parts.append(scope_string_from_name(param))
    else:
      raise ValueError("Encountered an unsupported param type {}".format(
          type(param)))
  return "_".join(name_parts)


def scope_string_from_name(tensor):
  if isinstance(tensor, (tuple, list)):
    return "__".join([scope_string_from_name(t) for t in tensor])
  # "gradients/add_4_grad/Reshape:0" -> "gradients_add_4_grad_Reshape"
  return tensor.name.split(":")[0].replace("/", "_")


def scalar_or_tensor_to_string(val):
  return repr(val) if np.isscalar(val) else scope_string_from_name(val)


@six.add_metaclass(abc.ABCMeta)
class FisherFactor(object):
  """Base class for objects modeling factors of approximate Fisher blocks.

  A FisherFactor represents part of an approximate Fisher Information matrix.
  For example, one approximation to the Fisher uses the Kronecker product of two
  FisherFactors A and B, F = kron(A, B). FisherFactors are composed with
  FisherBlocks to construct a block-diagonal approximation to the full Fisher.

  FisherFactors are backed by a single, non-trainable variable that is updated
  by running FisherFactor.make_covariance_update_op(). The shape and type of
  this variable is implementation specific.

  Note that for blocks that aren't based on approximations, a 'factor' can
  be the entire block itself, as is the case for the diagonal and full
  representations.

  Subclasses must implement the _compute_new_cov() method, and the _var_scope
  and _cov_shape properties.
  """

  def __init__(self):
    self.instantiate_covariance()

  @abc.abstractproperty
  def _var_scope(self):
    """Variable scope for this FisherFactor instance.

    Returns:
      string that unique identifies this FisherFactor instance.
    """
    pass

  @abc.abstractproperty
  def _cov_shape(self):
    """The shape of the variable backing this FisherFactor."""
    pass

  @abc.abstractproperty
  def _num_sources(self):
    """The number of things to sum over when updating covariance variable.

    The default make_covariance_update_op function will call _compute_new_cov
    with indices ranging from 0 to _num_sources-1. The typical situation is
    where the factor wants to sum the statistics it computes over multiple
    backpropped "gradients" (typically passed in via "tensors" or
    "outputs_grads" arguments).
    """
    pass

  @abc.abstractproperty
  def _dtype(self):
    """dtype for variable backing this factor."""
    pass

  @property
  def _cov_initializer(self):
    """Function for initializing covariance variable."""
    return covariance_initializer

  def instantiate_covariance(self):
    """Instantiates the covariance Variable as the instance member _cov."""
    with variable_scope.variable_scope(self._var_scope):
      self._cov = variable_scope.get_variable(
          "cov",
          initializer=self._cov_initializer,
          shape=self._cov_shape,
          trainable=False,
          dtype=self._dtype)

  @abc.abstractmethod
  def _compute_new_cov(self, idx=0):
    """Computes minibatch-estimated covariance for a single source.

    Args:
      idx: int in [0, self._num_sources). Which source to use when estimating
        covariance.

    Returns:
      Tensor of same shape as self.get_cov_var().
    """
    pass

  def make_covariance_update_op(self, ema_decay):
    """Constructs and returns the covariance update Op.

    Args:
      ema_decay: The exponential moving average decay (float or Tensor).
    Returns:
      An Op for updating the covariance Variable referenced by _cov.
    """
    new_cov_contribs = tuple(self._compute_new_cov(idx)
                             for idx in range(self._num_sources))
    # This gets the job done but we might want a better solution in the future.
    # In particular, we could have a separate way of specifying where the
    # the cov variables finally end up, independent of where their various
    # contributions are computed.  Right now these are the same thing, but in
    # the future we might want to perform the cov computations on each tower,
    # so that each tower will be considered a "source" (allowing us to reuse
    # the existing "source" code for this).
    with maybe_colocate_with(new_cov_contribs[0]):
      new_cov = math_ops.add_n(new_cov_contribs)
      # Synchronize value across all TPU cores.
      if utils.on_tpu():
        new_cov = utils.cross_replica_mean(new_cov)
      return moving_averages.assign_moving_average(
          self._cov, new_cov, ema_decay, zero_debias=ZERO_DEBIAS)

  @abc.abstractmethod
  def make_inverse_update_ops(self):
    """Create and return update ops corresponding to registered computations."""
    pass

  @abc.abstractmethod
  def get_cov(self):
    """Get full covariance matrix.

    Returns:
      Tensor of shape [n, n]. Represents all parameter-parameter correlations
      captured by this FisherFactor.
    """
    pass

  def get_cov_var(self):
    """Get variable backing this FisherFactor.

    May or may not be the same as self.get_cov()

    Returns:
      Variable of shape self._cov_shape.
    """
    return self._cov

  @abc.abstractmethod
  def left_multiply(self, x, damping):
    """Multiplies 'x' by the damped covariance of this factor.

    Let C be the covariance matrix this factor represents, and
    D = C + damping * I be its damped variant. This method calculates
    matmul(D, vec(x)).

    Args:
      x: Tensor. Represents a single vector. Shape depends on implementation.
      damping: 0-D Tensor. Damping to add to C's diagonal.

    Returns:
      Tensor of same shape as 'x'.
    """
    pass

  @abc.abstractmethod
  def right_multiply(self, x, damping):
    """Multiplies 'x' by the damped covariance of this factor.

    Let C be the covariance matrix this factor represents, and
    D = C + damping * I be its damped variant. This method calculates
    matmul(vec(x), D).

    Args:
      x: Tensor. Represents a single vector. Shape depends on implementation.
      damping: 0-D Tensor. Damping to add to C's diagonal.

    Returns:
      Tensor of same shape as 'x'.
    """
    pass

  @abc.abstractmethod
  def left_multiply_inverse(self, x, damping):
    """Multiplies 'x' by damped inverse of this factor.

    Let C be the covariance matrix this factor represents and
    E = inv(C + damping * I) be its damped inverse. This method calculates
    matmul(E, vec(x)).

    Args:
      x: Tensor. Represents a single vector. Shape depends on implementation.
      damping: 0-D Tensor. Damping to add to C's diagonal.

    Returns:
      Tensor of same shape as 'x'.
    """
    pass

  @abc.abstractmethod
  def right_multiply_inverse(self, x, damping):
    """Multiplies 'x' by damped inverse of this factor.

    Let C be the covariance matrix this factor represents and
    E = inv(C + damping * I) be its damped inverse. This method calculates
    matmul(vec(x), E).

    Args:
      x: Tensor. Represents a single vector. Shape depends on implementation.
      damping: 0-D Tensor. Damping to add to C's diagonal.

    Returns:
      Tensor of same shape as 'x'.
    """
    pass


class InverseProvidingFactor(FisherFactor):
  """Base class for FisherFactors that maintain inverses explicitly.

  This class explicitly calculates and stores inverses of covariance matrices
  provided by the underlying FisherFactor implementation. It is assumed that
  vectors can be represented as 2-D matrices.

  Subclasses must implement the _compute_new_cov method, and the _var_scope and
  _cov_shape properties.
  """

  # TODO(b/69108481): This class (and its subclasses) should be refactored to
  # serve the matrix quantities it computes as both (potentially stale)
  # variables, updated by the inverse update ops, and fresh values stored in
  # tensors that recomputed once every session.run() call.  Currently matpower
  # and damp_inverse have the former behavior, while eigendecomposition has
  # the latter.

  def __init__(self):
    self._inverses_by_damping = {}
    self._matpower_by_exp_and_damping = {}
    self._eigendecomp = None

    super(InverseProvidingFactor, self).__init__()

  def register_damped_inverse(self, damping):
    """Registers a damped inverse needed by a FisherBlock.

    This creates a variable and signals make_inverse_update_ops to make the
    corresponding update op.  The variable can be read via the method
    get_inverse.

    Args:
      damping: The damping value (float or Tensor) for this factor.
    """
    if damping not in self._inverses_by_damping:
      damping_string = scalar_or_tensor_to_string(damping)
      with variable_scope.variable_scope(self._var_scope):
        inv = variable_scope.get_variable(
            "inv_damp{}".format(damping_string),
            initializer=inverse_initializer,
            shape=self._cov_shape,
            trainable=False,
            dtype=self._dtype)
      self._inverses_by_damping[damping] = inv

  def register_matpower(self, exp, damping):
    """Registers a matrix power needed by a FisherBlock.

    This creates a variable and signals make_inverse_update_ops to make the
    corresponding update op.  The variable can be read via the method
    get_matpower.

    Args:
      exp: The exponent (float or Tensor) to raise the matrix to.
      damping: The damping value (float or Tensor).
    """
    if (exp, damping) not in self._matpower_by_exp_and_damping:
      exp_string = scalar_or_tensor_to_string(exp)
      damping_string = scalar_or_tensor_to_string(damping)
      with variable_scope.variable_scope(self._var_scope):
        matpower = variable_scope.get_variable(
            "matpower_exp{}_damp{}".format(exp_string, damping_string),
            initializer=inverse_initializer,
            shape=self._cov_shape,
            trainable=False,
            dtype=self._dtype)
      self._matpower_by_exp_and_damping[(exp, damping)] = matpower

  def make_inverse_update_ops(self):
    """Create and return update ops corresponding to registered computations."""
    ops = []

    # We do this to ensure that we don't reuse the eigendecomp from old calls
    # to make_inverse_update_ops that may be placed on different devices.  This
    # can happen is the user has both a permanent and lazily constructed
    # version of the inverse ops (and only uses one of them).
    self.reset_eigendecomp()

    num_inverses = len(self._inverses_by_damping)
    matrix_power_registered = bool(self._matpower_by_exp_and_damping)
    use_eig = (
        self._eigendecomp or matrix_power_registered or
        num_inverses >= EIGENVALUE_DECOMPOSITION_THRESHOLD)

    if use_eig:
      eigenvalues, eigenvectors = self.get_eigendecomp()  # pylint: disable=unpacking-non-sequence

      for damping, inv in self._inverses_by_damping.items():
        ops.append(
            inv.assign(
                math_ops.matmul(eigenvectors / (eigenvalues + damping),
                                array_ops.transpose(eigenvectors))))

      for (exp, damping), matpower in self._matpower_by_exp_and_damping.items():
        ops.append(
            matpower.assign(
                math_ops.matmul(eigenvectors *
                                (eigenvalues + damping)**exp,
                                array_ops.transpose(eigenvectors))))
      # These ops share computation and should be run on a single device.
      ops = [control_flow_ops.group(*ops)]
    else:
      for damping, inv in self._inverses_by_damping.items():
        ops.append(inv.assign(utils.posdef_inv(self._cov, damping)))

    return ops

  def get_damped_inverse(self, damping):
    # Note that this function returns a variable which gets updated by the
    # inverse ops.  It may be stale / inconsistent with the latest value of
    # get_cov().
    return self._inverses_by_damping[damping]

  def get_matpower(self, exp, damping):
    # Note that this function returns a variable which gets updated by the
    # inverse ops.  It may be stale / inconsistent with the latest value of
    # get_cov().
    return self._matpower_by_exp_and_damping[(exp, damping)]

  def get_eigendecomp(self):
    """Creates or retrieves eigendecomposition of self._cov."""
    # Unlike get_inverse and get_matpower this doesn't retrieve a stored
    # variable, but instead always computes a fresh version from the current
    # value of get_cov().
    if not self._eigendecomp:
      eigenvalues, eigenvectors = linalg_ops.self_adjoint_eig(self._cov)

      # The matrix self._cov is positive semidefinite by construction, but the
      # numerical eigenvalues could be negative due to numerical errors, so here
      # we clip them to be at least FLAGS.eigenvalue_clipping_threshold
      clipped_eigenvalues = math_ops.maximum(eigenvalues,
                                             EIGENVALUE_CLIPPING_THRESHOLD)
      self._eigendecomp = (clipped_eigenvalues, eigenvectors)

    return self._eigendecomp

  def reset_eigendecomp(self):
    self._eigendecomp = None

  def get_cov(self):
    # Variable contains full covariance matrix.
    return self.get_cov_var()

  def left_multiply(self, x, damping):
    n = self.get_cov().shape[0]
    damped_cov = self.get_cov() + damping * array_ops.eye(n)

    if isinstance(x, tf_ops.IndexedSlices):
      raise NotImplementedError(
          "Left-multiply not yet supported for IndexedSlices.")

    if len(x.shape) != 2:
      raise ValueError(
          "InverseProvidingFactors apply to matrix-shaped vectors. Found: %s."
          % (x,))

    return math_ops.matmul(damped_cov, x)

  def right_multiply(self, x, damping):
    n = self.get_cov().shape[0]
    damped_cov = self.get_cov() + damping * array_ops.eye(n)

    if isinstance(x, tf_ops.IndexedSlices):
      return utils.matmul_sparse_dense(x, damped_cov)

    if len(x.shape) != 2:
      raise ValueError(
          "InverseProvidingFactors apply to matrix-shaped vectors. Found: %s."
          % (x,))

    return math_ops.matmul(x, damped_cov)

  def left_multiply_inverse(self, x, damping):
    if isinstance(x, tf_ops.IndexedSlices):
      raise ValueError("Left-multiply not yet supported for IndexedSlices.")

    if x.shape.ndims != 2:
      raise ValueError(
          "InverseProvidingFactors apply to matrix-shaped vectors. Found: %s."
          % (x,))

    return math_ops.matmul(self.get_damped_inverse(damping), x)

  def right_multiply_inverse(self, x, damping):
    if isinstance(x, tf_ops.IndexedSlices):
      return utils.matmul_sparse_dense(x, self.get_damped_inverse(damping))

    if x.shape.ndims != 2:
      raise ValueError(
          "InverseProvidingFactors apply to matrix-shaped vectors. Found: %s."
          % (x,))

    return math_ops.matmul(x, self.get_damped_inverse(damping))


class FullFactor(InverseProvidingFactor):
  """FisherFactor for a full matrix representation of the Fisher of a parameter.

  Note that this uses the naive "square the sum estimator", and so is applicable
  to any type of parameter in principle, but has very high variance.
  """

  def __init__(self,
               params_grads,
               batch_size):
    self._batch_size = batch_size
    self._params_grads = tuple(utils.ensure_sequence(params_grad)
                               for params_grad in params_grads)
    super(FullFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_full/" + scope_string_from_params(
        [self._params_grads, self._batch_size])

  @property
  def _cov_shape(self):
    size = sum(param_grad.shape.num_elements()
               for param_grad in self._params_grads[0])
    return (size, size)

  @property
  def _num_sources(self):
    return len(self._params_grads)

  @property
  def _dtype(self):
    return self._params_grads[0][0].dtype

  def _compute_new_cov(self, idx=0):
    # This will be a very basic rank 1 estimate
    with maybe_colocate_with(self._params_grads[idx]):
      params_grads_flat = utils.tensors_to_column(self._params_grads[idx])
      return ((params_grads_flat * array_ops.transpose(
          params_grads_flat)) / math_ops.cast(self._batch_size,
                                              params_grads_flat.dtype))


class DiagonalFactor(FisherFactor):
  """A base class for FisherFactors that use diagonal approximations.

  A DiagonalFactor's covariance variable can be of any shape, but must contain
  exactly one entry per parameter.
  """

  def __init__(self):
    super(DiagonalFactor, self).__init__()

  @property
  def _cov_initializer(self):
    return diagonal_covariance_initializer

  def make_inverse_update_ops(self):
    return []

  def get_cov(self):
    # self.get_cov() could be any shape, but it must have one entry per
    # parameter. Flatten it into a vector.
    cov_diag_vec = array_ops.reshape(self.get_cov_var(), [-1])
    return array_ops.diag(cov_diag_vec)

  def left_multiply(self, x, damping):
    damped_cov = self.get_cov_var() + damping
    if isinstance(x, tf_ops.IndexedSlices):
      return utils.matmul_diag_sparse(array_ops.reshape(damped_cov, [-1]), x)

    if x.shape != damped_cov.shape:
      raise ValueError("x (%s) and cov (%s) must have same shape." %
                       (x, damped_cov))

    return damped_cov * x

  def right_multiply(self, x, damping):
    raise NotImplementedError("Only left-multiply is currently supported.")

  def left_multiply_inverse(self, x, damping):
    inverse = 1. / (self.get_cov_var() + damping)

    if isinstance(x, tf_ops.IndexedSlices):
      return utils.matmul_diag_sparse(array_ops.reshape(inverse, [-1]), x)

    if x.shape != inverse.shape:
      raise ValueError("x (%s) and cov (%s) must have same shape." %
                       (x, inverse))

    return inverse * x

  def right_multiply_inverse(self, x, damping):
    raise NotImplementedError("Only left-multiply is currently supported.")

  def register_damped_inverse(self, damping):
    # DiagonalFactors don't keep explicit inverses.
    pass


class NaiveDiagonalFactor(DiagonalFactor):
  """FisherFactor for a diagonal approximation of any type of param's Fisher.

  Note that this uses the naive "square the sum estimator", and so is applicable
  to any type of parameter in principle, but has very high variance.
  """

  def __init__(self,
               params_grads,
               batch_size):
    """Initializes NaiveDiagonalFactor instance.

    Args:
      params_grads: Sequence of Tensors, each with same shape as parameters this
        FisherFactor corresponds to. For example, the gradient of the loss with
        respect to parameters.
      batch_size: int or 0-D Tensor. Size
    """
    self._params_grads = tuple(utils.ensure_sequence(params_grad)
                               for params_grad in params_grads)
    self._batch_size = batch_size
    super(NaiveDiagonalFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_naivediag/" + scope_string_from_params(
        [self._params_grads, self._batch_size])

  @property
  def _cov_shape(self):
    size = sum(param_grad.shape.num_elements()
               for param_grad in self._params_grads[0])
    return [size, 1]

  @property
  def _num_sources(self):
    return len(self._params_grads)

  @property
  def _dtype(self):
    return self._params_grads[0][0].dtype

  def _compute_new_cov(self, idx=0):
    with maybe_colocate_with(self._params_grads[idx]):
      params_grads_flat = utils.tensors_to_column(self._params_grads[idx])
      return (math_ops.square(params_grads_flat) / math_ops.cast(
          self._batch_size, params_grads_flat.dtype))


class EmbeddingInputKroneckerFactor(DiagonalFactor):
  r"""FisherFactor for input to an embedding layer.

  Given input_ids = [batch_size, input_size] representing indices into an
  [vocab_size, embedding_size] embedding matrix, approximate input covariance by
  a diagonal matrix,

    Cov(input_ids, input_ids) =
        (1/batch_size) sum_{i} diag(n_hot(input[i]) ** 2).

  where n_hot() constructs an n-hot binary vector and diag() constructs a
  diagonal matrix of size [vocab_size, vocab_size].
  """

  def __init__(self, input_ids, vocab_size, dtype=None):
    """Instantiate EmbeddingInputKroneckerFactor.

    Args:
      input_ids: Tuple of Tensors of shape [batch_size, input_size] and dtype
        int32.  Indices into embedding matrix.
      vocab_size: int or 0-D Tensor. Maximum value for entries in 'input_ids'.
      dtype: dtype for covariance statistics. Must be a floating point type.
        Defaults to float32.
    """
    self._input_ids = input_ids
    self._vocab_size = vocab_size
    self._cov_dtype = dtype or dtypes.float32

    super(EmbeddingInputKroneckerFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_diag_embedding/" + scope_string_from_params(self._input_ids)

  @property
  def _cov_shape(self):
    return [self._vocab_size]

  @property
  def _num_sources(self):
    return len(self._input_ids)

  @property
  def _dtype(self):
    return self._cov_dtype

  def _compute_new_cov(self, idx=0):
    with maybe_colocate_with(self._input_ids):
      input_ids = self._input_ids[idx]
      if len(input_ids.shape) > 2:
        raise ValueError(
            "Input to embeddings must have rank <= 2. Found rank %d." % len(
                input_ids.shape))

      batch_size = array_ops.shape(input_ids)[0]

      # Transform indices into one-hot vectors.
      #
      # TODO(b/72714822): There must be a faster way to construct the diagonal
      # covariance matrix! This operation is O(batch_size * vocab_size), where
      # it should be O(batch_size * input_size).
      flat_input_ids = array_ops.reshape(input_ids, [-1])
      one_hots = array_ops.one_hot(flat_input_ids,
                                   self._vocab_size)  # [?, vocab_size]

      # Take average across examples. Note that, because all entries have
      # magnitude zero or one, there's no need to square the entries.
      #
      # TODO(b/72714822): Support for SparseTensor, other kinds of aggregation
      # within an example such as average.
      #
      # TODO(b/72714822): Support for partitioned embeddings.
      new_cov = math_ops.reduce_sum(one_hots, axis=0)  # [vocab_size]
      new_cov /= math_ops.cast(batch_size, new_cov.dtype)

      return new_cov


class FullyConnectedDiagonalFactor(DiagonalFactor):
  r"""FisherFactor for a diagonal approx of a fully-connected layer's Fisher.

  Given in = [batch_size, input_size] and out_grad = [batch_size, output_size],
  approximates the covariance as,

    Cov(in, out) = (1/batch_size) sum_{i} outer(in[i], out_grad[i]) ** 2.0

  where the square is taken element-wise.
  """

  def __init__(self,
               inputs,
               outputs_grads,
               has_bias=False):
    """Instantiate FullyConnectedDiagonalFactor.

    Args:
      inputs: Tensor of shape [batch_size, input_size]. Inputs to fully
        connected layer.
      outputs_grads: List of Tensors of shape [batch_size, output_size].
        Gradient of loss with respect to layer's preactivations.
      has_bias: bool. If True, append '1' to each input.
    """
    self._inputs = inputs
    self._has_bias = has_bias
    self._outputs_grads = outputs_grads
    self._batch_size = array_ops.shape(inputs)[0]
    self._squared_inputs = None

    super(FullyConnectedDiagonalFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_diagfc/" + scope_string_from_params(
        (self._inputs,) + tuple(self._outputs_grads))

  @property
  def _cov_shape(self):
    input_size = self._inputs.shape[1] + self._has_bias
    output_size = self._outputs_grads[0].shape[1]
    return [input_size, output_size]

  @property
  def _num_sources(self):
    return len(self._outputs_grads)

  @property
  def _dtype(self):
    return self._outputs_grads[0].dtype

  def _compute_new_cov(self, idx=0):
    # The well-known special formula that uses the fact that the entry-wise
    # square of an outer product is the outer-product of the entry-wise squares.
    # The gradient is the outer product of the input and the output gradients,
    # so we just square both and then take their outer-product.
    with maybe_colocate_with(self._outputs_grads[idx]):
      # We only need to compute squared_inputs once
      if self._squared_inputs is None:
        inputs = self._inputs
        if self._has_bias:
          inputs = append_homog(self._inputs)
        self._squared_inputs = math_ops.square(inputs)

      new_cov = math_ops.matmul(
          self._squared_inputs,
          math_ops.square(self._outputs_grads[idx]),
          transpose_a=True)
      new_cov /= math_ops.cast(self._batch_size, new_cov.dtype)
      return new_cov


class ConvDiagonalFactor(DiagonalFactor):
  """FisherFactor for a diagonal approx of a convolutional layer's Fisher."""

  def __init__(self,
               inputs,
               outputs_grads,
               filter_shape,
               strides,
               padding,
               has_bias=False):
    """Creates a ConvDiagonalFactor object.

    Args:
      inputs: Tensor of shape [batch_size, height, width, in_channels].
        Input activations to this layer.
      outputs_grads: Tensor of shape [batch_size, height, width, out_channels].
        Per-example gradients to the loss with respect to the layer's output
        preactivations.
      filter_shape: Tuple of 4 ints: (kernel_height, kernel_width, in_channels,
        out_channels). Represents shape of kernel used in this layer.
      strides: The stride size in this layer (1-D Tensor of length 4).
      padding: The padding in this layer (1-D of Tensor length 4).
      has_bias: Python bool. If True, the layer is assumed to have a bias
        parameter in addition to its filter parameter.
    """
    self._inputs = inputs
    self._filter_shape = filter_shape
    self._strides = strides
    self._padding = padding
    self._has_bias = has_bias
    self._outputs_grads = outputs_grads
    self._patches = None

    super(ConvDiagonalFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_convdiag/" + scope_string_from_name(
        (self._inputs,) + tuple(self._outputs_grads))

  @property
  def _cov_shape(self):
    filter_height, filter_width, in_channels, out_channels = self._filter_shape
    return [
        filter_height * filter_width * in_channels + self._has_bias,
        out_channels
    ]

  @property
  def _num_sources(self):
    return len(self._outputs_grads)

  @property
  def _dtype(self):
    return self._outputs_grads[0].dtype

  def make_covariance_update_op(self, ema_decay):
    with maybe_colocate_with(self._inputs):
      filter_height, filter_width, _, _ = self._filter_shape

      # TODO(b/64144716): there is potential here for a big savings in terms
      # of memory use.
      patches = array_ops.extract_image_patches(
          self._inputs,
          ksizes=[1, filter_height, filter_width, 1],
          strides=self._strides,
          rates=[1, 1, 1, 1],
          padding=self._padding)

      if self._has_bias:
        patches = append_homog(patches)

      self._patches = patches

    op = super(ConvDiagonalFactor, self).make_covariance_update_op(ema_decay)

    self._patches = None

    return op

  def _compute_new_cov(self, idx=0):
    with maybe_colocate_with(self._outputs_grads[idx]):
      outputs_grad = self._outputs_grads[idx]
      batch_size = array_ops.shape(self._patches)[0]

      new_cov = self._convdiag_sum_of_squares(self._patches, outputs_grad)
      new_cov /= math_ops.cast(batch_size, new_cov.dtype)

      return new_cov

  def _convdiag_sum_of_squares(self, patches, outputs_grad):
    # This computes the sum of the squares of the per-training-case "gradients".
    # It does this simply by computing a giant tensor containing all of these,
    # doing an entry-wise square, and them summing along the batch dimension.
    case_wise_gradients = special_math_ops.einsum("bijk,bijl->bkl", patches,
                                                  outputs_grad)
    return math_ops.reduce_sum(math_ops.square(case_wise_gradients), axis=0)


class FullyConnectedKroneckerFactor(InverseProvidingFactor):
  """Kronecker factor for the input or output side of a fully-connected layer.
  """

  def __init__(self,
               tensors,
               has_bias=False):
    """Instantiate FullyConnectedKroneckerFactor.

    Args:
      tensors: List of Tensors of shape [batch_size, n]. Represents either a
        layer's inputs or its output's gradients.
      has_bias: bool. If True, append '1' to each row.
    """
    # The tensor argument is either a tensor of input activations or a tensor of
    # output pre-activation gradients.
    self._has_bias = has_bias
    self._tensors = tensors
    super(FullyConnectedKroneckerFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_fckron/" + scope_string_from_params(
        [self._tensors, self._has_bias])

  @property
  def _cov_shape(self):
    size = self._tensors[0].shape[1] + self._has_bias
    return [size, size]

  @property
  def _num_sources(self):
    return len(self._tensors)

  @property
  def _dtype(self):
    return self._tensors[0].dtype

  def _compute_new_cov(self, idx=0):
    with maybe_colocate_with(self._tensors[idx]):
      tensor = self._tensors[idx]
      if self._has_bias:
        tensor = append_homog(tensor)
      return compute_cov(tensor)


class ConvInputKroneckerFactor(InverseProvidingFactor):
  r"""Kronecker factor for the input side of a convolutional layer.

  Estimates E[ a a^T ] where a is the inputs to a convolutional layer given
  example x. Expectation is taken over all examples and locations.

  Equivalent to Omega in https://arxiv.org/abs/1602.01407 for details. See
  Section 3.1 Estimating the factors.
  """

  def __init__(self,
               inputs,
               filter_shape,
               strides,
               padding,
               has_bias=False):
    """Initializes ConvInputKroneckerFactor.

    Args:
      inputs: Tensor of shape [batch_size, height, width, in_channels]. Inputs
        to layer.
      filter_shape: 1-D Tensor of length 4. Contains [kernel_height,
        kernel_width, in_channels, out_channels].
      strides: 1-D Tensor of length 4. Contains [batch_stride, height_stride,
        width_stride, in_channel_stride].
      padding: str. Padding method for layer. "SAME" or "VALID".
      has_bias: bool. If True, append 1 to in_channel.
    """
    self._filter_shape = filter_shape
    self._strides = strides
    self._padding = padding
    self._has_bias = has_bias
    self._inputs = inputs
    super(ConvInputKroneckerFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_convinkron/" + scope_string_from_params([
        self._inputs, self._filter_shape, self._strides, self._padding,
        self._has_bias
    ])

  @property
  def _cov_shape(self):
    filter_height, filter_width, in_channels, _ = self._filter_shape
    size = filter_height * filter_width * in_channels + self._has_bias
    return [size, size]

  @property
  def _num_sources(self):
    return 1

  @property
  def _dtype(self):
    return self._inputs.dtype

  def _compute_new_cov(self, idx=0):
    if idx != 0:
      raise ValueError("ConvInputKroneckerFactor only supports idx = 0")

    with maybe_colocate_with(self._inputs):
      filter_height, filter_width, in_channels, _ = self._filter_shape

      # TODO(b/64144716): there is potential here for a big savings in terms of
      # memory use.
      patches = array_ops.extract_image_patches(
          self._inputs,
          ksizes=[1, filter_height, filter_width, 1],
          strides=self._strides,
          rates=[1, 1, 1, 1],
          padding=self._padding)

      flatten_size = (filter_height * filter_width * in_channels)
      # patches_flat below is the matrix [[A_l]] from the KFC paper (tilde
      # omitted over A for clarity). It has shape M|T| x J|Delta| (eq. 14),
      # where M = minibatch size, |T| = number of spatial locations,
      # |Delta| = number of spatial offsets, and J = number of input maps
      # for convolutional layer l.
      patches_flat = array_ops.reshape(patches, [-1, flatten_size])
      # We append a homogenous coordinate to patches_flat if the layer has
      # bias parameters. This gives us [[A_l]]_H from the paper.
      if self._has_bias:
        patches_flat = append_homog(patches_flat)
      # We call compute_cov without passing in a normalizer. compute_cov uses
      # the first dimension of patches_flat i.e. M|T| as the normalizer by
      # default. Hence we end up computing 1/M|T| * [[A_l]]^T [[A_l]], with
      # shape J|Delta| x J|Delta|. This is related to hat{Omega}_l from
      # the paper but has a different scale here for consistency with
      # ConvOutputKroneckerFactor.
      # (Tilde omitted over A for clarity.)
      return compute_cov(patches_flat)


class ConvOutputKroneckerFactor(InverseProvidingFactor):
  r"""Kronecker factor for the output side of a convolutional layer.

  Estimates E[ ds ds^T ] where s is the preactivations of a convolutional layer
  given example x and ds = (d / d s) log(p(y|x, w)). Expectation is taken over
  all examples and locations.

  Equivalent to Gamma in https://arxiv.org/abs/1602.01407 for details. See
  Section 3.1 Estimating the factors.
  """

  def __init__(self, outputs_grads):
    """Initializes ConvOutputKroneckerFactor.

    Args:
      outputs_grads: list of Tensors. Each Tensor is of shape
          [batch_size, height, width, out_channels].
    """
    self._out_channels = outputs_grads[0].shape.as_list()[3]
    self._outputs_grads = outputs_grads
    super(ConvOutputKroneckerFactor, self).__init__()

  @property
  def _var_scope(self):
    return "ff_convoutkron/" + scope_string_from_params(self._outputs_grads)

  @property
  def _cov_shape(self):
    size = self._out_channels
    return [size, size]

  @property
  def _num_sources(self):
    return len(self._outputs_grads)

  @property
  def _dtype(self):
    return self._outputs_grads[0].dtype

  def _compute_new_cov(self, idx=0):
    with maybe_colocate_with(self._outputs_grads[idx]):
      # reshaped_tensor below is the matrix DS_l defined in the KFC paper
      # (tilde omitted over S for clarity). It has shape M|T| x I, where
      # M = minibatch size, |T| = number of spatial locations, and
      # I = number of output maps for convolutional layer l.
      reshaped_tensor = array_ops.reshape(self._outputs_grads[idx],
                                          [-1, self._out_channels])
      # Following the reasoning in ConvInputKroneckerFactor._compute_new_cov,
      # compute_cov here returns 1/M|T| * DS_l^T DS_l = hat{Gamma}_l
      # as defined in the paper, with shape I x I.
      # (Tilde omitted over S for clarity.)
      return compute_cov(reshaped_tensor)


class FullyConnectedMultiKF(InverseProvidingFactor):
  """Kronecker factor for a fully connected recurrent layer."""

  def __init__(self,
               tensor_lists,
               has_bias=False):
    """Constructs a new `FullyConnectedMultiKF`.

    Args:
      tensor_lists: List of lists of Tensors of shape [batch_size, n].
      has_bias: bool. If True, '1' is appended to each row.
    """

    self._tensor_lists = tensor_lists
    self._has_bias = has_bias
    self._batch_size = array_ops.shape(tensor_lists[0][0])[0]
    self._num_timesteps = len(tensor_lists[0])
    self._tensors = [None] * len(tensor_lists)

    self._cov_dt1 = None
    self._option1quants_by_damping = {}
    self._option2quants_by_damping = {}

    super(FullyConnectedMultiKF, self).__init__()

  @property
  def _var_scope(self):
    return "ff_fc_multi/" + scope_string_from_params(self._tensor_lists)

  @property
  def _num_sources(self):
    return len(self._tensor_lists)

  @property
  def _dtype(self):
    return self._tensor_lists[0][0].dtype

  def make_covariance_update_op(self, ema_decay):

    op = super(FullyConnectedMultiKF, self).make_covariance_update_op(ema_decay)

    if self._cov_dt1 is not None:
      new_cov_dt1_contribs = tuple(self._compute_new_cov_dt1(idx)
                                   for idx in range(self._num_sources))

      with maybe_colocate_with(new_cov_dt1_contribs[0]):
        new_cov_dt1 = math_ops.add_n(new_cov_dt1_contribs)

        op2 = moving_averages.assign_moving_average(
            self._cov_dt1, new_cov_dt1, ema_decay, zero_debias=ZERO_DEBIAS)

        # TODO(b/69112164):
        # It's important that _cov and _cov_dt1 remain consistent with each
        # other while the inverse ops are happening. How can we ensure this?
        # We will need to add explicit synchronization for this to
        # work with asynchronous training.
        op = control_flow_ops.group(op, op2)

    return op

  def _compute_new_cov(self, idx=0):
    with maybe_colocate_with(self._tensor_lists[idx]):
      tensor = array_ops.concat(self._tensor_lists[idx], 0)
      if self._has_bias:
        tensor = append_homog(tensor)
      # We save these so they can be used by _compute_new_cov_dt1
      self._tensors[idx] = tensor
      return compute_cov(tensor)

  def _compute_new_cov_dt1(self, idx=0):
    tensor = self._tensors[idx]
    with maybe_colocate_with(tensor):
      # Is there a more elegant way to do this computation?
      tensor_present = tensor[:-self._batch_size, :]
      tensor_future = tensor[self._batch_size:, :]
      # We specify a normalizer for this computation to ensure a PSD Fisher
      # block estimate.  This is equivalent to padding with zeros, as was done
      # in Section B.2 of the appendix.
      normalizer = self._num_timesteps * self._batch_size
      return compute_cov(
          tensor_future, tensor_right=tensor_present, normalizer=normalizer)

  @property
  def _cov_shape(self):
    size = self._tensor_lists[0][0].shape[1] + self._has_bias
    return [size, size]

  @property
  def _vec_shape(self):
    size = self._tensor_lists[0][0].shape[1] + self._has_bias
    return [size]

  def get_option1quants(self, damping):
    return self._option1quants_by_damping[damping]

  def get_option2quants(self, damping):
    return self._option2quants_by_damping[damping]

  def get_cov_dt1(self):
    assert self._cov_dt1 is not None
    return self._cov_dt1

  def register_cov_dt1(self):
    """Create a variable representing temporal cross-covariance.

    (This is technically the second moment, not covariance, since it's
    not mean subtracted.)
    """
    if self._cov_dt1 is None:
      with variable_scope.variable_scope(self._var_scope):
        self._cov_dt1 = variable_scope.get_variable(
            "cov_dt1",
            initializer=init_ops.zeros_initializer,
            shape=self._cov_shape,
            trainable=False,
            dtype=self._dtype)

  def register_option1quants(self, damping):

    self.register_cov_dt1()

    if damping not in self._option1quants_by_damping:
      # It's questionable as to whether we should initialize with stuff like
      # this at all.  Ideally these values should never be used until they are
      # updated at least once.
      damping_string = scalar_or_tensor_to_string(damping)
      with variable_scope.variable_scope(self._var_scope):
        Lmat = variable_scope.get_variable(  # pylint: disable=invalid-name
            "Lmat_damp{}".format(damping_string),
            initializer=inverse_initializer,
            shape=self._cov_shape,
            trainable=False,
            dtype=self._dtype)
        psi = variable_scope.get_variable(
            "psi_damp{}".format(damping_string),
            initializer=init_ops.ones_initializer,
            shape=self._vec_shape,
            trainable=False,
            dtype=self._dtype)

      self._option1quants_by_damping[damping] = (Lmat, psi)

  def register_option2quants(self, damping):

    self.register_cov_dt1()

    if damping not in self._option2quants_by_damping:
      # It's questionable as to whether we should initialize with stuff like
      # this at all.  Ideally these values should never be used until they are
      # updated at least once.
      damping_string = scalar_or_tensor_to_string(damping)
      with variable_scope.variable_scope(self._var_scope):
        Pmat = variable_scope.get_variable(  # pylint: disable=invalid-name
            "Lmat_damp{}".format(damping_string),
            initializer=inverse_initializer,
            shape=self._cov_shape,
            trainable=False,
            dtype=self._dtype)
        Kmat = variable_scope.get_variable(  # pylint: disable=invalid-name
            "Kmat_damp{}".format(damping_string),
            initializer=inverse_initializer,
            shape=self._cov_shape,
            trainable=False,
            dtype=self._dtype)
        mu = variable_scope.get_variable(
            "mu_damp{}".format(damping_string),
            initializer=init_ops.ones_initializer,
            shape=self._vec_shape,
            trainable=False,
            dtype=self._dtype)

      self._option2quants_by_damping[damping] = (Pmat, Kmat, mu)

  def make_inverse_update_ops(self):
    """Create and return update ops corresponding to registered computations."""
    # TODO(b/69918258): Add correctness tests for this method.
    # pylint: disable=invalid-name

    ops = super(FullyConnectedMultiKF, self).make_inverse_update_ops()

    if (len(self._option1quants_by_damping) +
        len(self._option2quants_by_damping)):

      # Note that C0 and C1 are stand-ins for A0 and A1, or G0 and G1, from
      # the pseudo-code in the original paper.  Because the computations for
      # the A and G case are essentially the same they can both be performed by
      # the same class (this one).

      C1 = self.get_cov_dt1()

      # Get the eigendecomposition of C0  (= self.get_cov())
      eigen_e, eigen_V = self.get_eigendecomp()

      # TODO(b/69678661): Note, there is an implicit assumption here that C1
      # and C0 (as represented here by its eigen-decomp) are consistent.  This
      # could fail to be the case if self._cov and self._cov_dt1 are not updated
      # consistently, or are somehow read between or during the cov updates.
      # Can this possibly happen?  Is there a way to prevent it?

      for damping, (Lmat_var,
                    psi_var) in self._option1quants_by_damping.items():

        invsqrtC0 = math_ops.matmul(
            eigen_V * (eigen_e + damping)**(-0.5), eigen_V, transpose_b=True)

        # Might need to enforce symmetry lost due to numerical issues.
        invsqrtC0 = (invsqrtC0 + array_ops.transpose(invsqrtC0)) / 2.0

        # The following line imposses the symmetry assumed by "Option 1" on C1.
        # Stangely the code can work okay with this line commented out,
        # depending on how psd_eig is defined.  I'm not sure why.
        C1 = (C1 + array_ops.transpose(C1)) / 2.0

        # hPsi = C0^(-1/2) * C1 * C0^(-1/2)  (hPsi means hat{Psi})
        hPsi = math_ops.matmul(math_ops.matmul(invsqrtC0, C1), invsqrtC0)

        # Compute the decomposition U*diag(psi)*U^T = hPsi
        psi, U = utils.posdef_eig(hPsi)

        # L = C0^(-1/2) * U
        Lmat = math_ops.matmul(invsqrtC0, U)

        ops.append(Lmat_var.assign(Lmat))
        ops.append(psi_var.assign(psi))

      for damping, (Pmat_var, Kmat_var,
                    mu_var) in self._option2quants_by_damping.items():

        # compute C0^(-1/2)
        invsqrtC0 = math_ops.matmul(
            eigen_V * (eigen_e + damping)**(-0.5), eigen_V, transpose_b=True)

        # Might need to enforce symmetry lost due to numerical issues.
        invsqrtC0 = (invsqrtC0 + array_ops.transpose(invsqrtC0)) / 2.0

        # Compute the product C0^(-1/2) * C1
        invsqrtC0C1 = math_ops.matmul(invsqrtC0, C1)

        # hPsi = C0^(-1/2) * C1 * C0^(-1/2)  (hPsi means hat{Psi})
        hPsi = math_ops.matmul(invsqrtC0C1, invsqrtC0)

        # Compute the decomposition E*diag(mu)*E^T = hPsi^T * hPsi
        # Note that we using the notation mu instead of "m" for the eigenvalues.
        # Instead of computing the product hPsi^T * hPsi and then doing an
        # eigen-decomposition of this we just compute the SVD of hPsi and then
        # square the singular values to get the eigenvalues. For a justification
        # of this approach, see:
        # https://en.wikipedia.org/wiki/Singular-value_decomposition#Relation_to_eigenvalue_decomposition
        sqrtmu, _, E = linalg_ops.svd(hPsi)
        mu = math_ops.square(sqrtmu)

        # Mathematically, the eigenvalues should not should not exceed 1.0, but
        # due to numerical issues, or possible issues with inconsistent
        # values of C1 and (the eigen-decomposition of) C0 they might. So
        # we enforce this condition.
        mu = math_ops.minimum(mu, 1.0)

        # P = (C0^(-1/2) * C1)^T * C0^(-1/2) = C_1^T * C_0^(-1)
        Pmat = math_ops.matmul(invsqrtC0C1, invsqrtC0, transpose_a=True)

        # K = C_0^(-1/2) * E
        Kmat = math_ops.matmul(invsqrtC0, E)

        ops.append(Pmat_var.assign(Pmat))
        ops.append(Kmat_var.assign(Kmat))
        ops.append(mu_var.assign(mu))

    return [control_flow_ops.group(*ops)]

    # pylint: enable=invalid-name
