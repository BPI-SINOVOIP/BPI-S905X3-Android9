# Copyright 2016 The TensorFlow Authors. All Rights Reserved.
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
"""Ops for representing Bayesian computation.

## This package provides classes for Bayesian computation with TensorFlow.
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

# pylint: disable=unused-import,line-too-long
from tensorflow.contrib.bayesflow.python.ops import csiszar_divergence
from tensorflow.contrib.bayesflow.python.ops import custom_grad
from tensorflow.contrib.bayesflow.python.ops import halton_sequence
from tensorflow.contrib.bayesflow.python.ops import hmc
from tensorflow.contrib.bayesflow.python.ops import layers
from tensorflow.contrib.bayesflow.python.ops import mcmc_diagnostics
from tensorflow.contrib.bayesflow.python.ops import metropolis_hastings
from tensorflow.contrib.bayesflow.python.ops import monte_carlo
from tensorflow.contrib.bayesflow.python.ops import optimizers
from tensorflow.contrib.bayesflow.python.ops import variable_utils
# pylint: enable=unused-import,line-too-long

from tensorflow.python.util.all_util import remove_undocumented


_allowed_symbols = [
    'csiszar_divergence',
    'custom_grad',
    'entropy',
    'halton_sequence',
    'hmc',
    'layers',
    'metropolis_hastings',
    'mcmc_diagnostics',
    'monte_carlo',
    'optimizers',
    'special_math',
    'stochastic_variables',
    'variable_utils',
    'variational_inference',
]

remove_undocumented(__name__, _allowed_symbols)
