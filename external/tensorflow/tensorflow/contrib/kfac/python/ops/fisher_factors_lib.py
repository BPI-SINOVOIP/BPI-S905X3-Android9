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
"""FisherFactor definitions."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

# pylint: disable=unused-import,line-too-long,wildcard-import
from tensorflow.contrib.kfac.python.ops.fisher_factors import *
from tensorflow.python.util.all_util import remove_undocumented
# pylint: enable=unused-import,line-too-long,wildcard-import

_allowed_symbols = [
    "inverse_initializer", "covariance_initializer",
    "diagonal_covariance_initializer", "scope_string_from_params",
    "scope_string_from_name", "scalar_or_tensor_to_string", "FisherFactor",
    "InverseProvidingFactor", "FullFactor", "DiagonalFactor",
    "NaiveDiagonalFactor", "EmbeddingInputKroneckerFactor",
    "FullyConnectedDiagonalFactor", "FullyConnectedKroneckerFactor",
    "ConvInputKroneckerFactor", "ConvOutputKroneckerFactor",
    "ConvDiagonalFactor", "set_global_constants", "maybe_colocate_with",
    "compute_cov", "append_homog"
]

remove_undocumented(__name__, allowed_exception_list=_allowed_symbols)
