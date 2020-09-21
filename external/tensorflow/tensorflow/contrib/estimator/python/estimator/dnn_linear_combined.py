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
"""TensorFlow estimator for Linear and DNN joined training models."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.python.estimator import estimator
from tensorflow.python.estimator.canned import dnn_linear_combined as dnn_linear_combined_lib
from tensorflow.python.ops import nn


class DNNLinearCombinedEstimator(estimator.Estimator):
  """An estimator for TensorFlow Linear and DNN joined models with custom head.

  Note: This estimator is also known as wide-n-deep.

  Example:

  ```python
  numeric_feature = numeric_column(...)
  categorical_column_a = categorical_column_with_hash_bucket(...)
  categorical_column_b = categorical_column_with_hash_bucket(...)

  categorical_feature_a_x_categorical_feature_b = crossed_column(...)
  categorical_feature_a_emb = embedding_column(
      categorical_column=categorical_feature_a, ...)
  categorical_feature_b_emb = embedding_column(
      categorical_column=categorical_feature_b, ...)

  estimator = DNNLinearCombinedEstimator(
      head=tf.contrib.estimator.multi_label_head(n_classes=3),
      # wide settings
      linear_feature_columns=[categorical_feature_a_x_categorical_feature_b],
      linear_optimizer=tf.train.FtrlOptimizer(...),
      # deep settings
      dnn_feature_columns=[
          categorical_feature_a_emb, categorical_feature_b_emb,
          numeric_feature],
      dnn_hidden_units=[1000, 500, 100],
      dnn_optimizer=tf.train.ProximalAdagradOptimizer(...))

  # To apply L1 and L2 regularization, you can set optimizers as follows:
  tf.train.ProximalAdagradOptimizer(
      learning_rate=0.1,
      l1_regularization_strength=0.001,
      l2_regularization_strength=0.001)
  # It is same for FtrlOptimizer.

  # Input builders
  def input_fn_train: # returns x, y
    pass
  estimator.train(input_fn=input_fn_train, steps=100)

  def input_fn_eval: # returns x, y
    pass
  metrics = estimator.evaluate(input_fn=input_fn_eval, steps=10)
  def input_fn_predict: # returns x, None
    pass
  predictions = estimator.predict(input_fn=input_fn_predict)
  ```

  Input of `train` and `evaluate` should have following features,
  otherwise there will be a `KeyError`:

  * for each `column` in `dnn_feature_columns` + `linear_feature_columns`:
    - if `column` is a `_CategoricalColumn`, a feature with `key=column.name`
      whose `value` is a `SparseTensor`.
    - if `column` is a `_WeightedCategoricalColumn`, two features: the first
      with `key` the id column name, the second with `key` the weight column
      name. Both features' `value` must be a `SparseTensor`.
    - if `column` is a `_DenseColumn`, a feature with `key=column.name`
      whose `value` is a `Tensor`.

  Loss is calculated by using mean squared error.

  @compatibility(eager)
  Estimators are not compatible with eager execution.
  @end_compatibility
  """

  def __init__(self,
               head,
               model_dir=None,
               linear_feature_columns=None,
               linear_optimizer='Ftrl',
               dnn_feature_columns=None,
               dnn_optimizer='Adagrad',
               dnn_hidden_units=None,
               dnn_activation_fn=nn.relu,
               dnn_dropout=None,
               input_layer_partitioner=None,
               config=None):
    """Initializes a DNNLinearCombinedEstimator instance.

    Args:
      head: A `_Head` instance constructed with a method such as
        `tf.contrib.estimator.multi_label_head`.
      model_dir: Directory to save model parameters, graph and etc. This can
        also be used to load checkpoints from the directory into a estimator
        to continue training a previously saved model.
      linear_feature_columns: An iterable containing all the feature columns
        used by linear part of the model. All items in the set must be
        instances of classes derived from `FeatureColumn`.
      linear_optimizer: An instance of `tf.Optimizer` used to apply gradients to
        the linear part of the model. Defaults to FTRL optimizer.
      dnn_feature_columns: An iterable containing all the feature columns used
        by deep part of the model. All items in the set must be instances of
        classes derived from `FeatureColumn`.
      dnn_optimizer: An instance of `tf.Optimizer` used to apply gradients to
        the deep part of the model. Defaults to Adagrad optimizer.
      dnn_hidden_units: List of hidden units per layer. All layers are fully
        connected.
      dnn_activation_fn: Activation function applied to each layer. If None,
        will use `tf.nn.relu`.
      dnn_dropout: When not None, the probability we will drop out
        a given coordinate.
      input_layer_partitioner: Partitioner for input layer. Defaults to
        `min_max_variable_partitioner` with `min_slice_size` 64 << 20.
      config: RunConfig object to configure the runtime settings.

    Raises:
      ValueError: If both linear_feature_columns and dnn_features_columns are
        empty at the same time.
    """
    linear_feature_columns = linear_feature_columns or []
    dnn_feature_columns = dnn_feature_columns or []
    self._feature_columns = (
        list(linear_feature_columns) + list(dnn_feature_columns))
    if not self._feature_columns:
      raise ValueError('Either linear_feature_columns or dnn_feature_columns '
                       'must be defined.')

    def _model_fn(features, labels, mode, config):
      return dnn_linear_combined_lib._dnn_linear_combined_model_fn(  # pylint: disable=protected-access
          features=features,
          labels=labels,
          mode=mode,
          head=head,
          linear_feature_columns=linear_feature_columns,
          linear_optimizer=linear_optimizer,
          dnn_feature_columns=dnn_feature_columns,
          dnn_optimizer=dnn_optimizer,
          dnn_hidden_units=dnn_hidden_units,
          dnn_activation_fn=dnn_activation_fn,
          dnn_dropout=dnn_dropout,
          input_layer_partitioner=input_layer_partitioner,
          config=config)

    super(DNNLinearCombinedEstimator, self).__init__(
        model_fn=_model_fn, model_dir=model_dir, config=config)
