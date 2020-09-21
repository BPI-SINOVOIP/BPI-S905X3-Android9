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
"""Tests for training routines."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import numpy as np

from tensorflow.python.framework import ops
from tensorflow.python.keras._impl import keras
from tensorflow.python.keras._impl.keras import testing_utils
from tensorflow.python.keras._impl.keras.utils.generic_utils import slice_arrays
from tensorflow.python.platform import test
from tensorflow.python.training.rmsprop import RMSPropOptimizer


class TrainingTest(test.TestCase):

  def test_fit_on_arrays(self):
    a = keras.layers.Input(shape=(3,), name='input_a')
    b = keras.layers.Input(shape=(3,), name='input_b')

    dense = keras.layers.Dense(4, name='dense')
    c = dense(a)
    d = dense(b)
    e = keras.layers.Dropout(0.5, name='dropout')(c)

    model = keras.models.Model([a, b], [d, e])

    optimizer = RMSPropOptimizer(learning_rate=0.001)
    loss = 'mse'
    loss_weights = [1., 0.5]
    metrics = ['mae']
    model.compile(optimizer, loss, metrics=metrics, loss_weights=loss_weights)

    input_a_np = np.random.random((10, 3))
    input_b_np = np.random.random((10, 3))

    output_d_np = np.random.random((10, 4))
    output_e_np = np.random.random((10, 4))

    # Test fit at different verbosity
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        epochs=1,
        batch_size=5,
        verbose=0)
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        epochs=1,
        batch_size=5,
        verbose=1)
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        epochs=2,
        batch_size=5,
        verbose=2)

    # Test with validation data
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        validation_data=([input_a_np, input_b_np], [output_d_np,
                                                    output_e_np]),
        epochs=1,
        batch_size=5,
        verbose=0)
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        validation_data=([input_a_np, input_b_np], [output_d_np,
                                                    output_e_np]),
        epochs=2,
        batch_size=5,
        verbose=1)
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        validation_data=([input_a_np, input_b_np], [output_d_np,
                                                    output_e_np]),
        epochs=2,
        batch_size=5,
        verbose=2)
    model.train_on_batch([input_a_np, input_b_np], [output_d_np, output_e_np])

  # Test with validation split
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        epochs=2,
        batch_size=5,
        verbose=0,
        validation_split=0.2)

    # Test with dictionary inputs
    model.fit(
        {
            'input_a': input_a_np,
            'input_b': input_b_np
        }, {'dense': output_d_np,
            'dropout': output_e_np},
        epochs=1,
        batch_size=5,
        verbose=0)
    model.fit(
        {
            'input_a': input_a_np,
            'input_b': input_b_np
        }, {'dense': output_d_np,
            'dropout': output_e_np},
        epochs=1,
        batch_size=5,
        verbose=1)
    model.fit(
        {
            'input_a': input_a_np,
            'input_b': input_b_np
        }, {'dense': output_d_np,
            'dropout': output_e_np},
        validation_data=({'input_a': input_a_np,
                          'input_b': input_b_np
                         },
                         {
                             'dense': output_d_np,
                             'dropout': output_e_np
                         }),
        epochs=1,
        batch_size=5,
        verbose=0)
    model.train_on_batch({
        'input_a': input_a_np,
        'input_b': input_b_np
    }, {'dense': output_d_np,
        'dropout': output_e_np})
    # Test with lists for loss, metrics
    loss = ['mae', 'mse']
    metrics = ['acc', 'mae']
    model.compile(optimizer, loss, metrics=metrics)
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        epochs=1,
        batch_size=5,
        verbose=0)

    # Test with dictionaries for loss, metrics, loss weights
    loss = {'dense': 'mse', 'dropout': 'mae'}
    loss_weights = {'dense': 1., 'dropout': 0.5}
    metrics = {'dense': 'mse', 'dropout': 'mae'}
    model.compile(optimizer, loss, metrics=metrics, loss_weights=loss_weights)
    model.fit(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        epochs=1,
        batch_size=5,
        verbose=0)

    # Invalid use cases
    with self.assertRaises(AttributeError):
      model.fit(
          [input_a_np, input_b_np], [output_d_np, output_e_np],
          epochs=1,
          validation_data=([input_a_np, input_b_np], 0, 0),
          verbose=0)
    with self.assertRaises(ValueError):
      model.train_on_batch({'input_a': input_a_np},
                           [output_d_np, output_e_np])
    with self.assertRaises(ValueError):
      model.train_on_batch([input_a_np], [output_d_np, output_e_np])
    with self.assertRaises(AttributeError):
      model.train_on_batch(1, [output_d_np, output_e_np])
    with self.assertRaises(ValueError):
      model.train_on_batch(input_a_np, [output_d_np, output_e_np])
    with self.assertRaises(ValueError):
      bad_input = np.random.random((11, 3))
      model.train_on_batch([bad_input, input_b_np],
                           [output_d_np, output_e_np])
    with self.assertRaises(ValueError):
      bad_target = np.random.random((11, 4))
      model.train_on_batch([input_a_np, input_b_np],
                           [bad_target, output_e_np])

    # Build single-input model
    x = keras.layers.Input(shape=(3,), name='input_a')
    y = keras.layers.Dense(4)(x)
    model = keras.models.Model(x, y)
    model.compile(optimizer=RMSPropOptimizer(learning_rate=0.001), loss='mse')
    # This will work
    model.fit([input_a_np], output_d_np, epochs=1)
    with self.assertRaises(ValueError):
      model.fit([input_a_np, input_a_np], output_d_np, epochs=1)

  def test_evaluate_predict_on_arrays(self):
    a = keras.layers.Input(shape=(3,), name='input_a')
    b = keras.layers.Input(shape=(3,), name='input_b')

    dense = keras.layers.Dense(4, name='dense')
    c = dense(a)
    d = dense(b)
    e = keras.layers.Dropout(0.5, name='dropout')(c)

    model = keras.models.Model([a, b], [d, e])

    optimizer = RMSPropOptimizer(learning_rate=0.001)
    loss = 'mse'
    loss_weights = [1., 0.5]
    metrics = ['mae']
    model.compile(
        optimizer,
        loss,
        metrics=metrics,
        loss_weights=loss_weights,
        sample_weight_mode=None)

    input_a_np = np.random.random((10, 3))
    input_b_np = np.random.random((10, 3))

    output_d_np = np.random.random((10, 4))
    output_e_np = np.random.random((10, 4))

    # Test evaluate at different verbosity
    out = model.evaluate(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        batch_size=5,
        verbose=0)
    self.assertEqual(len(out), 5)
    out = model.evaluate(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        batch_size=5,
        verbose=1)
    self.assertEqual(len(out), 5)
    out = model.evaluate(
        [input_a_np, input_b_np], [output_d_np, output_e_np],
        batch_size=5,
        verbose=2)
    self.assertEqual(len(out), 5)
    out = model.test_on_batch([input_a_np, input_b_np],
                              [output_d_np, output_e_np])
    self.assertEqual(len(out), 5)

    # Test evaluate with dictionary inputs
    model.evaluate(
        {
            'input_a': input_a_np,
            'input_b': input_b_np
        }, {'dense': output_d_np,
            'dropout': output_e_np},
        batch_size=5,
        verbose=0)
    model.evaluate(
        {
            'input_a': input_a_np,
            'input_b': input_b_np
        }, {'dense': output_d_np,
            'dropout': output_e_np},
        batch_size=5,
        verbose=1)

    # Test predict
    out = model.predict([input_a_np, input_b_np], batch_size=5)
    self.assertEqual(len(out), 2)
    out = model.predict({'input_a': input_a_np, 'input_b': input_b_np})
    self.assertEqual(len(out), 2)
    out = model.predict_on_batch({
        'input_a': input_a_np,
        'input_b': input_b_np
    })
    self.assertEqual(len(out), 2)

  def test_invalid_loss_or_metrics(self):
    num_classes = 5
    train_samples = 1000
    test_samples = 1000
    input_dim = 5

    model = keras.models.Sequential()
    model.add(keras.layers.Dense(10, input_shape=(input_dim,)))
    model.add(keras.layers.Activation('relu'))
    model.add(keras.layers.Dense(num_classes))
    model.add(keras.layers.Activation('softmax'))
    model.compile(loss='categorical_crossentropy',
                  optimizer=RMSPropOptimizer(learning_rate=0.001))
    np.random.seed(1337)

    (x_train, y_train), (_, _) = testing_utils.get_test_data(
        train_samples=train_samples,
        test_samples=test_samples,
        input_shape=(input_dim,),
        num_classes=num_classes)

    with self.assertRaises(ValueError):
      model.fit(x_train, np.concatenate([y_train, y_train], axis=-1))

    with self.assertRaises(TypeError):
      model.compile(loss='categorical_crossentropy',
                    optimizer=RMSPropOptimizer(learning_rate=0.001),
                    metrics=set(0))

    with self.assertRaises(ValueError):
      model.compile(loss=None,
                    optimizer='rms')


class LossWeightingTest(test.TestCase):

  def test_class_weights(self):
    num_classes = 5
    batch_size = 5
    epochs = 5
    weighted_class = 3
    train_samples = 3000
    test_samples = 3000
    input_dim = 5

    model = keras.models.Sequential()
    model.add(keras.layers.Dense(10, input_shape=(input_dim,)))
    model.add(keras.layers.Activation('relu'))
    model.add(keras.layers.Dense(num_classes))
    model.add(keras.layers.Activation('softmax'))
    model.compile(loss='categorical_crossentropy',
                  optimizer=RMSPropOptimizer(learning_rate=0.001))

    np.random.seed(1337)
    (x_train, y_train), (x_test, y_test) = testing_utils.get_test_data(
        train_samples=train_samples,
        test_samples=test_samples,
        input_shape=(input_dim,),
        num_classes=num_classes)
    int_y_test = y_test.copy()
    int_y_train = y_train.copy()
    # convert class vectors to binary class matrices
    y_train = keras.utils.to_categorical(y_train, num_classes)
    y_test = keras.utils.to_categorical(y_test, num_classes)
    test_ids = np.where(int_y_test == np.array(weighted_class))[0]

    class_weight = dict([(i, 1.) for i in range(num_classes)])
    class_weight[weighted_class] = 2.

    sample_weight = np.ones((y_train.shape[0]))
    sample_weight[int_y_train == weighted_class] = 2.

    model.fit(
        x_train,
        y_train,
        batch_size=batch_size,
        epochs=epochs // 3,
        verbose=0,
        class_weight=class_weight,
        validation_data=(x_train, y_train, sample_weight))
    model.fit(
        x_train,
        y_train,
        batch_size=batch_size,
        epochs=epochs // 2,
        verbose=0,
        class_weight=class_weight)
    model.fit(
        x_train,
        y_train,
        batch_size=batch_size,
        epochs=epochs // 2,
        verbose=0,
        class_weight=class_weight,
        validation_split=0.1)

    model.train_on_batch(
        x_train[:batch_size], y_train[:batch_size], class_weight=class_weight)
    ref_score = model.evaluate(x_test, y_test, verbose=0)
    score = model.evaluate(
        x_test[test_ids, :], y_test[test_ids, :], verbose=0)
    self.assertLess(score, ref_score)

  def test_sample_weights(self):
    num_classes = 5
    batch_size = 5
    epochs = 5
    weighted_class = 3
    train_samples = 3000
    test_samples = 3000
    input_dim = 5

    model = keras.models.Sequential()
    model.add(keras.layers.Dense(10, input_shape=(input_dim,)))
    model.add(keras.layers.Activation('relu'))
    model.add(keras.layers.Dense(num_classes))
    model.add(keras.layers.Activation('softmax'))
    model.compile(loss='categorical_crossentropy',
                  optimizer=RMSPropOptimizer(learning_rate=0.001))

    np.random.seed(43)
    (x_train, y_train), (x_test, y_test) = testing_utils.get_test_data(
        train_samples=train_samples,
        test_samples=test_samples,
        input_shape=(input_dim,),
        num_classes=num_classes)
    int_y_test = y_test.copy()
    int_y_train = y_train.copy()
    # convert class vectors to binary class matrices
    y_train = keras.utils.to_categorical(y_train, num_classes)
    y_test = keras.utils.to_categorical(y_test, num_classes)
    test_ids = np.where(int_y_test == np.array(weighted_class))[0]

    class_weight = dict([(i, 1.) for i in range(num_classes)])
    class_weight[weighted_class] = 2.

    sample_weight = np.ones((y_train.shape[0]))
    sample_weight[int_y_train == weighted_class] = 2.

    model.fit(
        x_train,
        y_train,
        batch_size=batch_size,
        epochs=epochs // 3,
        verbose=0,
        sample_weight=sample_weight)
    model.fit(
        x_train,
        y_train,
        batch_size=batch_size,
        epochs=epochs // 3,
        verbose=0,
        sample_weight=sample_weight,
        validation_split=0.1)
    model.train_on_batch(
        x_train[:batch_size],
        y_train[:batch_size],
        sample_weight=sample_weight[:batch_size])
    model.test_on_batch(
        x_train[:batch_size],
        y_train[:batch_size],
        sample_weight=sample_weight[:batch_size])

  def test_temporal_sample_weights(self):
    num_classes = 5
    weighted_class = 3
    train_samples = 1000
    test_samples = 1000
    input_dim = 5
    timesteps = 3

    model = keras.models.Sequential()
    model.add(
        keras.layers.TimeDistributed(
            keras.layers.Dense(num_classes),
            input_shape=(timesteps, input_dim)))
    model.add(keras.layers.Activation('softmax'))

    np.random.seed(1337)
    (_, y_train), _ = testing_utils.get_test_data(
        train_samples=train_samples,
        test_samples=test_samples,
        input_shape=(input_dim,),
        num_classes=num_classes)
    int_y_train = y_train.copy()
    # convert class vectors to binary class matrices
    y_train = keras.utils.to_categorical(y_train, num_classes)

    class_weight = dict([(i, 1.) for i in range(num_classes)])
    class_weight[weighted_class] = 2.

    sample_weight = np.ones((y_train.shape[0]))
    sample_weight[int_y_train == weighted_class] = 2.
    with self.assertRaises(ValueError):
      model.compile(
          loss='binary_crossentropy',
          optimizer=RMSPropOptimizer(learning_rate=0.001),
          sample_weight_mode='temporal')

  def test_class_weight_invalid_use_case(self):
    num_classes = 5
    train_samples = 1000
    test_samples = 1000
    input_dim = 5
    timesteps = 3

    model = keras.models.Sequential()
    model.add(
        keras.layers.TimeDistributed(
            keras.layers.Dense(num_classes),
            input_shape=(timesteps, input_dim)))
    model.add(keras.layers.Activation('softmax'))
    model.compile(
        loss='binary_crossentropy',
        optimizer=RMSPropOptimizer(learning_rate=0.001))

    (x_train, y_train), _ = testing_utils.get_test_data(
        train_samples=train_samples,
        test_samples=test_samples,
        input_shape=(input_dim,),
        num_classes=num_classes)
    # convert class vectors to binary class matrices
    y_train = keras.utils.to_categorical(y_train, num_classes)
    class_weight = dict([(i, 1.) for i in range(num_classes)])

    del class_weight[1]
    with self.assertRaises(ValueError):
      model.fit(x_train, y_train,
                epochs=0, verbose=0, class_weight=class_weight)

    with self.assertRaises(ValueError):
      model.compile(
          loss='binary_crossentropy',
          optimizer=RMSPropOptimizer(learning_rate=0.001),
          sample_weight_mode=[])

    # Build multi-output model
    x = keras.Input((3,))
    y1 = keras.layers.Dense(4, name='1')(x)
    y2 = keras.layers.Dense(4, name='2')(x)
    model = keras.models.Model(x, [y1, y2])
    model.compile(optimizer=RMSPropOptimizer(learning_rate=0.001), loss='mse')
    x_np = np.random.random((10, 3))
    y_np = np.random.random((10, 4))
    w_np = np.random.random((10,))
    # This will work
    model.fit(x_np, [y_np, y_np], epochs=1, sample_weight={'1': w_np})
    # These will not
    with self.assertRaises(ValueError):
      model.fit(x_np, [y_np, y_np], epochs=1, sample_weight=[w_np])
    with self.assertRaises(TypeError):
      model.fit(x_np, [y_np, y_np], epochs=1, sample_weight=w_np)
    with self.assertRaises(ValueError):
      bad_w_np = np.random.random((11,))
      model.fit(x_np, [y_np, y_np], epochs=1, sample_weight={'1': bad_w_np})
    with self.assertRaises(ValueError):
      bad_w_np = np.random.random((10, 2))
      model.fit(x_np, [y_np, y_np], epochs=1, sample_weight={'1': bad_w_np})
    with self.assertRaises(ValueError):
      bad_w_np = np.random.random((10, 2, 2))
      model.fit(x_np, [y_np, y_np], epochs=1, sample_weight={'1': bad_w_np})


class TestDynamicTrainability(test.TestCase):

  def test_trainable_warning(self):
    x = np.random.random((5, 3))
    y = np.random.random((5, 2))
    model = keras.models.Sequential()
    model.add(keras.layers.Dense(2, input_dim=3))
    model.trainable = False
    model.compile(RMSPropOptimizer(learning_rate=0.001), 'mse')
    model.trainable = True
    with self.assertRaises(ValueError):
      model.train_on_batch(x, y)

  def test_trainable_argument(self):
    x = np.random.random((5, 3))
    y = np.random.random((5, 2))

    model = keras.models.Sequential()
    model.add(keras.layers.Dense(2, input_dim=3, trainable=False))
    model.compile(RMSPropOptimizer(learning_rate=0.001), 'mse')
    out = model.predict(x)
    with self.assertRaises(ValueError):
      model.train_on_batch(x, y)
    out_2 = model.predict(x)
    self.assertAllClose(out, out_2)

    # test with nesting
    inputs = keras.layers.Input(shape=(3,))
    output = model(inputs)
    model = keras.models.Model(inputs, output)
    model.compile(RMSPropOptimizer(learning_rate=0.001), 'mse')
    out = model.predict(x)
    with self.assertRaises(ValueError):
      model.train_on_batch(x, y)
    out_2 = model.predict(x)
    self.assertAllClose(out, out_2)

  def test_layer_trainability_switch(self):
    # with constructor argument, in Sequential
    model = keras.models.Sequential()
    model.add(keras.layers.Dense(2, trainable=False, input_dim=1))
    self.assertListEqual(model.trainable_weights, [])

    # by setting the `trainable` argument, in Sequential
    model = keras.models.Sequential()
    layer = keras.layers.Dense(2, input_dim=1)
    model.add(layer)
    self.assertListEqual(model.trainable_weights, layer.trainable_weights)
    layer.trainable = False
    self.assertListEqual(model.trainable_weights, [])

    # with constructor argument, in Model
    x = keras.layers.Input(shape=(1,))
    y = keras.layers.Dense(2, trainable=False)(x)
    model = keras.models.Model(x, y)
    self.assertListEqual(model.trainable_weights, [])

    # by setting the `trainable` argument, in Model
    x = keras.layers.Input(shape=(1,))
    layer = keras.layers.Dense(2)
    y = layer(x)
    model = keras.models.Model(x, y)
    self.assertListEqual(model.trainable_weights, layer.trainable_weights)
    layer.trainable = False
    self.assertListEqual(model.trainable_weights, [])

  def test_model_trainability_switch(self):
    # a non-trainable model has no trainable weights
    x = keras.layers.Input(shape=(1,))
    y = keras.layers.Dense(2)(x)
    model = keras.models.Model(x, y)
    model.trainable = False
    self.assertListEqual(model.trainable_weights, [])

    # same for Sequential
    model = keras.models.Sequential()
    model.add(keras.layers.Dense(2, input_dim=1))
    model.trainable = False
    self.assertListEqual(model.trainable_weights, [])

  def test_nested_model_trainability(self):

    # a Sequential inside a Model
    inner_model = keras.models.Sequential()
    inner_model.add(keras.layers.Dense(2, input_dim=1))

    x = keras.layers.Input(shape=(1,))
    y = inner_model(x)
    outer_model = keras.models.Model(x, y)
    self.assertListEqual(outer_model.trainable_weights,
                         inner_model.trainable_weights)
    inner_model.trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])
    inner_model.trainable = True
    inner_model.layers[-1].trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])

    # a Sequential inside a Sequential
    inner_model = keras.models.Sequential()
    inner_model.add(keras.layers.Dense(2, input_dim=1))
    outer_model = keras.models.Sequential()
    outer_model.add(inner_model)
    self.assertListEqual(outer_model.trainable_weights,
                         inner_model.trainable_weights)
    inner_model.trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])
    inner_model.trainable = True
    inner_model.layers[-1].trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])

    # a Model inside a Model
    x = keras.layers.Input(shape=(1,))
    y = keras.layers.Dense(2)(x)
    inner_model = keras.models.Model(x, y)
    x = keras.layers.Input(shape=(1,))
    y = inner_model(x)
    outer_model = keras.models.Model(x, y)
    self.assertListEqual(outer_model.trainable_weights,
                         inner_model.trainable_weights)
    inner_model.trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])
    inner_model.trainable = True
    inner_model.layers[-1].trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])

    # a Model inside a Sequential
    x = keras.layers.Input(shape=(1,))
    y = keras.layers.Dense(2)(x)
    inner_model = keras.models.Model(x, y)
    outer_model = keras.models.Sequential()
    outer_model.add(inner_model)
    self.assertListEqual(outer_model.trainable_weights,
                         inner_model.trainable_weights)
    inner_model.trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])
    inner_model.trainable = True
    inner_model.layers[-1].trainable = False
    self.assertListEqual(outer_model.trainable_weights, [])


class TestTrainingUtils(test.TestCase):

  def test_check_array_lengths(self):
    keras.engine.training._check_array_lengths(None, None, None)
    a_np = np.random.random((4, 3, 3))
    keras.engine.training._check_array_lengths(a_np, a_np, a_np)
    keras.engine.training._check_array_lengths(
        [a_np, a_np], [a_np, a_np], [a_np, a_np])
    keras.engine.training._check_array_lengths([None], [None], [None])

    b_np = np.random.random((3, 4))
    with self.assertRaises(ValueError):
      keras.engine.training._check_array_lengths(a_np, None, None)
    with self.assertRaises(ValueError):
      keras.engine.training._check_array_lengths(a_np, a_np, None)
    with self.assertRaises(ValueError):
      keras.engine.training._check_array_lengths([a_np], [None], None)
    with self.assertRaises(ValueError):
      keras.engine.training._check_array_lengths([a_np], [b_np], None)
    with self.assertRaises(ValueError):
      keras.engine.training._check_array_lengths([a_np], None, [b_np])

  def test_slice_arrays(self):
    input_a = np.random.random((10, 3))
    slice_arrays(None)
    slice_arrays(input_a, 0)
    slice_arrays(input_a, 0, 1)
    slice_arrays(input_a, stop=2)
    input_a = [None, [1, 1], None, [1, 1]]
    slice_arrays(input_a, 0)
    slice_arrays(input_a, 0, 1)
    slice_arrays(input_a, stop=2)
    input_a = [None]
    slice_arrays(input_a, 0)
    slice_arrays(input_a, 0, 1)
    slice_arrays(input_a, stop=2)
    input_a = None
    slice_arrays(input_a, 0)
    slice_arrays(input_a, 0, 1)
    slice_arrays(input_a, stop=2)

  def test_fit_with_BatchNorm(self):
    model = keras.models.Sequential()
    model.add(keras.layers.Dense(10, input_dim=4))
    model.add(keras.layers.BatchNormalization())
    model.add(keras.layers.Activation('tanh'))
    model.add(keras.layers.Dropout(0.2))

    input_a_np = np.random.random((10, 4))
    output_b_np = np.random.random((10, 10))

    model.compile(loss='binary_crossentropy', optimizer=RMSPropOptimizer(0.001))
    model.fit(input_a_np, output_b_np, epochs=1, batch_size=5, verbose=0)

  def test_fit_with_regularization(self):
    model = keras.models.Sequential()
    with self.assertRaises(ValueError):
      model.add(
          keras.layers.Dense(4, input_dim=3,
                             kernel_regularizer=keras.regularizers.l2(0.01),
                             activity_regularizer=keras.regularizers.l1(0.01)))


if __name__ == '__main__':
  # Bazel sets these environment variables to very long paths.
  # Tempfile uses them to create long paths, and in turn multiprocessing
  # library tries to create sockets named after paths. Delete whatever bazel
  # writes to these to avoid tests failing due to socket addresses being too
  # long.
  for var in ('TMPDIR', 'TMP', 'TEMP'):
    if var in os.environ:
      del os.environ[var]

  ops.enable_eager_execution()
  test.main()
