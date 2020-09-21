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
"""Base class for tests in this module."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import contextlib
import imp

from tensorflow.contrib.py2tf import utils
from tensorflow.contrib.py2tf.pyct import compiler
from tensorflow.contrib.py2tf.pyct import context
from tensorflow.contrib.py2tf.pyct import parser
from tensorflow.contrib.py2tf.pyct import qual_names
from tensorflow.contrib.py2tf.pyct.static_analysis import activity
from tensorflow.contrib.py2tf.pyct.static_analysis import live_values
from tensorflow.contrib.py2tf.pyct.static_analysis import type_info
from tensorflow.python.platform import test


class FakeNamer(object):

  def new_symbol(self, name_root, used):
    i = 0
    while True:
      name = '%s%d' % (name_root, i)
      if name not in used:
        return name
      i += 1

  def compiled_function_name(self,
                             original_fqn,
                             live_entity=None,
                             owner_type=None):
    del live_entity
    if owner_type is not None:
      return None, False
    return ('renamed_%s' % '_'.join(original_fqn)), True


class TestCase(test.TestCase):
  """Base class for unit tests in this module. Contains relevant utilities."""

  @contextlib.contextmanager
  def compiled(self, node, *symbols):
    source = '<compile failed>'
    try:
      result, source = compiler.ast_to_object(node)
      result.tf = self.make_fake_tf(*symbols)
      result.py2tf_utils = utils
      yield result
    except Exception:  # pylint:disable=broad-except
      print('Offending compiled code:\n%s' % source)
      raise

  def make_fake_tf(self, *symbols):
    fake_tf = imp.new_module('fake_tf')
    for s in symbols:
      setattr(fake_tf, s.__name__, s)
    return fake_tf

  def attach_namespace(self, module, **ns):
    for k, v in ns.items():
      setattr(module, k, v)

  def parse_and_analyze(self,
                        test_fn,
                        namespace,
                        namer=None,
                        arg_types=None,
                        include_type_analysis=True,
                        recursive=True):
    node, source = parser.parse_entity(test_fn)
    ctx = context.EntityContext(
        namer=namer or FakeNamer(),
        source_code=source,
        source_file=None,
        namespace=namespace,
        arg_values=None,
        arg_types=arg_types,
        recursive=recursive)
    node = qual_names.resolve(node)
    node = activity.resolve(node, ctx)
    node = live_values.resolve(node, ctx, {})
    if include_type_analysis:
      node = type_info.resolve(node, ctx)
      node = live_values.resolve(node, ctx, {})
    self.ctx = ctx
    return node
