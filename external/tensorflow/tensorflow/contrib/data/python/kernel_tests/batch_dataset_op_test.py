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
"""Tests for the experimental input pipeline ops."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import math

import numpy as np

from tensorflow.contrib.data.python.kernel_tests import dataset_serialization_test_base
from tensorflow.contrib.data.python.ops import batching
from tensorflow.python.data.ops import dataset_ops
from tensorflow.python.framework import constant_op
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import errors
from tensorflow.python.framework import sparse_tensor
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import string_ops
from tensorflow.python.platform import test


class BatchDatasetTest(test.TestCase):

  def assertSparseValuesEqual(self, a, b):
    self.assertAllEqual(a.indices, b.indices)
    self.assertAllEqual(a.values, b.values)
    self.assertAllEqual(a.dense_shape, b.dense_shape)

  def testDenseToSparseBatchDataset(self):
    components = np.random.randint(12, size=(100,)).astype(np.int32)
    iterator = (
        dataset_ops.Dataset.from_tensor_slices(components)
        .map(lambda x: array_ops.fill([x], x)).apply(
            batching.dense_to_sparse_batch(4, [12]))
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)

      for start in range(0, len(components), 4):
        results = sess.run(get_next)
        self.assertAllEqual([[i, j]
                             for i, c in enumerate(components[start:start + 4])
                             for j in range(c)], results.indices)
        self.assertAllEqual(
            [c for c in components[start:start + 4] for _ in range(c)],
            results.values)
        self.assertAllEqual([min(4,
                                 len(components) - start), 12],
                            results.dense_shape)

      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testDenseToSparseBatchDatasetWithUnknownShape(self):
    components = np.random.randint(5, size=(40,)).astype(np.int32)
    iterator = (
        dataset_ops.Dataset.from_tensor_slices(components)
        .map(lambda x: array_ops.fill([x, x], x)).apply(
            batching.dense_to_sparse_batch(
                4, [5, None])).make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)

      for start in range(0, len(components), 4):
        results = sess.run(get_next)
        self.assertAllEqual([[i, j, z]
                             for i, c in enumerate(components[start:start + 4])
                             for j in range(c)
                             for z in range(c)], results.indices)
        self.assertAllEqual([
            c
            for c in components[start:start + 4] for _ in range(c)
            for _ in range(c)
        ], results.values)
        self.assertAllEqual([
            min(4,
                len(components) - start), 5,
            np.max(components[start:start + 4])
        ], results.dense_shape)

      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testDenseToSparseBatchDatasetWithInvalidShape(self):
    input_tensor = array_ops.constant([[1]])
    with self.assertRaisesRegexp(ValueError, "Dimension -2 must be >= 0"):
      dataset_ops.Dataset.from_tensors(input_tensor).apply(
          batching.dense_to_sparse_batch(4, [-2])).make_initializable_iterator()

  def testDenseToSparseBatchDatasetShapeErrors(self):
    input_tensor = array_ops.placeholder(dtypes.int32)
    iterator = (
        dataset_ops.Dataset.from_tensors(input_tensor).apply(
            batching.dense_to_sparse_batch(4, [12]))
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      # Initialize with an input tensor of incompatible rank.
      sess.run(init_op, feed_dict={input_tensor: [[1]]})
      with self.assertRaisesRegexp(errors.InvalidArgumentError,
                                   "incompatible with the row shape"):
        sess.run(get_next)

      # Initialize with an input tensor that is larger than `row_shape`.
      sess.run(init_op, feed_dict={input_tensor: range(13)})
      with self.assertRaisesRegexp(errors.DataLossError,
                                   "larger than the row shape"):
        sess.run(get_next)

  def testUnbatchScalarDataset(self):
    data = tuple([math_ops.range(10) for _ in range(3)])
    data = dataset_ops.Dataset.from_tensor_slices(data)
    expected_types = (dtypes.int32,) * 3
    data = data.batch(2)
    self.assertEqual(expected_types, data.output_types)
    data = data.apply(batching.unbatch())
    self.assertEqual(expected_types, data.output_types)

    iterator = data.make_one_shot_iterator()
    op = iterator.get_next()

    with self.test_session() as sess:
      for i in range(10):
        self.assertEqual((i,) * 3, sess.run(op))

      with self.assertRaises(errors.OutOfRangeError):
        sess.run(op)

  def testUnbatchSingleElementTupleDataset(self):
    data = tuple([(math_ops.range(10),) for _ in range(3)])
    data = dataset_ops.Dataset.from_tensor_slices(data)
    expected_types = ((dtypes.int32,),) * 3
    data = data.batch(2)
    self.assertEqual(expected_types, data.output_types)
    data = data.apply(batching.unbatch())
    self.assertEqual(expected_types, data.output_types)

    iterator = data.make_one_shot_iterator()
    op = iterator.get_next()

    with self.test_session() as sess:
      for i in range(10):
        self.assertEqual(((i,),) * 3, sess.run(op))

      with self.assertRaises(errors.OutOfRangeError):
        sess.run(op)

  def testUnbatchMultiElementTupleDataset(self):
    data = tuple([(math_ops.range(10 * i, 10 * i + 10),
                   array_ops.fill([10], "hi")) for i in range(3)])
    data = dataset_ops.Dataset.from_tensor_slices(data)
    expected_types = ((dtypes.int32, dtypes.string),) * 3
    data = data.batch(2)
    self.assertAllEqual(expected_types, data.output_types)
    data = data.apply(batching.unbatch())
    self.assertAllEqual(expected_types, data.output_types)

    iterator = data.make_one_shot_iterator()
    op = iterator.get_next()

    with self.test_session() as sess:
      for i in range(10):
        self.assertEqual(((i, b"hi"), (10 + i, b"hi"), (20 + i, b"hi")),
                         sess.run(op))

      with self.assertRaises(errors.OutOfRangeError):
        sess.run(op)

  def testBatchAndDropRemainder(self):
    components = (np.arange(7),
                  np.array([[1, 2, 3]]) * np.arange(7)[:, np.newaxis],
                  np.array(37.0) * np.arange(7))

    batch_size = array_ops.placeholder(dtypes.int64, shape=[])

    iterator = (
        dataset_ops.Dataset.from_tensor_slices(components).apply(
            batching.batch_and_drop_remainder(batch_size))
        .make_initializable_iterator())

    next_element = iterator.get_next()

    with self.test_session() as sess:
      for test_batch_size in [1, 3, 7, 10]:
        sess.run(iterator.initializer, feed_dict={batch_size: test_batch_size})
        num_batches = 7 // test_batch_size
        for i in range(num_batches):
          result = sess.run(next_element)
          for component, result_component in zip(components, result):
            for j in range(test_batch_size):
              self.assertAllEqual(component[(i * test_batch_size + j)],
                                  result_component[j])
        with self.assertRaises(errors.OutOfRangeError):
          sess.run(next_element)

  def testBatchAndDropRemainderSparse(self):

    def _sparse(i):
      return sparse_tensor.SparseTensorValue(
          indices=[[0]], values=(i * [1]), dense_shape=[1])

    iterator = dataset_ops.Dataset.range(12).map(_sparse).apply(
        batching.batch_and_drop_remainder(5)).make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(2):
        actual = sess.run(get_next)
        expected = sparse_tensor.SparseTensorValue(
            indices=[[0, 0], [1, 0], [2, 0], [3, 0], [4, 0]],
            values=[i * 5, i * 5 + 1, i * 5 + 2, i * 5 + 3, i * 5 + 4],
            dense_shape=[5, 1])
        self.assertTrue(sparse_tensor.is_sparse(actual))
        self.assertSparseValuesEqual(actual, expected)
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testPaddedBatchAndDropRemainder(self):
    els = []
    for length in [3, 6, 9, 4, 12, 10, 2]:
      els.append((np.array(length), np.arange(length) + 1,
                  np.array(length * 2)))

    dataset = dataset_ops.Dataset.from_tensors(els[0])
    for el in els[1:]:
      dataset = dataset.concatenate(dataset_ops.Dataset.from_tensors(el))

    batch_size = array_ops.placeholder(dtypes.int64, shape=[])
    iterator = (
        dataset.apply(
            batching.padded_batch_and_drop_remainder(
                batch_size, ([], [None], []))).make_initializable_iterator())

    next_element = iterator.get_next()

    with self.test_session() as sess:
      for test_batch_size in [1, 3, 7, 10]:
        sess.run(iterator.initializer, feed_dict={batch_size: test_batch_size})
        num_batches = 7 // test_batch_size
        for i in range(num_batches):
          result = sess.run(next_element)
          for component_idx, result_component in enumerate(result):
            for j in range(test_batch_size):
              data_idx = i * test_batch_size + j
              comp = result_component[j]
              unpadded = comp[comp > 0]
              if np.isscalar(comp):
                # The boolean mask indexing above adds a dim back. Rm it.
                unpadded = unpadded[0]
              self.assertAllEqual(els[data_idx][component_idx], unpadded)
        with self.assertRaises(errors.OutOfRangeError):
          sess.run(next_element)

  def testPaddedBatchAndDropRemainderSparseError(self):

    def _map_fn(i):
      return sparse_tensor.SparseTensorValue(
          indices=[[0, 0]], values=(i * [1]), dense_shape=[1, 1]), i

    with self.assertRaises(TypeError):
      _ = dataset_ops.Dataset.range(10).map(_map_fn).apply(
          batching.padded_batch_and_drop_remainder(5))

  def testBatchAndDropRemainderShapeInference(self):
    components = (array_ops.placeholder(dtypes.int32),
                  (array_ops.placeholder(dtypes.int32, shape=[None]),
                   array_ops.placeholder(dtypes.int32, shape=[20, 30])))

    # Test with a statically known batch size.
    dataset = (
        dataset_ops.Dataset.from_tensor_slices(components).apply(
            batching.batch_and_drop_remainder(128)))

    self.assertIs(None, dataset.output_shapes[0].ndims)
    self.assertEqual([128], dataset.output_shapes[1][0].as_list())
    self.assertEqual([128, 30], dataset.output_shapes[1][1].as_list())

    # Test with a dynamic batch size: the static shape will be unknown, because
    # `batch_size` is a placeholder.
    batch_size = array_ops.placeholder(dtypes.int64)
    dataset = (
        dataset_ops.Dataset.from_tensor_slices(components).apply(
            batching.batch_and_drop_remainder(batch_size)))

    self.assertIs(None, dataset.output_shapes[0].ndims)
    self.assertEqual([None], dataset.output_shapes[1][0].as_list())
    self.assertEqual([None, 30], dataset.output_shapes[1][1].as_list())

  def _testBatchAndMapDatasetHelper(self, num_parallel_batches=1):
    """Test a dataset that maps a TF function across its input elements."""
    # The pipeline is TensorSliceDataset ->
    # RepeatDataset(count) -> BatchAndMapDataset(square_3, batch_size).
    components = (np.arange(7),
                  np.array([[1, 2, 3]]) * np.arange(7)[:, np.newaxis],
                  np.array(37.0) * np.arange(7))

    count = array_ops.placeholder(dtypes.int64, shape=[])
    batch_size = array_ops.placeholder(dtypes.int64, shape=[])

    def _map_fn(x, y, z):
      return math_ops.square(x), math_ops.square(y), math_ops.square(z)

    iterator = (
        dataset_ops.Dataset.from_tensor_slices(components).repeat(count).apply(
            batching.map_and_batch(
                map_func=_map_fn,
                batch_size=batch_size,
                num_parallel_batches=num_parallel_batches))
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    self.assertEqual([[None] + list(c.shape[1:]) for c in components],
                     [t.shape.as_list() for t in get_next])

    with self.test_session() as sess:
      # Batch of a finite input, where the batch_size divides the
      # total number of elements.
      sess.run(init_op, feed_dict={count: 28, batch_size: 14})
      num_batches = (28 * 7) // 14
      for i in range(num_batches):
        result = sess.run(get_next)
        for component, result_component in zip(components, result):
          for j in range(14):
            self.assertAllEqual(component[(i * 14 + j) % 7]**2,
                                result_component[j])
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

      # Batch of a finite input, where the batch_size does not
      # divide the total number of elements.
      sess.run(init_op, feed_dict={count: 14, batch_size: 8})

      # We expect (num_batches - 1) full-sized batches.
      num_batches = int(math.ceil((14 * 7) / 8))
      for i in range(num_batches - 1):
        result = sess.run(get_next)
        for component, result_component in zip(components, result):
          for j in range(8):
            self.assertAllEqual(component[(i * 8 + j) % 7]**2,
                                result_component[j])
      result = sess.run(get_next)
      for component, result_component in zip(components, result):
        for j in range((14 * 7) % 8):
          self.assertAllEqual(component[((num_batches - 1) * 8 + j) % 7]**2,
                              result_component[j])
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

      # Batch of an empty input should fail straight away.
      sess.run(init_op, feed_dict={count: 0, batch_size: 8})
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

      # Empty batch should be an initialization time error.
      with self.assertRaises(errors.InvalidArgumentError):
        sess.run(init_op, feed_dict={count: 14, batch_size: 0})

  def testBatchAndMapDataset(self):
    return self._testBatchAndMapDatasetHelper()

  def testBatchAndMapDatasetWithParallelBatching(self):
    return self._testBatchAndMapDatasetHelper(num_parallel_batches=10)

  def testMapAndBatchSparse(self):

    def _sparse(i):
      return sparse_tensor.SparseTensorValue(
          indices=[[0]], values=(i * [1]), dense_shape=[1])

    iterator = dataset_ops.Dataset.range(10).apply(
        batching.map_and_batch(_sparse, 5)).make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(2):
        actual = sess.run(get_next)
        expected = sparse_tensor.SparseTensorValue(
            indices=[[0, 0], [1, 0], [2, 0], [3, 0], [4, 0]],
            values=[i * 5, i * 5 + 1, i * 5 + 2, i * 5 + 3, i * 5 + 4],
            dense_shape=[5, 1])
        self.assertTrue(sparse_tensor.is_sparse(actual))
        self.assertSparseValuesEqual(actual, expected)
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testBatchAndMapDatasetFails(self):
    """Test a dataset that maps a TF function across its input elements."""
    dataset = dataset_ops.Dataset.from_tensors(
        array_ops.check_numerics(
            constant_op.constant(1.0) / constant_op.constant(0.0), "oops"))
    batch_size = array_ops.placeholder(dtypes.int64, shape=[])
    iterator = (
        dataset.apply(batching.map_and_batch(lambda x: x, batch_size))
        .make_initializable_iterator())
    init_op = iterator.initializer
    with self.test_session() as sess:
      with self.assertRaisesRegexp(errors.InvalidArgumentError, "oops"):
        sess.run(init_op, feed_dict={batch_size: 14})

  def testBatchAndMapDatasetShapeMismatch(self):
    """Test a dataset that maps a TF function across its input elements."""

    def generator():
      yield [1]
      yield [2]
      yield [3]
      yield [[4, 5, 6]]

    dataset = dataset_ops.Dataset.from_generator(
        generator, output_types=dtypes.int32)
    batch_size = 4
    iterator = (
        dataset.apply(batching.map_and_batch(lambda x: x, batch_size))
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()
    with self.test_session() as sess:
      sess.run(init_op)
      with self.assertRaisesRegexp(errors.InvalidArgumentError,
                                   "number of elements does not match"):
        sess.run(get_next)


class BatchDatasetSerializationTest(
    dataset_serialization_test_base.DatasetSerializationTestBase):

  def build_dataset(self, multiplier=15.0, tensor_slice_len=2, batch_size=2):
    components = (
        np.arange(tensor_slice_len),
        np.array([[1, 2, 3]]) * np.arange(tensor_slice_len)[:, np.newaxis],
        np.array(multiplier) * np.arange(tensor_slice_len))

    return dataset_ops.Dataset.from_tensor_slices(components).batch(batch_size)

  def testCore(self):
    tensor_slice_len = 8
    batch_size = 2
    num_outputs = tensor_slice_len // batch_size
    self.run_core_tests(
        lambda: self.build_dataset(15.0, tensor_slice_len, batch_size),
        lambda: self.build_dataset(20.0, tensor_slice_len, batch_size),
        num_outputs)

  def _build_dataset_dense_to_sparse(self, components):
    return dataset_ops.Dataset.from_tensor_slices(components).map(
        lambda x: array_ops.fill([x], x)).apply(
            batching.dense_to_sparse_batch(4, [12]))

  # TODO(b/70988345): Re-enable when sparse tensors are properly supported by
  # the DatasetSerializationTestBase.
  def _testDenseToSparseBatchDatasetCore(self):
    components = np.random.randint(5, size=(40,)).astype(np.int32)
    diff_comp = np.random.randint(2, size=(100,)).astype(np.int32)

    num_outputs = len(components) // 4
    self.run_core_tests(lambda: self._build_dataset_dense_to_sparse(components),
                        lambda: self._build_dataset_dense_to_sparse(diff_comp),
                        num_outputs)

  def _sparse(self, i):
    return sparse_tensor.SparseTensorValue(
        indices=[[0]], values=(i * [1]), dense_shape=[1])

  def _build_dataset_sparse(self, batch_size=5):
    return dataset_ops.Dataset.range(10).map(self._sparse).batch(batch_size)

  def testSparseCore(self):
    self.run_core_tests(self._build_dataset_sparse,
                        lambda: self._build_dataset_sparse(2), 2)

  def _build_dataset_nested_sparse(self):
    return dataset_ops.Dataset.range(10).map(self._sparse).batch(5).batch(2)

  def testNestedSparseCore(self):
    self.run_core_tests(self._build_dataset_nested_sparse, None, 1)


class PaddedBatchDatasetSerializationTest(
    dataset_serialization_test_base.DatasetSerializationTestBase):

  def testPaddedBatch(self):

    def build_dataset(seq_lens):
      return dataset_ops.Dataset.from_tensor_slices(seq_lens).map(
          lambda x: array_ops.fill([x], x)).padded_batch(
              4, padded_shapes=[-1])

    seq_lens1 = np.random.randint(1, 20, size=(32,)).astype(np.int32)
    seq_lens2 = np.random.randint(21, 40, size=(32,)).astype(np.int32)
    self.run_core_tests(lambda: build_dataset(seq_lens1),
                        lambda: build_dataset(seq_lens2), 8)

  def testPaddedBatchNonDefaultPadding(self):

    def build_dataset(seq_lens):

      def fill_tuple(x):
        filled = array_ops.fill([x], x)
        return (filled, string_ops.as_string(filled))

      padded_shape = [-1]
      return dataset_ops.Dataset.from_tensor_slices(seq_lens).map(
          fill_tuple).padded_batch(
              4,
              padded_shapes=(padded_shape, padded_shape),
              padding_values=(-1, "<end>"))

    seq_lens1 = np.random.randint(1, 20, size=(32,)).astype(np.int32)
    seq_lens2 = np.random.randint(21, 40, size=(32,)).astype(np.int32)
    self.run_core_tests(lambda: build_dataset(seq_lens1),
                        lambda: build_dataset(seq_lens2), 8)


if __name__ == "__main__":
  test.main()
