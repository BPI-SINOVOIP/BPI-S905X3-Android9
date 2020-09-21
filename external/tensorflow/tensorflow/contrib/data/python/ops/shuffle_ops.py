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
"""Experimental shuffle ops."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.python.data.ops import dataset_ops
from tensorflow.python.data.util import nest
from tensorflow.python.data.util import sparse
from tensorflow.python.framework import constant_op
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import ops
from tensorflow.python.framework import random_seed
from tensorflow.python.ops import gen_dataset_ops


class _ShuffleAndRepeatDataset(dataset_ops.Dataset):
  """A `Dataset` that fuses `shuffle` and `repeat`."""

  def __init__(self,
               input_dataset,
               buffer_size,
               count=None,
               seed=None):
    """See `Dataset.map()` for details."""
    super(_ShuffleAndRepeatDataset, self).__init__()
    self._input_dataset = input_dataset
    self._buffer_size = ops.convert_to_tensor(
        buffer_size, dtype=dtypes.int64, name="buffer_size")
    if count is None:
      self._count = constant_op.constant(-1, dtype=dtypes.int64, name="count")
    else:
      self._count = ops.convert_to_tensor(
          count, dtype=dtypes.int64, name="count")

    seed, seed2 = random_seed.get_seed(seed)
    if seed is None:
      self._seed = constant_op.constant(0, dtype=dtypes.int64, name="seed")
    else:
      self._seed = ops.convert_to_tensor(seed, dtype=dtypes.int64, name="seed")
    if seed2 is None:
      self._seed2 = constant_op.constant(0, dtype=dtypes.int64, name="seed2")
    else:
      self._seed2 = ops.convert_to_tensor(
          seed2, dtype=dtypes.int64, name="seed2")

  def _as_variant_tensor(self):
    # pylint: disable=protected-access
    input_resource = self._input_dataset._as_variant_tensor()
    return gen_dataset_ops.shuffle_and_repeat_dataset(
        input_resource,
        buffer_size=self._buffer_size,
        count=self._count,
        seed=self._seed,
        seed2=self._seed2,
        output_types=nest.flatten(
            sparse.as_dense_types(self.output_types, self.output_classes)),
        output_shapes=nest.flatten(
            sparse.as_dense_shapes(self.output_shapes, self.output_classes)))
    # pylint: enable=protected-access

  @property
  def output_classes(self):
    return self._input_dataset.output_classes

  @property
  def output_shapes(self):
    return self._input_dataset.output_shapes

  @property
  def output_types(self):
    return self._input_dataset.output_types


def shuffle_and_repeat(buffer_size, count=None, seed=None):
  """Shuffles and repeats a Dataset returning a new permutation for each epoch.

  `dataset.apply(tf.contrib.data.shuffle_and_repeat(buffer_size, count))`

  is equivalent to

  `dataset.shuffle(buffer_size, reshuffle_each_iteration=True).repeat(count)`

  The difference is that the latter dataset is not serializable. So,
  if you need to checkpoint an input pipeline with reshuffling you must use
  this implementation.

  Args:
    buffer_size: A `tf.int64` scalar `tf.Tensor`, representing the
      maximum number elements that will be buffered when prefetching.
    count: (Optional.) A `tf.int64` scalar `tf.Tensor`, representing the
      number of times the dataset should be repeated. The default behavior
      (if `count` is `None` or `-1`) is for the dataset be repeated
      indefinitely.
    seed: (Optional.) A `tf.int64` scalar `tf.Tensor`, representing the
      random seed that will be used to create the distribution. See
      @{tf.set_random_seed} for behavior.

  Returns:
    A `Dataset` transformation function, which can be passed to
    @{tf.data.Dataset.apply}.
  """

  def _apply_fn(dataset):  # pylint: disable=missing-docstring
    return _ShuffleAndRepeatDataset(dataset, buffer_size, count, seed)

  return _apply_fn
