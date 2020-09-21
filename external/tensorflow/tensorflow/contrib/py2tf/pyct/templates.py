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
"""AST conversion templates.

Adapted from Tangent.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import ast
import textwrap

import gast

from tensorflow.contrib.py2tf.pyct import ast_util
from tensorflow.contrib.py2tf.pyct import parser
from tensorflow.contrib.py2tf.pyct import qual_names


class ReplaceTransformer(gast.NodeTransformer):
  """Replace AST nodes."""

  def __init__(self, replacements):
    """Create a new ReplaceTransformer.

    Args:
      replacements: A mapping from placeholder names to (lists of) AST nodes
          that these placeholders will be replaced by.
    """
    self.replacements = replacements
    self.in_replacements = False

  # TODO(mdan): Make a more detailed pass and clean up if needed.

  def visit_Expr(self, node):
    if (isinstance(node.value, gast.Name) and
        node.value.id in self.replacements):
      return self.visit(node.value)
    self.generic_visit(node)
    return node

  def visit_FunctionDef(self, node):
    node = self.generic_visit(node)
    if node.name in self.replacements:
      repl = self.replacements[node.name]
      if not isinstance(repl, (gast.Name, ast.Name)):
        raise ValueError(
            'A function name can only be replaced by a Name node. Found: %s' %
            repl)
      node.name = repl.id
    return node

  def _set_inner_child_context(self, node, ctx):
    if isinstance(node, gast.Attribute):
      self._set_inner_child_context(node.value, ctx)
      node.ctx = gast.Load()
    elif isinstance(node, gast.Tuple):
      for e in node.elts:
        self._set_inner_child_context(e, ctx)
      node.ctx = ctx
    elif isinstance(node, gast.Name):
      node.ctx = ctx
    elif isinstance(node, (gast.Str, gast.Num)):
      pass
    else:
      raise ValueError('unexpected node type "%s"' % node)

  def visit_Name(self, node):
    if node.id not in self.replacements:
      return node

    new_nodes = ast_util.copy_clean(self.replacements[node.id])
    if isinstance(new_nodes, gast.AST):
      new_nodes = [new_nodes]

    # Preserve the target context.
    for n in new_nodes:
      if isinstance(n, gast.Tuple):
        for e in n.elts:
          self._set_inner_child_context(e, node.ctx)
      if isinstance(n, gast.Attribute):
        # For attributes, the inner Name node receives the context, while the
        # outer ones have it set to Load.
        self._set_inner_child_context(n, node.ctx)
      else:
        n.ctx = node.ctx

    if len(new_nodes) == 1:
      new_nodes, = new_nodes

    return new_nodes


def _convert_to_ast(n):
  """Convert from a known data type to AST."""
  if isinstance(n, str):
    # Note: the node will receive the ctx value from the template, see
    # ReplaceTransformer.visit_Name.
    return gast.Name(id=n, ctx=None, annotation=None)
  if isinstance(n, qual_names.QN):
    return n.ast()
  if isinstance(n, list):
    return [_convert_to_ast(e) for e in n]
  if isinstance(n, tuple):
    return tuple(_convert_to_ast(e) for e in n)
  return n


def replace(template, **replacements):
  """Replace placeholders in a Python template.

  AST Name and Tuple nodes always receive the context that inferred from
  the template. However, when replacing more complex nodes (that can potentially
  contain Name children), then the caller is responsible for setting the
  appropriate context.

  Args:
    template: A string representing Python code. Any symbol name can be used
        that appears in the template code can be used as placeholder.
    **replacements: A mapping from placeholder names to (lists of) AST nodes
        that these placeholders will be replaced by. String values are also
        supported as a shorthand for AST Name nodes with the respective ID.

  Returns:
    An AST node or list of AST nodes with the replacements made. If the
    template was a function, a list will be returned. If the template was a
    node, the same node will be returned. If the template was a string, an
    AST node will be returned (a `Module` node in the case of a multi-line
    string, an `Expr` node otherwise).

  Raises:
    ValueError: if the arguments are incorrect.
  """
  if not isinstance(template, str):
    raise ValueError('Expected string template, got %s' % type(template))
  tree = parser.parse_str(textwrap.dedent(template))
  for k in replacements:
    replacements[k] = _convert_to_ast(replacements[k])
  results = ReplaceTransformer(replacements).visit(tree).body
  if isinstance(results, list):
    return [qual_names.resolve(r) for r in results]
  return qual_names.resolve(results)
