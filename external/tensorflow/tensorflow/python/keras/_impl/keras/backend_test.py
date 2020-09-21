# Copyright 2016 The TensorFlow Authors. All Rights Reserved.
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
"""Tests for Keras backend."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import scipy.sparse

from tensorflow.python.framework import sparse_tensor
from tensorflow.python.keras._impl import keras
from tensorflow.python.ops import variables
from tensorflow.python.platform import test
from tensorflow.python.util import tf_inspect


def compare_single_input_op_to_numpy(keras_op,
                                     np_op,
                                     input_shape,
                                     dtype='float32',
                                     negative_values=True,
                                     keras_args=None,
                                     keras_kwargs=None,
                                     np_args=None,
                                     np_kwargs=None):
  keras_args = keras_args or []
  keras_kwargs = keras_kwargs or {}
  np_args = np_args or []
  np_kwargs = np_kwargs or {}
  inputs = 2. * np.random.random(input_shape)
  if negative_values:
    inputs -= 1.
  keras_output = keras_op(keras.backend.variable(inputs, dtype=dtype),
                          *keras_args, **keras_kwargs)
  keras_output = keras.backend.eval(keras_output)
  np_output = np_op(inputs.astype(dtype), *np_args, **np_kwargs)
  try:
    np.testing.assert_allclose(keras_output, np_output, atol=1e-4)
  except AssertionError:
    raise AssertionError('Test for op `' + str(keras_op.__name__) + '` failed; '
                         'Expected ' + str(np_output) + ' but got ' +
                         str(keras_output))


def compare_two_inputs_op_to_numpy(keras_op,
                                   np_op,
                                   input_shape_a,
                                   input_shape_b,
                                   dtype='float32',
                                   keras_args=None,
                                   keras_kwargs=None,
                                   np_args=None,
                                   np_kwargs=None):
  keras_args = keras_args or []
  keras_kwargs = keras_kwargs or {}
  np_args = np_args or []
  np_kwargs = np_kwargs or {}
  input_a = np.random.random(input_shape_a)
  input_b = np.random.random(input_shape_b)
  keras_output = keras_op(keras.backend.variable(input_a, dtype=dtype),
                          keras.backend.variable(input_b, dtype=dtype),
                          *keras_args, **keras_kwargs)
  keras_output = keras.backend.eval(keras_output)
  np_output = np_op(input_a.astype(dtype), input_b.astype(dtype),
                    *np_args, **np_kwargs)
  try:
    np.testing.assert_allclose(keras_output, np_output, atol=1e-4)
  except AssertionError:
    raise AssertionError('Test for op `' + str(keras_op.__name__) + '` failed; '
                         'Expected ' + str(np_output) + ' but got ' +
                         str(keras_output))


class BackendUtilsTest(test.TestCase):

  def test_backend(self):
    self.assertEqual(keras.backend.backend(), 'tensorflow')

  def test_espilon(self):
    epsilon = 1e-2
    keras.backend.set_epsilon(epsilon)
    self.assertEqual(keras.backend.epsilon(), epsilon)
    keras.backend.set_epsilon(1e-7)

  def test_floatx(self):
    floatx = 'float64'
    keras.backend.set_floatx(floatx)
    self.assertEqual(keras.backend.floatx(), floatx)
    keras.backend.set_floatx('float32')

  def test_image_data_format(self):
    image_data_format = 'channels_first'
    keras.backend.set_image_data_format(image_data_format)
    self.assertEqual(keras.backend.image_data_format(), image_data_format)
    keras.backend.set_image_data_format('channels_last')

  def test_get_reset_uids(self):
    self.assertEqual(keras.backend.get_uid('foo'), 1)
    self.assertEqual(keras.backend.get_uid('foo'), 2)

    keras.backend.reset_uids()
    self.assertEqual(keras.backend.get_uid('foo'), 1)

  def test_learning_phase(self):
    with self.test_session() as sess:
      keras.backend.set_learning_phase(1)
      self.assertEqual(keras.backend.learning_phase(), 1)
      with self.assertRaises(ValueError):
        keras.backend.set_learning_phase(2)

      # Test running with a learning-phase-consuming layer
      keras.backend.set_learning_phase(0)
      x = keras.Input((3,))
      y = keras.layers.BatchNormalization()(x)
      sess.run(variables.global_variables_initializer())
      sess.run(y, feed_dict={x: np.random.random((2, 3))})

  def test_int_shape(self):
    x = keras.backend.placeholder(shape=(3, 4))
    self.assertEqual(keras.backend.int_shape(x), (3, 4))

    x = keras.backend.placeholder(shape=(None, 4))
    self.assertEqual(keras.backend.int_shape(x), (None, 4))

  def test_in_train_phase(self):
    with self.test_session():
      y1 = keras.backend.variable(1)
      y2 = keras.backend.variable(2)
      y = keras.backend.in_train_phase(y1, y2)
      f = keras.backend.function([keras.backend.learning_phase()], [y])
      y_val = f([0])[0]
      self.assertAllClose(y_val, 2)
      y_val = f([1])[0]
      self.assertAllClose(y_val, 1)

  def test_is_keras_tensor(self):
    x = keras.backend.variable(1)
    self.assertEqual(keras.backend.is_keras_tensor(x), False)
    x = keras.Input(shape=(1,))
    self.assertEqual(keras.backend.is_keras_tensor(x), True)
    with self.assertRaises(ValueError):
      keras.backend.is_keras_tensor(0)

  def test_is_placeholder(self):
    x = keras.backend.placeholder(shape=(1,))
    self.assertEqual(keras.backend.is_placeholder(x), True)
    # Test with TF placeholder
    x = keras.backend.array_ops.placeholder(dtype='float32', shape=(1,))
    self.assertEqual(keras.backend.is_placeholder(x), True)
    x = keras.backend.variable(1)
    self.assertEqual(keras.backend.is_placeholder(x), False)

  def test_stop_gradient(self):
    x = keras.backend.variable(1)
    y = keras.backend.stop_gradient(x)
    self.assertEqual(y.op.name[:12], 'StopGradient')

    xs = [keras.backend.variable(1) for _ in range(3)]
    ys = keras.backend.stop_gradient(xs)
    for y in ys:
      self.assertEqual(y.op.name[:12], 'StopGradient')

  def test_function_tf_fetches(self):
    # Additional operations can be passed to tf.Session().run() via its
    # `fetches` arguments. In contrast to `updates` argument of
    # keras.backend.function() these do not have control dependency on `outputs`
    # so they can run in parallel. Also they should not contribute to output of
    # keras.backend.function().
    with self.test_session():
      x = keras.backend.variable(0.)
      y = keras.backend.variable(0.)
      x_placeholder = keras.backend.placeholder(shape=())
      y_placeholder = keras.backend.placeholder(shape=())

      f = keras.backend.function(inputs=[x_placeholder, y_placeholder],
                                 outputs=[x_placeholder + y_placeholder],
                                 updates=[(x, x_placeholder + 1.)],
                                 fetches=[keras.backend.update(y, 5.)])
      output = f([10., 20.])
      assert output == [30.]
      assert keras.backend.get_session().run(fetches=[x, y]) == [11., 5.]

  def test_function_tf_feed_dict(self):
    # Additional substitutions can be passed to `tf.Session().run()` via its
    # `feed_dict` arguments. Note that the feed_dict is passed once in the
    # constructor but we can modify the values in the dictionary. Through
    # this feed_dict we can provide additional substitutions besides Keras
    # inputs.
    with self.test_session():
      x = keras.backend.variable(0.)
      y = keras.backend.variable(0.)
      x_placeholder = keras.backend.placeholder(shape=())
      y_placeholder = keras.backend.placeholder(shape=())

      feed_dict = {y_placeholder: 3.}
      fetches = [keras.backend.update(y, y_placeholder * 10.)]
      f = keras.backend.function(inputs=[x_placeholder],
                                 outputs=[x_placeholder + 1.],
                                 updates=[(x, x_placeholder + 10.)],
                                 feed_dict=feed_dict,
                                 fetches=fetches)
      output = f([10.])
      assert output == [11.]
      assert keras.backend.get_session().run(fetches=[x, y]) == [20., 30.]

      # updated value in feed_dict will be modified within the K.function()
      feed_dict[y_placeholder] = 4.
      output = f([20.])
      assert output == [21.]
      assert keras.backend.get_session().run(fetches=[x, y]) == [30., 40.]


class BackendVariableTest(test.TestCase):

  def test_zeros(self):
    with self.test_session():
      x = keras.backend.zeros((3, 4))
      val = keras.backend.eval(x)
      self.assertAllClose(val, np.zeros((3, 4)))

  def test_ones(self):
    with self.test_session():
      x = keras.backend.ones((3, 4))
      val = keras.backend.eval(x)
      self.assertAllClose(val, np.ones((3, 4)))

  def test_eye(self):
    with self.test_session():
      x = keras.backend.eye(4)
      val = keras.backend.eval(x)
      self.assertAllClose(val, np.eye(4))

  def test_zeros_like(self):
    with self.test_session():
      x = keras.backend.zeros((3, 4))
      y = keras.backend.zeros_like(x)
      val = keras.backend.eval(y)
      self.assertAllClose(val, np.zeros((3, 4)))

  def test_ones_like(self):
    with self.test_session():
      x = keras.backend.zeros((3, 4))
      y = keras.backend.ones_like(x)
      val = keras.backend.eval(y)
      self.assertAllClose(val, np.ones((3, 4)))

  def test_random_uniform_variable(self):
    with self.test_session():
      x = keras.backend.random_uniform_variable((30, 20), low=1, high=2, seed=0)
      val = keras.backend.eval(x)
      self.assertAllClose(val.mean(), 1.5, atol=1e-1)
      self.assertAllClose(val.max(), 2., atol=1e-1)
      self.assertAllClose(val.min(), 1., atol=1e-1)

  def test_random_normal_variable(self):
    with self.test_session():
      x = keras.backend.random_normal_variable((30, 20), 1., 0.5,
                                               seed=0)
      val = keras.backend.eval(x)
      self.assertAllClose(val.mean(), 1., atol=1e-1)
      self.assertAllClose(val.std(), 0.5, atol=1e-1)

  def test_count_params(self):
    with self.test_session():
      x = keras.backend.zeros((4, 5))
      val = keras.backend.count_params(x)
      self.assertAllClose(val, 20)

  def test_constant(self):
    with self.test_session():
      ref_val = np.random.random((3, 4)).astype('float32')
      x = keras.backend.constant(ref_val)
      val = keras.backend.eval(x)
      self.assertAllClose(val, ref_val)

  def test_sparse_variable(self):
    with self.test_session():
      val = scipy.sparse.eye(10)
      x = keras.backend.variable(val)
      self.assertTrue(isinstance(x, sparse_tensor.SparseTensor))

      y = keras.backend.to_dense(x)
      self.assertFalse(keras.backend.is_sparse(y))

  def test_placeholder(self):
    x = keras.backend.placeholder(shape=(3, 4))
    self.assertEqual(x.get_shape().as_list(), [3, 4])
    x = keras.backend.placeholder(shape=(3, 4), sparse=True)
    self.assertEqual(x.get_shape().as_list(), [3, 4])


class BackendLinearAlgebraTest(test.TestCase):

  def test_dot(self):
    x = keras.backend.placeholder(shape=(2, 3))
    y = keras.backend.placeholder(shape=(3, 4))
    xy = keras.backend.dot(x, y)
    self.assertEqual(xy.get_shape().as_list(), [2, 4])

    x = keras.backend.placeholder(shape=(32, 28, 3))
    y = keras.backend.placeholder(shape=(3, 4))
    xy = keras.backend.dot(x, y)
    self.assertEqual(xy.get_shape().as_list(), [32, 28, 4])

  def test_batch_dot(self):
    x = keras.backend.ones(shape=(32, 20, 1))
    y = keras.backend.ones(shape=(32, 30, 20))
    xy = keras.backend.batch_dot(x, y, axes=[1, 2])
    self.assertEqual(xy.get_shape().as_list(), [32, 1, 30])

    # TODO(fchollet): insufficiently tested.

  def test_reduction_ops(self):
    ops_to_test = [
        (keras.backend.max, np.max),
        (keras.backend.min, np.min),
        (keras.backend.sum, np.sum),
        (keras.backend.prod, np.prod),
        (keras.backend.var, np.var),
        (keras.backend.std, np.std),
        (keras.backend.mean, np.mean),
        (keras.backend.argmin, np.argmin),
        (keras.backend.argmax, np.argmax),
    ]
    for keras_op, np_op in ops_to_test:
      with self.test_session():
        compare_single_input_op_to_numpy(keras_op, np_op, input_shape=(4, 7, 5),
                                         keras_kwargs={'axis': 1},
                                         np_kwargs={'axis': 1})
        compare_single_input_op_to_numpy(keras_op, np_op, input_shape=(4, 7, 5),
                                         keras_kwargs={'axis': -1},
                                         np_kwargs={'axis': -1})
        if 'keepdims' in tf_inspect.getargspec(keras_op).args:
          compare_single_input_op_to_numpy(keras_op, np_op,
                                           input_shape=(4, 7, 5),
                                           keras_kwargs={'axis': 1,
                                                         'keepdims': True},
                                           np_kwargs={'axis': 1,
                                                      'keepdims': True})

  def test_elementwise_ops(self):
    ops_to_test = [
        (keras.backend.square, np.square),
        (keras.backend.abs, np.abs),
        (keras.backend.round, np.round),
        (keras.backend.sign, np.sign),
        (keras.backend.sin, np.sin),
        (keras.backend.cos, np.cos),
        (keras.backend.exp, np.exp),
    ]
    for keras_op, np_op in ops_to_test:
      with self.test_session():
        compare_single_input_op_to_numpy(keras_op, np_op, input_shape=(4, 7))

    ops_to_test = [
        (keras.backend.sqrt, np.sqrt),
        (keras.backend.log, np.log),
    ]
    for keras_op, np_op in ops_to_test:
      with self.test_session():
        compare_single_input_op_to_numpy(keras_op, np_op,
                                         input_shape=(4, 7),
                                         negative_values=False)

    with self.test_session():
      compare_single_input_op_to_numpy(
          keras.backend.clip, np.clip,
          input_shape=(6, 4),
          keras_kwargs={'min_value': 0.1, 'max_value': 2.4},
          np_kwargs={'a_min': 0.1, 'a_max': 1.4})

    with self.test_session():
      compare_single_input_op_to_numpy(
          keras.backend.pow, np.power,
          input_shape=(6, 4),
          keras_args=[3],
          np_args=[3])

  def test_two_tensor_ops(self):
    ops_to_test = [
        (keras.backend.equal, np.equal),
        (keras.backend.not_equal, np.not_equal),
        (keras.backend.greater, np.greater),
        (keras.backend.greater_equal, np.greater_equal),
        (keras.backend.less, np.less),
        (keras.backend.less_equal, np.less_equal),
        (keras.backend.maximum, np.maximum),
        (keras.backend.minimum, np.minimum),
    ]
    for keras_op, np_op in ops_to_test:
      with self.test_session():
        compare_two_inputs_op_to_numpy(keras_op, np_op,
                                       input_shape_a=(4, 7),
                                       input_shape_b=(4, 7))


class BackendShapeOpsTest(test.TestCase):

  def test_reshape(self):
    with self.test_session():
      compare_single_input_op_to_numpy(keras.backend.reshape, np.reshape,
                                       input_shape=(4, 7),
                                       keras_args=[(2, 14)],
                                       np_args=[(2, 14)])

  def test_concatenate(self):
    a = keras.backend.variable(np.ones((1, 2, 3)))
    b = keras.backend.variable(np.ones((1, 2, 2)))
    y = keras.backend.concatenate([a, b], axis=-1)
    self.assertEqual(y.get_shape().as_list(), [1, 2, 5])

  def test_permute_dimensions(self):
    with self.test_session():
      compare_single_input_op_to_numpy(keras.backend.permute_dimensions,
                                       np.transpose,
                                       input_shape=(4, 7),
                                       keras_args=[(1, 0)],
                                       np_args=[(1, 0)])

  def test_resize_images(self):
    height_factor = 2
    width_factor = 2
    data_format = 'channels_last'
    x = keras.backend.variable(np.ones((1, 2, 2, 3)))
    y = keras.backend.resize_images(x,
                                    height_factor,
                                    width_factor,
                                    data_format)
    self.assertEqual(y.get_shape().as_list(), [1, 4, 4, 3])

    data_format = 'channels_first'
    x = keras.backend.variable(np.ones((1, 3, 2, 2)))
    y = keras.backend.resize_images(x,
                                    height_factor,
                                    width_factor,
                                    data_format)
    self.assertEqual(y.get_shape().as_list(), [1, 3, 4, 4])

    # Invalid use:
    with self.assertRaises(ValueError):
      keras.backend.resize_images(x,
                                  height_factor,
                                  width_factor,
                                  data_format='unknown')

  def test_resize_volumes(self):
    height_factor = 2
    width_factor = 2
    depth_factor = 2
    data_format = 'channels_last'
    x = keras.backend.variable(np.ones((1, 2, 2, 2, 3)))
    y = keras.backend.resize_volumes(x,
                                     depth_factor,
                                     height_factor,
                                     width_factor,
                                     data_format)
    self.assertEqual(y.get_shape().as_list(), [1, 4, 4, 4, 3])

    data_format = 'channels_first'
    x = keras.backend.variable(np.ones((1, 3, 2, 2, 2)))
    y = keras.backend.resize_volumes(x,
                                     depth_factor,
                                     height_factor,
                                     width_factor,
                                     data_format)
    self.assertEqual(y.get_shape().as_list(), [1, 3, 4, 4, 4])

    # Invalid use:
    with self.assertRaises(ValueError):
      keras.backend.resize_volumes(x,
                                   depth_factor,
                                   height_factor,
                                   width_factor,
                                   data_format='unknown')

  def test_repeat_elements(self):
    x = keras.backend.variable(np.ones((1, 3, 2)))
    y = keras.backend.repeat_elements(x, 3, axis=1)
    self.assertEqual(y.get_shape().as_list(), [1, 9, 2])

    # Use with a dynamic axis:
    x = keras.backend.placeholder(shape=(2, None, 2))
    y = keras.backend.repeat_elements(x, 3, axis=1)
    self.assertEqual(y.get_shape().as_list(), [2, None, 2])

  def test_repeat(self):
    x = keras.backend.variable(np.ones((1, 3)))
    y = keras.backend.repeat(x, 2)
    self.assertEqual(y.get_shape().as_list(), [1, 2, 3])

  def test_flatten(self):
    with self.test_session():
      compare_single_input_op_to_numpy(keras.backend.flatten,
                                       np.reshape,
                                       input_shape=(4, 7, 6),
                                       np_args=[(4 * 7 * 6,)])

  def test_batch_flatten(self):
    with self.test_session():
      compare_single_input_op_to_numpy(keras.backend.batch_flatten,
                                       np.reshape,
                                       input_shape=(4, 7, 6),
                                       np_args=[(4, 7 * 6)])

  def test_temporal_padding(self):

    def ref_op(x, padding):
      shape = list(x.shape)
      shape[1] += padding[0] + padding[1]
      y = np.zeros(tuple(shape))
      y[:, padding[0]:-padding[1], :] = x
      return y

    with self.test_session():
      compare_single_input_op_to_numpy(keras.backend.temporal_padding,
                                       ref_op,
                                       input_shape=(4, 7, 6),
                                       keras_args=[(2, 3)],
                                       np_args=[(2, 3)])

  def test_spatial_2d_padding(self):

    def ref_op(x, padding, data_format='channels_last'):
      shape = list(x.shape)
      if data_format == 'channels_last':
        shape[1] += padding[0][0] + padding[0][1]
        shape[2] += padding[1][0] + padding[1][1]
        y = np.zeros(tuple(shape))
        y[:, padding[0][0]:-padding[0][1], padding[1][0]:-padding[1][1], :] = x
      else:
        shape[2] += padding[0][0] + padding[0][1]
        shape[3] += padding[1][0] + padding[1][1]
        y = np.zeros(tuple(shape))
        y[:, :, padding[0][0]:-padding[0][1], padding[1][0]:-padding[1][1]] = x
      return y

    with self.test_session():
      compare_single_input_op_to_numpy(
          keras.backend.spatial_2d_padding,
          ref_op,
          input_shape=(2, 3, 2, 3),
          keras_args=[((2, 3), (1, 2))],
          keras_kwargs={'data_format': 'channels_last'},
          np_args=[((2, 3), (1, 2))],
          np_kwargs={'data_format': 'channels_last'})
      compare_single_input_op_to_numpy(
          keras.backend.spatial_2d_padding,
          ref_op,
          input_shape=(2, 3, 2, 3),
          keras_args=[((2, 3), (1, 2))],
          keras_kwargs={'data_format': 'channels_first'},
          np_args=[((2, 3), (1, 2))],
          np_kwargs={'data_format': 'channels_first'})

  def test_spatial_3d_padding(self):

    def ref_op(x, padding, data_format='channels_last'):
      shape = list(x.shape)
      if data_format == 'channels_last':
        shape[1] += padding[0][0] + padding[0][1]
        shape[2] += padding[1][0] + padding[1][1]
        shape[3] += padding[2][0] + padding[2][1]
        y = np.zeros(tuple(shape))
        y[:,
          padding[0][0]:-padding[0][1],
          padding[1][0]:-padding[1][1],
          padding[2][0]:-padding[2][1],
          :] = x
      else:
        shape[2] += padding[0][0] + padding[0][1]
        shape[3] += padding[1][0] + padding[1][1]
        shape[4] += padding[2][0] + padding[2][1]
        y = np.zeros(tuple(shape))
        y[:, :,
          padding[0][0]:-padding[0][1],
          padding[1][0]:-padding[1][1],
          padding[2][0]:-padding[2][1]] = x
      return y

    with self.test_session():
      compare_single_input_op_to_numpy(
          keras.backend.spatial_3d_padding,
          ref_op,
          input_shape=(2, 3, 2, 3, 2),
          keras_args=[((2, 3), (1, 2), (2, 3))],
          keras_kwargs={'data_format': 'channels_last'},
          np_args=[((2, 3), (1, 2), (2, 3))],
          np_kwargs={'data_format': 'channels_last'})
      compare_single_input_op_to_numpy(
          keras.backend.spatial_3d_padding,
          ref_op,
          input_shape=(2, 3, 2, 3, 2),
          keras_args=[((2, 3), (1, 2), (2, 3))],
          keras_kwargs={'data_format': 'channels_first'},
          np_args=[((2, 3), (1, 2), (2, 3))],
          np_kwargs={'data_format': 'channels_first'})


class BackendNNOpsTest(test.TestCase):

  def test_bias_add(self):
    with self.test_session():
      keras_op = keras.backend.bias_add
      np_op = np.add
      compare_two_inputs_op_to_numpy(keras_op, np_op,
                                     input_shape_a=(4, 7),
                                     input_shape_b=(7,))
      compare_two_inputs_op_to_numpy(keras_op, np_op,
                                     input_shape_a=(4, 3, 7),
                                     input_shape_b=(7,))
      compare_two_inputs_op_to_numpy(keras_op, np_op,
                                     input_shape_a=(4, 3, 5, 7),
                                     input_shape_b=(7,))
      compare_two_inputs_op_to_numpy(keras_op, np_op,
                                     input_shape_a=(4, 3, 5, 2, 7),
                                     input_shape_b=(7,))

      with self.assertRaises(ValueError):
        x = keras.backend.variable((3, 4))
        b = keras.backend.variable((3, 4))
        keras.backend.bias_add(x, b)
      with self.assertRaises(ValueError):
        x = keras.backend.variable((3, 4))
        b = keras.backend.variable((4,))
        keras.backend.bias_add(x, b, data_format='unknown')

  def test_bias_add_channels_first(self):
    with self.test_session():
      def keras_op(x, b):
        return keras.backend.bias_add(x, b, data_format='channels_first')

      def np_op(x, b):
        if x.ndim == 3:
          b = b.reshape((1, b.shape[0], 1))
        if x.ndim == 4:
          b = b.reshape((1, b.shape[0], 1, 1))
        return x + b

      compare_two_inputs_op_to_numpy(keras_op, np_op,
                                     input_shape_a=(4, 3, 7),
                                     input_shape_b=(3,))
      compare_two_inputs_op_to_numpy(keras_op, np_op,
                                     input_shape_a=(4, 3, 5, 7),
                                     input_shape_b=(3,))

  def test_pool2d(self):
    val = np.random.random((10, 3, 10, 10))
    x = keras.backend.variable(val)
    y = keras.backend.pool2d(x, (2, 2), strides=(1, 1),
                             padding='valid', data_format='channels_first',
                             pool_mode='max')
    self.assertEqual(y.get_shape().as_list(), [10, 3, 9, 9])

    y = keras.backend.pool2d(x, (2, 2), strides=(1, 1),
                             padding='valid', data_format='channels_first',
                             pool_mode='avg')
    self.assertEqual(y.get_shape().as_list(), [10, 3, 9, 9])

    val = np.random.random((10, 10, 10, 3))
    x = keras.backend.variable(val)
    y = keras.backend.pool2d(x, (2, 2), strides=(1, 1),
                             padding='valid', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 9, 9, 3])

    val = np.random.random((10, 10, 10, 3))
    x = keras.backend.variable(val)
    y = keras.backend.pool2d(x, (2, 2), strides=(1, 1),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 10, 10, 3])

    val = np.random.random((10, 10, 10, 3))
    x = keras.backend.variable(val)
    y = keras.backend.pool2d(x, (2, 2), strides=(2, 2),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 5, 3])

    with self.assertRaises(ValueError):
      y = keras.backend.pool2d(x, (2, 2), strides=(2, 2),
                               padding='other', data_format='channels_last')
    with self.assertRaises(ValueError):
      y = keras.backend.pool2d(x, (2, 2), strides=(2, 2),
                               data_format='other')
    with self.assertRaises(ValueError):
      y = keras.backend.pool2d(x, (2, 2, 2), strides=(2, 2))
    with self.assertRaises(ValueError):
      y = keras.backend.pool2d(x, (2, 2), strides=(2, 2, 2))
    with self.assertRaises(ValueError):
      y = keras.backend.pool2d(x, (2, 2), strides=(2, 2), pool_mode='other')

  def test_pool3d(self):
    val = np.random.random((10, 3, 10, 10, 10))
    x = keras.backend.variable(val)
    y = keras.backend.pool3d(x, (2, 2, 2), strides=(1, 1, 1),
                             padding='valid', data_format='channels_first',
                             pool_mode='max')
    self.assertEqual(y.get_shape().as_list(), [10, 3, 9, 9, 9])

    y = keras.backend.pool3d(x, (2, 2, 2), strides=(1, 1, 1),
                             padding='valid', data_format='channels_first',
                             pool_mode='avg')
    self.assertEqual(y.get_shape().as_list(), [10, 3, 9, 9, 9])

    val = np.random.random((10, 10, 10, 10, 3))
    x = keras.backend.variable(val)
    y = keras.backend.pool3d(x, (2, 2, 2), strides=(1, 1, 1),
                             padding='valid', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 9, 9, 9, 3])

    val = np.random.random((10, 10, 10, 10, 3))
    x = keras.backend.variable(val)
    y = keras.backend.pool3d(x, (2, 2, 2), strides=(1, 1, 1),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 10, 10, 10, 3])

    val = np.random.random((10, 10, 10, 10, 3))
    x = keras.backend.variable(val)
    y = keras.backend.pool3d(x, (2, 2, 2), strides=(2, 2, 2),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 5, 5, 3])

  def test_conv1d(self):
    val = np.random.random((10, 4, 10))
    x = keras.backend.variable(val)
    kernel_val = np.random.random((3, 4, 5))
    k = keras.backend.variable(kernel_val)
    y = keras.backend.conv1d(x, k, strides=(1,),
                             padding='valid', data_format='channels_first')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 8])

    val = np.random.random((10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv1d(x, k, strides=(1,),
                             padding='valid', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 8, 5])

    val = np.random.random((10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv1d(x, k, strides=(1,),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 10, 5])

    val = np.random.random((10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv1d(x, k, strides=(2,),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 5])

  def test_conv2d(self):
    val = np.random.random((10, 4, 10, 10))
    x = keras.backend.variable(val)
    kernel_val = np.random.random((3, 3, 4, 5))
    k = keras.backend.variable(kernel_val)
    y = keras.backend.conv2d(x, k,
                             padding='valid', data_format='channels_first')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 8, 8])

    val = np.random.random((10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv2d(x, k, strides=(1, 1),
                             padding='valid', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 8, 8, 5])

    val = np.random.random((10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv2d(x, k, strides=(1, 1),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 10, 10, 5])

    val = np.random.random((10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv2d(x, k, strides=(2, 2),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 5, 5])
    with self.assertRaises(ValueError):
      y = keras.backend.conv2d(x, k, (2, 2),
                               padding='other', data_format='channels_last')
    with self.assertRaises(ValueError):
      y = keras.backend.conv2d(x, k, (2, 2),
                               data_format='other')
    with self.assertRaises(ValueError):
      y = keras.backend.conv2d(x, k, (2, 2, 2))

  def test_separable_conv2d(self):
    val = np.random.random((10, 4, 10, 10))
    x = keras.backend.variable(val)
    depthwise_kernel_val = np.random.random((3, 3, 4, 1))
    pointwise_kernel_val = np.random.random((1, 1, 4, 5))
    dk = keras.backend.variable(depthwise_kernel_val)
    pk = keras.backend.variable(pointwise_kernel_val)
    y = keras.backend.separable_conv2d(
        x, dk, pk, padding='valid', data_format='channels_first')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 8, 8])

    val = np.random.random((10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.separable_conv2d(
        x, dk, pk, strides=(1, 1), padding='valid', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 8, 8, 5])

    val = np.random.random((10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.separable_conv2d(
        x, dk, pk, strides=(1, 1), padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 10, 10, 5])

    val = np.random.random((10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.separable_conv2d(
        x, dk, pk, strides=(2, 2), padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 5, 5])
    with self.assertRaises(ValueError):
      y = keras.backend.separable_conv2d(
          x, dk, pk, (2, 2), padding='other', data_format='channels_last')
    with self.assertRaises(ValueError):
      y = keras.backend.separable_conv2d(
          x, dk, pk, (2, 2), data_format='other')
    with self.assertRaises(ValueError):
      y = keras.backend.separable_conv2d(x, dk, pk, (2, 2, 2))

  def test_conv3d(self):
    val = np.random.random((10, 4, 10, 10, 10))
    x = keras.backend.variable(val)
    kernel_val = np.random.random((3, 3, 3, 4, 5))
    k = keras.backend.variable(kernel_val)
    y = keras.backend.conv3d(x, k,
                             padding='valid', data_format='channels_first')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 8, 8, 8])

    val = np.random.random((10, 10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv3d(x, k, strides=(1, 1, 1),
                             padding='valid', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 8, 8, 8, 5])

    val = np.random.random((10, 10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv3d(x, k, strides=(1, 1, 1),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 10, 10, 10, 5])

    val = np.random.random((10, 10, 10, 10, 4))
    x = keras.backend.variable(val)
    y = keras.backend.conv3d(x, k, strides=(2, 2, 2),
                             padding='same', data_format='channels_last')
    self.assertEqual(y.get_shape().as_list(), [10, 5, 5, 5, 5])
    with self.assertRaises(ValueError):
      y = keras.backend.conv3d(x, k, (2, 2, 2),
                               padding='other', data_format='channels_last')
    with self.assertRaises(ValueError):
      y = keras.backend.conv3d(x, k, (2, 2, 2),
                               data_format='other')
    with self.assertRaises(ValueError):
      y = keras.backend.conv3d(x, k, (2, 2))

  def test_rnn(self):
    # implement a simple RNN
    num_samples = 4
    input_dim = 5
    output_dim = 3
    timesteps = 6

    input_val = np.random.random(
        (num_samples, timesteps, input_dim)).astype(np.float32)
    init_state_val = np.random.random(
        (num_samples, output_dim)).astype(np.float32)
    w_i_val = np.random.random((input_dim, output_dim)).astype(np.float32)
    w_o_val = np.random.random((output_dim, output_dim)).astype(np.float32)
    np_mask = np.random.randint(2, size=(num_samples, timesteps))

    def rnn_step_fn():
      w_i = keras.backend.variable(w_i_val)
      w_o = keras.backend.variable(w_o_val)

      def step_function(x, states):
        assert len(states) == 1
        prev_output = states[0]
        output = keras.backend.dot(x, w_i) + keras.backend.dot(prev_output, w_o)
        return output, [output]

      return step_function

    # test default setup
    last_output_list = [[], [], [], [], [], []]
    outputs_list = [[], [], [], [], [], []]
    state_list = [[], [], [], [], [], []]

    rnn_fn = rnn_step_fn()
    inputs = keras.backend.variable(input_val)
    initial_states = [keras.backend.variable(init_state_val)]
    mask = keras.backend.variable(np_mask)

    kwargs_list = [
        {'go_backwards': False, 'mask': None},
        {'go_backwards': False, 'mask': None, 'unroll': True},
        {'go_backwards': True, 'mask': None},
        {'go_backwards': True, 'mask': None, 'unroll': True},
        {'go_backwards': False, 'mask': mask},
        {'go_backwards': False, 'mask': mask, 'unroll': True},
    ]
    with self.test_session():
      for (i, kwargs) in enumerate(kwargs_list):
        last_output, outputs, new_states = keras.backend.rnn(rnn_fn, inputs,
                                                             initial_states,
                                                             **kwargs)
        # check static shape inference
        self.assertEquals(last_output.get_shape().as_list(),
                          [num_samples, output_dim])
        self.assertEquals(outputs.get_shape().as_list(),
                          [num_samples, timesteps, output_dim])
        for state in new_states:
          self.assertEquals(state.get_shape().as_list(),
                            [num_samples, output_dim])

        last_output_list[i].append(keras.backend.eval(last_output))
        outputs_list[i].append(keras.backend.eval(outputs))
        self.assertEqual(len(new_states), 1)
        state_list[i].append(keras.backend.eval(new_states[0]))

      def assert_list_pairwise(z_list, atol=1e-05):
        for (z1, z2) in zip(z_list[1:], z_list[:-1]):
          self.assertAllClose(z1, z2, atol=atol)

      assert_list_pairwise(last_output_list[0], atol=1e-04)
      assert_list_pairwise(outputs_list[0], atol=1e-04)
      assert_list_pairwise(state_list[0], atol=1e-04)
      assert_list_pairwise(last_output_list[2], atol=1e-04)
      assert_list_pairwise(outputs_list[2], atol=1e-04)
      assert_list_pairwise(state_list[2], atol=1e-04)

      for l, u_l in zip(last_output_list[0], last_output_list[1]):
        self.assertAllClose(l, u_l, atol=1e-04)

      for o, u_o in zip(outputs_list[0], outputs_list[1]):
        self.assertAllClose(o, u_o, atol=1e-04)

      for s, u_s in zip(state_list[0], state_list[1]):
        self.assertAllClose(s, u_s, atol=1e-04)

      for b_l, b_u_l in zip(last_output_list[2], last_output_list[3]):
        self.assertAllClose(b_l, b_u_l, atol=1e-04)

      for b_o, b_u_o in zip(outputs_list[2], outputs_list[3]):
        self.assertAllClose(b_o, b_u_o, atol=1e-04)

      for b_s, b_u_s in zip(state_list[2], state_list[3]):
        self.assertAllClose(b_s, b_u_s, atol=1e-04)

  def test_normalize_batch_in_training(self):
    val = np.random.random((10, 3, 10, 10))
    x = keras.backend.variable(val)
    reduction_axes = (0, 2, 3)

    g_val = np.random.random((3,))
    b_val = np.random.random((3,))
    gamma = keras.backend.variable(g_val)
    beta = keras.backend.variable(b_val)
    normed, mean, var = keras.backend.normalize_batch_in_training(
        x, gamma, beta, reduction_axes, epsilon=1e-3)
    self.assertEqual(normed.get_shape().as_list(), [10, 3, 10, 10])
    self.assertEqual(mean.get_shape().as_list(), [3,])
    self.assertEqual(var.get_shape().as_list(), [3,])

    # case: gamma=None
    gamma = None
    normed, mean, var = keras.backend.normalize_batch_in_training(
        x, gamma, beta, reduction_axes, epsilon=1e-3)
    self.assertEqual(normed.get_shape().as_list(), [10, 3, 10, 10])
    self.assertEqual(mean.get_shape().as_list(), [3,])
    self.assertEqual(var.get_shape().as_list(), [3,])

    # case: beta=None
    beta = None
    normed, mean, var = keras.backend.normalize_batch_in_training(
        x, gamma, beta, reduction_axes, epsilon=1e-3)
    self.assertEqual(normed.get_shape().as_list(), [10, 3, 10, 10])
    self.assertEqual(mean.get_shape().as_list(), [3,])
    self.assertEqual(var.get_shape().as_list(), [3,])


class TestCTC(test.TestCase):

  def test_ctc_decode(self):
    with self.test_session():
      depth = 6
      seq_len_0 = 5
      input_prob_matrix_0 = np.asarray(
          [[0.30999, 0.309938, 0.0679938, 0.0673362, 0.0708352, 0.173908],
           [0.215136, 0.439699, 0.0370931, 0.0393967, 0.0381581, 0.230517],
           [0.199959, 0.489485, 0.0233221, 0.0251417, 0.0233289, 0.238763],
           [0.279611, 0.452966, 0.0204795, 0.0209126, 0.0194803, 0.20655],
           [0.51286, 0.288951, 0.0243026, 0.0220788, 0.0219297, 0.129878],
           # Random entry added in at time=5
           [0.155251, 0.164444, 0.173517, 0.176138, 0.169979, 0.160671]],
          dtype=np.float32)

      # len max_time_steps array of batch_size x depth matrices
      inputs = ([input_prob_matrix_0[t, :][np.newaxis, :]
                 for t in range(seq_len_0)] +  # Pad to max_time_steps = 8
                2 * [np.zeros((1, depth), dtype=np.float32)])

      inputs = keras.backend.variable(np.asarray(inputs).transpose((1, 0, 2)))

      # batch_size length vector of sequence_lengths
      input_length = keras.backend.variable(
          np.array([seq_len_0], dtype=np.int32))
      # batch_size length vector of negative log probabilities
      log_prob_truth = np.array([
          0.584855,  # output beam 0
          0.389139  # output beam 1
      ], np.float32)[np.newaxis, :]

      decode_truth = [np.array([1, 0]), np.array([0, 1, 0])]
      beam_width = 2
      top_paths = 2

      decode_pred_tf, log_prob_pred_tf = keras.backend.ctc_decode(
          inputs,
          input_length,
          greedy=False,
          beam_width=beam_width,
          top_paths=top_paths)

      self.assertEqual(len(decode_pred_tf), top_paths)
      log_prob_pred = keras.backend.eval(log_prob_pred_tf)
      for i in range(top_paths):
        self.assertTrue(
            np.alltrue(
                decode_truth[i] == keras.backend.eval(decode_pred_tf[i])))
      self.assertAllClose(log_prob_truth, log_prob_pred)

  def test_ctc_batch_cost(self):
    with self.test_session():
      label_lens = np.expand_dims(np.asarray([5, 4]), 1)
      input_lens = np.expand_dims(np.asarray([5, 5]), 1)  # number of timesteps
      loss_log_probs = [3.34211, 5.42262]

      # dimensions are batch x time x categories
      labels = np.asarray([[0, 1, 2, 1, 0], [0, 1, 1, 0, -1]])
      inputs = np.asarray(
          [[[0.633766, 0.221185, 0.0917319, 0.0129757, 0.0142857, 0.0260553],
            [0.111121, 0.588392, 0.278779, 0.0055756, 0.00569609, 0.010436],
            [0.0357786, 0.633813, 0.321418, 0.00249248, 0.00272882, 0.0037688],
            [0.0663296, 0.643849, 0.280111, 0.00283995, 0.0035545, 0.00331533],
            [0.458235, 0.396634, 0.123377, 0.00648837, 0.00903441, 0.00623107]],
           [[0.30176, 0.28562, 0.0831517, 0.0862751, 0.0816851, 0.161508],
            [0.24082, 0.397533, 0.0557226, 0.0546814, 0.0557528, 0.19549],
            [0.230246, 0.450868, 0.0389607, 0.038309, 0.0391602, 0.202456],
            [0.280884, 0.429522, 0.0326593, 0.0339046, 0.0326856, 0.190345],
            [0.423286, 0.315517, 0.0338439, 0.0393744, 0.0339315, 0.154046]]],
          dtype=np.float32)

      labels = keras.backend.variable(labels, dtype='int32')
      inputs = keras.backend.variable(inputs, dtype='float32')
      input_lens = keras.backend.variable(input_lens, dtype='int32')
      label_lens = keras.backend.variable(label_lens, dtype='int32')
      res = keras.backend.eval(
          keras.backend.ctc_batch_cost(labels, inputs, input_lens, label_lens))
      self.assertAllClose(res[:, 0], loss_log_probs, atol=1e-05)


class TestRandomOps(test.TestCase):

  def test_random_binomial(self):
    with self.test_session():
      np.random.seed(123)
      x = keras.backend.random_binomial((1000, 1000), p=0.5)
      self.assertAllClose(np.mean(keras.backend.eval(x)), 0.5, atol=0.1)

  def test_truncated_normal(self):
    with self.test_session():
      np.random.seed(123)
      x = keras.backend.truncated_normal((1000, 1000), mean=0.0, stddev=1.0)
      y = keras.backend.eval(x)
      self.assertAllClose(np.mean(y), 0., atol=0.1)
      self.assertAllClose(np.std(y), 0.88, atol=0.1)
      self.assertAllClose(np.max(y), 2., atol=0.1)
      self.assertAllClose(np.min(y), -2., atol=0.1)


if __name__ == '__main__':
  test.main()
