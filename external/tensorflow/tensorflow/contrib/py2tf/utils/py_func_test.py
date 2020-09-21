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
"""Tests for wrap_py_func module."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.contrib.py2tf.utils import py_func
from tensorflow.python.framework import constant_op
from tensorflow.python.framework import dtypes
from tensorflow.python.platform import test


class PyFuncTest(test.TestCase):

  def test_wrap_py_func_simple(self):

    def test_fn(a, b, c):
      return a + b + c

    with self.test_session() as sess:
      tensor_1 = constant_op.constant(1)
      self.assertEqual(3,
                       sess.run(
                           py_func.wrap_py_func(test_fn, dtypes.int64,
                                                (1, tensor_1, 1))))
      self.assertEqual(3,
                       sess.run(
                           py_func.wrap_py_func(test_fn, dtypes.int64,
                                                (1, 1, 1))))
      self.assertEqual(3,
                       sess.run(
                           py_func.wrap_py_func(test_fn, dtypes.int64,
                                                (tensor_1, 1, tensor_1))))

  def test_wrap_py_func_complex_args(self):

    class TestClass(object):

      def __init__(self):
        self.foo = 5

    def test_fn(a, b):
      return a * b.foo

    with self.test_session() as sess:
      self.assertEqual(35,
                       sess.run(
                           py_func.wrap_py_func(test_fn, dtypes.int64,
                                                (7, TestClass()))))
      self.assertEqual(
          35,
          sess.run(
              py_func.wrap_py_func(test_fn, dtypes.int64,
                                   (constant_op.constant(7), TestClass()))))

  def test_wrap_py_func_dummy_return(self):

    side_counter = [0]

    def test_fn(_):
      side_counter[0] += 1

    with self.test_session() as sess:
      self.assertEqual(1,
                       sess.run(
                           py_func.wrap_py_func(test_fn, None, (5,), True)))
      self.assertEqual([1], side_counter)
      self.assertEqual(1,
                       sess.run(
                           py_func.wrap_py_func(test_fn, None,
                                                (constant_op.constant(5),),
                                                True)))
      self.assertEqual([2], side_counter)


if __name__ == '__main__':
  test.main()
