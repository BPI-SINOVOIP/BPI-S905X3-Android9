# Copyright 2018 The TensorFlow Authors. All Rights Reserved.
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
"""Tests for Model subclassing."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import tempfile

import numpy as np

from tensorflow.python.framework import tensor_shape
from tensorflow.python.framework import test_util
from tensorflow.python.keras._impl import keras
from tensorflow.python.ops import array_ops
from tensorflow.python.platform import test
from tensorflow.python.training.rmsprop import RMSPropOptimizer

try:
  import h5py  # pylint:disable=g-import-not-at-top
except ImportError:
  h5py = None


class SimpleTestModel(keras.Model):

  def __init__(self, use_bn=False, use_dp=False, num_classes=10):
    super(SimpleTestModel, self).__init__(name='test_model')
    self.use_bn = use_bn
    self.use_dp = use_dp
    self.num_classes = num_classes

    self.dense1 = keras.layers.Dense(32, activation='relu')
    self.dense2 = keras.layers.Dense(num_classes, activation='softmax')
    if self.use_dp:
      self.dp = keras.layers.Dropout(0.5)
    if self.use_bn:
      self.bn = keras.layers.BatchNormalization(axis=-1)

  def call(self, inputs):
    x = self.dense1(inputs)
    if self.use_dp:
      x = self.dp(x)
    if self.use_bn:
      x = self.bn(x)
    return self.dense2(x)


class MultiIOTestModel(keras.Model):

  def __init__(self, use_bn=False, use_dp=False, num_classes=(2, 3)):
    super(MultiIOTestModel, self).__init__(name='test_model')
    self.use_bn = use_bn
    self.use_dp = use_dp
    self.num_classes = num_classes

    self.dense1 = keras.layers.Dense(32, activation='relu')
    self.dense2 = keras.layers.Dense(num_classes[0], activation='softmax')
    self.dense3 = keras.layers.Dense(num_classes[1], activation='softmax')
    if use_dp:
      self.dp = keras.layers.Dropout(0.5)
    if use_bn:
      self.bn = keras.layers.BatchNormalization()

  def call(self, inputs):
    x1, x2 = inputs
    x1 = self.dense1(x1)
    x2 = self.dense1(x2)
    if self.use_dp:
      x1 = self.dp(x1)
    if self.use_bn:
      x2 = self.bn(x2)
    return [self.dense2(x1), self.dense3(x2)]


class NestedTestModel1(keras.Model):
  """A model subclass nested inside a model subclass.
  """

  def __init__(self, num_classes=2):
    super(NestedTestModel1, self).__init__(name='nested_model_1')
    self.num_classes = num_classes
    self.dense1 = keras.layers.Dense(32, activation='relu')
    self.dense2 = keras.layers.Dense(num_classes, activation='relu')
    self.bn = keras.layers.BatchNormalization()
    self.test_net = SimpleTestModel(num_classes=4,
                                    use_bn=True,
                                    use_dp=True)

  def call(self, inputs):
    x = self.dense1(inputs)
    x = self.bn(x)
    x = self.test_net(x)  # pylint: disable=not-callable
    return self.dense2(x)


def get_functional_graph_model(input_dim, num_classes):
  # A simple functional-API model (a.k.a. graph network)
  inputs = keras.Input(shape=(input_dim,))
  x = keras.layers.Dense(32, activation='relu')(inputs)
  x = keras.layers.BatchNormalization()(x)
  outputs = keras.layers.Dense(num_classes)(x)
  return keras.Model(inputs, outputs)


class NestedTestModel2(keras.Model):
  """A model subclass with a functional-API graph network inside.
  """

  def __init__(self, num_classes=2):
    super(NestedTestModel2, self).__init__(name='nested_model_2')
    self.num_classes = num_classes
    self.dense1 = keras.layers.Dense(32, activation='relu')
    self.dense2 = keras.layers.Dense(num_classes, activation='relu')
    self.bn = self.bn = keras.layers.BatchNormalization()
    self.test_net = get_functional_graph_model(32, 4)

  def call(self, inputs):
    x = self.dense1(inputs)
    x = self.bn(x)
    x = self.test_net(x)
    return self.dense2(x)


def get_nested_model_3(input_dim, num_classes):
  # A functional-API model with a subclassed model inside.
  # NOTE: this requires the inner subclass to implement `compute_output_shape`.

  inputs = keras.Input(shape=(input_dim,))
  x = keras.layers.Dense(32, activation='relu')(inputs)
  x = keras.layers.BatchNormalization()(x)

  class Inner(keras.Model):

    def __init__(self):
      super(Inner, self).__init__()
      self.dense1 = keras.layers.Dense(32, activation='relu')
      self.dense2 = keras.layers.Dense(5, activation='relu')
      self.bn = keras.layers.BatchNormalization()

    def call(self, inputs):
      x = self.dense1(inputs)
      x = self.dense2(x)
      return self.bn(x)

    def compute_output_shape(self, input_shape):
      return tensor_shape.TensorShape((input_shape[0], 5))

  test_model = Inner()
  x = test_model(x)  # pylint: disable=not-callable
  outputs = keras.layers.Dense(num_classes)(x)
  return keras.Model(inputs, outputs, name='nested_model_3')


class ModelSubclassingTest(test.TestCase):

  @test_util.run_in_graph_and_eager_modes()
  def test_single_io_workflow_with_np_arrays(self):
    num_classes = 2
    num_samples = 100
    input_dim = 50

    with self.test_session():
      model = SimpleTestModel(num_classes=num_classes,
                              use_dp=True,
                              use_bn=True)
      model.compile(loss='mse',
                    optimizer=RMSPropOptimizer(learning_rate=0.001),
                    metrics=['acc'])

      x = np.ones((num_samples, input_dim))
      y = np.zeros((num_samples, num_classes))

      model.fit(x, y, epochs=2, batch_size=32, verbose=0)
      _ = model.evaluate(x, y, verbose=0)

  @test_util.run_in_graph_and_eager_modes()
  def test_multi_io_workflow_with_np_arrays(self):
    num_classes = (2, 3)
    num_samples = 1000
    input_dim = 50

    with self.test_session():
      model = MultiIOTestModel(num_classes=num_classes,
                               use_dp=True,
                               use_bn=True)
      model.compile(loss='mse',
                    optimizer=RMSPropOptimizer(learning_rate=0.001),
                    metrics=['acc'])

      x1 = np.ones((num_samples, input_dim))
      x2 = np.ones((num_samples, input_dim))
      y1 = np.zeros((num_samples, num_classes[0]))
      y2 = np.zeros((num_samples, num_classes[1]))

      model.fit([x1, x2], [y1, y2], epochs=2, batch_size=32, verbose=0)
      _ = model.evaluate([x1, x2], [y1, y2], verbose=0)

  def test_single_io_workflow_with_tensors(self):

    num_classes = 2
    num_samples = 10
    input_dim = 50

    with self.test_session():
      model = SimpleTestModel(num_classes=num_classes,
                              use_dp=True,
                              use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))

      x = array_ops.ones((num_samples, input_dim))
      y = array_ops.zeros((num_samples, num_classes))

      model.fit(x, y, epochs=2, steps_per_epoch=10, verbose=0)
      _ = model.evaluate(steps=10, verbose=0)

  def test_multi_io_workflow_with_tensors(self):

    num_classes = (2, 3)
    num_samples = 10
    input_dim = 50

    with self.test_session():
      model = MultiIOTestModel(num_classes=num_classes,
                               use_dp=True,
                               use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))

      x1 = array_ops.ones((num_samples, input_dim))
      x2 = array_ops.ones((num_samples, input_dim))
      y1 = array_ops.zeros((num_samples, num_classes[0]))
      y2 = array_ops.zeros((num_samples, num_classes[1]))

      model.fit([x1, x2], [y1, y2], epochs=2, steps_per_epoch=10, verbose=0)
      _ = model.evaluate(steps=10, verbose=0)

  def test_multi_io_workflow_with_numpy_arrays_and_custom_placeholders(self):

    num_classes = (2, 3)
    num_samples = 1000
    input_dim = 50

    with self.test_session():
      model = MultiIOTestModel(num_classes=num_classes,
                               use_dp=True,
                               use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))

      x1 = np.ones((num_samples, input_dim))
      x2 = np.ones((num_samples, input_dim))
      y1 = np.zeros((num_samples, num_classes[0]))
      y2 = np.zeros((num_samples, num_classes[1]))

      x2_placeholder = array_ops.placeholder(
          dtype='float32', shape=(None, input_dim))
      model._set_inputs([x1, x2_placeholder])

      model.fit([x1, x2], [y1, y2], epochs=2, batch_size=32, verbose=0)
      _ = model.evaluate([x1, x2], [y1, y2], verbose=0)

  @test_util.run_in_graph_and_eager_modes(assert_no_eager_garbage=True)
  def test_attributes(self):
    # layers, weights, trainable_weights, non_trainable_weights, inputs, outputs

    num_classes = (2, 3)
    num_samples = 100
    input_dim = 50

    model = MultiIOTestModel(num_classes=num_classes, use_bn=True)

    x1 = np.ones((num_samples, input_dim))
    x2 = np.ones((num_samples, input_dim))
    y1 = np.zeros((num_samples, num_classes[0]))
    y2 = np.zeros((num_samples, num_classes[1]))

    self.assertEqual(model.name, 'test_model')
    self.assertEqual(model.built, False)
    self.assertEqual(len(model.weights), 0)

    model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
    model.train_on_batch([x1, x2], [y1, y2])

    self.assertEqual(model.built, True)
    self.assertEqual(len(model.layers), 4)
    self.assertEqual(len(model.weights), 10)
    self.assertEqual(len(model.trainable_weights), 8)
    self.assertEqual(len(model.non_trainable_weights), 2)
    self.assertEqual(len(model.inputs), 2)
    self.assertEqual(len(model.outputs), 2)

  @test_util.run_in_graph_and_eager_modes()
  def test_updates(self):
    # test that updates get run during training
    num_samples = 100
    input_dim = 50

    class BNNet(keras.Model):

      def __init__(self):
        super(BNNet, self).__init__()
        self.bn = keras.layers.BatchNormalization(beta_initializer='ones',
                                                  gamma_initializer='ones')

      def call(self, inputs):
        return self.bn(inputs)

    x = np.ones((num_samples, input_dim))
    y = np.ones((num_samples, input_dim))

    with self.test_session():
      model = BNNet()
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      y_ref = model.predict(x)

      model.train_on_batch(x, y)
      y_new = model.predict(x)
      self.assertGreater(np.sum(np.abs(y_ref - y_new)), 0.1)

  @test_util.run_in_graph_and_eager_modes()
  def test_training_and_inference_behavior(self):
    # test that dropout is applied in training and not inference

    num_samples = 100
    input_dim = 50

    class DPNet(keras.Model):

      def __init__(self):
        super(DPNet, self).__init__()
        self.dp = keras.layers.Dropout(0.5)
        self.dense = keras.layers.Dense(1,
                                        use_bias=False,
                                        kernel_initializer='ones')

      def call(self, inputs):
        x = self.dp(inputs)
        return self.dense(x)

    with self.test_session():
      model = DPNet()
      x = np.ones((num_samples, input_dim))
      y = model.predict(x)
      self.assertEqual(np.sum(y), np.sum(x))
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      loss = model.train_on_batch(x, y)
      self.assertGreater(loss, 0.1)

  @test_util.run_in_graph_and_eager_modes()
  def test_training_methods(self):
    # test fit, train_on_batch
    # on different input types: list, dict

    num_classes = (2, 3)
    num_samples = 100
    input_dim = 50

    x1 = np.ones((num_samples, input_dim))
    x2 = np.ones((num_samples, input_dim))
    y1 = np.zeros((num_samples, num_classes[0]))
    y2 = np.zeros((num_samples, num_classes[1]))

    with self.test_session():
      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      model.fit([x1, x2], [y1, y2], epochs=2, batch_size=32, verbose=0)
      model.fit({'input_1': x1, 'input_2': x2},
                {'output_1': y1, 'output_2': y2},
                epochs=2, batch_size=32)
      model.fit([x1, x2], [y1, y2], epochs=2, batch_size=32, verbose=0,
                validation_data=([x1, x2], [y1, y2]))

      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      model.train_on_batch([x1, x2], [y1, y2])
      model.train_on_batch({'input_1': x1, 'input_2': x2},
                           {'output_1': y1, 'output_2': y2})

  @test_util.run_in_graph_and_eager_modes(assert_no_eager_garbage=True)
  def test_inference_methods(self):
    # test predict, evaluate, test_on_batch, predict_on_batch
    # on different input types: list, dict
    num_classes = (2, 3)
    num_samples = 100
    input_dim = 50

    x1 = np.ones((num_samples, input_dim))
    x2 = np.ones((num_samples, input_dim))
    y1 = np.zeros((num_samples, num_classes[0]))
    y2 = np.zeros((num_samples, num_classes[1]))

    with self.test_session():
      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      model.evaluate([x1, x2], [y1, y2])
      model.test_on_batch([x1, x2], [y1, y2])

      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      model.predict([x1, x2])

      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      model.predict_on_batch([x1, x2])

  @test_util.run_in_graph_and_eager_modes()
  def test_trainable_mutation(self):
    # test that you can change `trainable` on a model or layer, and that
    # it freezes the model state during training
    # TODO(fchollet): add test after we unify BN behavior in eager and symbolic.
    pass

  @test_util.run_in_graph_and_eager_modes()
  def test_saving(self):
    if h5py is None:
      return  # Skip test if models cannot be saved.

    num_classes = (2, 3)
    num_samples = 100
    input_dim = 50

    x1 = np.ones((num_samples, input_dim))
    x2 = np.ones((num_samples, input_dim))
    y1 = np.zeros((num_samples, num_classes[0]))
    y2 = np.zeros((num_samples, num_classes[1]))

    with self.test_session():
      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      model.fit([x1, x2], [y1, y2], epochs=2, batch_size=32, verbose=0)
      y_ref_1, y_ref_2 = model.predict([x1, x2])

      fd, fname = tempfile.mkstemp('.h5')
      model.save_weights(fname)

      model = MultiIOTestModel(num_classes=num_classes, use_bn=True)
      # need to build the model before loading weights
      # (otherwise no weights to load)
      model._set_inputs([x1, x2])
      model.load_weights(fname)

      y1, y2 = model.predict([x1, x2])
      self.assertAllClose(y_ref_1, y1, atol=1e-5)
      self.assertAllClose(y_ref_2, y2, atol=1e-5)
      os.close(fd)
      os.remove(fname)

  @test_util.run_in_graph_and_eager_modes()
  def test_summary(self):

    class ToString(object):

      def __init__(self):
        self.contents = ''

      def __call__(self, msg):
        self.contents += msg + '\n'

    # Single-io
    model = SimpleTestModel(num_classes=4, use_bn=True, use_dp=True)
    model._set_inputs(np.ones((3, 4)))  # need to build model first
    print_fn = ToString()
    model.summary(print_fn=print_fn)
    self.assertTrue('Trainable params: 356' in print_fn.contents)

    # Multi-io
    model = MultiIOTestModel(num_classes=(5, 6), use_bn=True, use_dp=True)
    model._set_inputs([np.ones((3, 4)),
                       np.ones((3, 4))])  # need to build model first
    print_fn = ToString()
    model.summary(print_fn=print_fn)
    self.assertTrue('Trainable params: 587' in print_fn.contents)

  @test_util.run_in_graph_and_eager_modes()
  def test_subclass_nested_in_subclass(self):
    num_classes = 2
    num_samples = 100
    input_dim = 50

    with self.test_session():
      model = NestedTestModel1(num_classes=num_classes)
      model.compile(loss='mse',
                    optimizer=RMSPropOptimizer(learning_rate=0.001),
                    metrics=['acc'])

      x = np.ones((num_samples, input_dim))
      y = np.zeros((num_samples, num_classes))

      model.fit(x, y, epochs=2, batch_size=32, verbose=0)
      _ = model.evaluate(x, y, verbose=0)

      self.assertEqual(len(model.weights), 8 + len(model.test_net.weights))
      self.assertEqual(len(model.non_trainable_weights),
                       2 + len(model.test_net.non_trainable_weights))
      self.assertEqual(len(model.trainable_weights),
                       6 + len(model.test_net.trainable_weights))

  @test_util.run_in_graph_and_eager_modes()
  def test_graph_nested_in_subclass(self):
    num_classes = 2
    num_samples = 100
    input_dim = 50

    with self.test_session():
      model = NestedTestModel2(num_classes=num_classes)
      model.compile(loss='mse',
                    optimizer=RMSPropOptimizer(learning_rate=0.001),
                    metrics=['acc'])

      x = np.ones((num_samples, input_dim))
      y = np.zeros((num_samples, num_classes))

      model.fit(x, y, epochs=2, batch_size=32, verbose=0)
      _ = model.evaluate(x, y, verbose=0)

      self.assertEqual(len(model.weights), 8 + len(model.test_net.weights))
      self.assertEqual(len(model.non_trainable_weights),
                       2 + len(model.test_net.non_trainable_weights))
      self.assertEqual(len(model.trainable_weights),
                       6 + len(model.test_net.trainable_weights))

  @test_util.run_in_graph_and_eager_modes()
  def test_subclass_nested_in_graph(self):
    num_classes = 2
    num_samples = 100
    input_dim = 50

    with self.test_session():
      model = get_nested_model_3(input_dim=input_dim, num_classes=num_classes)
      model.compile(loss='mse',
                    optimizer=RMSPropOptimizer(learning_rate=0.001),
                    metrics=['acc'])

      x = np.ones((num_samples, input_dim))
      y = np.zeros((num_samples, num_classes))

      model.fit(x, y, epochs=2, batch_size=32, verbose=0)
      _ = model.evaluate(x, y, verbose=0)

      self.assertEqual(len(model.weights), 16)
      self.assertEqual(
          len(model.non_trainable_weights), 4)
      self.assertEqual(len(model.trainable_weights), 12)

  @test_util.run_in_graph_and_eager_modes()
  def test_support_for_manual_training_arg(self):
    # In most cases, the `training` argument is left unspecified, in which
    # case it defaults to value corresponding to the Model method being used
    # (fit -> True, predict -> False, etc).
    # If the user writes their model `call` method to take
    # an explicit `training` argument, we must check that the correct value
    # is being passed to the model for each method call.

    class DPNet(keras.Model):

      def __init__(self):
        super(DPNet, self).__init__()
        self.dp = keras.layers.Dropout(0.5)
        self.dense = keras.layers.Dense(1,
                                        use_bias=False,
                                        kernel_initializer='ones')

      def call(self, inputs, training=False):
        x = self.dp(inputs, training=training)
        return self.dense(x)

    with self.test_session():
      model = DPNet()
      x = np.ones((10, 10))
      y = model.predict(x)
      self.assertEqual(np.sum(y), np.sum(x))
      model.compile(loss='mse', optimizer=RMSPropOptimizer(learning_rate=0.001))
      loss = model.train_on_batch(x, y)
      self.assertGreater(loss, 0.1)


if __name__ == '__main__':
  test.main()
