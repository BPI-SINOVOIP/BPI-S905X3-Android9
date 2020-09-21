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

"""Base Estimator class."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import copy
import os
import tempfile

import numpy as np
import six

from google.protobuf import message
from tensorflow.core.framework import summary_pb2
from tensorflow.core.protobuf import config_pb2
from tensorflow.python.client import session as tf_session
from tensorflow.python.data.ops import dataset_ops
from tensorflow.python.eager import context
from tensorflow.python.estimator import model_fn as model_fn_lib
from tensorflow.python.estimator import run_config
from tensorflow.python.estimator import util
from tensorflow.python.estimator import warm_starting_util
from tensorflow.python.estimator.export.export import build_all_signature_defs
from tensorflow.python.estimator.export.export import get_temp_export_dir
from tensorflow.python.estimator.export.export import get_timestamped_export_dir
from tensorflow.python.framework import ops
from tensorflow.python.framework import random_seed
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import metrics as metrics_lib
from tensorflow.python.platform import gfile
from tensorflow.python.platform import tf_logging as logging
from tensorflow.python.saved_model import builder as saved_model_builder
from tensorflow.python.saved_model import tag_constants
from tensorflow.python.summary import summary
from tensorflow.python.summary.writer import writer_cache
from tensorflow.python.training import evaluation
from tensorflow.python.training import monitored_session
from tensorflow.python.training import saver
from tensorflow.python.training import training
from tensorflow.python.training import training_util
from tensorflow.python.util import compat
from tensorflow.python.util import compat_internal
from tensorflow.python.util import nest
from tensorflow.python.util.tf_export import tf_export


_VALID_MODEL_FN_ARGS = set(
    ['features', 'labels', 'mode', 'params', 'self', 'config'])


@tf_export('estimator.Estimator')
class Estimator(object):
  """Estimator class to train and evaluate TensorFlow models.

  The `Estimator` object wraps a model which is specified by a `model_fn`,
  which, given inputs and a number of other parameters, returns the ops
  necessary to perform training, evaluation, or predictions.

  All outputs (checkpoints, event files, etc.) are written to `model_dir`, or a
  subdirectory thereof. If `model_dir` is not set, a temporary directory is
  used.

  The `config` argument can be passed `RunConfig` object containing information
  about the execution environment. It is passed on to the `model_fn`, if the
  `model_fn` has a parameter named "config" (and input functions in the same
  manner). If the `config` parameter is not passed, it is instantiated by the
  `Estimator`. Not passing config means that defaults useful for local execution
  are used. `Estimator` makes config available to the model (for instance, to
  allow specialization based on the number of workers available), and also uses
  some of its fields to control internals, especially regarding checkpointing.

  The `params` argument contains hyperparameters. It is passed to the
  `model_fn`, if the `model_fn` has a parameter named "params", and to the input
  functions in the same manner. `Estimator` only passes params along, it does
  not inspect it. The structure of `params` is therefore entirely up to the
  developer.

  None of `Estimator`'s methods can be overridden in subclasses (its
  constructor enforces this). Subclasses should use `model_fn` to configure
  the base class, and may add methods implementing specialized functionality.

  @compatibility(eager)
  Estimators are not compatible with eager execution.
  @end_compatibility
  """

  def __init__(self, model_fn, model_dir=None, config=None, params=None,
               warm_start_from=None):
    """Constructs an `Estimator` instance.

    See @{$estimators} for more information. To warm-start an `Estimator`:

    ```python
    estimator = tf.estimator.DNNClassifier(
        feature_columns=[categorical_feature_a_emb, categorical_feature_b_emb],
        hidden_units=[1024, 512, 256],
        warm_start_from="/path/to/checkpoint/dir")
    ```

    For more details on warm-start configuration, see
    @{tf.estimator.WarmStartSettings$WarmStartSettings}.

    Args:
      model_fn: Model function. Follows the signature:

        * Args:

          * `features`: This is the first item returned from the `input_fn`
                 passed to `train`, `evaluate`, and `predict`. This should be a
                 single `Tensor` or `dict` of same.
          * `labels`: This is the second item returned from the `input_fn`
                 passed to `train`, `evaluate`, and `predict`. This should be a
                 single `Tensor` or `dict` of same (for multi-head models). If
                 mode is `ModeKeys.PREDICT`, `labels=None` will be passed. If
                 the `model_fn`'s signature does not accept `mode`, the
                 `model_fn` must still be able to handle `labels=None`.
          * `mode`: Optional. Specifies if this training, evaluation or
                 prediction. See `ModeKeys`.
          * `params`: Optional `dict` of hyperparameters.  Will receive what
                 is passed to Estimator in `params` parameter. This allows
                 to configure Estimators from hyper parameter tuning.
          * `config`: Optional configuration object. Will receive what is passed
                 to Estimator in `config` parameter, or the default `config`.
                 Allows updating things in your model_fn based on configuration
                 such as `num_ps_replicas`, or `model_dir`.

        * Returns:
          `EstimatorSpec`

      model_dir: Directory to save model parameters, graph and etc. This can
        also be used to load checkpoints from the directory into a estimator to
        continue training a previously saved model. If `PathLike` object, the
        path will be resolved. If `None`, the model_dir in `config` will be used
        if set. If both are set, they must be same. If both are `None`, a
        temporary directory will be used.
      config: Configuration object.
      params: `dict` of hyper parameters that will be passed into `model_fn`.
              Keys are names of parameters, values are basic python types.
      warm_start_from: Optional string filepath to a checkpoint to warm-start
                       from, or a `tf.estimator.WarmStartSettings` object to
                       fully configure warm-starting.  If the string filepath is
                       provided instead of a `WarmStartSettings`, then all
                       variables are warm-started, and it is assumed that
                       vocabularies and Tensor names are unchanged.

    Raises:
      RuntimeError: If eager execution is enabled.
      ValueError: parameters of `model_fn` don't match `params`.
      ValueError: if this is called via a subclass and if that class overrides
        a member of `Estimator`.
    """
    if context.in_eager_mode():
      raise RuntimeError(
          'Estimators are not supported when eager execution is enabled.')

    Estimator._assert_members_are_not_overridden(self)

    if config is None:
      self._config = run_config.RunConfig()
      logging.info('Using default config.')
    else:
      if not isinstance(config, run_config.RunConfig):
        raise ValueError(
            'config must be an instance of RunConfig, but provided %s.' %
            config)
      self._config = config

    # Model directory.
    model_dir = compat_internal.path_to_str(model_dir)
    if (model_dir is not None) and (self._config.model_dir is not None):
      if model_dir != self._config.model_dir:
        # TODO(alanyee): remove this suppression after it is no longer needed
        # pylint: disable=g-doc-exception
        raise ValueError(
            "model_dir are set both in constructor and RunConfig, but with "
            "different values. In constructor: '{}', in RunConfig: "
            "'{}' ".format(model_dir, self._config.model_dir))
        # pylint: enable=g-doc-exception

    self._model_dir = model_dir or self._config.model_dir
    if self._model_dir is None:
      self._model_dir = tempfile.mkdtemp()
      logging.warning('Using temporary folder as model directory: %s',
                      self._model_dir)
    if self._config.model_dir is None:
      self._config = self._config.replace(model_dir=self._model_dir)
    logging.info('Using config: %s', str(vars(self._config)))

    if self._config.session_config is None:
      self._session_config = config_pb2.ConfigProto(allow_soft_placement=True)
    else:
      self._session_config = self._config.session_config

    self._device_fn = _get_replica_device_setter(self._config)

    if model_fn is None:
      raise ValueError('model_fn must be provided to Estimator.')
    _verify_model_fn_args(model_fn, params)
    self._model_fn = model_fn
    self._params = copy.deepcopy(params or {})

    # pylint: disable=protected-access
    self._warm_start_settings = (
        warm_starting_util._get_default_warm_start_settings(warm_start_from))
    # pylint: enable=protected-access

  @property
  def model_dir(self):
    return self._model_dir

  @property
  def config(self):
    return copy.deepcopy(self._config)

  @property
  def params(self):
    return copy.deepcopy(self._params)

  @property
  def model_fn(self):
    """Returns the model_fn which is bound to self.params.

    Returns:
      The model_fn with following signature:
        `def model_fn(features, labels, mode, config)`
    """

    def public_model_fn(features, labels, mode, config):
      return self._call_model_fn(features, labels, mode, config)

    return public_model_fn

  # TODO(ispir): support a list of names
  def get_variable_value(self, name):
    """Returns value of the variable given by name.

    Args:
      name: string or a list of string, name of the tensor.

    Returns:
      Numpy array - value of the tensor.

    Raises:
      ValueError: If the Estimator has not produced a checkpoint yet.
    """
    _check_checkpoint_available(self.model_dir)
    return training.load_variable(self.model_dir, name)

  def get_variable_names(self):
    """Returns list of all variable names in this model.

    Returns:
      List of names.

    Raises:
      ValueError: If the Estimator has not produced a checkpoint yet.
    """
    _check_checkpoint_available(self.model_dir)
    return [name for name, _ in training.list_variables(self.model_dir)]

  def latest_checkpoint(self):
    """Finds the filename of latest saved checkpoint file in `model_dir`.

    Returns:
      The full path to the latest checkpoint or `None` if no checkpoint was
      found.
    """
    return saver.latest_checkpoint(self.model_dir)

  def train(self,
            input_fn,
            hooks=None,
            steps=None,
            max_steps=None,
            saving_listeners=None):
    """Trains a model given training data input_fn.

    Args:
      input_fn: A function that provides input data for training as minibatches.
        See @{$get_started/premade_estimators#create_input_functions} for more
        information. The function should construct and return one of
        the following:

          * A 'tf.data.Dataset' object: Outputs of `Dataset` object must be a
            tuple (features, labels) with same constraints as below.
          * A tuple (features, labels): Where features is a `Tensor` or a
            dictionary of string feature name to `Tensor` and labels is a
            `Tensor` or a dictionary of string label name to `Tensor`. Both
            features and labels are consumed by `model_fn`. They should satisfy
            the expectation of `model_fn` from inputs.

      hooks: List of `SessionRunHook` subclass instances. Used for callbacks
        inside the training loop.
      steps: Number of steps for which to train model. If `None`, train forever
        or train until input_fn generates the `OutOfRange` error or
        `StopIteration` exception. 'steps' works incrementally. If you call two
        times train(steps=10) then training occurs in total 20 steps. If
        `OutOfRange` or `StopIteration` occurs in the middle, training stops
        before 20 steps. If you don't want to have incremental behavior please
        set `max_steps` instead. If set, `max_steps` must be `None`.
      max_steps: Number of total steps for which to train model. If `None`,
        train forever or train until input_fn generates the `OutOfRange` error
        or `StopIteration` exception. If set, `steps` must be `None`. If
        `OutOfRange` or `StopIteration` occurs in the middle, training stops
        before `max_steps` steps.
        Two calls to `train(steps=100)` means 200 training
        iterations. On the other hand, two calls to `train(max_steps=100)` means
        that the second call will not do any iteration since first call did
        all 100 steps.
      saving_listeners: list of `CheckpointSaverListener` objects. Used for
        callbacks that run immediately before or after checkpoint savings.

    Returns:
      `self`, for chaining.

    Raises:
      ValueError: If both `steps` and `max_steps` are not `None`.
      ValueError: If either `steps` or `max_steps` is <= 0.
    """
    if (steps is not None) and (max_steps is not None):
      raise ValueError('Can not provide both steps and max_steps.')
    if steps is not None and steps <= 0:
      raise ValueError('Must specify steps > 0, given: {}'.format(steps))
    if max_steps is not None and max_steps <= 0:
      raise ValueError(
          'Must specify max_steps > 0, given: {}'.format(max_steps))

    if max_steps is not None:
      start_step = _load_global_step_from_checkpoint_dir(self._model_dir)
      if max_steps <= start_step:
        logging.info('Skipping training since max_steps has already saved.')
        return self

    hooks = _check_hooks_type(hooks)
    hooks.extend(self._convert_train_steps_to_hooks(steps, max_steps))

    saving_listeners = _check_listeners_type(saving_listeners)
    loss = self._train_model(input_fn, hooks, saving_listeners)
    logging.info('Loss for final step: %s.', loss)
    return self

  def _convert_train_steps_to_hooks(self, steps, max_steps):
    if steps is not None or max_steps is not None:
      return [training.StopAtStepHook(steps, max_steps)]
    else:
      return []

  def evaluate(self, input_fn, steps=None, hooks=None, checkpoint_path=None,
               name=None):
    """Evaluates the model given evaluation data input_fn.

    For each step, calls `input_fn`, which returns one batch of data.
    Evaluates until:
    - `steps` batches are processed, or
    - `input_fn` raises an end-of-input exception (`OutOfRangeError` or
    `StopIteration`).

    Args:
      input_fn: A function that constructs the input data for evaluation.
        See @{$get_started/premade_estimators#create_input_functions} for more
        information. The function should construct and return one of
        the following:

          * A 'tf.data.Dataset' object: Outputs of `Dataset` object must be a
            tuple (features, labels) with same constraints as below.
          * A tuple (features, labels): Where features is a `Tensor` or a
            dictionary of string feature name to `Tensor` and labels is a
            `Tensor` or a dictionary of string label name to `Tensor`. Both
            features and labels are consumed by `model_fn`. They should satisfy
            the expectation of `model_fn` from inputs.

      steps: Number of steps for which to evaluate model. If `None`, evaluates
        until `input_fn` raises an end-of-input exception.
      hooks: List of `SessionRunHook` subclass instances. Used for callbacks
        inside the evaluation call.
      checkpoint_path: Path of a specific checkpoint to evaluate. If `None`, the
        latest checkpoint in `model_dir` is used.
      name: Name of the evaluation if user needs to run multiple evaluations on
        different data sets, such as on training data vs test data. Metrics for
        different evaluations are saved in separate folders, and appear
        separately in tensorboard.

    Returns:
      A dict containing the evaluation metrics specified in `model_fn` keyed by
      name, as well as an entry `global_step` which contains the value of the
      global step for which this evaluation was performed.

    Raises:
      ValueError: If `steps <= 0`.
      ValueError: If no model has been trained, namely `model_dir`, or the
        given `checkpoint_path` is empty.
    """
    hooks = _check_hooks_type(hooks)
    hooks.extend(self._convert_eval_steps_to_hooks(steps))

    return self._evaluate_model(
        input_fn=input_fn,
        hooks=hooks,
        checkpoint_path=checkpoint_path,
        name=name)

  def _convert_eval_steps_to_hooks(self, steps):
    if steps is None:
      return []

    if steps <= 0:
      raise ValueError('Must specify steps > 0, given: {}'.format(steps))
    return [evaluation._StopAfterNEvalsHook(num_evals=steps)]  # pylint: disable=protected-access

  def predict(self,
              input_fn,
              predict_keys=None,
              hooks=None,
              checkpoint_path=None,
              yield_single_examples=True):
    """Yields predictions for given features.

    Args:
      input_fn: A function that constructs the features. Prediction continues
        until `input_fn` raises an end-of-input exception (`OutOfRangeError` or
        `StopIteration`).
        See @{$get_started/premade_estimators#create_input_functions} for more
        information. The function should construct and return one of
        the following:

          * A 'tf.data.Dataset' object: Outputs of `Dataset` object must have
            same constraints as below.
          * features: A `Tensor` or a dictionary of string feature name to
            `Tensor`. features are consumed by `model_fn`. They should satisfy
            the expectation of `model_fn` from inputs.
          * A tuple, in which case the first item is extracted as features.

      predict_keys: list of `str`, name of the keys to predict. It is used if
        the `EstimatorSpec.predictions` is a `dict`. If `predict_keys` is used
        then rest of the predictions will be filtered from the dictionary. If
        `None`, returns all.
      hooks: List of `SessionRunHook` subclass instances. Used for callbacks
        inside the prediction call.
      checkpoint_path: Path of a specific checkpoint to predict. If `None`, the
        latest checkpoint in `model_dir` is used.
      yield_single_examples: If False, yield the whole batch as returned by the
        model_fn instead of decomposing the batch into individual elements. This
        is useful if model_fn return some tensor with first dimension not
        equal to the batch size

    Yields:
      Evaluated values of `predictions` tensors.

    Raises:
      ValueError: Could not find a trained model in model_dir.
      ValueError: if batch length of predictions are not same and
        yield_single_examples is True.
      ValueError: If there is a conflict between `predict_keys` and
        `predictions`. For example if `predict_keys` is not `None` but
        `EstimatorSpec.predictions` is not a `dict`.
    """
    hooks = _check_hooks_type(hooks)
    # Check that model has been trained.
    if not checkpoint_path:
      checkpoint_path = saver.latest_checkpoint(self._model_dir)
    if not checkpoint_path:
      raise ValueError('Could not find trained model in model_dir: {}.'.format(
          self._model_dir))

    with ops.Graph().as_default() as g:
      random_seed.set_random_seed(self._config.tf_random_seed)
      self._create_and_assert_global_step(g)
      features, input_hooks = self._get_features_from_input_fn(
          input_fn, model_fn_lib.ModeKeys.PREDICT)
      estimator_spec = self._call_model_fn(
          features, None, model_fn_lib.ModeKeys.PREDICT, self.config)
      predictions = self._extract_keys(estimator_spec.predictions, predict_keys)
      all_hooks = list(input_hooks)
      all_hooks.extend(hooks)
      all_hooks.extend(list(estimator_spec.prediction_hooks or []))
      with training.MonitoredSession(
          session_creator=training.ChiefSessionCreator(
              checkpoint_filename_with_path=checkpoint_path,
              master=self._config.master,
              scaffold=estimator_spec.scaffold,
              config=self._session_config),
          hooks=all_hooks) as mon_sess:
        while not mon_sess.should_stop():
          preds_evaluated = mon_sess.run(predictions)
          if not yield_single_examples:
            yield preds_evaluated
          elif not isinstance(predictions, dict):
            for pred in preds_evaluated:
              yield pred
          else:
            for i in range(self._extract_batch_length(preds_evaluated)):
              yield {
                  key: value[i]
                  for key, value in six.iteritems(preds_evaluated)
              }

  def _assert_members_are_not_overridden(self):
    """Asserts members of `Estimator` are not overridden."""
    allowed_overrides = set([
        '_call_input_fn', '_create_global_step',
        '_convert_train_steps_to_hooks', '_convert_eval_steps_to_hooks',
        '_tf_api_names'
    ])
    estimator_members = set([m for m in Estimator.__dict__.keys()
                             if not m.startswith('__')])
    subclass_members = set(self.__class__.__dict__.keys())
    common_members = estimator_members & subclass_members - allowed_overrides
    overridden_members = [
        m for m in common_members
        if Estimator.__dict__[m] != self.__class__.__dict__[m]]
    if overridden_members:
      raise ValueError(
          'Subclasses of Estimator cannot override members of Estimator. '
          '{} does override {}'.format(self.__class__, overridden_members))

  def export_savedmodel(
      self, export_dir_base, serving_input_receiver_fn,
      assets_extra=None,
      as_text=False,
      checkpoint_path=None,
      strip_default_attrs=False):
    # pylint: disable=line-too-long
    """Exports inference graph as a SavedModel into given dir.

    For a detailed guide, see
    @{$saved_model#using_savedmodel_with_estimators$Using SavedModel with Estimators}.

    This method builds a new graph by first calling the
    serving_input_receiver_fn to obtain feature `Tensor`s, and then calling
    this `Estimator`'s model_fn to generate the model graph based on those
    features. It restores the given checkpoint (or, lacking that, the most
    recent checkpoint) into this graph in a fresh session.  Finally it creates
    a timestamped export directory below the given export_dir_base, and writes
    a `SavedModel` into it containing a single `MetaGraphDef` saved from this
    session.

    The exported `MetaGraphDef` will provide one `SignatureDef` for each
    element of the export_outputs dict returned from the model_fn, named using
    the same keys.  One of these keys is always
    signature_constants.DEFAULT_SERVING_SIGNATURE_DEF_KEY, indicating which
    signature will be served when a serving request does not specify one.
    For each signature, the outputs are provided by the corresponding
    `ExportOutput`s, and the inputs are always the input receivers provided by
    the serving_input_receiver_fn.

    Extra assets may be written into the SavedModel via the assets_extra
    argument.  This should be a dict, where each key gives a destination path
    (including the filename) relative to the assets.extra directory.  The
    corresponding value gives the full path of the source file to be copied.
    For example, the simple case of copying a single file without renaming it
    is specified as `{'my_asset_file.txt': '/path/to/my_asset_file.txt'}`.

    Args:
      export_dir_base: A string containing a directory in which to create
        timestamped subdirectories containing exported SavedModels.
      serving_input_receiver_fn: A function that takes no argument and
        returns a `ServingInputReceiver`.
      assets_extra: A dict specifying how to populate the assets.extra directory
        within the exported SavedModel, or `None` if no extra assets are needed.
      as_text: whether to write the SavedModel proto in text format.
      checkpoint_path: The checkpoint path to export.  If `None` (the default),
        the most recent checkpoint found within the model directory is chosen.
      strip_default_attrs: Boolean. If `True`, default-valued attributes will be
        removed from the NodeDefs. For a detailed guide, see
        [Stripping Default-Valued Attributes](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/python/saved_model/README.md#stripping-default-valued-attributes).

    Returns:
      The string path to the exported directory.

    Raises:
      ValueError: if no serving_input_receiver_fn is provided, no export_outputs
          are provided, or no checkpoint can be found.
    """
    # pylint: enable=line-too-long
    if serving_input_receiver_fn is None:
      raise ValueError('serving_input_receiver_fn must be defined.')

    with ops.Graph().as_default() as g:
      self._create_and_assert_global_step(g)
      random_seed.set_random_seed(self._config.tf_random_seed)
      serving_input_receiver = serving_input_receiver_fn()

      # Call the model_fn and collect the export_outputs.
      estimator_spec = self._call_model_fn(
          features=serving_input_receiver.features,
          labels=None,
          mode=model_fn_lib.ModeKeys.PREDICT,
          config=self.config)

      # Build the SignatureDefs from receivers and all outputs
      signature_def_map = build_all_signature_defs(
          serving_input_receiver.receiver_tensors,
          estimator_spec.export_outputs,
          serving_input_receiver.receiver_tensors_alternatives)

      if not checkpoint_path:
        # Locate the latest checkpoint
        checkpoint_path = saver.latest_checkpoint(self._model_dir)
      if not checkpoint_path:
        raise ValueError("Couldn't find trained model at %s." % self._model_dir)

      export_dir = get_timestamped_export_dir(export_dir_base)
      temp_export_dir = get_temp_export_dir(export_dir)

      # TODO(soergel): Consider whether MonitoredSession makes sense here
      with tf_session.Session(config=self._session_config) as session:

        saver_for_restore = estimator_spec.scaffold.saver or saver.Saver(
            sharded=True)
        saver_for_restore.restore(session, checkpoint_path)

        # pylint: disable=protected-access
        local_init_op = (
            estimator_spec.scaffold.local_init_op or
            monitored_session.Scaffold._default_local_init_op())
        # pylint: enable=protected-access

        # Perform the export
        builder = saved_model_builder.SavedModelBuilder(temp_export_dir)
        builder.add_meta_graph_and_variables(
            session, [tag_constants.SERVING],
            signature_def_map=signature_def_map,
            assets_collection=ops.get_collection(
                ops.GraphKeys.ASSET_FILEPATHS),
            legacy_init_op=local_init_op,
            strip_default_attrs=strip_default_attrs)
        builder.save(as_text)

      # Add the extra assets
      if assets_extra:
        assets_extra_path = os.path.join(compat.as_bytes(temp_export_dir),
                                         compat.as_bytes('assets.extra'))
        for dest_relative, source in assets_extra.items():
          dest_absolute = os.path.join(compat.as_bytes(assets_extra_path),
                                       compat.as_bytes(dest_relative))
          dest_path = os.path.dirname(dest_absolute)
          gfile.MakeDirs(dest_path)
          gfile.Copy(source, dest_absolute)

      gfile.Rename(temp_export_dir, export_dir)
      return export_dir

  def _get_features_from_input_fn(self, input_fn, mode):
    """Extracts the `features` from return values of `input_fn`."""
    result = self._call_input_fn(input_fn, mode)
    input_hooks = []
    if isinstance(result, dataset_ops.Dataset):
      iterator = result.make_initializable_iterator()
      input_hooks.append(_DatasetInitializerHook(iterator))
      result = iterator.get_next()
    if isinstance(result, (list, tuple)):
      # Unconditionally drop the label (the second element of result).
      result = result[0]

    if not _has_dataset_or_queue_runner(result):
      logging.warning('Input graph does not use tf.data.Dataset or contain a '
                      'QueueRunner. That means predict yields forever. '
                      'This is probably a mistake.')
    return result, input_hooks

  def _get_features_and_labels_from_input_fn(self, input_fn, mode):
    """Extracts the `features` and labels from return values of `input_fn`."""
    result = self._call_input_fn(input_fn, mode)
    input_hooks = []
    if isinstance(result, dataset_ops.Dataset):
      iterator = result.make_initializable_iterator()
      input_hooks.append(_DatasetInitializerHook(iterator))
      result = iterator.get_next()
    if isinstance(result, (list, tuple)):
      if len(result) != 2:
        raise ValueError(
            'input_fn should return (features, labels) as a len 2 tuple.')
      return result[0], result[1], input_hooks
    return result, None, input_hooks

  def _extract_batch_length(self, preds_evaluated):
    """Extracts batch length of predictions."""
    batch_length = None
    for key, value in six.iteritems(preds_evaluated):
      batch_length = batch_length or value.shape[0]
      if value.shape[0] != batch_length:
        raise ValueError('Batch length of predictions should be same. %s has '
                         'different batch length then others.' % key)
    return batch_length

  def _extract_keys(self, predictions, predict_keys):
    """Extracts `predict_keys` from `predictions`."""
    if not predict_keys:
      return predictions
    if not isinstance(predictions, dict):
      raise ValueError(
          'predict_keys argument is not valid in case of non-dict predictions.')
    existing_keys = predictions.keys()
    predictions = {
        key: value
        for key, value in six.iteritems(predictions) if key in predict_keys
    }
    if not predictions:
      raise ValueError('Expected to run at least one output from %s, '
                       'provided %s.' % (existing_keys, predict_keys))
    return predictions

  def _create_global_step(self, graph):
    """Creates the global step tensor in graph.

    The global step tensor must be an integer type with name 'global_step' and
    be added to the collection ${tf.GraphKeys.GLOBAL_STEP}.

    Args:
      graph: The graph in which to create the global step tensor.

    Returns:
      The global step `Tensor`.
    """
    return training.create_global_step(graph)

  def _create_and_assert_global_step(self, graph):
    """Creates and asserts properties of the global step.

    Args:
      graph: The graph in which to create the global step tensor.

    Returns:
      The global step `Tensor`.
    """
    step = self._create_global_step(graph)
    assert step == training.get_global_step()
    assert step.dtype.is_integer
    return step

  def _call_input_fn(self, input_fn, mode):
    """Calls the input function.

    Args:
      input_fn: The input function.
      mode: ModeKeys

    Returns:
      Either features or (features, labels) where features and labels are:
        features - `Tensor` or dictionary of string feature name to `Tensor`.
        labels - `Tensor` or dictionary of `Tensor` with labels.

    Raises:
      ValueError: if input_fn takes invalid arguments.
    """
    input_fn_args = util.fn_args(input_fn)
    kwargs = {}
    if 'mode' in input_fn_args:
      kwargs['mode'] = mode
    if 'params' in input_fn_args:
      kwargs['params'] = self.params
    if 'config' in input_fn_args:
      kwargs['config'] = self.config
    with ops.device('/cpu:0'):
      return input_fn(**kwargs)

  def _call_model_fn(self, features, labels, mode, config):
    """Calls model function.

    Args:
      features: features dict.
      labels: labels dict.
      mode: ModeKeys
      config: RunConfig

    Returns:
      An `EstimatorSpec` object.

    Raises:
      ValueError: if model_fn returns invalid objects.
    """
    model_fn_args = util.fn_args(self._model_fn)
    kwargs = {}
    if 'labels' in model_fn_args:
      kwargs['labels'] = labels
    else:
      if labels is not None:
        raise ValueError(
            'model_fn does not take labels, but input_fn returns labels.')
    if 'mode' in model_fn_args:
      kwargs['mode'] = mode
    if 'params' in model_fn_args:
      kwargs['params'] = self.params
    if 'config' in model_fn_args:
      kwargs['config'] = config

    logging.info('Calling model_fn.')
    model_fn_results = self._model_fn(features=features, **kwargs)
    logging.info('Done calling model_fn.')

    if not isinstance(model_fn_results, model_fn_lib.EstimatorSpec):
      raise ValueError('model_fn should return an EstimatorSpec.')

    return model_fn_results

  def _train_model(self, input_fn, hooks, saving_listeners):
    worker_hooks = []
    with ops.Graph().as_default() as g, g.device(self._device_fn):
      random_seed.set_random_seed(self._config.tf_random_seed)
      global_step_tensor = self._create_and_assert_global_step(g)
      training_util._get_or_create_global_step_read()  # pylint: disable=protected-access
      features, labels, input_hooks = (
          self._get_features_and_labels_from_input_fn(
              input_fn, model_fn_lib.ModeKeys.TRAIN))
      worker_hooks.extend(input_hooks)
      estimator_spec = self._call_model_fn(
          features, labels, model_fn_lib.ModeKeys.TRAIN, self.config)

      if self._warm_start_settings:
        logging.info('Warm-starting with WarmStartSettings: %s' %
                     (self._warm_start_settings,))
        # pylint: disable=protected-access
        warm_starting_util._warm_start(self._warm_start_settings)
        # pylint: enable=protected-access
      # Check if the user created a loss summary, and add one if they didn't.
      # We assume here that the summary is called 'loss'. If it is not, we will
      # make another one with the name 'loss' to ensure it shows up in the right
      # graph in TensorBoard.
      if not any([x.op.name == 'loss'
                  for x in ops.get_collection(ops.GraphKeys.SUMMARIES)]):
        summary.scalar('loss', estimator_spec.loss)
      ops.add_to_collection(ops.GraphKeys.LOSSES, estimator_spec.loss)
      worker_hooks.extend(hooks)
      worker_hooks.extend([
          training.NanTensorHook(estimator_spec.loss),
          training.LoggingTensorHook(
              {
                  'loss': estimator_spec.loss,
                  'step': global_step_tensor
              },
              every_n_iter=100)
      ])
      worker_hooks.extend(estimator_spec.training_hooks)

      if not (estimator_spec.scaffold.saver or
              ops.get_collection(ops.GraphKeys.SAVERS)):
        ops.add_to_collection(
            ops.GraphKeys.SAVERS,
            training.Saver(
                sharded=True,
                max_to_keep=self._config.keep_checkpoint_max,
                keep_checkpoint_every_n_hours=(
                    self._config.keep_checkpoint_every_n_hours),
                defer_build=True,
                save_relative_paths=True))

      chief_hooks = []
      all_hooks = worker_hooks + list(estimator_spec.training_chief_hooks)
      saver_hooks = [
          h for h in all_hooks if isinstance(h, training.CheckpointSaverHook)]
      if (self._config.save_checkpoints_secs or
          self._config.save_checkpoints_steps):
        if not saver_hooks:
          chief_hooks = [
              training.CheckpointSaverHook(
                  self._model_dir,
                  save_secs=self._config.save_checkpoints_secs,
                  save_steps=self._config.save_checkpoints_steps,
                  scaffold=estimator_spec.scaffold)
          ]
          saver_hooks = [chief_hooks[0]]
      if saving_listeners:
        if not saver_hooks:
          raise ValueError(
              'There should be a CheckpointSaverHook to use saving_listeners. '
              'Please set one of the RunConfig.save_checkpoints_steps or '
              'RunConfig.save_checkpoints_secs.')
        else:
          # It is expected to have one CheckpointSaverHook. If multiple, we pick
          # up the first one to add listener.
          saver_hooks[0]._listeners.extend(saving_listeners)  # pylint: disable=protected-access
      with training.MonitoredTrainingSession(
          master=self._config.master,
          is_chief=self._config.is_chief,
          checkpoint_dir=self._model_dir,
          scaffold=estimator_spec.scaffold,
          hooks=worker_hooks,
          chief_only_hooks=(
              tuple(chief_hooks) + tuple(estimator_spec.training_chief_hooks)),
          save_checkpoint_secs=0,  # Saving is handled by a hook.
          save_summaries_steps=self._config.save_summary_steps,
          config=self._session_config,
          log_step_count_steps=self._config.log_step_count_steps) as mon_sess:
        loss = None
        while not mon_sess.should_stop():
          _, loss = mon_sess.run([estimator_spec.train_op, estimator_spec.loss])
      return loss

  def _evaluate_model(self,
                      input_fn,
                      hooks=None,
                      checkpoint_path=None,
                      name=''):
    """Evaluates the model using the training.evaluation library."""
    # Check that model has been trained (if nothing has been set explicitly).
    if not checkpoint_path:
      latest_path = saver.latest_checkpoint(self._model_dir)
      if not latest_path:
        raise ValueError('Could not find trained model in model_dir: {}.'.
                         format(self._model_dir))
      checkpoint_path = latest_path

    # Setup output directory.
    eval_dir = os.path.join(self._model_dir, 'eval' if not name else
                            'eval_' + name)

    with ops.Graph().as_default() as g:
      random_seed.set_random_seed(self._config.tf_random_seed)
      global_step_tensor = self._create_and_assert_global_step(g)
      features, labels, input_hooks = (
          self._get_features_and_labels_from_input_fn(
              input_fn, model_fn_lib.ModeKeys.EVAL))
      estimator_spec = self._call_model_fn(
          features, labels, model_fn_lib.ModeKeys.EVAL, self.config)

      if model_fn_lib.LOSS_METRIC_KEY in estimator_spec.eval_metric_ops:
        raise ValueError(
            'Metric with name "%s" is not allowed, because Estimator ' % (
                model_fn_lib.LOSS_METRIC_KEY) +
            'already defines a default metric with the same name.')
      estimator_spec.eval_metric_ops[
          model_fn_lib.LOSS_METRIC_KEY] = metrics_lib.mean(estimator_spec.loss)

      update_op, eval_dict = _extract_metric_update_ops(
          estimator_spec.eval_metric_ops)

      if ops.GraphKeys.GLOBAL_STEP in eval_dict:
        raise ValueError(
            'Metric with name `global_step` is not allowed, because Estimator '
            'already defines a default metric with the same name.')
      eval_dict[ops.GraphKeys.GLOBAL_STEP] = global_step_tensor

      all_hooks = list(input_hooks)
      all_hooks.extend(hooks)
      all_hooks.extend(list(estimator_spec.evaluation_hooks or []))

      eval_results = evaluation._evaluate_once(  # pylint: disable=protected-access
          checkpoint_path=checkpoint_path,
          master=self._config.evaluation_master,
          scaffold=estimator_spec.scaffold,
          eval_ops=update_op,
          final_ops=eval_dict,
          hooks=all_hooks,
          config=self._session_config)

      _write_dict_to_summary(
          output_dir=eval_dir,
          dictionary=eval_results,
          current_global_step=eval_results[ops.GraphKeys.GLOBAL_STEP])

    return eval_results


def _check_checkpoint_available(model_dir):
  latest_path = saver.latest_checkpoint(model_dir)
  if not latest_path:
    raise ValueError(
        'Could not find trained model in model_dir: {}.'.format(model_dir))


def _check_hooks_type(hooks):
  """Returns hooks if all are SessionRunHook, raises TypeError otherwise."""
  hooks = list(hooks or [])
  for h in hooks:
    if not isinstance(h, training.SessionRunHook):
      raise TypeError('Hooks must be a SessionRunHook, given: {}'.format(h))
  return hooks


def _check_listeners_type(saving_listeners):
  """Check listeners type."""
  listeners = list(saving_listeners or [])
  for l in listeners:
    if not isinstance(l, training.CheckpointSaverListener):
      raise TypeError(
          'saving_listeners must be a list of CheckpointSaverListener, '
          'given: {}'.format(l))
  return listeners


def _get_replica_device_setter(config):
  """Creates a replica device setter if required as a default device_fn.

  `Estimator` uses ReplicaDeviceSetter as a default device placer. It sets the
  distributed related arguments such as number of ps_replicas based on given
  config.

  Args:
    config: A `RunConfig` instance.

  Returns:
    A replica device setter, or None.
  """
  ps_ops = [
      'Variable', 'VariableV2', 'AutoReloadVariable', 'MutableHashTable',
      'MutableHashTableV2', 'MutableHashTableOfTensors',
      'MutableHashTableOfTensorsV2', 'MutableDenseHashTable',
      'MutableDenseHashTableV2', 'VarHandleOp'
  ]

  if config.task_type:
    worker_device = '/job:%s/task:%d' % (config.task_type, config.task_id)
  else:
    worker_device = '/job:worker'

  if config.num_ps_replicas > 0:
    return training.replica_device_setter(
        ps_tasks=config.num_ps_replicas,
        worker_device=worker_device,
        merge_devices=True,
        ps_ops=ps_ops,
        cluster=config.cluster_spec)
  else:
    return None


def _verify_model_fn_args(model_fn, params):
  """Verifies model fn arguments."""
  args = set(util.fn_args(model_fn))
  if 'features' not in args:
    raise ValueError('model_fn (%s) must include features argument.' % model_fn)
  if params is not None and 'params' not in args:
    raise ValueError('model_fn (%s) does not include params argument, '
                     'but params (%s) is passed to Estimator.' % (model_fn,
                                                                  params))
  if params is None and 'params' in args:
    logging.warning('Estimator\'s model_fn (%s) includes params '
                    'argument, but params are not passed to Estimator.',
                    model_fn)
  non_valid_args = list(args - _VALID_MODEL_FN_ARGS)
  if non_valid_args:
    raise ValueError('model_fn (%s) has following not expected args: %s' %
                     (model_fn, non_valid_args))


def _load_global_step_from_checkpoint_dir(checkpoint_dir):
  try:
    checkpoint_reader = training.NewCheckpointReader(
        training.latest_checkpoint(checkpoint_dir))
    return checkpoint_reader.get_tensor(ops.GraphKeys.GLOBAL_STEP)
  except:  # pylint: disable=bare-except
    return 0


def _extract_metric_update_ops(eval_dict):
  """Separate update operations from metric value operations."""
  update_ops = []
  value_ops = {}
  # Sort metrics lexicographically so graph is identical every time.
  for name, metric_ops in sorted(six.iteritems(eval_dict)):
    value_ops[name] = metric_ops[0]
    update_ops.append(metric_ops[1])

  if update_ops:
    update_op = control_flow_ops.group(*update_ops)
  else:
    update_op = None

  return update_op, value_ops


def _dict_to_str(dictionary):
  """Get a `str` representation of a `dict`.

  Args:
    dictionary: The `dict` to be represented as `str`.

  Returns:
    A `str` representing the `dictionary`.
  """
  return ', '.join('%s = %s' % (k, v)
                   for k, v in sorted(six.iteritems(dictionary)))


def _write_dict_to_summary(output_dir,
                           dictionary,
                           current_global_step):
  """Writes a `dict` into summary file in given output directory.

  Args:
    output_dir: `str`, directory to write the summary file in.
    dictionary: the `dict` to be written to summary file.
    current_global_step: `int`, the current global step.
  """
  logging.info('Saving dict for global step %d: %s', current_global_step,
               _dict_to_str(dictionary))
  summary_writer = writer_cache.FileWriterCache.get(output_dir)
  summary_proto = summary_pb2.Summary()
  for key in dictionary:
    if dictionary[key] is None:
      continue
    if key == 'global_step':
      continue
    if (isinstance(dictionary[key], np.float32) or
        isinstance(dictionary[key], float)):
      summary_proto.value.add(tag=key, simple_value=float(dictionary[key]))
    elif (isinstance(dictionary[key], np.int64) or
          isinstance(dictionary[key], np.int32) or
          isinstance(dictionary[key], int)):
      summary_proto.value.add(tag=key, simple_value=int(dictionary[key]))
    elif isinstance(dictionary[key], six.binary_type):
      try:
        summ = summary_pb2.Summary.FromString(dictionary[key])
        for i, _ in enumerate(summ.value):
          summ.value[i].tag = key
        summary_proto.value.extend(summ.value)
      except message.DecodeError:
        logging.warn('Skipping summary for %s, cannot parse string to Summary.',
                     key)
        continue
    else:
      logging.warn(
          'Skipping summary for %s, must be a float, np.float32, np.int64, '
          'np.int32 or int or a serialized string of Summary.', key)
  summary_writer.add_summary(summary_proto, current_global_step)
  summary_writer.flush()


def _has_dataset_or_queue_runner(maybe_tensor):
  """Returns True if TF dataset or QueueRunner has been used."""
  # Check TF dataset first. Here, we use a simple algorithm to check the top
  # level Tensors only, which should be sufficient for most users.
  tensors = [x for x in nest.flatten(maybe_tensor) if isinstance(x, ops.Tensor)]
  if any([t.op.type == 'IteratorGetNext' for t in tensors]):
    return True

  # Now, check queue.
  return ops.get_default_graph().get_collection(ops.GraphKeys.QUEUE_RUNNERS)


class _DatasetInitializerHook(training.SessionRunHook):

  def __init__(self, iterator):
    self._iterator = iterator

  def begin(self):
    self._initializer = self._iterator.initializer

  def after_create_session(self, session, coord):
    del coord
    session.run(self._initializer)
