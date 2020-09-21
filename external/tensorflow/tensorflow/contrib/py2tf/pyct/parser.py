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
"""Converting code to AST.

Adapted from Tangent.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import textwrap

import gast

from tensorflow.python.util import tf_inspect


def parse_entity(entity):
  """Return the AST of given entity."""
  source = tf_inspect.getsource(entity)
  source = textwrap.dedent(source)
  return parse_str(source), source


def parse_str(src):
  """Return the AST of given piece of code."""
  return gast.parse(src)
