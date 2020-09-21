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
"""The KFAC optimizer."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

# pylint disable=long-line
from tensorflow.contrib.kfac.python.ops import curvature_matrix_vector_products as cmvp
from tensorflow.contrib.kfac.python.ops import estimator as est
# pylint enable=long-line

from tensorflow.python.framework import ops
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import linalg_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import variables as tf_variables
from tensorflow.python.training import gradient_descent


class KfacOptimizer(gradient_descent.GradientDescentOptimizer):
  """The KFAC Optimizer (https://arxiv.org/abs/1503.05671)."""

  def __init__(self,
               learning_rate,
               cov_ema_decay,
               damping,
               layer_collection,
               var_list=None,
               momentum=0.9,
               momentum_type="regular",
               norm_constraint=None,
               name="KFAC",
               estimation_mode="gradients",
               colocate_gradients_with_ops=True,
               cov_devices=None,
               inv_devices=None):
    """Initializes the KFAC optimizer with the given settings.

    Args:
      learning_rate: The base learning rate for the optimizer.  Should probably
          be set to 1.0 when using momentum_type = 'qmodel', but can still be
          set lowered if desired (effectively lowering the trust in the
          quadratic model.)
      cov_ema_decay: The decay factor used when calculating the covariance
          estimate moving averages.
      damping: The damping factor used to stabilize training due to errors in
          the local approximation with the Fisher information matrix, and to
          regularize the update direction by making it closer to the gradient.
          (Higher damping means the update looks more like a standard gradient
          update - see Tikhonov regularization.)
      layer_collection: The layer collection object, which holds the fisher
          blocks, kronecker factors, and losses associated with the
          graph.  The layer_collection cannot be modified after KfacOptimizer's
          initialization.
      var_list: Optional list or tuple of variables to train. Defaults to the
          list of variables collected in the graph under the key
          `GraphKeys.TRAINABLE_VARIABLES`.
      momentum: The momentum decay constant to use. Only applies when
          momentum_type is 'regular' or 'adam'. (Default: 0.9)
      momentum_type: The type of momentum to use in this optimizer, one of
          'regular', 'adam', or 'qmodel'. (Default: 'regular')
      norm_constraint: float or Tensor. If specified, the update is scaled down
          so that its approximate squared Fisher norm v^T F v is at most the
          specified value. May only be used with momentum type 'regular'.
          (Default: None)
      name: The name for this optimizer. (Default: 'KFAC')
      estimation_mode: The type of estimator to use for the Fishers.  Can be
          'gradients', 'empirical', 'curvature_propagation', or 'exact'.
          (Default: 'gradients'). See the doc-string for FisherEstimator for
          more a more detailed description of these options.
      colocate_gradients_with_ops: Whether we should request gradients we
          compute in the estimator be colocated with their respective ops.
          (Default: True)
      cov_devices: Iterable of device strings (e.g. '/gpu:0'). Covariance
          computations will be placed on these devices in a round-robin fashion.
          Can be None, which means that no devices are specified.
      inv_devices: Iterable of device strings (e.g. '/gpu:0'). Inversion
          computations will be placed on these devices in a round-robin fashion.
          Can be None, which means that no devices are specified.

    Raises:
      ValueError: If the momentum type is unsupported.
      ValueError: If clipping is used with momentum type other than 'regular'.
      ValueError: If no losses have been registered with layer_collection.
      ValueError: If momentum is non-zero and momentum_type is not 'regular'
          or 'adam'.
    """

    variables = var_list
    if variables is None:
      variables = tf_variables.trainable_variables()

    self._fisher_est = est.FisherEstimator(
        variables,
        cov_ema_decay,
        damping,
        layer_collection,
        estimation_mode=estimation_mode,
        colocate_gradients_with_ops=colocate_gradients_with_ops,
        cov_devices=cov_devices,
        inv_devices=inv_devices)

    momentum_type = momentum_type.lower()
    legal_momentum_types = ["regular", "adam", "qmodel"]

    if momentum_type not in legal_momentum_types:
      raise ValueError("Unsupported momentum type {}. Must be one of {}."
                       .format(momentum_type, legal_momentum_types))
    if momentum_type != "regular" and norm_constraint is not None:
      raise ValueError("Update clipping is only supported with momentum"
                       "type 'regular'.")
    if momentum_type not in ["regular", "adam"] and momentum != 0:
      raise ValueError("Momentum must be unspecified if using a momentum_type "
                       "other than 'regular' or 'adam'.")

    self._momentum = momentum
    self._momentum_type = momentum_type
    self._norm_constraint = norm_constraint

    # this is a bit of a hack
    # TODO(duckworthd): Handle this in a better way (e.g. pass it in?)
    self._batch_size = array_ops.shape(layer_collection.losses[0].inputs)[0]
    self._losses = layer_collection.losses

    super(KfacOptimizer, self).__init__(learning_rate, name=name)

  @property
  def cov_update_thunks(self):
    return self._fisher_est.cov_update_thunks

  @property
  def cov_update_ops(self):
    return self._fisher_est.cov_update_ops

  @property
  def cov_update_op(self):
    return self._fisher_est.cov_update_op

  @property
  def inv_update_thunks(self):
    return self._fisher_est.inv_update_thunks

  @property
  def inv_update_ops(self):
    return self._fisher_est.inv_update_ops

  @property
  def inv_update_op(self):
    return self._fisher_est.inv_update_op

  @property
  def variables(self):
    return self._fisher_est.variables

  @property
  def damping(self):
    return self._fisher_est.damping

  def minimize(self, *args, **kwargs):
    kwargs["var_list"] = kwargs.get("var_list") or self.variables
    if set(kwargs["var_list"]) != set(self.variables):
      raise ValueError("var_list doesn't match with set of Fisher-estimating "
                       "variables.")
    return super(KfacOptimizer, self).minimize(*args, **kwargs)

  def compute_gradients(self, *args, **kwargs):
    # args[1] could be our var_list
    if len(args) > 1:
      var_list = args[1]
    else:
      kwargs["var_list"] = kwargs.get("var_list") or self.variables
      var_list = kwargs["var_list"]
    if set(var_list) != set(self.variables):
      raise ValueError("var_list doesn't match with set of Fisher-estimating "
                       "variables.")
    return super(KfacOptimizer, self).compute_gradients(*args, **kwargs)

  def apply_gradients(self, grads_and_vars, *args, **kwargs):
    """Applies gradients to variables.

    Args:
      grads_and_vars: List of (gradient, variable) pairs.
      *args: Additional arguments for super.apply_gradients.
      **kwargs: Additional keyword arguments for super.apply_gradients.

    Returns:
      An `Operation` that applies the specified gradients.
    """
    # In Python 3, grads_and_vars can be a zip() object which can only be
    # iterated over once. By converting it to a list, we ensure that it can be
    # iterated over more than once.
    grads_and_vars = list(grads_and_vars)

    # Compute step.
    steps_and_vars = self._compute_update_steps(grads_and_vars)

    # Update trainable variables with this step.
    return super(KfacOptimizer, self).apply_gradients(steps_and_vars, *args,
                                                      **kwargs)

  def _squared_fisher_norm(self, grads_and_vars, precon_grads_and_vars):
    """Computes the squared (approximate) Fisher norm of the updates.

    This is defined as v^T F v, where F is the approximate Fisher matrix
    as computed by the estimator, and v = F^{-1} g, where g is the gradient.
    This is computed efficiently as v^T g.

    Args:
      grads_and_vars: List of (gradient, variable) pairs.
      precon_grads_and_vars: List of (preconditioned gradient, variable) pairs.
        Must be the result of calling `self._fisher_est.multiply_inverse`
        on `grads_and_vars`.

    Returns:
      Scalar representing the squared norm.

    Raises:
      ValueError: if the two list arguments do not contain the same variables,
        in the same order.
    """
    for (_, gvar), (_, pgvar) in zip(grads_and_vars, precon_grads_and_vars):
      if gvar is not pgvar:
        raise ValueError("The variables referenced by the two arguments "
                         "must match.")
    terms = [
        math_ops.reduce_sum(grad * pgrad)
        for (grad, _), (pgrad, _) in zip(grads_and_vars, precon_grads_and_vars)
    ]
    return math_ops.reduce_sum(terms)

  def _update_clip_coeff(self, grads_and_vars, precon_grads_and_vars):
    """Computes the scale factor for the update to satisfy the norm constraint.

    Defined as min(1, sqrt(c / r^T F r)), where c is the norm constraint,
    F is the approximate Fisher matrix, and r is the update vector, i.e.
    -alpha * v, where alpha is the learning rate, and v is the preconditioned
    gradient.

    This is based on Section 5 of Ba et al., Distributed Second-Order
    Optimization using Kronecker-Factored Approximations. Note that they
    absorb the learning rate alpha (which they denote eta_max) into the formula
    for the coefficient, while in our implementation, the rescaling is done
    before multiplying by alpha. Hence, our formula differs from theirs by a
    factor of alpha.

    Args:
      grads_and_vars: List of (gradient, variable) pairs.
      precon_grads_and_vars: List of (preconditioned gradient, variable) pairs.
        Must be the result of calling `self._fisher_est.multiply_inverse`
        on `grads_and_vars`.

    Returns:
      Scalar representing the coefficient which should be applied to the
      preconditioned gradients to satisfy the norm constraint.
    """
    sq_norm_grad = self._squared_fisher_norm(grads_and_vars,
                                             precon_grads_and_vars)
    sq_norm_up = sq_norm_grad * self._learning_rate**2
    return math_ops.minimum(1.,
                            math_ops.sqrt(self._norm_constraint / sq_norm_up))

  def _clip_updates(self, grads_and_vars, precon_grads_and_vars):
    """Rescales the preconditioned gradients to satisfy the norm constraint.

    Rescales the preconditioned gradients such that the resulting update r
    (after multiplying by the learning rate) will satisfy the norm constraint.
    This constraint is that r^T F r <= C, where F is the approximate Fisher
    matrix, and C is the norm_constraint attribute. See Section 5 of
    Ba et al., Distributed Second-Order Optimization using Kronecker-Factored
    Approximations.

    Args:
      grads_and_vars: List of (gradient, variable) pairs.
      precon_grads_and_vars: List of (preconditioned gradient, variable) pairs.
        Must be the result of calling `self._fisher_est.multiply_inverse`
        on `grads_and_vars`.

    Returns:
      List of (rescaled preconditioned gradient, variable) pairs.
    """
    coeff = self._update_clip_coeff(grads_and_vars, precon_grads_and_vars)
    return [(pgrad * coeff, var) for pgrad, var in precon_grads_and_vars]

  def _compute_qmodel_hyperparams(self, precon_grads, prev_updates, grads,
                                  variables):
    """Compute optimal update hyperparameters from the quadratic model.

    More specifically, if L is the loss we minimize a quadratic approximation
    of L(theta + d) which we denote by qmodel(d) with
    d = alpha*precon_grad + mu*prev_update with respect to alpha and mu, where

      qmodel(d) = (1/2) * d^T * B * d + grad^T*d + L(theta) .

    Unlike in the KL clipping approach we use the non-approximated quadratic
    model where the curvature matrix C is the true Fisher on the current
    mini-batch (computed without any approximations beyond mini-batch sampling),
    with the usual Tikhonov damping/regularization applied,

      C = F + damping * I

    See Section 7 of https://arxiv.org/abs/1503.05671 for a derivation of
    the formula.  See Appendix C for a discussion of the trick of using
    a factorized Fisher matrix to more efficiently compute the required
    vector-matrix-vector products.

    Note that the elements of all 4 lists passed to this function must
    be in correspondence with each other.

    Args:
      precon_grads: List of preconditioned gradients.
      prev_updates: List of updates computed at the previous iteration.
      grads: List of gradients.
      variables: List of variables in the graph that the update will be
          applied to. (Note that this function doesn't actually apply the
          update.)

    Returns:
      (alpha, mu, qmodel_change), where alpha and mu are chosen to optimize the
      quadratic model, and
      qmodel_change = qmodel(alpha*precon_grad + mu*prev_update) - qmodel(0)
                    = qmodel(alpha*precon_grad + mu*prev_update) - L(theta).
    """

    cmvpc = cmvp.CurvatureMatrixVectorProductComputer(self._losses, variables)

    # compute the matrix-vector products with the transposed Fisher factor
    fft_precon_grads = cmvpc.multiply_fisher_factor_transpose(precon_grads)
    fft_prev_updates = cmvpc.multiply_fisher_factor_transpose(prev_updates)

    batch_size = math_ops.cast(
        self._batch_size, dtype=fft_precon_grads[0].dtype)

    # compute the entries of the 2x2 matrix
    m_11 = (
        _inner_product_list(fft_precon_grads, fft_precon_grads) / batch_size +
        self.damping * _inner_product_list(precon_grads, precon_grads))

    m_21 = (
        _inner_product_list(fft_prev_updates, fft_precon_grads) / batch_size +
        self.damping * _inner_product_list(prev_updates, precon_grads))

    m_22 = (
        _inner_product_list(fft_prev_updates, fft_prev_updates) / batch_size +
        self.damping * _inner_product_list(prev_updates, prev_updates))

    def non_zero_prevupd_case():
      r"""Computes optimal (alpha, mu) given non-zero previous update.

      We solve the full 2x2 linear system. See Martens & Grosse (2015),
      Section 7, definition of $\alpha^*$ and $\mu^*$.

      Returns:
        (alpha, mu, qmodel_change), where alpha and mu are chosen to optimize
        the quadratic model, and
        qmodel_change = qmodel(alpha*precon_grad + mu*prev_update) - qmodel(0).
      """
      m = ops.convert_to_tensor([[m_11, m_21], [m_21, m_22]])

      c = ops.convert_to_tensor([[_inner_product_list(grads, precon_grads)],
                                 [_inner_product_list(grads, prev_updates)]])

      sol = _two_by_two_solve(m, c)
      alpha = -sol[0]
      mu = -sol[1]
      qmodel_change = 0.5 * math_ops.reduce_sum(sol * c)

      return alpha, mu, qmodel_change

    def zero_prevupd_case():
      r"""Computes optimal (alpha, mu) given all-zero previous update.

      The linear system reduces to 1x1. See Martens & Grosse (2015),
      Section 6.4, definition of $\alpha^*$.

      Returns:
        (alpha, 0.0, qmodel_change), where alpha is chosen to optimize the
        quadratic model, and
        qmodel_change = qmodel(alpha*precon_grad) - qmodel(0)
      """
      m = m_11
      c = _inner_product_list(grads, precon_grads)

      alpha = -c / m
      mu = 0.0
      qmodel_change = 0.5 * alpha * c

      return alpha, mu, qmodel_change

    return control_flow_ops.cond(
        math_ops.equal(m_22, 0.0), zero_prevupd_case, non_zero_prevupd_case)

  def _compute_update_steps(self, grads_and_vars):
    """Computes the update steps for the variables given the gradients.

    Args:
      grads_and_vars: List of (gradient, variable) pairs.

    Returns:
      An 'Operation that computes the update steps for the given variables.
    """
    if self._momentum_type == "regular":
      # Compute "preconditioned" gradient.
      precon_grads_and_vars = self._fisher_est.multiply_inverse(grads_and_vars)

      # Apply "KL clipping" if asked for.
      if self._norm_constraint is not None:
        precon_grads_and_vars = self._clip_updates(grads_and_vars,
                                                   precon_grads_and_vars)

      # Update the velocity with this and return it as the step.
      return self._update_velocities(precon_grads_and_vars, self._momentum)

    elif self._momentum_type == "adam":
      # Update velocity.
      velocities_and_vars = self._update_velocities(grads_and_vars,
                                                    self._momentum)
      # Return "preconditioned" velocity vector as the step.
      return self._fisher_est.multiply_inverse(velocities_and_vars)

    elif self._momentum_type == "qmodel":
      # Compute "preconditioned" gradient.
      precon_grads_and_vars = self._fisher_est.multiply_inverse(grads_and_vars)

      # Extract out singleton lists from the tuple-lists
      precon_grads = list(
          precon_grad for (precon_grad, _) in precon_grads_and_vars)
      grads = list(grad for (grad, _) in grads_and_vars)
      variables = list(var for (_, var) in grads_and_vars)
      # previous updates are the negative velocities (up to scaling by LR)
      prev_updates = list(
          -self._zeros_slot(var, "velocity", self._name) for var in variables)

      # Compute optimal velocity update parameters according to quadratic model
      alpha, mu, _ = self._compute_qmodel_hyperparams(
          precon_grads, prev_updates, grads, variables)

      # Update the velocity with precon_grads according to these params
      # and return it as the step.
      return self._update_velocities(
          precon_grads_and_vars, mu, vec_coeff=-alpha)

  def _update_velocities(self, vecs_and_vars, decay, vec_coeff=1.0):
    """Updates the velocities of the variables with the given vectors.

    Args:
      vecs_and_vars: List of (vector, variable) pairs.
      decay: How much to decay the old velocity by.  This is often referred to
        as the 'momentum constant'.
      vec_coeff: Coefficient to apply to the vectors before adding them to the
        velocity.

    Returns:
      A list of (velocity, var) indicating the new velocity for each var.
    """

    def _update_velocity(vec, var):
      velocity = self._zeros_slot(var, "velocity", self._name)
      with ops.colocate_with(velocity):
        # NOTE(mattjj): read/modify/write race condition not suitable for async.

        # Compute the new velocity for this variable.
        new_velocity = decay * velocity + vec_coeff * vec

        # Save the updated velocity.
        return (array_ops.identity(velocity.assign(new_velocity)), var)

    # Go through variable and update its associated part of the velocity vector.
    return [_update_velocity(vec, var) for vec, var in vecs_and_vars]


def _inner_product_list(list1, list2):
  return math_ops.add_n(
      [math_ops.reduce_sum(elt1 * elt2) for elt1, elt2 in zip(list1, list2)])


def _two_by_two_solve(m, c):
  # it might be better just to crank out the exact formula for 2x2 inverses
  return math_ops.matmul(linalg_ops.matrix_inverse(m), c)
