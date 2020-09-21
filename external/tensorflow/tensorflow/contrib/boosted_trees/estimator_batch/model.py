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
"""GTFlow Model definitions."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import copy

from tensorflow.contrib.boosted_trees.estimator_batch import trainer_hooks
from tensorflow.contrib.boosted_trees.python.ops import model_ops
from tensorflow.contrib.boosted_trees.python.training.functions import gbdt_batch
from tensorflow.python.framework import ops
from tensorflow.python.ops import state_ops
from tensorflow.python.training import training_util


def model_builder(features, labels, mode, params, config):
  """Multi-machine batch gradient descent tree model.

  Args:
    features: `Tensor` or `dict` of `Tensor` objects.
    labels: Labels used to train on.
    mode: Mode we are in. (TRAIN/EVAL/INFER)
    params: A dict of hyperparameters.
      The following hyperparameters are expected:
      * head: A `Head` instance.
      * learner_config: A config for the learner.
      * feature_columns: An iterable containing all the feature columns used by
          the model.
      * examples_per_layer: Number of examples to accumulate before growing a
          layer. It can also be a function that computes the number of examples
          based on the depth of the layer that's being built.
      * weight_column_name: The name of weight column.
      * center_bias: Whether a separate tree should be created for first fitting
          the bias.
    config: `RunConfig` of the estimator.

  Returns:
    A `ModelFnOps` object.
  Raises:
    ValueError: if inputs are not valid.
  """
  head = params["head"]
  learner_config = params["learner_config"]
  examples_per_layer = params["examples_per_layer"]
  feature_columns = params["feature_columns"]
  weight_column_name = params["weight_column_name"]
  num_trees = params["num_trees"]
  logits_modifier_function = params["logits_modifier_function"]
  if features is None:
    raise ValueError("At least one feature must be specified.")

  if config is None:
    raise ValueError("Missing estimator RunConfig.")

  center_bias = params["center_bias"]

  if isinstance(features, ops.Tensor):
    features = {features.name: features}

  # Make a shallow copy of features to ensure downstream usage
  # is unaffected by modifications in the model function.
  training_features = copy.copy(features)
  training_features.pop(weight_column_name, None)
  global_step = training_util.get_global_step()
  with ops.device(global_step.device):
    ensemble_handle = model_ops.tree_ensemble_variable(
        stamp_token=0,
        tree_ensemble_config="",  # Initialize an empty ensemble.
        name="ensemble_model")

  # Create GBDT model.
  gbdt_model = gbdt_batch.GradientBoostedDecisionTreeModel(
      is_chief=config.is_chief,
      num_ps_replicas=config.num_ps_replicas,
      ensemble_handle=ensemble_handle,
      center_bias=center_bias,
      examples_per_layer=examples_per_layer,
      learner_config=learner_config,
      feature_columns=feature_columns,
      logits_dimension=head.logits_dimension,
      features=training_features)
  with ops.name_scope("gbdt", "gbdt_optimizer"):
    predictions_dict = gbdt_model.predict(mode)
    logits = predictions_dict["predictions"]
    if logits_modifier_function:
      logits = logits_modifier_function(logits, features, mode)

    def _train_op_fn(loss):
      """Returns the op to optimize the loss."""
      update_op = gbdt_model.train(loss, predictions_dict, labels)
      with ops.control_dependencies(
          [update_op]), (ops.colocate_with(global_step)):
        update_op = state_ops.assign_add(global_step, 1).op
        return update_op

  model_fn_ops = head.create_model_fn_ops(
      features=features,
      mode=mode,
      labels=labels,
      train_op_fn=_train_op_fn,
      logits=logits)
  if num_trees:
    if center_bias:
      num_trees += 1
    finalized_trees, attempted_trees = gbdt_model.get_number_of_trees_tensor()
    model_fn_ops.training_hooks.append(
        trainer_hooks.StopAfterNTrees(num_trees, attempted_trees,
                                      finalized_trees))
  return model_fn_ops
