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

from tensorflow.contrib.data.python.kernel_tests import dataset_serialization_test_base
from tensorflow.contrib.data.python.ops import unique
from tensorflow.python.data.ops import dataset_ops
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import errors
from tensorflow.python.platform import test
from tensorflow.python.util import compat


class UniqueDatasetTest(test.TestCase):

  def _testSimpleHelper(self, dtype, test_cases):
    """Test the `unique()` transformation on a list of test cases.

    Args:
      dtype: The `dtype` of the elements in each test case.
      test_cases: A list of pairs of lists. The first component is the test
        input that will be passed to the transformation; the second component
        is the expected sequence of outputs from the transformation.
    """

    # The `current_test_case` will be updated when we loop over `test_cases`
    # below; declare it here so that the generator can capture it once.
    current_test_case = []
    dataset = dataset_ops.Dataset.from_generator(lambda: current_test_case,
                                                 dtype).apply(unique.unique())
    iterator = dataset.make_initializable_iterator()
    next_element = iterator.get_next()

    with self.test_session() as sess:
      for test_case, expected in test_cases:
        current_test_case = test_case
        sess.run(iterator.initializer)
        for element in expected:
          if dtype == dtypes.string:
            element = compat.as_bytes(element)
          self.assertAllEqual(element, sess.run(next_element))
        with self.assertRaises(errors.OutOfRangeError):
          sess.run(next_element)

  def testSimpleInt(self):
    for dtype in [dtypes.int32, dtypes.int64]:
      self._testSimpleHelper(dtype, [
          ([], []),
          ([1], [1]),
          ([1, 1, 1, 1, 1, 1, 1], [1]),
          ([1, 2, 3, 4], [1, 2, 3, 4]),
          ([1, 2, 4, 3, 2, 1, 2, 3, 4], [1, 2, 4, 3]),
          ([[1], [1, 1], [1, 1, 1]], [[1], [1, 1], [1, 1, 1]]),
          ([[1, 1], [1, 1], [2, 2], [3, 3], [1, 1]], [[1, 1], [2, 2], [3, 3]]),
      ])

  def testSimpleString(self):
    self._testSimpleHelper(dtypes.string, [
        ([], []),
        (["hello"], ["hello"]),
        (["hello", "hello", "hello"], ["hello"]),
        (["hello", "world"], ["hello", "world"]),
        (["foo", "bar", "baz", "baz", "bar", "foo"], ["foo", "bar", "baz"]),
    ])


class UniqueSerializationTest(
    dataset_serialization_test_base.DatasetSerializationTestBase):

  def testUnique(self):

    def build_dataset(num_elements, unique_elem_range):
      return dataset_ops.Dataset.range(num_elements).map(
          lambda x: x % unique_elem_range).apply(unique.unique())

    self.run_core_tests(lambda: build_dataset(200, 100),
                        lambda: build_dataset(40, 100), 100)


if __name__ == "__main__":
  test.main()
