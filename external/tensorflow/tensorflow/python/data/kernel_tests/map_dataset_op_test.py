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

from collections import namedtuple
import threading
import time

import numpy as np

from tensorflow.python.client import session
from tensorflow.python.data.ops import dataset_ops
from tensorflow.python.framework import constant_op
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import errors
from tensorflow.python.framework import ops
from tensorflow.python.framework import sparse_tensor
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import data_flow_ops
from tensorflow.python.ops import functional_ops
from tensorflow.python.ops import lookup_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import random_ops
from tensorflow.python.ops import script_ops
from tensorflow.python.ops import sparse_ops
from tensorflow.python.ops import string_ops
from tensorflow.python.ops import variable_scope
from tensorflow.python.platform import test


class MapDatasetTest(test.TestCase):

  def _buildMapDataset(self, components, count):
    def _map_fn(x, y, z):
      return math_ops.square(x), math_ops.square(y), math_ops.square(z)
    return (dataset_ops.Dataset.from_tensor_slices(components).map(_map_fn)
            .repeat(count))

  def testMapDataset(self):
    """Test an dataset that maps a TF function across its input elements."""
    # The pipeline is TensorSliceDataset -> MapDataset(square_3) ->
    # RepeatDataset(count).
    components = (np.arange(7),
                  np.array([[1, 2, 3]]) * np.arange(7)[:, np.newaxis],
                  np.array(37.0) * np.arange(7))
    count = array_ops.placeholder(dtypes.int64, shape=[])

    dataset = self._buildMapDataset(components, count)
    iterator = dataset.make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    self.assertEqual([c.shape[1:] for c in components],
                     [t.shape for t in get_next])

    with self.test_session() as sess:
      # Test single-threaded access to the iterator.
      sess.run(init_op, feed_dict={count: 14})
      for _ in range(14):
        for i in range(7):
          result = sess.run(get_next)
          for component, result_component in zip(components, result):
            self.assertAllEqual(component[i]**2, result_component)
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

      # Test multi-threaded access to the same iterator.
      sess.run(init_op, feed_dict={count: 18})
      results = []
      def iterator_thread():
        while True:
          try:
            results.append(sess.run(get_next))
          except errors.OutOfRangeError:
            return
      threads = [self.checkedThread(target=iterator_thread) for _ in range(8)]
      for t in threads:
        t.start()
      for t in threads:
        t.join()

      # `results` will contain the same elements components**2
      # repeated 18 times, but in a non-deterministic order. Sort the
      # results, and assert that each element of components**2 is
      # produced 18 times.
      results.sort(key=lambda x: x[0])
      for i in range(7):
        for j in range(18):
          for component, result_component in zip(components,
                                                 results[i * 18 + j]):
            self.assertAllEqual(component[i]**2, result_component)

  def _buildParallelMapDataset(self, components, count, num_parallel_calls,
                               output_buffer_size):
    def _map_fn(x, y, z):
      return math_ops.square(x), math_ops.square(y), math_ops.square(z)
    return (dataset_ops.Dataset.from_tensor_slices(components)
            .map(_map_fn, num_parallel_calls=num_parallel_calls)
            .prefetch(output_buffer_size)
            .repeat(count))

  def testParallelMapDataset(self):
    """Test an dataset that maps a TF function across its input elements."""
    # The pipeline is TensorSliceDataset -> ParallelMapDataset(square_3) ->
    # RepeatDataset(count).
    components = (np.arange(7),
                  np.array([[1, 2, 3]]) * np.arange(7)[:, np.newaxis],
                  np.array(37.0) * np.arange(7))
    count = array_ops.placeholder(dtypes.int64, shape=[])
    num_parallel_calls = array_ops.placeholder(dtypes.int32, shape=[])
    output_buffer_size = array_ops.placeholder(dtypes.int64, shape=[])

    dataset = self._buildParallelMapDataset(
        components, count, num_parallel_calls, output_buffer_size)
    iterator = dataset.make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    self.assertEqual([c.shape[1:] for c in components],
                     [t.shape for t in get_next])

    with self.test_session() as sess:
      def do_test(num_parallel_calls_val, output_buffer_size_val):
        # Test single-threaded access to the iterator.
        sess.run(init_op, feed_dict={
            count: 14,
            num_parallel_calls: num_parallel_calls_val,
            output_buffer_size: output_buffer_size_val})
        for _ in range(14):
          for i in range(7):
            result = sess.run(get_next)
            for component, result_component in zip(components, result):
              self.assertAllEqual(component[i]**2, result_component)
        with self.assertRaises(errors.OutOfRangeError):
          sess.run(get_next)

        # Test multi-threaded access to the same iterator.
        sess.run(init_op, feed_dict={
            count: 18,
            num_parallel_calls: num_parallel_calls_val,
            output_buffer_size: output_buffer_size_val})
        results = []
        def iterator_thread():
          while True:
            try:
              results.append(sess.run(get_next))
            except errors.OutOfRangeError:
              return
        threads = [self.checkedThread(target=iterator_thread)
                   for _ in range(64)]
        for t in threads:
          t.start()
        for t in threads:
          t.join()

        # `results` will contain the same elements components**2
        # repeated 18 times, but in a non-deterministic order. Sort the
        # results, and assert that each element of components**2 is
        # produced 18 times.
        results.sort(key=lambda x: x[0])
        for i in range(7):
          for j in range(18):
            for component, result_component in zip(components,
                                                   results[i * 18 + j]):
              self.assertAllEqual(component[i]**2, result_component)

      for num_parallel_calls_val, output_buffer_size_val in [
          (1, 1), (1, 2), (2, 2), (2, 4), (8, 8), (8, 16)]:
        do_test(num_parallel_calls_val, output_buffer_size_val)

  def testImplicitDisposeParallelMapDataset(self):
    # Tests whether a parallel map dataset will be cleaned up correctly when
    # the pipeline does not run it until exhaustion.
    # The pipeline is TensorSliceDataset -> MapDataset(square_3) ->
    # RepeatDataset(1000).
    components = (np.arange(1000),
                  np.array([[1, 2, 3]]) * np.arange(1000)[:, np.newaxis],
                  np.array(37.0) * np.arange(1000))

    dataset = self._buildParallelMapDataset(components, 1000, 100, 100)
    # NOTE(mrry): Also test that the prefetching thread is cancelled correctly.
    dataset = dataset.prefetch(100)
    iterator = dataset.make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for _ in range(3):
        sess.run(get_next)

  def testParallelMapUnspecifiedOutputSize(self):
    components = np.array([1., 2., 3., np.nan, 5.]).astype(np.float32)

    dataset = (dataset_ops.Dataset.from_tensor_slices(components)
               .map(lambda x: array_ops.check_numerics(x, "message"),
                    num_parallel_calls=2))
    iterator = dataset.make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for _ in range(3):
        sess.run(get_next)

  def testParallelMapError(self):
    components = np.array([1., 2., 3., np.nan, 5.]).astype(np.float32)

    dataset = (dataset_ops.Dataset.from_tensor_slices(components)
               .map(lambda x: array_ops.check_numerics(x, "message"),
                    num_parallel_calls=2))
    iterator = dataset.make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for _ in range(3):
        sess.run(get_next)
      # The 4th element is NaN, so `array_ops.check_numerics()` should fail.
      with self.assertRaises(errors.InvalidArgumentError):
        sess.run(get_next)
      sess.run(get_next)
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testPrefetchError(self):
    components = np.array([1., 2., 3., np.nan, 5.]).astype(np.float32)

    dataset = (dataset_ops.Dataset.from_tensor_slices(components)
               .map(lambda x: array_ops.check_numerics(x, "message"))
               .prefetch(2))
    iterator = dataset.make_initializable_iterator()
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for _ in range(3):
        sess.run(get_next)
      # The 4th element is NaN, so `array_ops.check_numerics()` should fail.
      with self.assertRaises(errors.InvalidArgumentError):
        sess.run(get_next)
      sess.run(get_next)
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testCaptureHashTable(self):
    # NOTE(mrry): We must use the V2 variants of `HashTable`
    # etc. because these produce a `tf.resource`-typed output that is
    # compatible with the in-graph function implementation.
    default_val = -1
    keys = constant_op.constant(["brain", "salad", "surgery"])
    values = constant_op.constant([0, 1, 2], dtypes.int64)
    table = lookup_ops.HashTable(
        lookup_ops.KeyValueTensorInitializer(keys, values), default_val)

    input_sentences = dataset_ops.Dataset.from_tensor_slices(
        ["brain brain tank salad surgery", "surgery brain"])

    iterator = (input_sentences
                .map(lambda x: string_ops.string_split([x]).values)
                .map(table.lookup)
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(table.init)
      sess.run(init_op)
      sess.run(get_next)
      sess.run(get_next)
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testCaptureQueue(self):
    elements = np.random.randint(100, size=[200])
    queue = data_flow_ops.FIFOQueue(200, dtypes.int64, shapes=[])
    enqueue_op = queue.enqueue_many(elements)
    close_op = queue.close()
    iterator = (dataset_ops.Dataset.from_tensors(0).repeat(-1)
                .map(lambda _: queue.dequeue()).make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(enqueue_op)
      sess.run(close_op)
      sess.run(init_op)
      for element in elements:
        self.assertEqual(element, sess.run(get_next))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testCaptureSameResourceMultipleTimes(self):
    elements = np.random.randint(100, size=[200])
    queue = data_flow_ops.FIFOQueue(
        200, dtypes.int64, shapes=[], shared_name="shared_queue")
    queue_2 = data_flow_ops.FIFOQueue(
        200, dtypes.int64, shapes=[], shared_name="shared_queue")

    enqueue_op = queue.enqueue_many(elements)
    close_op = queue.close()

    iterator = (dataset_ops.Dataset.from_tensors(0).repeat(-1)
                .map(lambda _: (queue.dequeue(), queue_2.dequeue()))
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(enqueue_op)
      sess.run(close_op)
      sess.run(init_op)
      for i in range(100):
        self.assertEqual(sorted([elements[i * 2], elements[i * 2 + 1]]),
                         sorted(sess.run(get_next)))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testCaptureVariable(self):
    counter_var = variable_scope.get_variable(
        "counter", (), dtypes.int32, use_resource=True)
    iterator = (dataset_ops.Dataset.from_tensors(0).repeat(10)
                .map(lambda _: counter_var.assign_add(1))
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(counter_var.initializer)
      sess.run(init_op)
      for i in range(10):
        self.assertEqual(i, sess.run(counter_var))
        self.assertEqual(i + 1, sess.run(get_next))
      self.assertEqual(10, sess.run(counter_var))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)
      self.assertEqual(10, sess.run(counter_var))

  def testCaptureUninitializedVariableError(self):
    counter_var = variable_scope.get_variable(
        "counter", (), dtypes.int32, use_resource=True)
    iterator = (dataset_ops.Dataset.from_tensors(0).repeat(10)
                .map(lambda _: counter_var.assign_add(1))
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      with self.assertRaises(errors.NotFoundError):
        sess.run(get_next)

  def testSeededStatefulOperatorIsProperlyStateful(self):
    iterator = (dataset_ops.Dataset.from_tensors(0).repeat(10)
                .map(lambda _: random_ops.random_uniform((), seed=11)).batch(2)
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      random_values = []
      with self.assertRaises(errors.OutOfRangeError):
        while True:
          random_values.extend(sess.run(get_next))
      self.assertEqual(10, len(random_values))
      self.assertGreater(np.abs(np.diff(random_values)).max(), 1e-6)
      sess.run(init_op)
      random_values_2 = []
      with self.assertRaises(errors.OutOfRangeError):
        while True:
          random_values_2.extend(sess.run(get_next))

      # Randomness is repeatable given same seed
      self.assertAllClose(random_values, random_values_2)

  def testMapDict(self):
    iterator = (dataset_ops.Dataset.range(10)
                .map(lambda x: {"foo": x * 2, "bar": x ** 2})
                .map(lambda d: d["foo"] + d["bar"])
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(10):
        self.assertEqual(i * 2 + i ** 2, sess.run(get_next))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testMapNamedtuple(self, count=10):
    # construct dataset of tuples
    labels = dataset_ops.Dataset.range(count)
    images = labels.map(lambda l: -l)
    dataset_tuple = dataset_ops.Dataset.zip((labels, images))

    # convert dataset of tuples to dataset of namedtuples
    example = namedtuple("Example", ["label", "image"])
    dataset_namedtuple = dataset_tuple.map(example)

    def preprocess_tuple(label, image):
      image = 2 * image
      return label, image

    def preprocess_namedtuple(example):
      return example._replace(image=2 * example.image)

    # preprocess both datasets
    dataset_tuple = dataset_tuple.map(preprocess_tuple)
    dataset_namedtuple = dataset_namedtuple.map(preprocess_namedtuple)

    next_tuple = dataset_tuple.make_one_shot_iterator().get_next()
    next_namedtuple = dataset_namedtuple.make_one_shot_iterator().get_next()

    # make sure both datasets contain the same data
    with self.test_session() as sess:
      for i in range(count):
        tuple_, namedtuple_ = sess.run([next_tuple, next_namedtuple])
        self.assertEqual(tuple_, namedtuple_)
        self.assertEqual(tuple_, (i, -2 * i))

      with self.assertRaises(errors.OutOfRangeError):
        sess.run(next_namedtuple)

  def testUseStepContainerInMap(self):
    row = np.arange(6)
    iterator = (
        dataset_ops.Dataset.from_tensors(row)
        .map(lambda elems: functional_ops.map_fn(lambda x: x * x, elems))
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      self.assertAllEqual(row ** 2, sess.run(get_next))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testPrefetch(self):
    # We will use this event to test that `_map_py_func()` has been
    # invoked a certain number of times (6 times, to be exact) after
    # consuming fewer elements from the iterator.
    ev = threading.Event()

    set_event_during_invocation = 5

    def _map_py_func(x):
      if x == set_event_during_invocation:
        ev.set()
      return x * x

    def _map_fn(x):
      return script_ops.py_func(_map_py_func, [x], x.dtype)

    buffer_size_placeholder = array_ops.placeholder(dtypes.int64, shape=[])
    iterator = (
        dataset_ops.Dataset.range(100)
        .map(_map_fn)
        .prefetch(buffer_size_placeholder)
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      # Simple test that prefetch yields the expected values in the
      # expected order.
      for buffer_size in [1, 10, 100, 1000]:
        sess.run(init_op, feed_dict={buffer_size_placeholder: buffer_size})
        for i in range(100):
          self.assertEqual(i * i, sess.run(get_next))
        with self.assertRaises(errors.OutOfRangeError):
          sess.run(get_next)

      # We can indirectly observe that varying the buffer size has the
      # intended effect by observing when `ev` is set (on the 6th
      # invocation of `_map_py_func()`).
      # NOTE(mrry): We do not test with `buffer_size ==
      # set_event_during_invocation`, because we must consume at least
      # one element to start the prefetching.
      for buffer_size in range(1, set_event_during_invocation):
        event_will_be_set_after_consuming = (
            set_event_during_invocation - buffer_size + 1)

        ev.clear()
        sess.run(init_op, feed_dict={buffer_size_placeholder: buffer_size})
        for i in range(event_will_be_set_after_consuming):
          self.assertFalse(ev.is_set())
          self.assertEqual(i * i, sess.run(get_next))
        ev.wait()
        for i in range(event_will_be_set_after_consuming, 100):
          self.assertEqual(i * i, sess.run(get_next))
        with self.assertRaises(errors.OutOfRangeError):
          sess.run(get_next)

  def testReturnList(self):
    iterator = (dataset_ops.Dataset.range(10)
                .map(lambda x: [x, constant_op.constant(37.0)])
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(10):
        self.assertEqual((i, 37.0), sess.run(get_next))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testMultiOutputPyFunc(self):
    # The `tf.py_func()` op returns a list of tensors for its outputs.
    def _map_fn(x_tensor):
      def _map_py_func(x):
        return x, np.array(37.0, dtype=np.float64)
      return script_ops.py_func(
          _map_py_func, [x_tensor], [dtypes.int64, dtypes.float64])

    iterator = (dataset_ops.Dataset.range(10)
                .map(_map_fn)
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(10):
        self.assertEqual((i, 37.0), sess.run(get_next))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def assertSparseValuesEqual(self, a, b):
    self.assertAllEqual(a.indices, b.indices)
    self.assertAllEqual(a.values, b.values)
    self.assertAllEqual(a.dense_shape, b.dense_shape)

  def testSparse(self):

    def _sparse(i):
      return sparse_tensor.SparseTensorValue(
          indices=np.array([[0, 0]]),
          values=(i * np.array([1])),
          dense_shape=np.array([1, 1]))

    iterator = (dataset_ops.Dataset.range(10)
                .map(_sparse)
                .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(10):
        actual = sess.run(get_next)
        self.assertTrue(isinstance(actual, sparse_tensor.SparseTensorValue))
        self.assertSparseValuesEqual(actual, _sparse(i))
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)

  def testSparseChain(self):

    def _sparse(i):
      return sparse_tensor.SparseTensorValue(
          indices=np.array([[0, 0]]),
          values=(i * np.array([1])),
          dense_shape=np.array([1, 1]))

    def _check(i):
      self.assertTrue(sparse_tensor.is_sparse(i))
      return sparse_ops.sparse_concat(0, [i, i])

    iterator = (
        dataset_ops.Dataset.range(10).map(_sparse).map(_check)
        .make_initializable_iterator())
    init_op = iterator.initializer
    get_next = iterator.get_next()

    with self.test_session() as sess:
      sess.run(init_op)
      for i in range(10):
        actual = sess.run(get_next)
        self.assertTrue(isinstance(actual, sparse_tensor.SparseTensorValue))
        self.assertSparseValuesEqual(actual, _check(_sparse(i)).eval())
      with self.assertRaises(errors.OutOfRangeError):
        sess.run(get_next)


class MapDatasetBenchmark(test.Benchmark):

  def benchmarkChainOfMaps(self):
    chain_lengths = [0, 1, 2, 5, 10, 20, 50]
    for chain_length in chain_lengths:
      with ops.Graph().as_default():
        dataset = dataset_ops.Dataset.from_tensors(0).repeat(None)
        for _ in range(chain_length):
          dataset = dataset.map(lambda x: x)
        iterator = dataset.make_one_shot_iterator()
        next_element = iterator.get_next()

        with session.Session() as sess:
          for _ in range(5):
            sess.run(next_element.op)
          deltas = []
          for _ in range(100):
            start = time.time()
            for _ in range(100):
              sess.run(next_element.op)
            end = time.time()
            deltas.append(end - start)

          median_wall_time = np.median(deltas) / 100
          print("Map dataset chain length: %d Median wall time: %f"
                % (chain_length, median_wall_time))
          self.report_benchmark(
              iters=1000, wall_time=median_wall_time,
              name="benchmark_map_dataset_chain_latency_%d" % chain_length)

  def benchmarkMapFanOut(self):
    fan_outs = [1, 2, 5, 10, 20, 50, 100]
    for fan_out in fan_outs:
      with ops.Graph().as_default():
        dataset = dataset_ops.Dataset.from_tensors(
            tuple(0 for _ in range(fan_out))).repeat(None).map(lambda *xs: xs)
        iterator = dataset.make_one_shot_iterator()
        next_element = iterator.get_next()

        with session.Session() as sess:
          for _ in range(5):
            sess.run(next_element[0].op)
          deltas = []
          for _ in range(100):
            start = time.time()
            for _ in range(100):
              sess.run(next_element[0].op)
            end = time.time()
            deltas.append(end - start)

          median_wall_time = np.median(deltas) / 100
          print("Map dataset fan out: %d Median wall time: %f"
                % (fan_out, median_wall_time))
          self.report_benchmark(
              iters=1000, wall_time=median_wall_time,
              name="benchmark_map_dataset_fan_out_%d" % fan_out)


if __name__ == "__main__":
  test.main()
