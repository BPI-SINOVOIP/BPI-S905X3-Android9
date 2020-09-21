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
"""Canonicalizes for loops into while loops.

This canonicalizer uses the len function on its argument. That should be
converted to a tf.shape separately.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.contrib.py2tf.pyct import anno
from tensorflow.contrib.py2tf.pyct import templates
from tensorflow.contrib.py2tf.pyct import transformer
from tensorflow.contrib.py2tf.pyct.static_analysis.annos import NodeAnno


class ForLoopCanonicalizationTransformer(transformer.Base):
  """Canonicalizes for loops (e.g. into while loops)."""

  def __init__(self, context):
    super(ForLoopCanonicalizationTransformer, self).__init__(context)

  def visit_For(self, node):
    self.generic_visit(node)
    body_scope = anno.getanno(node, NodeAnno.BODY_SCOPE)

    if anno.hasanno(node, 'extra_cond'):
      template = """
        i = 0
        n = len(loop_iter)
        while i < n and extra_cond:
          # TODO(mdan): Use TensorListFromTensor(loop_iter) here.
          target = loop_iter[i]
          body
          i += 1
      """
      return templates.replace(
          template,
          loop_iter=node.iter,
          target=node.target,
          body=node.body,
          i=self.context.namer.new_symbol('i', body_scope.referenced),
          n=self.context.namer.new_symbol('n', body_scope.referenced),
          extra_cond=anno.getanno(node, 'extra_cond'))
    else:
      template = """
        i = 0
        n = len(loop_iter)
        while i < n:
          # TODO(mdan): Use TensorListFromTensor(loop_iter) here.
          target = loop_iter[i]
          body  # pylint:disable=pointless-statement
          i += 1
      """
      repl = templates.replace(
          template,
          loop_iter=node.iter,
          target=node.target,
          body=node.body,
          i=self.context.namer.new_symbol('i', body_scope.referenced),
          n=self.context.namer.new_symbol('n', body_scope.referenced))
      return repl

  def visit_Continue(self, node):
    assert False, 'continue statement should be desugared at this point'

  def visit_Break(self, node):
    assert False, 'break statement should be desugared at this point'


def transform(node, context):
  return ForLoopCanonicalizationTransformer(context).visit(node)
