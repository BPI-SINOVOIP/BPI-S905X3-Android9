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
r"""Train a ConvNet on MNIST using K-FAC.

This library fits a 5-layer ConvNet on MNIST using K-FAC. The model has the
following structure,

- Conv Layer: 5x5 kernel, 16 output channels.
- Max Pool: 3x3 kernel, stride 2.
- Conv Layer: 5x5 kernel, 16 output channels.
- Max Pool: 3x3 kernel, stride 2.
- Linear: 10 output dims.

After 3k~6k steps, this should reach perfect accuracy on the training set.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os

import numpy as np
import tensorflow as tf

from tensorflow.contrib.kfac.examples import mlp
from tensorflow.contrib.kfac.examples import mnist

lc = tf.contrib.kfac.layer_collection
oq = tf.contrib.kfac.op_queue
opt = tf.contrib.kfac.optimizer

__all__ = [
    "conv_layer",
    "max_pool_layer",
    "linear_layer",
    "build_model",
    "minimize_loss_single_machine",
    "minimize_loss_distributed",
    "train_mnist_single_machine",
    "train_mnist_distributed",
]


def conv_layer(layer_id, inputs, kernel_size, out_channels):
  """Builds a convolutional layer with ReLU non-linearity.

  Args:
    layer_id: int. Integer ID for this layer's variables.
    inputs: Tensor of shape [num_examples, width, height, in_channels]. Each row
      corresponds to a single example.
    kernel_size: int. Width and height of the convolution kernel. The kernel is
      assumed to be square.
    out_channels: int. Number of output features per pixel.

  Returns:
    preactivations: Tensor of shape [num_examples, width, height, out_channels].
      Values of the layer immediately before the activation function.
    activations: Tensor of shape [num_examples, width, height, out_channels].
      Values of the layer immediately after the activation function.
    params: Tuple of (kernel, bias), parameters for this layer.
  """
  # TODO(b/67004004): Delete this function and rely on tf.layers exclusively.
  layer = tf.layers.Conv2D(
      out_channels,
      kernel_size=[kernel_size, kernel_size],
      kernel_initializer=tf.random_normal_initializer(stddev=0.01),
      padding="SAME",
      name="conv_%d" % layer_id)
  preactivations = layer(inputs)
  activations = tf.nn.relu(preactivations)

  # layer.weights is a list. This converts it a (hashable) tuple.
  return preactivations, activations, (layer.kernel, layer.bias)


def max_pool_layer(layer_id, inputs, kernel_size, stride):
  """Build a max-pooling layer.

  Args:
    layer_id: int. Integer ID for this layer's variables.
    inputs: Tensor of shape [num_examples, width, height, in_channels]. Each row
      corresponds to a single example.
    kernel_size: int. Width and height to pool over per input channel. The
      kernel is assumed to be square.
    stride: int. Step size between pooling operations.

  Returns:
    Tensor of shape [num_examples, width/stride, height/stride, out_channels].
    Result of applying max pooling to 'inputs'.
  """
  # TODO(b/67004004): Delete this function and rely on tf.layers exclusively.
  with tf.variable_scope("pool_%d" % layer_id):
    return tf.nn.max_pool(
        inputs, [1, kernel_size, kernel_size, 1], [1, stride, stride, 1],
        padding="SAME",
        name="pool")


def linear_layer(layer_id, inputs, output_size):
  """Builds the final linear layer for an MNIST classification problem.

  Args:
    layer_id: int. Integer ID for this layer's variables.
    inputs: Tensor of shape [num_examples, width, height, in_channels]. Each row
      corresponds to a single example.
    output_size: int. Number of output dims per example.

  Returns:
    activations: Tensor of shape [num_examples, output_size]. Values of the
      layer immediately after the activation function.
    params: Tuple of (weights, bias), parameters for this layer.
  """
  # TODO(b/67004004): Delete this function and rely on tf.layers exclusively.
  pre, _, params = mlp.fc_layer(layer_id, inputs, output_size)
  return pre, params


def build_model(examples, labels, num_labels, layer_collection):
  """Builds a ConvNet classification model.

  Args:
    examples: Tensor of shape [num_examples, num_features]. Represents inputs of
      model.
    labels: Tensor of shape [num_examples]. Contains integer IDs to be predicted
      by softmax for each example.
    num_labels: int. Number of distinct values 'labels' can take on.
    layer_collection: LayerCollection instance. Layers will be registered here.

  Returns:
    loss: 0-D Tensor representing loss to be minimized.
    accuracy: 0-D Tensor representing model's accuracy.
  """
  # Build a ConvNet. For each layer with parameters, we'll keep track of the
  # preactivations, activations, weights, and bias.
  tf.logging.info("Building model.")
  pre0, act0, params0 = conv_layer(
      layer_id=0, inputs=examples, kernel_size=5, out_channels=16)
  act1 = max_pool_layer(layer_id=1, inputs=act0, kernel_size=3, stride=2)
  pre2, act2, params2 = conv_layer(
      layer_id=2, inputs=act1, kernel_size=5, out_channels=16)
  act3 = max_pool_layer(layer_id=3, inputs=act2, kernel_size=3, stride=2)
  flat_act3 = tf.reshape(act3, shape=[-1, int(np.prod(act3.shape[1:4]))])
  logits, params4 = linear_layer(
      layer_id=4, inputs=flat_act3, output_size=num_labels)
  loss = tf.reduce_mean(
      tf.nn.sparse_softmax_cross_entropy_with_logits(
          labels=labels, logits=logits))
  accuracy = tf.reduce_mean(
      tf.cast(tf.equal(labels, tf.argmax(logits, axis=1)), dtype=tf.float32))

  tf.summary.scalar("loss", loss)
  tf.summary.scalar("accuracy", accuracy)

  # Register parameters. K-FAC needs to know about the inputs, outputs, and
  # parameters of each conv/fully connected layer and the logits powering the
  # posterior probability over classes.
  tf.logging.info("Building LayerCollection.")
  layer_collection.register_conv2d(params0, (1, 1, 1, 1), "SAME", examples,
                                   pre0)
  layer_collection.register_conv2d(params2, (1, 1, 1, 1), "SAME", act1, pre2)
  layer_collection.register_fully_connected(params4, flat_act3, logits)
  layer_collection.register_categorical_predictive_distribution(
      logits, name="logits")

  return loss, accuracy


def minimize_loss_single_machine(loss,
                                 accuracy,
                                 layer_collection,
                                 session_config=None):
  """Minimize loss with K-FAC on a single machine.

  A single Session is responsible for running all of K-FAC's ops.

  Args:
    loss: 0-D Tensor. Loss to be minimized.
    accuracy: 0-D Tensor. Accuracy of classifier on current minibatch.
    layer_collection: LayerCollection instance describing model architecture.
      Used by K-FAC to construct preconditioner.
    session_config: None or tf.ConfigProto. Configuration for tf.Session().

  Returns:
    final value for 'accuracy'.
  """
  # Train with K-FAC.
  global_step = tf.train.get_or_create_global_step()
  optimizer = opt.KfacOptimizer(
      learning_rate=0.0001,
      cov_ema_decay=0.95,
      damping=0.001,
      layer_collection=layer_collection,
      momentum=0.9)
  train_op = optimizer.minimize(loss, global_step=global_step)

  tf.logging.info("Starting training.")
  with tf.train.MonitoredTrainingSession(config=session_config) as sess:
    while not sess.should_stop():
      global_step_, loss_, accuracy_, _, _ = sess.run(
          [global_step, loss, accuracy, train_op, optimizer.cov_update_op])

      if global_step_ % 100 == 0:
        sess.run(optimizer.inv_update_op)

      if global_step_ % 100 == 0:
        tf.logging.info("global_step: %d | loss: %f | accuracy: %s",
                        global_step_, loss_, accuracy_)

  return accuracy_


def _is_gradient_task(task_id, num_tasks):
  """Returns True if this task should update the weights."""
  if num_tasks < 3:
    return True
  return 0 <= task_id < 0.6 * num_tasks


def _is_cov_update_task(task_id, num_tasks):
  """Returns True if this task should update K-FAC's covariance matrices."""
  if num_tasks < 3:
    return False
  return 0.6 * num_tasks <= task_id < num_tasks - 1


def _is_inv_update_task(task_id, num_tasks):
  """Returns True if this task should update K-FAC's preconditioner."""
  if num_tasks < 3:
    return False
  return task_id == num_tasks - 1


def _num_gradient_tasks(num_tasks):
  """Number of tasks that will update weights."""
  if num_tasks < 3:
    return num_tasks
  return int(np.ceil(0.6 * num_tasks))


def minimize_loss_distributed(task_id, num_worker_tasks, num_ps_tasks, master,
                              checkpoint_dir, loss, accuracy, layer_collection):
  """Minimize loss with an synchronous implementation of K-FAC.

  Different tasks are responsible for different parts of K-FAC's Ops. The first
  60% of tasks update weights; the next 20% accumulate covariance statistics;
  the last 20% invert the matrices used to precondition gradients.

  Args:
    task_id: int. Integer in [0, num_worker_tasks). ID for this worker.
    num_worker_tasks: int. Number of workers in this distributed training setup.
    num_ps_tasks: int. Number of parameter servers holding variables. If 0,
      parameter servers are not used.
    master: string. IP and port of TensorFlow runtime process. Set to empty
      string to run locally.
    checkpoint_dir: string or None. Path to store checkpoints under.
    loss: 0-D Tensor. Loss to be minimized.
    accuracy: dict mapping strings to 0-D Tensors. Additional accuracy to
      run with each step.
    layer_collection: LayerCollection instance describing model architecture.
      Used by K-FAC to construct preconditioner.

  Returns:
    final value for 'accuracy'.

  Raises:
    ValueError: if task_id >= num_worker_tasks.
  """
  with tf.device(tf.train.replica_device_setter(num_ps_tasks)):
    global_step = tf.train.get_or_create_global_step()
    optimizer = opt.KfacOptimizer(
        learning_rate=0.0001,
        cov_ema_decay=0.95,
        damping=0.001,
        layer_collection=layer_collection,
        momentum=0.9)
    inv_update_queue = oq.OpQueue(optimizer.inv_update_ops)
    sync_optimizer = tf.train.SyncReplicasOptimizer(
        opt=optimizer,
        replicas_to_aggregate=_num_gradient_tasks(num_worker_tasks))
    train_op = sync_optimizer.minimize(loss, global_step=global_step)

  tf.logging.info("Starting training.")
  is_chief = (task_id == 0)
  hooks = [sync_optimizer.make_session_run_hook(is_chief)]
  with tf.train.MonitoredTrainingSession(
      master=master,
      is_chief=is_chief,
      checkpoint_dir=checkpoint_dir,
      hooks=hooks,
      stop_grace_period_secs=0) as sess:
    while not sess.should_stop():
      # Choose which op this task is responsible for running.
      if _is_gradient_task(task_id, num_worker_tasks):
        learning_op = train_op
      elif _is_cov_update_task(task_id, num_worker_tasks):
        learning_op = optimizer.cov_update_op
      elif _is_inv_update_task(task_id, num_worker_tasks):
        # TODO(duckworthd): Running this op before cov_update_op has been run a
        # few times can result in "InvalidArgumentError: Cholesky decomposition
        # was not successful." Delay running this op until cov_update_op has
        # been run a few times.
        learning_op = inv_update_queue.next_op(sess)
      else:
        raise ValueError("Which op should task %d do?" % task_id)

      global_step_, loss_, accuracy_, _ = sess.run(
          [global_step, loss, accuracy, learning_op])
      tf.logging.info("global_step: %d | loss: %f | accuracy: %s", global_step_,
                      loss_, accuracy_)

  return accuracy_


def train_mnist_single_machine(data_dir, num_epochs, use_fake_data=False):
  """Train a ConvNet on MNIST.

  Args:
    data_dir: string. Directory to read MNIST examples from.
    num_epochs: int. Number of passes to make over the training set.
    use_fake_data: bool. If True, generate a synthetic dataset.

  Returns:
    accuracy of model on the final minibatch of training data.
  """
  # Load a dataset.
  tf.logging.info("Loading MNIST into memory.")
  examples, labels = mnist.load_mnist(
      data_dir,
      num_epochs=num_epochs,
      batch_size=128,
      use_fake_data=use_fake_data,
      flatten_images=False)

  # Build a ConvNet.
  layer_collection = lc.LayerCollection()
  loss, accuracy = build_model(
      examples, labels, num_labels=10, layer_collection=layer_collection)

  # Fit model.
  return minimize_loss_single_machine(loss, accuracy, layer_collection)


def train_mnist_multitower(data_dir, num_epochs, num_towers,
                           use_fake_data=True):
  """Train a ConvNet on MNIST.

  Args:
    data_dir: string. Directory to read MNIST examples from.
    num_epochs: int. Number of passes to make over the training set.
    num_towers: int. Number of CPUs to split inference across.
    use_fake_data: bool. If True, generate a synthetic dataset.

  Returns:
    accuracy of model on the final minibatch of training data.
  """
  # Load a dataset.
  tf.logging.info("Loading MNIST into memory.")
  tower_batch_size = 128
  batch_size = tower_batch_size * num_towers
  tf.logging.info(
      ("Loading MNIST into memory. Using batch_size = %d = %d towers * %d "
       "tower batch size.") % (batch_size, num_towers, tower_batch_size))
  examples, labels = mnist.load_mnist(
      data_dir,
      num_epochs=num_epochs,
      batch_size=batch_size,
      use_fake_data=use_fake_data,
      flatten_images=False)

  # Split minibatch across towers.
  examples = tf.split(examples, num_towers)
  labels = tf.split(labels, num_towers)

  # Build an MLP. Each tower's layers will be added to the LayerCollection.
  layer_collection = lc.LayerCollection()
  tower_results = []
  for tower_id in range(num_towers):
    with tf.device("/cpu:%d" % tower_id):
      with tf.name_scope("tower%d" % tower_id):
        with tf.variable_scope(tf.get_variable_scope(), reuse=(tower_id > 0)):
          tf.logging.info("Building tower %d." % tower_id)
          tower_results.append(
              build_model(examples[tower_id], labels[tower_id], 10,
                          layer_collection))
  losses, accuracies = zip(*tower_results)

  # Average across towers.
  loss = tf.reduce_mean(losses)
  accuracy = tf.reduce_mean(accuracies)

  # Fit model.
  session_config = tf.ConfigProto(
      allow_soft_placement=False, device_count={
          "CPU": num_towers
      })
  return minimize_loss_single_machine(
      loss, accuracy, layer_collection, session_config=session_config)


def train_mnist_distributed(task_id,
                            num_worker_tasks,
                            num_ps_tasks,
                            master,
                            data_dir,
                            num_epochs,
                            use_fake_data=False):
  """Train a ConvNet on MNIST.

  Args:
    task_id: int. Integer in [0, num_worker_tasks). ID for this worker.
    num_worker_tasks: int. Number of workers in this distributed training setup.
    num_ps_tasks: int. Number of parameter servers holding variables.
    master: string. IP and port of TensorFlow runtime process.
    data_dir: string. Directory to read MNIST examples from.
    num_epochs: int. Number of passes to make over the training set.
    use_fake_data: bool. If True, generate a synthetic dataset.

  Returns:
    accuracy of model on the final minibatch of training data.
  """
  # Load a dataset.
  tf.logging.info("Loading MNIST into memory.")
  examples, labels = mnist.load_mnist(
      data_dir,
      num_epochs=num_epochs,
      batch_size=128,
      use_fake_data=use_fake_data,
      flatten_images=False)

  # Build a ConvNet.
  layer_collection = lc.LayerCollection()
  with tf.device(tf.train.replica_device_setter(num_ps_tasks)):
    loss, accuracy = build_model(
        examples, labels, num_labels=10, layer_collection=layer_collection)

  # Fit model.
  checkpoint_dir = None if data_dir is None else os.path.join(data_dir, "kfac")
  return minimize_loss_distributed(task_id, num_worker_tasks, num_ps_tasks,
                                   master, checkpoint_dir, loss, accuracy,
                                   layer_collection)


if __name__ == "__main__":
  tf.app.run()
