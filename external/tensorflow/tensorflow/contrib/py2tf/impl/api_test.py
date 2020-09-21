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
"""Tests for api module."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.contrib.py2tf.impl import api
from tensorflow.contrib.py2tf.impl import config
from tensorflow.contrib.py2tf.pyct import parser
from tensorflow.python.framework import constant_op
from tensorflow.python.ops import math_ops
from tensorflow.python.platform import test


class ApiTest(test.TestCase):

  def setUp(self):
    config.DEFAULT_UNCOMPILED_MODULES.add((math_ops.__name__,))
    config.COMPILED_IMPORT_STATEMENTS = (
        'from tensorflow.python.ops '
        'import control_flow_ops as tf',
        'from tensorflow.contrib.py2tf import utils as '
        'py2tf_utils')

  def test_decorator_recurses(self):

    class TestClass(object):

      def called_member(self, a):
        if a < 0:
          a = -a
        return a

      @api.convert(recursive=True)
      def test_method(self, x, s, a):
        while math_ops.reduce_sum(x) > s:
          x //= self.called_member(a)
        return x

    tc = TestClass()
    with self.test_session() as sess:
      x = tc.test_method(
          constant_op.constant([2, 4]), constant_op.constant(1),
          constant_op.constant(-2))
      self.assertListEqual([0, 1], sess.run(x).tolist())

  def test_decorator_does_not_recurse(self):

    class TestClass(object):

      def called_member(self, a):
        return math_ops.negative(a)

      @api.convert(recursive=False)
      def test_method(self, x, s, a):
        while math_ops.reduce_sum(x) > s:
          x //= self.called_member(a)
        return x

    tc = TestClass()
    with self.test_session() as sess:
      x = tc.test_method(
          constant_op.constant([2, 4]), constant_op.constant(1),
          constant_op.constant(-2))
      self.assertListEqual([0, 1], sess.run(x).tolist())

  def test_decorator_calls_converted(self):

    class TestClass(object):

      @api.graph_ready
      def called_member(self, a):
        return math_ops.negative(a)

      @api.convert(recursive=True)
      def test_method(self, x, s, a):
        while math_ops.reduce_sum(x) > s:
          x //= self.called_member(a)
        return x

    tc = TestClass()
    with self.test_session() as sess:
      x = tc.test_method(
          constant_op.constant([2, 4]), constant_op.constant(1),
          constant_op.constant(-2))
      self.assertListEqual([0, 1], sess.run(x).tolist())

  def test_decorator_calls_decorated(self):

    class TestClass(object):

      @api.convert()
      def called_member(self, a):
        if a < 0:
          a = -a
        return a

      @api.convert(recursive=True)
      def test_method(self, x, s, a):
        while math_ops.reduce_sum(x) > s:
          x //= self.called_member(a)
        return x

    tc = TestClass()
    with self.test_session() as sess:
      x = tc.test_method(
          constant_op.constant([2, 4]), constant_op.constant(1),
          constant_op.constant(-2))
      self.assertListEqual([0, 1], sess.run(x).tolist())

  def test_convert_call_site_decorator(self):

    class TestClass(object):

      def called_member(self, a):
        if a < 0:
          a = -a
        return a

      @api.convert(recursive=True)
      def test_method(self, x, s, a):
        while math_ops.reduce_sum(x) > s:
          x //= api.convert_inline(self.called_member, a)
        return x

    tc = TestClass()
    with self.test_session() as sess:
      x = tc.test_method(
          constant_op.constant([2, 4]), constant_op.constant(1),
          constant_op.constant(-2))
      self.assertListEqual([0, 1], sess.run(x).tolist())

  def test_graph_ready_call_site_decorator(self):

    class TestClass(object):

      def called_member(self, a):
        return math_ops.negative(a)

      @api.convert(recursive=True)
      def test_method(self, x, s, a):
        while math_ops.reduce_sum(x) > s:
          x //= api.graph_ready(self.called_member(a))
        return x

    tc = TestClass()
    with self.test_session() as sess:
      x = tc.test_method(
          constant_op.constant([2, 4]), constant_op.constant(1),
          constant_op.constant(-2))
      self.assertListEqual([0, 1], sess.run(x).tolist())

  def test_to_graph_basic(self):
    def test_fn(x, s):
      while math_ops.reduce_sum(x) > s:
        x //= 2
      return x

    compiled_fn = api.to_graph(test_fn)

    with self.test_session() as sess:
      x = compiled_fn(constant_op.constant([4, 8]), 4)
      self.assertListEqual([1, 2], sess.run(x).tolist())

  def test_to_code_basic(self):
    def test_fn(x, s):
      while math_ops.reduce_sum(x) > s:
        x /= 2
      return x

    compiled_code = api.to_code(test_fn)

    # Just check for some key words and that it is parseable Python code.
    self.assertRegexpMatches(compiled_code, 'py2tf_utils\\.run_while')
    self.assertIsNotNone(parser.parse_str(compiled_code))


if __name__ == '__main__':
  test.main()
