# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Library Imports for Cluster Resolvers."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.contrib.cluster_resolver.python.training.cluster_resolver import ClusterResolver
from tensorflow.contrib.cluster_resolver.python.training.cluster_resolver import SimpleClusterResolver
from tensorflow.contrib.cluster_resolver.python.training.cluster_resolver import UnionClusterResolver
from tensorflow.contrib.cluster_resolver.python.training.gce_cluster_resolver import GceClusterResolver
from tensorflow.contrib.cluster_resolver.python.training.tpu_cluster_resolver import TPUClusterResolver
