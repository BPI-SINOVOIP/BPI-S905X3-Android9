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
"""Experimental API for building input pipelines.

This module contains experimental `Dataset` sources and transformations that can
be used in conjunction with the @{tf.data.Dataset} API. Note that the
`tf.contrib.data` API is not subject to the same backwards compatibility
guarantees as `tf.data`, but we will provide deprecation advice in advance of
removing existing functionality.

See the @{$datasets$Importing Data} Programmer's Guide for an overview.

@@Counter

@@batch_and_drop_remainder
@@dense_to_sparse_batch
@@enumerate_dataset
@@group_by_window
@@ignore_errors
@@make_saveable_from_iterator
@@map_and_batch
@@padded_batch_and_drop_remainder
@@parallel_interleave
@@read_batch_features
@@rejection_resample
@@scan
@@shuffle_and_repeat
@@sloppy_interleave
@@unbatch

@@get_single_element
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

# pylint: disable=unused-import

from tensorflow.contrib.data.python.ops.batching import batch_and_drop_remainder
from tensorflow.contrib.data.python.ops.batching import dense_to_sparse_batch
from tensorflow.contrib.data.python.ops.batching import map_and_batch
from tensorflow.contrib.data.python.ops.batching import padded_batch_and_drop_remainder
from tensorflow.contrib.data.python.ops.batching import unbatch
from tensorflow.contrib.data.python.ops.counter import Counter
from tensorflow.contrib.data.python.ops.enumerate_ops import enumerate_dataset
from tensorflow.contrib.data.python.ops.error_ops import ignore_errors
from tensorflow.contrib.data.python.ops.get_single_element import get_single_element
from tensorflow.contrib.data.python.ops.grouping import group_by_window
from tensorflow.contrib.data.python.ops.interleave_ops import parallel_interleave
from tensorflow.contrib.data.python.ops.interleave_ops import sloppy_interleave
from tensorflow.contrib.data.python.ops.iterator_ops import make_saveable_from_iterator
from tensorflow.contrib.data.python.ops.readers import read_batch_features
from tensorflow.contrib.data.python.ops.readers import SqlDataset
from tensorflow.contrib.data.python.ops.resampling import rejection_resample
from tensorflow.contrib.data.python.ops.scan_ops import scan
from tensorflow.contrib.data.python.ops.shuffle_ops import shuffle_and_repeat
# pylint: enable=unused-import

from tensorflow.python.util.all_util import remove_undocumented
remove_undocumented(__name__)
